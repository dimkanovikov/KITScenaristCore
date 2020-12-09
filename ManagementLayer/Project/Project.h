#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QDateTime>


namespace ManagementLayer
{
    /**
     * @brief Проект сценария
     */
    class Project
    {
    public:
        /**
         * @brief Тип проекта
         */
        enum Type {
            Invalid,
            Local,
            Remote
        };

        /**
         * @brief Роль пользователя в проекте
         */
        enum Role {
            Owner,
            Redactor,
            Commentator
        };

        /**
         * @brief Невалидный идентификатор проекта
         */
        static const int kInvalidId = -1;

        /**
         * @brief Получить строку из роли
         */
        static QString roleToString(Role _role);

        /**
         * @brief Получить значение роли из строки
         */
        static Role roleFromString(const QString& _role);

        /**
         * @brief Получить путь к папке хранения облачных проектов
         */
        static QString remoteProjectsDirPath();

    public:
        Project();
        Project(Type _type, const QString& _name, const QString& _path,
            const QDateTime& _lastEditDatetime, int _id = 0, const QString& _owner = QString(),
            Role _role = Owner, const QStringList& _users = QStringList());

        /**
         * @brief Тип проекта
         */
        Type type() const;

        /**
         * @brief Это локальный проект?
         */
        bool isLocal() const;

        /**
         * @brief Это проект из облака?
         */
        bool isRemote() const;

        /**
         * @brief Отображаемое название проекта
         */
        QString displayName() const;

        /**
         * @brief Название проекта
         */
        /** @{ */
        QString name() const;
        void setName(const QString& _name);
        /** @} */

        /**
         * @brief Отображаемый путь к проекту
         */
        QString displayPath() const;

        /**
         * @brief Путь к проекту
         */
        /** @{ */
        QString path() const;
        void setPath(const QString& _path);
        /** @} */

        /**
         * @brief Дата и время последнего изменения проекта
         */
        /** @{ */
        QDateTime lastEditDatetime() const;
        void setLastEditDatetime(const QDateTime& _datetime);
        /** @} */

        /**
         * @brief Идентификатор проекта
         */
        int id() const;

        /**
         * @brief Является ли пользователь владельцем файла
         */
        bool isUserOwner() const;

        /**
         * @brief Сценарий возможно только комментировать?
         */
        bool isCommentOnly() const;

        /**
         * @brief Файл проекта доступен только для чтения
         */
        bool isWritable() const;
        void updateIsWritable();

        /**
         * @brief Список пользователей
         */
        QStringList users() const;

        /**
         * @brief Возможна ли синхронизация
         */
        /** @{ */
        bool isSyncAvailable() const;
        void setSyncAvailable(bool _syncAvailable, int _errorCode = 0);
        /** @} */

        /**
         * @brief Получить путь к файлу полной резервной копии проекта
         */
        QString fullBackupFileName(const QString& _backupDirPath) const;

        /**
         * @brief Получить путь к файлу с резервными копиями последних версий сценария
         */
        QString versionsBackupFileName(const QString& _backupDirPath) const;

    private:
        /**
         * @brief Получить путь к файлу резервной копии заданного типа
         */
        QString backupFileName(const QString& _backupDirPath, bool _isVersionsBackup) const;

    private:
        /**
         * @brief Тип проекта
         */
        Type m_type;

        /**
         * @brief Название проекта
         */
        QString m_name;

        /**
         * @brief Путь к файлу проекта
         */
        QString m_path;

        /**
         * @brief Дата и время последнего изменения проекта
         */
        QDateTime m_lastEditDatetime;

        /**
         * @brief Идентификатор проекта (для проектов из облака)
         */
        int m_id;

        /**
         * @brief Логин владельца проекта (для проектов из облака)
         */
        QString m_owner;

        /**
         * @brief Роль пользователя в проекте (для проектов из облака)
         */
        Role m_role;

        /**
         * @brief Список пользователей проекта
         */
        QStringList m_users;

        /**
         * @brief Возможна ли синхронизация
         */
        /** @{ */
        bool m_isSyncAvailable;
        int m_errorCode;
        /** @} */

        /**
         * @brief Возможна ли запись в файл
         */
        bool m_isWritable = true;
    };

    /**
     * @brief Сравнить два проекта
     */
    bool operator==(const Project& _lhs, const Project& _rhs);
}

#endif // PROJECT_H
