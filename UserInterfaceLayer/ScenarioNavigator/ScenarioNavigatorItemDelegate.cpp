#include "ScenarioNavigatorItemDelegate.h"

#include <BusinessLayer/ScenarioDocument/ScenarioModel.h>
#include <BusinessLayer/Chronometry/ChronometerFacade.h>

#include <3rd_party/Helpers/ImageHelper.h>
#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QPainter>

using UserInterface::ScenarioNavigatorItemDelegate;

namespace {
#ifdef MOBILE_OS
    const int ICON_SIZE = 28;
    const int ICON_MARGIN = 12;
    const int MARGIN = 15;
    const int HORIZONTAL_SPACING = 12;
    const int VERTICAL_SPACING = 6;
#else
    const int ICON_SIZE = 20;
    const int ICON_MARGIN = 6;
    const int MARGIN = 8;
    const int HORIZONTAL_SPACING = 6;
    const int VERTICAL_SPACING = 6;
#endif
}


ScenarioNavigatorItemDelegate::ScenarioNavigatorItemDelegate(QObject* _parent) :
    QStyledItemDelegate(_parent),
    m_showSceneNumber(false),
    m_showSceneTitle(false),
    m_showSceneDescription(true),
    m_sceneDescriptionIsSceneText(true),
    m_sceneDescriptionHeight(1),
    m_iconSize(StyleSheetHelper::dpToPx(ICON_SIZE)),
    m_iconTopMargin(StyleSheetHelper::dpToPx(ICON_MARGIN)),
    m_topMargin(StyleSheetHelper::dpToPx(MARGIN)),
    m_bottomMargin(StyleSheetHelper::dpToPx(MARGIN)),
    m_itemsHorizontalSpacing(StyleSheetHelper::dpToPx(HORIZONTAL_SPACING)),
    m_itemsVerticalSpacing(StyleSheetHelper::dpToPx(VERTICAL_SPACING))
{
#ifdef MOBILE_OS
    if (!StyleSheetHelper::deviceIsTablet()) {
        m_iconSize = 0;
        m_itemsHorizontalSpacing = 0;
    }
#endif
}

void ScenarioNavigatorItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    const QPalette palette = QApplication::palette();
#ifdef MOBILE_OS
    QBrush backgroundBrush = palette.background();
    QBrush headerBrush = palette.text();
    QBrush textBrush = palette.mid();
    QFont headerFont = opt.font;
    headerFont.setPointSize(16);
    headerFont.setWeight(QFont::Medium);
    QFont textFont = opt.font;
    textFont.setPointSize(14);
    textFont.setWeight(QFont::Medium);
#else
    QBrush backgroundBrush = palette.base();
    QBrush headerBrush = palette.text();
    QBrush textBrush = palette.text();
    QFont headerFont = opt.font;
    headerFont.setBold(m_showSceneDescription ? true : false);
    QFont textFont = opt.font;
    textFont.setBold(false);
#endif
    //
    // ... для выделенных элементов
    //
    if (opt.state.testFlag(QStyle::State_Selected)) {
        backgroundBrush = palette.highlight();
        headerBrush = palette.highlightedText();
        textBrush = palette.highlightedText();
    }
    //
    // ... для остальных
    //
    else {
        //
        // Реализация альтернативных цветов в представлении
        //
        if (opt.features.testFlag(QStyleOptionViewItem::Alternate)) {
            backgroundBrush = palette.alternateBase();
        } else {
#ifdef MOBILE_OS
            backgroundBrush = palette.window();
#else
            backgroundBrush = palette.base();
#endif
        }
        headerBrush = palette.windowText();
    }

    //
    // Рисуем
    //
    const int TREE_INDICATOR_WIDTH = StyleSheetHelper::dpToPx(20);
    const int COLOR_RECT_WIDTH = StyleSheetHelper::dpToPx(12);
    const int RIGHT_MARGIN = StyleSheetHelper::dpToPx(12);
    const int TEXT_LINE_HEIGHT = _painter->fontMetrics().height();

    //
    // ... фон
    //
    _painter->fillRect(opt.rect, backgroundBrush);

#ifndef MOBILE_OS
    //
    // ... разделитель
    //
    QPoint borderLeft = opt.rect.bottomLeft();
    borderLeft.setX(0);
    _painter->setPen(QPen(palette.dark(), 0.5));
    _painter->drawLine(borderLeft, opt.rect.bottomRight());
#endif

    //
    // Меняем координаты, чтобы рисовать было удобнее
    //
    _painter->translate(opt.rect.topLeft());

    //
    // ... иконка
    //
    int iconRectX = QLocale().textDirection() == Qt::LeftToRight
                    ? 0
                    : TREE_INDICATOR_WIDTH + opt.rect.width() - m_iconSize - m_itemsHorizontalSpacing - RIGHT_MARGIN;
    const QRect iconRect(iconRectX, m_iconTopMargin, m_iconSize, m_iconSize);
    QPixmap icon = _index.data(Qt::DecorationRole).value<QPixmap>();
    QIcon iconColorized(icon);
    QColor iconColor = headerBrush.color();
    // ... если есть заметка, рисуем красноватым цветом
    if (_index.data(BusinessLogic::ScenarioModel::HasNoteIndex).toBool()) {
        iconColor = QColor("#ec3838");
    }
    ImageHelper::setIconColor(iconColorized, iconRect.size(), iconColor);
    icon = iconColorized.pixmap(iconRect.size());
    _painter->drawPixmap(iconRect, icon);

    //
    // ... цвета сцены
    //
    const QString colorsNames = _index.data(BusinessLogic::ScenarioModel::ColorIndex).toString();
    QStringList colorsNamesList = colorsNames.split(";", QString::SkipEmptyParts);
    int colorsCount = colorsNamesList.size();
    int colorRectX = QLocale().textDirection() == Qt::LeftToRight
                     ? TREE_INDICATOR_WIDTH + opt.rect.width() - COLOR_RECT_WIDTH - m_itemsHorizontalSpacing - RIGHT_MARGIN
                     : 0;
    if (colorsCount > 0) {
        //
        // Если цвет один, то просто рисуем его
        //
        if (colorsCount == 1) {
            const QColor color(colorsNamesList.first());
            const QRectF colorRect(colorRectX, 0, COLOR_RECT_WIDTH, opt.rect.height());
            _painter->fillRect(colorRect, color);
        }
        //
        // Если цветов много
        //
        else {
            //
            // ... первый цвет рисуем на всю вышину в первой колонке
            //
            {
                colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? -1 : 1) * COLOR_RECT_WIDTH;
                const QColor color(colorsNamesList.takeFirst());
                const QRectF colorRect(colorRectX, 0, COLOR_RECT_WIDTH, opt.rect.height());
                _painter->fillRect(colorRect, color);
            }
            //
            // ... остальные цвета рисуем во второй колонке цветов
            //
            colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? 1 : -1) * COLOR_RECT_WIDTH;
            colorsCount = colorsNamesList.size();
            for (int colorIndex = 0; colorIndex < colorsCount; ++colorIndex) {
                const QString colorName = colorsNamesList.takeFirst();
                const QColor color(colorName);
                const QRectF colorRect(
                            colorRectX,
                            opt.rect.height() / qreal(colorsCount) * colorIndex,
                            COLOR_RECT_WIDTH,
                            opt.rect.height() / qreal(colorsCount)
                            );
                _painter->fillRect(colorRect, color);
            }
            //
            // ... смещаем позицию назад, для корректной отрисовки остального контента
            //
            colorRectX += (QLocale().textDirection() == Qt::LeftToRight ? -1 : 1) * COLOR_RECT_WIDTH;
        }
    }

    //
    // ... текстовая часть
    //

    //
    // ... длительность
    //
    _painter->setPen(textBrush.color());
    _painter->setFont(textFont);
    const int duration = _index.data(BusinessLogic::ScenarioModel::DurationIndex).toInt();
    const QString chronometry =
            BusinessLogic::ChronometerFacade::chronometryUsed()
            ? "(" + BusinessLogic::ChronometerFacade::secondsToTime(duration)+ ") "
            : "";
    const int chronometryRectWidth = _painter->fontMetrics().width(chronometry);
    const QRect chronometryRect(
        QLocale().textDirection() == Qt::LeftToRight
                ? colorRectX - chronometryRectWidth - m_itemsHorizontalSpacing
                : colorRectX + COLOR_RECT_WIDTH + m_itemsHorizontalSpacing,
        m_topMargin,
        chronometryRectWidth,
        TEXT_LINE_HEIGHT
        );
    _painter->drawText(chronometryRect, Qt::AlignLeft | Qt::AlignVCenter, chronometry);

    //
    // ... заголовок
    //
    _painter->setPen(headerBrush.color());
    _painter->setFont(headerFont);
    const QRect headerRect(
        QLocale().textDirection() == Qt::LeftToRight
                ? iconRect.right() + m_itemsHorizontalSpacing
                : chronometryRect.right() + m_itemsHorizontalSpacing,
        m_topMargin,
        QLocale().textDirection() == Qt::LeftToRight
                ? chronometryRect.left() - iconRect.right() - m_itemsHorizontalSpacing*2
                : iconRect.left() - chronometryRect.right() - m_itemsHorizontalSpacing*2,
        TEXT_LINE_HEIGHT
        );
    QString header = TextEditHelper::smartToUpper(_index.data(Qt::DisplayRole).toString());
    if (m_showSceneTitle) {
        //
        // Если нужно выводим название сцены вместо заголовка
        //
        const QString title = TextEditHelper::smartToUpper(_index.data(BusinessLogic::ScenarioModel::TitleIndex).toString());
        if (!title.isEmpty()) {
            header = title;
        }
    }
    if (m_showSceneNumber) {
        //
        // Если нужно добавляем номер сцены
        //
        QVariant sceneNumber = _index.data(BusinessLogic::ScenarioModel::SceneNumberIndex);
        if (!sceneNumber.isNull()) {
            header = m_sceneNumbersPrefix + sceneNumber.toString() + ". " + header;
        }
    }
    header = _painter->fontMetrics().elidedText(header, Qt::ElideRight, headerRect.width());
    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, header);

    //
    // ... описание
    //
    if (m_showSceneDescription) {
        _painter->setPen(textBrush.color());
        _painter->setFont(textFont);
        const QRect descriptionRect(
            QLocale().textDirection() == Qt::LeftToRight
                    ? headerRect.left()
                    : chronometryRect.left(),
            headerRect.bottom() + m_itemsVerticalSpacing,
            QLocale().textDirection() == Qt::LeftToRight
                    ? chronometryRect.right() - headerRect.left()
                    : headerRect.right() - chronometryRect.left(),
            TEXT_LINE_HEIGHT * m_sceneDescriptionHeight
            );
        const QString descriptionText =
                m_sceneDescriptionIsSceneText
                ? _index.data(BusinessLogic::ScenarioModel::SceneTextIndex).toString()
                : _index.data(BusinessLogic::ScenarioModel::DescriptionIndex).toString();
        _painter->drawText(descriptionRect, Qt::AlignLeft | Qt::TextWordWrap, descriptionText);
    }

    _painter->restore();
}

QSize ScenarioNavigatorItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    Q_UNUSED(_option);
    Q_UNUSED(_index);

    //
    // Размер составляется из лейблов
    // строка на заголовок (ICON_SIZE) и m_sceneDescriptionHeight строк на описание
    // + отступы TOP_MARGIN сверху + BOTTOM_MARGIN снизу + ITEMS_SPACING между текстом
    //
    int lines = 0;
#ifdef MOBILE_OS
    int additionalHeight = m_topMargin + _option.fontMetrics.height() + m_bottomMargin;
#else
    int additionalHeight = m_topMargin + m_iconSize + m_bottomMargin;
#endif
    if (m_showSceneDescription) {
        lines += m_sceneDescriptionHeight;
        additionalHeight += m_itemsVerticalSpacing;
    }
    const int height = _option.fontMetrics.height() * lines + additionalHeight;
    const int width = StyleSheetHelper::dpToPx(50);
    return QSize(width, height);
}

void ScenarioNavigatorItemDelegate::setShowSceneNumber(bool _show)
{
    m_showSceneNumber = _show;
}

void ScenarioNavigatorItemDelegate::setShowSceneTitle(bool _show)
{
    m_showSceneTitle = _show;
}

void ScenarioNavigatorItemDelegate::setShowSceneDescription(bool _show)
{
    m_showSceneDescription = _show;
}

void ScenarioNavigatorItemDelegate::setSceneDescriptionIsSceneText(bool _isSceneText)
{
    m_sceneDescriptionIsSceneText = _isSceneText;
}

void ScenarioNavigatorItemDelegate::setSceneDescriptionHeight(int _height)
{
    m_sceneDescriptionHeight = _height;
}

void ScenarioNavigatorItemDelegate::setSceneNumbersPrefix(const QString& _prefix)
{
    m_sceneNumbersPrefix = _prefix;
}
