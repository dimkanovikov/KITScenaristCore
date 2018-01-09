#ifndef SCROLLERHELPER
#define SCROLLERHELPER

#include <QScroller>
#include <QScrollerProperties>


/**
 * @brief Вспомогательный класс, для настройки скроллера
 */
class ScrollerHelper
{
public:
	/**
	 * @brief Добавить скроллер к заданному виджету
	 */
	static void addScroller(QWidget* _forWidget) {
		//
		// Добавляем скроллер
		//
		QScroller::grabGesture(_forWidget, QScroller::LeftMouseButtonGesture);
		QScroller* scroller = QScroller::scroller(_forWidget);

		//
		// Настраиваем параметры скроллера
		//
        QScrollerProperties properties = scroller->scrollerProperties();
        //
        // ... время, через которое на прокручиваемый виджет отправляется события нажатия мыши,
        //     если прокрутку начали, не отпустили, но и не сместили в течении этого времени
        //
        properties.setScrollMetric(QScrollerProperties::MousePressEventDelay, qreal(1.0));
        //
        // ... минимальная дистанция смещения для определения жестка прокрутки
        //
        properties.setScrollMetric(QScrollerProperties::DragStartDistance, qreal(0.001));
        //
        // ... минимальная и максимальная скорости прокрутки
        //
        properties.setScrollMetric(QScrollerProperties::MinimumVelocity, qreal(1.0 / 1000));
        properties.setScrollMetric(QScrollerProperties::MaximumVelocity, qreal(550.0 / 1000));
        //
        // ... максимальная скорость, при которой события клика отправляются прокручиваемому виджету
        //
        properties.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, qreal(0.0001));
        //
        // ... время жест в течении которого воспринимается, как ускоряющий прокрутку
        //
        properties.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, qreal(0.1));
        //
        // ... во сколько раз ускоряется прокрутка жестом ускорения
        //
        properties.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, qreal(2.0));
        //
        // ... секция настроек завершения прокрутки и отскока от границы
        //
        QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
        properties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, overshootPolicy);
        properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, qreal(0.3));
        properties.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, qreal(0.02));
        properties.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, qreal(0.3));
        properties.setScrollMetric(QScrollerProperties::OvershootScrollTime, qreal(0.4));
        scroller->setScrollerProperties(properties);
	}
};

#endif // SCROLLERHELPER

