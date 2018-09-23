#ifndef PROJECTSLIST_H
#define PROJECTSLIST_H

#include <QScrollArea>

class QAbstractItemModel;


namespace UserInterface
{
    class ProjectFileWidget;

    /**
     * @brief Виджет списка проектов
     */
    class ProjectsList : public QScrollArea
    {
        Q_OBJECT

    public:
        explicit ProjectsList(QWidget* _parent = nullptr);

        /**
         * @brief Установить модель проектов
         */
        void setModel(QAbstractItemModel* _model, bool _isRemote);

        /**
         * @brief Установить название проекта
         */
        void setProjectName(int _index, const QString& _name);

        /**
         * @brief Сделать доступным/недоступным меню каждого проекта
         */
        void setMenusVisible(bool _isVisible);

        /**
         * @brief Есть ли хотя бы один проект в списке
         */
        bool hasProjects() const;

    signals:
        /**
         * @brief Был выбран проект для открытия
         */
        void clicked(const QModelIndex& _projectIndex);

        /**
         * @brief Запрос на изменение проекта
         */
        void editRequested(const QModelIndex& _projectIndex);

        /**
         * @brief Запрос на удаление проекта
         */
        void removeRequested(const QModelIndex& _projectIndex);

        /**
         * @brief Запрос на скрытие проекта из списка
         */
        void hideRequested(const QModelIndex& _projectIndex);

        /**
         * @brief Запрос на перемещение проекта в облако
         */
        void moveToCloudRequested(const QModelIndex& _projectIndex);

        /**
         * @brief Запрос на открытие доступа к проекту
         */
        void shareRequested(const QModelIndex& _projectIndex);

        /**
         * @brief Удалить пользователя с заданным адресом электронной почты
         */
        void unshareRequested(const QModelIndex& _projectIndex, const QString& _email);

    private:
        /**
         * @brief Получить порядковый номер заданного проекта
         */
        int projectRow(ProjectFileWidget* _project) const;

        /**
         * @brief Обработать нажатия соответствующих кнопок-действий над проектом
         */
        /** @{ */
        void handleProjectClick();
        void handleEditClick();
        void handleRemoveClick();
        void handleHideClick();
        void handleMoveToCloudClick();
        void handleShareClick();
        /** @} */

        /**
         * @brief Обработать закрытие доступа к проекту для пользователя
         */
        void handleRemoveUserRequest(const QString& _email);

    private:
        /**
         * @brief Настроить представление
         */
        void initView();

    private:
        /**
         * @brief Модель списка проектов
         */
        QAbstractItemModel* m_model = nullptr;
    };
}

#endif // PROJECTSLIST_H
