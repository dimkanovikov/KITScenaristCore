#ifndef PROJECTSMANAGER_H
#define PROJECTSMANAGER_H

#include <ManagementLayer/Project/Project.h>

#include <QObject>

class QAbstractItemModel;
class QModelIndex;
class QStandardItemModel;


namespace ManagementLayer
{
    /**
     * @brief Управляющий проектами
     */
    class ProjectsManager : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Валиден ли текущий проект
         */
        static bool isCurrentProjectValid();

        /**
         * @brief Получить текущий проект, с которым работает пользователь
         */
        static const Project& currentProject();

        /**
         * @brief Получить директорию для проектов
         */
        static QString defaultLocation();

    private:
        /**
         * @brief Текущий проект
         */
        static Project s_currentProject;

    public:
        explicit ProjectsManager(QObject* _parent);
        ~ProjectsManager();

        /**
         * @brief Получить список недавно используемых проектов
         * @note Владение моделью передаётся клиенту
         */
        QAbstractItemModel* recentProjects();

        /**
         * @brief Получить список доступных проектов из облака
         * @note Владение моделью передаётся клиенту
         */
        QAbstractItemModel* remoteProjects();

        /**
         * @brief Установить текущий проект по пути к файлу
         */
        bool setCurrentProject(const QString& _path, bool _isLocal = true, bool _forceOpen = false);

        /**
         * @brief Установить текущий проект из модели проектов
         */
        bool setCurrentProject(const QModelIndex& _index, bool _isLocal = true, bool _forceOpen = false);

        /**
         * @brief Установить текущий проект по идентификатору
         * @note Используется только для проектов из облака
         */
        bool setCurrentProject(int _id, bool _isLocal = true, bool _forceOpen = false);

        /**
         * @brief Установить название текущего проекта
         */
        void setCurrentProjectName(const QString& _projectName);

        /**
         * @brief Установить флаг возможна ли синхронизация текущего проекта
         */
        void setCurrentProjectSyncAvailable(bool _syncAvailable, int _errorCode = 0);

        /**
         * @brief Текущий проект закрыт
         */
        void closeCurrentProject();

        /**
         * @brief Получить проект
         */
        Project project(const QModelIndex& _index, bool _isLocal = true) const;
        Project& project(const QModelIndex& _index, bool _isLocal = true);

        /**
         * @brief Скрыть проект из списка локальных проектов
         */
        void hideProjectFromLocal(const QModelIndex& _index);

    public slots:
        /**
         * @brief Обновить список проектов
         */
        void refreshProjects();

        /**
         * @brief Установить список проектов из облака в виде xml с сервиса
         */
        void setRemoteProjects(const QString& _xml);

        /**
         * @brief Установить для всех проектов из облака флаг о недоступной синхронизации
         */
        void setRemoteProjectsSyncAvailable(bool _syncAvailable);

    signals:
        /**
         * @brief Обновлён список недавних проектов
         */
        void recentProjectsUpdated();

        /**
         * @brief Обновлён список облачных проектов
         */
        void remoteProjectsUpdated();

        /**
         * @brief Изменено имя проекта из списка недавних
         */
        void recentProjectNameChanged(int index, const QString& _name);

        /**
         * @brief Изменено имя проекта из списка облачных
         */
        void remoteProjectNameChanged(int index, const QString& _name);

    private:
        /**
         * @brief Загрузить список недавних проектов
         */
        void loadRecentProjects();

        /**
         * @brief Сохранить список недавних проектов
         */
        void saveRecentProjects();

        //
        // TODO: Убрать через несколько релизов (22.09.17)
        //
        /**
         * @brief Поменяем пути на относительные для iOS.
         */
        void makeRelativePaths();

    private:
        /**
         * @brief Недавно используемые проекты
         */
        QList<Project> m_recentProjects;

        /**
         * @brief Модель недавно использованных проектов
         */
        QStandardItemModel* m_recentProjectsModel = nullptr;

        /**
         * @brief Проекты доступные из облака
         */
        QList<Project> m_remoteProjects;

        /**
         * @brief Модель проектов из облака
         */
        QStandardItemModel* m_remoteProjectsModel = nullptr;
    };
}

#endif // PROJECTSMANAGER_H
