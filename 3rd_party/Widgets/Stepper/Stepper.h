#ifndef STEPPER_H
#define STEPPER_H

#include <QWidget>


/**
 * @brief Виджет визуализации текущего шага в мастере настройки
 */
class Stepper : public QWidget
{
    Q_OBJECT

public:
    explicit Stepper(QWidget* _parent = nullptr);

    /**
     * @brief Устаноить количество шагов
     */
    void setStepsCount(int _count);

    /**
     * @brief Установить текущий шаг
     */
    void setCurrentStep(int _step);

    /**
     * @brief Переопределяем для определения минимального размера
     */
    QSize sizeHint() const;

protected:
    /**
     * @brief Переопределяем для отрисовки
     */
    void paintEvent(QPaintEvent* _event);

private:
    /**
     * @brief Количество шагов
     */
    int m_stepsCount = 5;

    /**
     * @brief Номер текущего шага
     */
    int m_currentStep = 0;
};

#endif // STEPPER_H
