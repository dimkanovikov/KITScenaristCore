#include "WordHighlightColorsPane.h"

#include <3rd_party/Helpers/StyleSheetHelper.h>

#include <QPainter>
#include <QMouseEvent>

namespace {
    /**
     * @brief Количество колонок с цветовыми квадратами
     */
    const int kColorRectColumns = 5;

    /**
     * @brief Количество строк с цветовыми квадратами
     */
    const int kColorRectRows = 3;

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


WordHighlightColorsPane::WordHighlightColorsPane(QWidget* _parent) :
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
            + (kColorRectSpace * (kColorRectRows)) // высота отступов между ними (+3 т.к. между 1, 2 и 3 увеличенные отступы)
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

QColor WordHighlightColorsPane::currentColor() const
{
    return m_currentColorInfo.color;
}

bool WordHighlightColorsPane::contains(const QColor& _color) const
{
    bool contains = false;
    foreach (const ColorKeyInfo& _colorKeyInfo, m_colorInfos) {
        if (_colorKeyInfo.color == _color) {
            contains = true;
            break;
        }
    }
    return contains;
}

void WordHighlightColorsPane::setCurrentColor(const QColor& _color)
{
    for (int colorIndex = 0; colorIndex < m_colorInfos.count(); ++colorIndex) {
        if (m_colorInfos[colorIndex].color == _color) {
            m_currentColorInfo = m_colorInfos[colorIndex];
            break;
        }
    }
}

void WordHighlightColorsPane::paintEvent(QPaintEvent * _event)
{
    Q_UNUSED(_event);

    QPainter painter( this );


    //
    // Рисуем панель
    //
    foreach (const ColorKeyInfo& colorInfo, m_colorInfos) {
        painter.fillRect(colorInfo.rect, colorInfo.color);
    }

    //
    // Обрамление
    //
    const QPoint mousePos = mapFromGlobal(QCursor::pos());
    for(int colorIndex = 0; colorIndex < m_colorInfos.count(); ++colorIndex) {
        if(m_colorInfos[colorIndex].rect.contains(mousePos))
        {
            QRectF borderRect = m_colorInfos[colorIndex].rect;
            borderRect.setTop(borderRect.top() - 2);
            borderRect.setLeft(borderRect.left() - 2);
            borderRect.setBottom(borderRect.bottom() + 1);
            borderRect.setRight(borderRect.right() + 1);

            painter.setPen(palette().text().color());
            painter.drawRect(borderRect);

            break;
        }
    }

    //
    // Текущий
    //
    if (m_currentColorInfo.isValid()) {
#ifdef MOBILE_OS
        //
        // ... метка в центре
        //
        const QPointF center = m_currentColorInfo.rect.center();
        QRectF markRect(center.x() - kColorMarkSize() / 2, center.y() - kColorMarkSize() / 2,
            kColorMarkSize(), kColorMarkSize());
        painter.fillRect(markRect, palette().text());
        painter.setPen(QPen(palette().base().color(), kMarkBorderWidth()));
        painter.drawRect(markRect);
#else
        //
        // ... рамка
        //
        QRectF borderRect = m_currentColorInfo.rect;
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
#endif
    }
}

void WordHighlightColorsPane::mouseMoveEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    repaint();
}

void WordHighlightColorsPane::mousePressEvent(QMouseEvent* _event)
{
    for (int colorIndex = 0; colorIndex < m_colorInfos.count(); ++colorIndex) {
        if (m_colorInfos[colorIndex].rect.contains(_event->pos())) {
            m_currentColorInfo = m_colorInfos[colorIndex];
            emit selected(m_currentColorInfo.color);

            repaint();
            break;
        }
    }
}

void WordHighlightColorsPane::initColors()
{
    //
    // Формируем цвета
    //
    int topMargin = kPanelMargin;
    int leftMargin = kPanelMargin;
    QList<QColor> colors;
    colors << QColor("#ffff00")
           << QColor("#00ff00")
           << QColor("#00ffff")
           << QColor("#ff00ff")
           << QColor("#0000ff") // ****
           << QColor("#ff0000")
           << QColor("#000080")
           << QColor("#008080")
           << QColor("#008000")
           << QColor("#800080") // ****
           << QColor("#800000")
           << QColor("#808000")
           << QColor("#808080")
           << QColor("#c0c0c0")
           << QColor("#000000");
    for (int row = 0; row < kColorRectRows; ++row) {
        leftMargin = kPanelMargin;
        for (int column = 0; column < kColorRectColumns; ++column) {
            QRectF colorRect;
            colorRect.setLeft(leftMargin);
            colorRect.setTop(topMargin);
            colorRect.setWidth(kColorRectSize());
            colorRect.setHeight(kColorRectSize());

            m_colorInfos.append(ColorKeyInfo(colors.at((row * kColorRectColumns) + column), colorRect));

            leftMargin += kColorRectSize() + kColorRectSpace;
        }
        topMargin += kColorRectSize() + kColorRectSpace;
    }
}
