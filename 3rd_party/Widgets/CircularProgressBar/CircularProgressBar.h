#ifndef CIRCULARPROGRESSBAR_H
#define CIRCULARPROGRESSBAR_H

#include <QWidget>


/**
 * @brief Виджет для отображения прогресса в виде круга
 */
class CircularProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit CircularProgressBar(QWidget* _parent = nullptr);

    /**
     * @brief Задать значение прогресса
     */
    void setValue(qreal _value);

protected:
    /**
     * @brief Переопределяем, чтобы рисовать вручную
     */
    void paintEvent(QPaintEvent* _event);

private:
    /**
     * @brief Текущее значение прогресса
     */
    qreal m_value = 0.17;
};

#endif // CIRCULARPROGRESSBAR_H
