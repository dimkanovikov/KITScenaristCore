#ifndef INPUTVIEW_H
#define INPUTVIEW_H

#include <QWidget>


/**
 * @brief Представление, для работы с виртуальными клавиатурами не изменяющих размер окон
 */
class InputView : public QWidget
{
    Q_OBJECT

public:
    explicit InputView(QWidget* _parent = nullptr);

    /**
     * @brief Подготовиться к выполнению
     *        - показать клавиатуру
     *        - выполнить prepareImpl
     */
    void prepare();

    /**
     * @brief Завершить выполнение
     *        - скрыть клавиатуру
     *        - выполнить finlizeImpl
     */
    void finalize();

protected:
    /**
     * @brief Возможность для дочерних классов выполнить собственную подготовку к работе
     */
    virtual void prepareImpl();

    /**
     * @brief Возможность для дочерних классов выполнить собственное завершение работы
     */
    virtual void finalizeImpl();

    /**
     * @brief Дополнительная высота, которую нужно учитывать при корректировка резмера
     *        например высота тулбара
     */
    virtual int additionalHeight() const;

private:
    /**
     * @brief Обновить размер представления в зависимости от состояния клавиатуры
     */
    void updateViewSize();
};

#endif // INPUTVIEW_H
