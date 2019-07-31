#include "GoogleColorsPane.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QPainter>
#include <QMouseEvent>

namespace {
    /**
     * @brief Количество колонок с цветовыми квадратами
     */
    const int kColorRectColumns = 10;

    /**
     * @brief Количество строк с цветовыми квадратами
     */
    const int kColorRectRows = 8;

#ifdef MOBILE_OS
    /**
     * @brief Размер ребра квадрата с цветом
     */
    static int kColorRectSize() {
        static const int s_colorRectSize = StyleSheetHelper::dpToPx(46);
        return s_colorRectSize;
    }

    /**
     * @brief Расстояние между двумя соседними квадратами с цветом
     */
    const int kColorRectSpace = 0;

    /**
     * @brief Отступы панели
     */
    const int kPanelMargin = 0;

    /**
     * @brief Размер метки текущего цвета
     */
    static int kColorMarkSize() {
        static const int s_colorMarkSize = StyleSheetHelper::dpToPx(16);
        return s_colorMarkSize;
    }

    /**
     * @brief Ширина рамки метки текущего цвета
     */
    static int kMarkBorderWidth() {
        static const int s_markBorderWidth = StyleSheetHelper::dpToPx(3);
        return s_markBorderWidth;
    }
#else
    /**
     * @brief Размер ребра квадрата с цветом
     */
    static int kColorRectSize() { return 20; }

    /**
     * @brief Расстояние между двумя соседними квадратами с цветом
     */
    const int kColorRectSpace = 3;

    /**
     * @brief Отступы панели
     */
    const int kPanelMargin = 16;

    /**
     * @brief Размер метки текущего цвета
     */
    static int kColorMarkSize() { return 5; }
#endif
}


GoogleColorsPane::GoogleColorsPane(QWidget* _parent) :
    ColorsPane(_parent)
{
    //
    // Рассчитаем фиксированный размер панели
    //
    const int width =
            (kColorRectSize() * kColorRectColumns) // ширина цветовых квадратов
            + (kColorRectSpace * (kColorRectColumns - 1)) // ширина оступов между квадратами
            + (kPanelMargin * 2); // ширина полей
    const int height =
            (kColorRectSize() * kColorRectRows) // высота цветовых квадратов
            + (kColorRectSpace * (kColorRectRows + 3)) // высота отступов между ними (+3 т.к. между 1, 2 и 3 увеличенные отступы)
            + (kPanelMargin * 2); // высота полей
    setFixedSize(width, height);

    //
    // Отслеживаем движения мыши
    //
    setMouseTracking( true );

    //
    // Настроим курсор
    //
    setCursor(Qt::PointingHandCursor);

    initColors();
}

void GoogleColorsPane::selectFirstEnabledColor()
{
    const int startEnabledColorIndex = 10;
    for (int colorIndex = startEnabledColorIndex; colorIndex < colorsInfos().size(); ++colorIndex) {
        const auto& colorInfo = colorsInfos()[colorIndex];
        if (colorInfo.isEnabled) {
            setCurrentColor(colorInfo.color);
            break;
        }
    }
}

void GoogleColorsPane::paintEvent(QPaintEvent * _event)
{
    Q_UNUSED(_event);

    QPainter painter( this );

    //
    // Рисуем квадратики с цветами
    //
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    for (const auto& colorInfo : colorsInfos()) {
        //
        // Неактивный
        //
        if (colorInfo.isEnabled == false) {
            continue;
        }

        //
        // Сам цвет
        //
        painter.fillRect(colorInfo.rect, colorInfo.color);

        //
        // Под мышкой
        //
        if (colorInfo.rect.contains(mousePos)) {
            QRectF borderRect = colorInfo.rect;
            borderRect.setTop(borderRect.top() - 2);
            borderRect.setLeft(borderRect.left() - 2);
            borderRect.setBottom(borderRect.bottom() + 1);
            borderRect.setRight(borderRect.right() + 1);

            painter.save();
            painter.setPen(palette().text().color());
            painter.drawRect(borderRect);
            painter.restore();
        }
    }

    //
    // Текущий
    //
    if (currentColorInfo().isValid()) {
        //
        // ... рамка
        //
        QRectF borderRect = currentColorInfo().rect;
        borderRect.setTop(borderRect.top() - 2);
        borderRect.setLeft(borderRect.left() - 2);
        borderRect.setBottom(borderRect.bottom() + 1);
        borderRect.setRight(borderRect.right() + 1);

        painter.setPen(palette().text().color());
        painter.drawRect(borderRect);

        //
        // ... метка в центре
        //
        const QPointF center = borderRect.center();
        QRectF markRect(center.x() - kColorMarkSize() / 2, center.y() - kColorMarkSize() / 2,
            kColorMarkSize(), kColorMarkSize());
        painter.fillRect(markRect, palette().text());
        painter.setPen(palette().base().color());
        painter.drawRect(markRect);
    }
}

void GoogleColorsPane::mouseMoveEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    repaint();
}

void GoogleColorsPane::mousePressEvent(QMouseEvent* _event)
{
    for (int colorIndex = 0; colorIndex < colorsInfos().count(); ++colorIndex) {
        if (colorsInfos()[colorIndex].rect.contains(_event->pos())) {
            //
            // Ничего не делаем, если пользователь хочет выбрать недоступный цвет
            //
            if (colorsInfos()[colorIndex].isEnabled == false) {
                break;
            }

            currentColorInfo() = colorsInfos()[colorIndex];
            emit selected(currentColorInfo().color);

            repaint();
            break;
        }
    }
}

void GoogleColorsPane::initColors()
{
    //
    // Формируем первый ряд
    //
    int topMargin = kPanelMargin;
    int leftMargin = kPanelMargin;
    QList<QColor> colors;
    colors << QColor(0,0,0)
           << QColor(67,67,67)
           << QColor(102,102,102)
           << QColor(153,153,153)
           << QColor(183,183,183)
           << QColor(204,204,204)
           << QColor(217,217,217)
           << QColor(239,239,239)
           << QColor(243,243,243)
           << QColor(255,255,255);
    for (int column = 0; column < kColorRectColumns; ++column) {
        QRectF colorRect;
        colorRect.setLeft(leftMargin);
        colorRect.setTop(topMargin);
        colorRect.setWidth(kColorRectSize());
        colorRect.setHeight(kColorRectSize());

        colorsInfos().append(ColorKeyInfo(colors.at(column), colorRect));

        leftMargin += kColorRectSize() + kColorRectSpace;
    }
    topMargin += kColorRectSize() + (kColorRectSpace * 3);


    //
    // Второй ряд
    //
    leftMargin = kPanelMargin;
    colors.clear();
    colors << QColor(152,0,0)
           << QColor(255,0,0)
           << QColor(255,135,0)
           << QColor(255,255,0)
           << QColor(0,255,0)
           << QColor(0,255,255)
           << QColor(74,134,232)
           << QColor(0,0,255)
           << QColor(153,0,255)
           << QColor(255,0,255);
    for (int column = 0; column < kColorRectColumns; ++column) {
        QRectF colorRect;
        colorRect.setLeft(leftMargin);
        colorRect.setTop(topMargin);
        colorRect.setWidth(kColorRectSize());
        colorRect.setHeight(kColorRectSize());

        colorsInfos().append(ColorKeyInfo(colors.at(column), colorRect));

        leftMargin += kColorRectSize() + kColorRectSpace;
    }
    topMargin += kColorRectSize() + (kColorRectSpace * 3);


    //
    // Остальные ряды
    //
    colors.clear();
    colors << QColor("#e6b8af")
           << QColor("#f4cccc")
           << QColor("#fce5cd")
           << QColor("#fff2cc")
           << QColor("#d9ead3")
           << QColor("#d0e0e3")
           << QColor("#c9daf8")
           << QColor("#cfe2f3")
           << QColor("#d9d2e9")
           << QColor("#ead1dc") // ****
           << QColor("#da7d6a")
           << QColor("#ea9999")
           << QColor("#f9cb9c")
           << QColor("#ffe599")
           << QColor("#b6d7a8")
           << QColor("#a2c4c9")
           << QColor("#a4c2f4")
           << QColor("#9fc5e8")
           << QColor("#b4a7d6")
           << QColor("#d5a6bd") // ****
           << QColor("#c63f24")
           << QColor("#e06666")
           << QColor("#f6b26b")
           << QColor("#ffd966")
           << QColor("#93c47d")
           << QColor("#76a5af")
           << QColor("#6d9eeb")
           << QColor("#6fa8dc")
           << QColor("#8e7cc3")
           << QColor("#c27ba0") // ****
           << QColor("#a21b00")
           << QColor("#cc0000")
           << QColor("#e69138")
           << QColor("#f1c232")
           << QColor("#6aa84f")
           << QColor("#45818e")
           << QColor("#3c78d8")
           << QColor("#3d85c6")
           << QColor("#674ea7")
           << QColor("#a64d79") // ****
           << QColor("#831f0c")
           << QColor("#990000")
           << QColor("#b45f06")
           << QColor("#bf9000")
           << QColor("#38761d")
           << QColor("#134f5c")
           << QColor("#1155cc")
           << QColor("#0b5394")
           << QColor("#351c75")
           << QColor("#741b47") // ****
           << QColor("#5a0f00")
           << QColor("#660000")
           << QColor("#783f04")
           << QColor("#7f6000")
           << QColor("#274e13")
           << QColor("#0c343d")
           << QColor("#1c4587")
           << QColor("#073763")
           << QColor("#20124d")
           << QColor("#4c1130");
    for (int row = 0; row < (kColorRectRows - 2); ++row) {
        leftMargin = kPanelMargin;
        for (int column = 0; column < kColorRectColumns; ++column) {
            QRectF colorRect;
            colorRect.setLeft(leftMargin);
            colorRect.setTop(topMargin);
            colorRect.setWidth(kColorRectSize());
            colorRect.setHeight(kColorRectSize());

            colorsInfos().append(ColorKeyInfo(colors.at((row * kColorRectColumns) + column), colorRect));

            leftMargin += kColorRectSize() + kColorRectSpace;
        }
        topMargin += kColorRectSize() + kColorRectSpace;
    }
}
