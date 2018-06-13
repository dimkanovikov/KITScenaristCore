#include "TreeViewItemDelegate.h"

#include <3rd_party/Helpers/ImageHelper.h>
#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QPainter>

namespace {
#ifdef MOBILE_OS
    const int ICON_SIZE = 28;
    const int TOP_MARGIN = 9;
    const int BOTTOM_MARGIN = 9;
    const int ITEMS_SPACING = 8;
#else
    const int ICON_SIZE = 20;
    const int TOP_MARGIN = 8;
    const int BOTTOM_MARGIN = 8;
    const int ITEMS_SPACING = 8;
#endif
}


TreeViewItemDelegate::TreeViewItemDelegate(QObject* _parent) :
    QStyledItemDelegate(_parent),
    m_iconSize(StyleSheetHelper::dpToPx(ICON_SIZE)),
    m_topMargin(StyleSheetHelper::dpToPx(TOP_MARGIN)),
    m_bottomMargin(StyleSheetHelper::dpToPx(BOTTOM_MARGIN)),
    m_itemsSpacing(StyleSheetHelper::dpToPx(ITEMS_SPACING))
{
}

void TreeViewItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);

    //
    // Определим кисти и шрифты
    //
#ifdef MOBILE_OS
    QBrush backgroundBrush = opt.palette.background();
    QBrush textBrush = opt.palette.mid();
    QFont headerFont = opt.font;
    headerFont.setPointSize(16);
    headerFont.setWeight(QFont::Medium);
#else
    QBrush backgroundBrush = opt.palette.background();
    QBrush textBrush = opt.palette.text();
    QFont headerFont = opt.font;
#endif
    //
    // ... если это родитель верхнего уровня
    //
    if (!_index.parent().isValid()) {
        headerFont.setBold(true);
    }
    //
    // ... для выделенных элементов
    //
    if (opt.state.testFlag(QStyle::State_Selected))
    {
        backgroundBrush = opt.palette.highlight();
        textBrush = opt.palette.highlightedText();
    }
    //
    // ... для остальных
    //
    else
    {
        //
        // Реализация альтернативных цветов в представлении
        //
        if (opt.features.testFlag(QStyleOptionViewItem::Alternate))
        {
            backgroundBrush = opt.palette.alternateBase();
            textBrush = opt.palette.windowText();
        }
        else
        {
            backgroundBrush = opt.palette.base();
            textBrush = opt.palette.windowText();
        }
    }

    //
    // Рисуем
    //
    const int HEADER_MARGIN = StyleSheetHelper::dpToPx(12);
    //
    // ... фон
    //
    _painter->fillRect(opt.rect, backgroundBrush);
#ifndef MOBILE_OS
    //
    // ... разделитель
    //
    QPoint left = opt.rect.bottomLeft();
    left.setX(0);
    _painter->setPen(QPen(opt.palette.dark(), 0.5));
    _painter->drawLine(left, opt.rect.bottomRight());
#endif
    //
    // Меняем координаты, чтобы рисовать было удобнее
    //
    _painter->translate(opt.rect.topLeft());
    //
    // ... иконка
    //
    const int iconXPos = QLocale().textDirection() == Qt::LeftToRight ? 0 : opt.rect.right() - m_iconSize;
    const QRect iconRect(iconXPos, m_topMargin, m_iconSize, m_iconSize);
    {
        //
        // ... если есть флажёк, то рисуем его
        //
        if (!_index.data(Qt::CheckStateRole).isNull()) {
            _painter->save();
            _painter->translate(iconRect.center().x(), iconRect.center().y());
            QStyleOptionButton checkBoxOption;
            checkBoxOption.state = QStyle::State_Enabled;
            const int checkState = _index.data(Qt::CheckStateRole).toInt();
            checkBoxOption.state |= checkState == Qt::Checked
                                    ? QStyle::State_On
                                    : checkState == Qt::PartiallyChecked
                                      ? QStyle::State_NoChange
                                      : QStyle::State_Off;
            qApp->style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, _painter);
            _painter->restore();
        }
        //
        // ... в противном случае иконку
        //
        else {
            QPixmap iconPixmap = _index.data(Qt::DecorationRole).value<QPixmap>();
            QIcon icon = _index.data(Qt::DecorationRole).value<QIcon>();
            QIcon iconColorized(!iconPixmap.isNull() ? iconPixmap : icon);
            if (m_needColorize) {
                const QColor itemColor = _index.data(Qt::BackgroundColorRole).value<QColor>();
                const QColor iconColor = itemColor.isValid() ? itemColor : textBrush.color();
                ImageHelper::setIconColor(iconColorized, iconRect.size(), iconColor);
            }
            iconPixmap = iconColorized.pixmap(iconRect.size());
            _painter->drawPixmap(iconRect, iconPixmap);
        }
    }
    //
    // ... заголовок
    //
    _painter->setPen(textBrush.color());
    _painter->setFont(headerFont);
    const int headerXPos = QLocale().textDirection() == Qt::LeftToRight ? iconRect.right() + m_itemsSpacing : 0;
    const QRect headerRect(
                headerXPos,
                m_topMargin,
                opt.rect.width() - iconRect.width() - m_itemsSpacing - HEADER_MARGIN,
                m_iconSize
                );
    QString header = _index.data(Qt::DisplayRole).toString();
    header = _painter->fontMetrics().elidedText(header, Qt::ElideRight, headerRect.width());
    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, header);

    _painter->restore();
}

QSize TreeViewItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option);
    Q_UNUSED(_index);

    const int height = m_topMargin + m_iconSize + m_bottomMargin;
    const int width = 50;
    return QSize(width, height);
}

bool TreeViewItemDelegate::editorEvent(QEvent* _event, QAbstractItemModel* _model, const QStyleOptionViewItem& _option, const QModelIndex& _index)
{
    if (_event->type() == QEvent::MouseButtonRelease
        && !_index.data(Qt::CheckStateRole).isNull())
    {
        const int checkState = _index.data(Qt::CheckStateRole).toInt();

        if (checkState == Qt::PartiallyChecked
            || checkState == Qt::Unchecked) {
            _model->setData(_index, Qt::Checked, Qt::CheckStateRole);
        }
        else {
            _model->setData(_index, Qt::Unchecked, Qt::CheckStateRole);
        }

        return true;
    }

    return QStyledItemDelegate::editorEvent(_event, _model, _option, _index);
}

void TreeViewItemDelegate::setNeedColorize(bool _need)
{
    m_needColorize = _need;
}
