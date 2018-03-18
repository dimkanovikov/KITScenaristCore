#ifndef SLIDINGPANEL_H
#define SLIDINGPANEL_H

#include <QFrame>


/**
 * @brief Фрейм, который умеет прижаться к заданной точке и находиться в ней при изменении размера
 *        Используется для реализации выкатываемых по горизонтали панелей
 */
class SlidingPanel : public QFrame
{
    Q_OBJECT

public:
    explicit SlidingPanel(QWidget *_parent = nullptr);

    /**
     * @brief Задать фиксированную точку верхнего правого угла
     */
    void setFixedRightTopPos(const QPoint& _pos);

protected:
    /**
     * @brief Переопределяем, чтобы при изменении размера корректировать положение
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    /**
     * @brief Фиксированная точка верхнего правого угла панели
     */
    QPoint m_pos;
};

#endif // SLIDINGPANEL_H
