#include "RestoreFromBackupTool.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

using BusinessLogic::BackupInfo;
using BusinessLogic::RestoreFromBackupTool;

namespace {
    /**
     * @brief Название соединения для БД с резервными копиями сценария
     */
    const QString kBackupDbConnectionName = "restore_backup_db";
}


RestoreFromBackupTool::RestoreFromBackupTool(QObject* _parent) :
    QObject(_parent)
{
    qRegisterMetaType<BusinessLogic::BackupInfo>("BusinessLogic::BackupInfo");
}

void RestoreFromBackupTool::loadBackupInfo(const QString& _backupFilePath)
{
    m_lastBackupFile = _backupFilePath;
    if (m_lastBackupFile.isEmpty()
        || !QFile::exists(m_lastBackupFile)) {
        emit backupInfoNotLoaded();
        return;
    }


    //
    // Открыть базу данных в указанном файле
    //
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kBackupDbConnectionName);
        db.setDatabaseName(m_lastBackupFile);
        db.open();
        //
        // Считываем сохранённые бэкапы
        //
        QSqlQuery backupInfoLoader(db);
        backupInfoLoader.exec("SELECT datetime, length(version) as size FROM versions ORDER BY datetime DESC");
        BackupInfo backupInfo;
        while (backupInfoLoader.next()) {
            const QDateTime datetime = backupInfoLoader.value("datetime").toDateTime();
            const qint64 size = backupInfoLoader.value("size").toLongLong();
            backupInfo.versions.push_back({datetime, size});
        }

        //
        // Уведомляем клиентов о считанной информации о бэкапе
        //
        emit backupInfoLoaded(backupInfo);
    }

    QSqlDatabase::removeDatabase(kBackupDbConnectionName);
}

void RestoreFromBackupTool::loadBackup(const QDateTime& _backupDateTime)
{
    if (m_lastBackupFile.isEmpty()
        || !QFile::exists(m_lastBackupFile)) {
        emit backupInfoNotLoaded();
        return;
    }


    //
    // Открыть базу данных в указанном файле
    //
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kBackupDbConnectionName);
        db.setDatabaseName(m_lastBackupFile);
        db.open();
        //
        // Считываем необходимый бэкап
        //
        QSqlQuery backupLoader(db);
        backupLoader.prepare("SELECT version FROM versions WHERE datetime = ?");
        backupLoader.addBindValue(_backupDateTime.toString(Qt::ISODate));
        if (backupLoader.exec()
            && backupLoader.next()) {
            //
            // Уведомляем клиентов о считанном бэкапе
            //
            const QString backupText = backupLoader.value("version").toString();
            emit backupLoaded(backupText);
        }
    }

    QSqlDatabase::removeDatabase(kBackupDbConnectionName);
}
