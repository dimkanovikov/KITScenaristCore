#ifndef SCRIPTVERSIONS_H
#define SCRIPTVERSIONS_H

#include <QScrollArea>

class QAbstractItemModel;


namespace UserInterface
{
    class ScriptVersionWidget;

    /**
     * @brief Виджет списка версий
     */
    class ScriptVersions : public QScrollArea
    {
        Q_OBJECT

    public:
        explicit ScriptVersions(QWidget *parent = nullptr);

        /**
         * @brief Установить модель версий
         */
        void setModel(QAbstractItemModel* _model);

    signals:
        /**
         * @brief Запрос на удаление версии
         */
        void removeRequested(const QModelIndex& _projectIndex);

    private:
        /**
         * @brief Настроить представление
         */
        void initView();

        /**
         * @brief Получить порядковый номер заданной версии
         */
        int versionRow(ScriptVersionWidget* _version) const;

        /**
         * @brief Обработать нажатия соответствующих кнопок-действий над проектом
         */
        /** @{ */
        void handleRemoveClick();
        /** @} */

    private:
        /**
         * @brief Модель списка версий
         */
        QAbstractItemModel* m_model = nullptr;
    };
}

#endif // SCRIPTVERSIONS_H
