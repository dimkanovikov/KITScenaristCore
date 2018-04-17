#ifndef RESTOREFROMBACKUPTOOL_H
#define RESTOREFROMBACKUPTOOL_H

#include <QObject>
#include <QDateTime>


namespace BusinessLogic
{
    /**
     * @brief Версия бэкапа
     */
    struct BackupVersion {
        /**
         * @brief Дата и время создания бэкапа
         */
        QDateTime datetime;

        /**
         * @brief размер бэкапа
         */
        qint64 size;
    };

    /**
     * @brief Информация о бэкапе
     */
    struct BackupInfo
    {
        /**
         * @brief Список версий бэкапов
         */
        std::vector<BackupVersion> versions;
    };

    /**
     * @brief Инструмент восстановления версий из бэкапа
     */
    class RestoreFromBackupTool : public QObject
    {
        Q_OBJECT

    public:
        explicit RestoreFromBackupTool(QObject* _parent = nullptr);

        /**
         * @brief Загрузить информацию о бэкапах из заданного файла
         */
        void loadBackupInfo(const QString& _backupFilePath);

        /**
         * @brief Загрузить бэкап за заданную дату
         */
        void loadBackup(const QDateTime& _backupDateTime);

    signals:
        /**
         * @brief Загружена информация о бэкапах проекта
         */
        void backupInfoLoaded(const BusinessLogic::BackupInfo& backupInfo);

        /**
         * @brief Бэкап загружен
         */
        void backupLoaded(const QString& _backup);

    private:
        /**
         * @brief Последний используемый файл бэкапа
         */
        QString m_lastBackupFile;
    };
}

Q_DECLARE_METATYPE(BusinessLogic::BackupInfo)

#endif // RESTOREFROMBACKUPTOOL_H
