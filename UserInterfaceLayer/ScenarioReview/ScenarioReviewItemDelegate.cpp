#include "ScenarioReviewItemDelegate.h"

#include <BusinessLayer/ScenarioDocument/ScenarioReviewModel.h>

#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QDateTime>
#include <QLabel>
#include <QPainter>

using BusinessLogic::ScenarioReviewModel;

using UserInterface::ScenarioReviewItemDelegate;

namespace {
    /**
     * @brief Ширина цветовой метки
     */
    static int kColorMarkWidth() {
        static const int s_colorMarkWidth = StyleSheetHelper::dpToPx(12);
        return s_colorMarkWidth;
    }

    /**
     * @brief Расстояния
     */
#ifndef MOBILE_OS
    const int TOP_MARGIN = 8;
    const int SPACING = 8;
    const int BOTTOM_MARGIN = 8;
    const int RIGHT_MARGIN = 3;
#else
    static int kTopMargin() {
        static const int s_topMargin = StyleSheetHelper::dpToPx(15);
        return s_topMargin;
    }
    static int kSpacing() {
        static const int s_spacing = StyleSheetHelper::dpToPx(6);
        return s_spacing;
    }
    static int kBottomMargin() {
        static const int s_bottomMargin = StyleSheetHelper::dpToPx(15);
        return s_bottomMargin;
    }
    static int kRightMargin() {
        static const int s_rightMargin = StyleSheetHelper::dpToPx(8);
        return s_rightMargin;
    }
#endif

    /**
     * @brief Рассчитать высоту текста по заданной ширине
     */
    static int heightForWidth(const QString& _text, int _width) {
        static QLabel s_label;
        s_label.setWordWrap(true);

        int height = 0;
        if (!_text.isEmpty()) {
            s_label.setText(_text);
            height = s_label.heightForWidth(_width);
        }
        return height;
    }

    /**
     * @brief Рассчитать высоту комментария по заданной ширине области элемента
     */
    static int commentHeightForWidth(const QString& _text, int _width) {
        //
        // Рассчитаем ширину, которую займёт комментарий
        //
        const int commentWidth = _width - kColorMarkWidth() - kSpacing() - kRightMargin();
        return heightForWidth(_text, commentWidth);
    }
}


int ScenarioReviewItemDelegate::commentIndexFor(const QModelIndex& _index, int _y, QWidget* _widget)
{
    int width = _widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_widget)) {
        width = view->viewport()->width();
    }

    //
    // Считаем высоту
    //
    // ... высота заголовка: отступ сверху + две строки (автор и дата) + отступ снизу
    //
    const int headerHeight = kTopMargin() + _widget->fontMetrics().height() * 2 + kBottomMargin();
    //
    // ... полная высота
    //
    int height = 0;
    if (_index.data(ScenarioReviewModel::IsDoneRole).toBool() == false) {
        const QStringList comments = _index.data(ScenarioReviewModel::CommentsRole).toStringList();
        foreach (const QString& comment, comments) {
            const int top = height;
            height += headerHeight + ::commentHeightForWidth(comment, width) + kSpacing();
            const int bottom = height;

            if (top <= _y && _y <= bottom) {
                return comments.indexOf(comment);
            }
        }
    }

    return 0;
}

ScenarioReviewItemDelegate::ScenarioReviewItemDelegate(QObject* _parent) :
    QStyledItemDelegate(_parent)
{
}

void ScenarioReviewItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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
    QColor backgroundColor = opt.palette.background().color();
    QColor replyBackgroundColor = opt.palette.alternateBase().color();
    QColor textColor = opt.palette.windowText().color();
    QColor replyColor = opt.palette.windowText().color();
    QColor dateColor = opt.palette.mid().color();
    QFont headerFont = opt.font;
    headerFont.setWeight(QFont::Medium);
    headerFont.setPointSize(16);
    QFont dateFont = opt.font;
    dateFont.setWeight(QFont::Medium);
    dateFont.setPointSize(12);
    QFont textFont = QApplication::font();
    textFont.setPointSize(14);
#else
    QColor backgroundColor = opt.palette.base().color();
    QColor replyBackgroundColor = opt.palette.alternateBase().color();
    QColor textColor = opt.palette.windowText().color();
    QColor replyColor = opt.palette.windowText().color();
    QColor dateColor = opt.palette.dark().color();
    QFont headerFont = opt.font;
    headerFont.setBold(true);
    QFont dateFont = opt.font;
    dateFont.setBold(true);
#ifdef Q_OS_WIN
    dateFont.setPointSize(dateFont.pointSize() - 1);
#else
    dateFont.setPointSize(dateFont.pointSize() - 4);
#endif
    QFont textFont = QApplication::font();
#endif
    //
    // ... для выделенных элементов
    //
    if (opt.state.testFlag(QStyle::State_Selected)) {
        backgroundColor = opt.palette.highlight().color();
        textColor = opt.palette.highlightedText().color();
        dateColor = textColor.darker(135);
        replyBackgroundColor = backgroundColor;
        replyColor = textColor;
    }

    //
    // Рисуем
    //
    const int HEADER_LINE_HEIGHT = QFontMetrics(headerFont).height();
    const int DATE_LINE_HEIGHT = QFontMetrics(dateFont).height();
    //
    // Меняем координаты, чтобы рисовать было удобнее
    //
    _painter->translate(opt.rect.topLeft());

    //
    // Определим данные
    //
    const bool done  = _index.data(ScenarioReviewModel::IsDoneRole).toBool();
    QStringList authors = _index.data(ScenarioReviewModel::CommentsAuthorsRole).toStringList();
    const QStringList dates = _index.data(ScenarioReviewModel::CommentsDatesRole).toStringList();
    const QStringList comments = _index.data(ScenarioReviewModel::CommentsRole).toStringList();
    //
    // ... если не задано ни одного автора, то эмулируем одного, для формирования пустой строки
    //
    if (authors.isEmpty()) {
        authors.append("");
    }

    //
    // Для каждого комментария
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    const int headerHeight = kTopMargin() + _option.fontMetrics.height() * 2 + kBottomMargin();
    int lastTop = 0;
    for (int commentIndex = 0; commentIndex < authors.size(); ++commentIndex) {
        //
        // Определим область комментария
        //
        int height = headerHeight;
        if (!done) {
            height += ::commentHeightForWidth(comments.value(commentIndex), width) + kSpacing();
        }
        const QRect rect(0, lastTop, width, height);

        //
        // ... фон
        //
        _painter->fillRect(rect, commentIndex == 0 ? backgroundColor : replyBackgroundColor);

        //
        // ... цвет заметки
        //
        const QColor color = _index.data(Qt::DecorationRole).value<QColor>();
        const int colorRectX = QLocale().textDirection() == Qt::LeftToRight ? 0 : width - kColorMarkWidth();
        const QRect colorRect(colorRectX, lastTop, kColorMarkWidth(), height);
        _painter->fillRect(colorRect, commentIndex == 0 ? color : replyBackgroundColor);

#ifndef MOBILE_OS
        //
        // ... разделитель
        //
        const QPoint left(0, lastTop + height);
        const QPoint right(rect.right(), lastTop + height);
        _painter->setPen(QPen(opt.palette.dark(), 1));
        _painter->drawLine(left, right);
#endif

        //
        // ... автор
        //
        _painter->setPen(commentIndex == 0 ? textColor : replyColor);
        _painter->setFont(headerFont);
        const QRect headerRect(
            QLocale().textDirection() == Qt::LeftToRight
                    ? colorRect.right() + kSpacing()
                    : kRightMargin(),
            lastTop + kTopMargin(),
            QLocale().textDirection() == Qt::LeftToRight
                    ? width - colorRect.right() - kSpacing() - kRightMargin()
                    : colorRect.left() - kSpacing(),
            HEADER_LINE_HEIGHT
            );
        _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, authors.value(commentIndex));

        //
        // ... дата
        //
        _painter->setPen(dateColor);
        _painter->setFont(dateFont);
        const QRect dateRect(
            QLocale().textDirection() == Qt::LeftToRight
                    ? colorRect.right() + kSpacing()
                    : kRightMargin(),
            headerRect.bottom() + 2,
            QLocale().textDirection() == Qt::LeftToRight
                    ? width - colorRect.right() - kSpacing() - kRightMargin()
                    : colorRect.left() - kSpacing(),
            DATE_LINE_HEIGHT
            );
        const QString date = QDateTime::fromString(dates.value(commentIndex), Qt::ISODate).toString("dd.MM.yyyy hh:mm");
        _painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignVCenter, date);

        //
        // ... если комментарий исправлен на этом месте завершаем рисование
        //
        if (done) {
            break;
        }

        //
        // ... комментарий
        //
        _painter->setPen(commentIndex == 0 ? textColor : replyColor);
        _painter->setFont(textFont);
        const QRect commentRect(
            QLocale().textDirection() == Qt::LeftToRight
                    ? colorRect.right() + kSpacing()
                    : kRightMargin(),
            dateRect.bottom() + kSpacing(),
            QLocale().textDirection() == Qt::LeftToRight
                    ? width - colorRect.right() - kSpacing() - kRightMargin()
                    : colorRect.left() - kSpacing(),
            height - headerHeight - kSpacing()
            );
        _painter->drawText(commentRect, Qt::TextWordWrap, comments.value(commentIndex));

        lastTop += height;
    }

    _painter->restore();
}

QSize ScenarioReviewItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    QSize size = QStyledItemDelegate::sizeHint(_option, _index);

    if (_option.widget != 0) {
        //
        // Ширина
        //
        int width = _option.widget->width();
        if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
            width = view->viewport()->width();
        }

        //
        // Считаем высоту
        //
        // ... высота заголовка: отступ сверху + две строки (автор и дата) + отступ снизу
        //
        const int headerHeight = kTopMargin() + _option.fontMetrics.height() * 2 + kBottomMargin();
        //
        // ... полная высота
        //
        int height = 0;
        if (_index.data(ScenarioReviewModel::IsDoneRole).toBool() == false) {
            const QStringList comments = _index.data(ScenarioReviewModel::CommentsRole).toStringList();
            foreach (const QString& comment, comments) {
                height += headerHeight + ::commentHeightForWidth(comment, width) + kSpacing();
            }
        }
        if (height == 0) {
            height = headerHeight;
        }

        size = QSize(width, height);
    }

    return size;
}

