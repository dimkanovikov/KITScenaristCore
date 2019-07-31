#include "CircularProgressBar.h"

#include <QPainter>


CircularProgressBar::CircularProgressBar(QWidget* _parent)
    : QWidget(_parent)
{

}

void CircularProgressBar::setValue(qreal _value)
{
    m_value = _value;
    update();
}

void CircularProgressBar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int progressWidth = 15;
    const int minSideLength = qMin(width(), height()) - progressWidth;
    const QRectF rectangle(static_cast<qreal>(width() - minSideLength) / 2,
                           static_cast<qreal>(height() - minSideLength) / 2,
                           minSideLength,
                           minSideLength);

    QPen pen;
    pen.setWidth(progressWidth);
    painter.setPen(pen);

    // Фон
    pen.setColor(QColor("#b3c4da"));
    painter.setPen(pen);
    int startAngle = 90 * 16;
    int spanAngle = 360 * 360 * 16;
    painter.drawArc(rectangle, startAngle, spanAngle);

    // Заполнение
    pen.setColor(palette().highlight().color());
    painter.setPen(pen);
    startAngle = 90 * 16;
    qreal valueCorrected = m_value;
    while (valueCorrected > 1.0) {
        valueCorrected -= 1.0;
    }
    spanAngle = valueCorrected * 360 * 16;
    painter.drawArc(rectangle, startAngle, spanAngle);

    // Текст
    pen.setColor(palette().text().color());
    painter.setPen(pen);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(rectangle, Qt::AlignCenter, QString::number(m_value*100, 'f', 1)+"%");
}
