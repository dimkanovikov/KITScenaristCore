#include "Stepper.h"

#include <QLocale>
#include <QPainter>

namespace {
    const qreal stepRadius = 6;
    const qreal stepDiameter = 2 * stepRadius;
    const qreal stepMargin = stepRadius;
    static qreal stepsWidth(int _count) {
        return _count * stepDiameter + (_count - 1) * stepMargin;
    }
}


Stepper::Stepper(QWidget *_parent) :
    QWidget(_parent)
{
    m_animationTimeline.setDuration(300);
    m_animationTimeline.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_animationTimeline, &QTimeLine::frameChanged, [this] (int _frame) {
        m_currentStepAnimationPosition = _frame;
        update();
    });
}

void Stepper::setStepsCount(int _count)
{
    if (m_stepsCount != _count) {
        m_stepsCount = _count;
        if (m_currentStep >= _count) {
            setCurrentStep(_count - 1);
        }

        update();
    }
}

void Stepper::setCurrentStep(int _step)
{
    if (_step < m_stepsCount
        && m_currentStep != _step) {
        m_animationTimeline.setStartFrame(m_currentStep * 100);
        m_currentStep = _step;
        m_animationTimeline.setEndFrame(m_currentStep * 100);
        m_animationTimeline.start();

        update();
    }
}

QSize Stepper::sizeHint() const
{
    // +2 - по пикселю слева и справа
    return QSize(::stepsWidth(m_stepsCount) + 2, ::stepDiameter);
}

void Stepper::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    const qreal stepsWidth = ::stepsWidth(m_stepsCount);
    const qreal startX = width() > stepsWidth
                        ? (width() - stepsWidth) / 2 + stepRadius
                        : stepRadius;
    const qreal startY = height() > stepDiameter
                        ? (height() - stepDiameter) / 2 + stepRadius
                        : stepRadius;
    //
    // Найдём расположение центров точек шагов
    //
    QVector<QPointF> points(m_stepsCount);
    for (int index = 0; index < m_stepsCount; ++index) {
        points[index] = QPointF{startX + index * (stepDiameter + stepMargin), startY};
    }

    //
    // Рисуем
    //
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //
    // ... закрасим фон
    //
    painter.fillRect(rect(), palette().window());
    //
    // ... нарисуем точки
    //
    painter.setPen(palette().dark().color());
    painter.setBrush(palette().base());
    for (const QPointF& point : points) {
        painter.drawEllipse(point, stepRadius, stepRadius);
    }
    //
    // ... закрасим пройденные
    //
    painter.setPen(QPen(palette().highlight(), 1));
    painter.setBrush(palette().highlight());
    if (m_currentStepAnimationPosition >= 0) {
        QPointF firstPoint;
        QPointF lastPoint;
        if (QLocale().textDirection() == Qt::LeftToRight) {
            firstPoint = points.first();
            lastPoint.setX(firstPoint.x()
                           + (m_currentStepAnimationPosition / 100) * (::stepDiameter + ::stepMargin) // от первого до последнего целого
                           + (qreal(m_currentStepAnimationPosition % 100) / 100) * (::stepDiameter + ::stepMargin) // динамическая часть
                           );
            lastPoint.setY(firstPoint.y());
        } else {
            lastPoint = points.last();
            firstPoint.setX(lastPoint.x()
                           - (m_currentStepAnimationPosition / 100) * (::stepDiameter + ::stepMargin) // от первого до последнего целого
                           - (qreal(m_currentStepAnimationPosition % 100) / 100) * (::stepDiameter + ::stepMargin) // динамическая часть
                           );
            firstPoint.setY(lastPoint.y());
        }
        painter.drawEllipse(firstPoint, stepRadius, stepRadius);
        painter.drawEllipse(lastPoint, stepRadius, stepRadius);
        QRectF rect(firstPoint.x(), firstPoint.y() - stepRadius,
                    lastPoint.x() - firstPoint.x(), stepDiameter);
        painter.drawRect(rect);
    }
}
