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
     * @brief Задать фиксированную точку и угол фиксации
     */
    void setFixedCornerPos(const QPoint& _pos, Qt::Corner _corner);

protected:
    /**
     * @brief Переопределяем, чтобы при изменении размера корректировать положение
     */
    void resizeEvent(QResizeEvent* _event) override;

private:
    /**
     * @brief Скоректировать позицию для заданного размера
     */
    void correctPosition(const QSize& _size);

private:
    /**
     * @brief Фиксированная точка одного из углов
     */
    QPoint m_pos;

    /**
     * @brief Угол в котором зафиксирована точка
     */
    Qt::Corner m_corner = Qt::TopRightCorner;
};

#endif // SLIDINGPANEL_H
