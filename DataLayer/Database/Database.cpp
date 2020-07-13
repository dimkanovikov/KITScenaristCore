#include "Database.h"

#include <BusinessLayer/ScenarioDocument/ScenarioXml.h>

#include <Domain/Research.h>

#include <3rd_party/Helpers/DiffMatchPatchHelper.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextCodec>
#include <QUuid>
#include <QVariant>
#include <QXmlStreamWriter>

using namespace DatabaseLayer;

namespace {
    const QString kProjectFleExtension = "kitsp"; // kit scenarist project

    /**
     * @brief Получить ключ хранения номера версии приложения
     */
    static QString applicationVersionKey() {
        return
#ifdef MOBILE_OS
                "application-version-mobile";
#else
                "application-version";
#endif
    }
    /**
     * @brief Инвертированный ключ хранения номера версии, для проверок
     */
    static QString invertedApplicationVersionKey() {
        return
#ifdef MOBILE_OS
                "application-version";
#else
                "application-version-mobile";
#endif
    }
}


bool Database::canOpenFile(const QString& _databaseFileName, bool _isLocal)
{
    bool canOpen = true;

    //
    // Проверки специфичные для локальных файлов
    //
    if (_isLocal) {
        const QFileInfo databaseFileInfo(_databaseFileName);
        if (databaseFileInfo.completeSuffix() == kProjectFleExtension) {
            QSqlDatabase database = QSqlDatabase::addDatabase(SQL_DRIVER, "tmp_database");
            database.setDatabaseName(_databaseFileName);
            if (database.open()) {
                QSqlQuery q_checker(database);

                //
                // 1. Если файл был создан в более поздней версии приложения, его нельзя открывать
                //
                if (q_checker.exec("SELECT value FROM system_variables WHERE variable = 'application-version' ")
                        && q_checker.next()
                        && q_checker.value("value").toString().split(" ").first() > QApplication::applicationVersion()) {
                    canOpen = false;
                    s_openFileError =
                            QApplication::translate("DatabaseLayer::Database",
                                                    "Project was modified in higher version. You need update application to latest version for open it.");
                }
            } else {
                s_openFileError = QApplication::translate(
                                      "DatabaseLayer::Database",
                                      "File \"%1\" can't be opened, please check out that you tries to open file which was created in the KIT Scenarist."
                                      ).arg(databaseFileInfo.fileName());
                canOpen = false;
            }
        } else {
            s_openFileError = QApplication::translate(
                                  "DatabaseLayer::Database",
                                  "File \"%1\" can't be opened, please check out that you tries to open KIT Scenarist file (*.kitsp)."
                                  ).arg(databaseFileInfo.fileName());
            canOpen = false;
        }

        QSqlDatabase::removeDatabase("tmp_database");
    }

    return canOpen;
}

QString Database::openFileError()
{
    return s_openFileError;
}

bool Database::hasError()
{
    return !s_lastError.isEmpty();
}

QString Database::lastError()
{
    return s_lastError;
}

void Database::setLastError(const QString& _error)
{
    if (s_lastError != _error)   {
        s_lastError = _error;
    }
}

void Database::setCurrentFile(const QString& _databaseFileName)
{
    //
    // Если использовалась база данных, то удалим старое соединение
    //
    closeCurrentFile();

    //
    // Установим текущее имя базы данных
    //
    if (DATABASE_NAME != _databaseFileName) {
        DATABASE_NAME = _databaseFileName;
        CONNECTION_NAME = "local_database [" + DATABASE_NAME + "]";
    }

    //
    // Откроем базу данных, или создадим новую
    //
    instanse();
}

void Database::closeCurrentFile()
{
    if (QSqlDatabase::contains(CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(CONNECTION_NAME);
    }
}

QString Database::currentFile()
{
    return instanse().databaseName();
}

QSqlQuery Database::query()
{
    return QSqlQuery(instanse());
}

void Database::transaction()
{
    //
    // Для первого запроса открываем транзакцию
    //
    if (s_openedTransactions == 0) {
        instanse().transaction();
    }

    //
    // Увеличиваем счётчик открытых транзакций
    //
    ++s_openedTransactions;
}

void Database::commit()
{
    //
    // Уменьшаем счётчик транзакций
    //
    --s_openedTransactions;

    //
    // При закрытии корневой транзакции фиксируем изменения в базе данных
    //
    if (s_openedTransactions == 0) {
        instanse().commit();
    }
}


//********
// Скрытая часть


QString Database::CONNECTION_NAME = "local_database";
QString Database::SQL_DRIVER      = "QSQLITE";
QString Database::DATABASE_NAME   = ":memory:";

QString Database::s_openFileError = QString();
QString Database::s_lastError = QString();
int Database::s_openedTransactions = 0;

QSqlDatabase Database::instanse()
{
    QSqlDatabase database;

    if (!QSqlDatabase::contains(CONNECTION_NAME)) {
        open(database, CONNECTION_NAME, DATABASE_NAME);
    } else {
        database = QSqlDatabase::database(CONNECTION_NAME);
    }

    return database;
}

void Database::open(QSqlDatabase& _database, const QString& _connectionName, const QString& _databaseName)
{
    s_lastError.clear();

    _database = QSqlDatabase::addDatabase(SQL_DRIVER, _connectionName);
    _database.setDatabaseName(_databaseName);
    _database.open();

    Database::States states = checkState(_database);

    if (!states.testFlag(SchemeFlag))
        createTables(_database);
    if (!states.testFlag(IndexesFlag))
        createIndexes(_database);
    if (!states.testFlag(EnumsFlag))
        createEnums(_database);
    if (states.testFlag(OldVersionFlag))
        updateDatabase(_database);
}

// Проверка состояния базы данных
// например:
// - БД отсутствует
// - БД пуста
// - БД имеет старую версию
// - БД имеет последнюю версию
// - и т.д.
Database::States Database::checkState(QSqlDatabase& _database)
{
    QSqlQuery q_checker(_database);
    Database::States states = Database::EmptyFlag;

    //
    // Созданы ли таблицы
    //
    if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'table' ") &&
        q_checker.next() &&
        q_checker.record().value("size").toInt()) {
        //
        // Все остальные проверки имеют смысл, только если проходит данная проверка
        //
        states = states | Database::SchemeFlag;

        //
        // Созданы ли индексы
        //
        if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'index' ") &&
            q_checker.next() &&
            q_checker.record().value("size").toInt()) {
            states = states | Database::IndexesFlag;
        }

        //
        // Проверка версии
        //
        if (q_checker.exec(
                QString("SELECT value as version FROM system_variables WHERE variable = '%1' ")
                .arg(applicationVersionKey()))) {
            //
            // Если версия не задана, или она не соответствует текущей, то надо обновить файл
            //
            if (!q_checker.next()
                || q_checker.record().value("version").toString() != QApplication::applicationVersion()) {
                states = states | Database::OldVersionFlag;
            }
        }
    }

    return states;
}

void Database::createTables(QSqlDatabase& _database)
{
    QSqlQuery q_creator(_database);
    _database.transaction();

    // Таблица с историей запросов
    q_creator.exec("CREATE TABLE _database_history "
                   "( "
                   "id TEXT PRIMARY KEY, " // uuid
                   "query TEXT NOT NULL, "
                   "query_values TEXT NOT NULL, "
                   "username TEXT NOT NULL, "
                   "datetime TEXT NOT NULL "
                   "); "
                   );

    // Таблица системных переменных
    q_creator.exec("CREATE TABLE system_variables "
                   "( "
                   "variable TEXT PRIMARY KEY ON CONFLICT REPLACE, "
                   "value TEXT NOT NULL "
                   "); "
                   );

    // Таблица "Место"
    q_creator.exec("CREATE TABLE places "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );


    // Таблица "Сценарний день"
    q_creator.exec("CREATE TABLE scenary_days "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );

    // Таблица "Время"
    q_creator.exec("CREATE TABLE times "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );

    // Таблица "Состояния персонажей"
    q_creator.exec("CREATE TABLE character_states "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );

    // Таблица "Переходы"
    q_creator.exec("CREATE TABLE transitions "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );


    // Таблица "Текст сценария"
    q_creator.exec("CREATE TABLE scenario "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "scheme TEXT NOT NULL, "
                   "text TEXT NOT NULL, "
                   "is_draft INTEGER NOT NULL DEFAULT(0) "
                   ")"
                   );

    //
    // Создаём таблицу изменений сценария
    //
    q_creator.exec("CREATE TABLE scenario_changes "
                   "("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "uuid TEXT NOT NULL, "
                   "datetime TEXT NOT NULL, "
                   "username TEXT NOT NULL, "
                   "undo_patch TEXT NOT NULL, " // отмена изменения
                   "redo_patch TEXT NOT NULL, " // повтор изменения (наложение для соавторов)
                   "is_draft INTEGER NOT NULL DEFAULT(0) "
                   ")"
                   );

    //
    // Таблица с данными сценария
    //
    q_creator.exec("CREATE TABLE scenario_data "
                   "("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "data_name TEXT NOT NULL UNIQUE, "
                   "data_value TEXT DEFAULT(NULL) "
                   ")"
                   );

    //
    // Таблица "Разработка"
    //
    q_creator.exec("CREATE TABLE research "
                   "("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "parent_id INTEGER DEFAULT(NULL), "
                   "type INTEGER NOT NULL DEFAULT(0), " // 0 - папка, 1 - текст
                   "name TEXT NOT NULL, "
                   "description TEXT DEFAULT(NULL), "
                   "url TEXT DEFAULT(NULL), "
                   "image BLOB DEFAULT(NULL), "
                   "color TEXT DEFAULT(NULL), "
                   "sort_order INTEGER NOT NULL DEFAULT(0) "
                   ")"
                   );

    //
    // Таблица "Версии сценария"
    //
    q_creator.exec("CREATE TABLE script_versions "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "fk_script_id INTEGER NOT NULL, "
                   "username TEXT NOT NULL, "
                   "datetime TEXT NOT NULL, "
                   "color TEXT DEFAULT(NULL), "
                   "name TEXT UNIQUE NOT NULL, "
                   "description TEXT DEFAULT(NULL), "
                   "script_text TEXT NOT NULL"
                   "); "
                   );

    _database.commit();
}

void Database::createIndexes(QSqlDatabase& _database)
{
    Q_UNUSED(_database);
}

void Database::createEnums(QSqlDatabase& _database)
{
    QSqlQuery q_creator(_database);
    _database.transaction();

    // Служебная информация
    {
        q_creator.exec(
                    QString("INSERT INTO system_variables VALUES ('application-version-on-create', '%1')")
                    .arg(QApplication::applicationVersion())
                    );
        q_creator.exec(
                    QString("INSERT INTO system_variables VALUES ('%1', '%2')")
                    .arg(applicationVersionKey())
                    .arg(QApplication::applicationVersion())
                    );
        q_creator.exec(
                    QString("INSERT INTO script_versions (id, fk_script_id, username, datetime, name, script_text) "
                            "VALUES (null, 0, '', '%1', '%2', '')")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz"))
                    .arg(QApplication::translate("DatabaseLayer::Database", "First draft"))
                    );
    }

    // Справочник мест
    {
        q_creator.exec(
                    QString("INSERT INTO places (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "INT"))
                    );
        q_creator.exec(
                    QString("INSERT INTO places (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "EXT"))
                    );
        q_creator.exec(
                    QString("INSERT INTO places (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "INT./EXT"))
                    );
    }

    // Справочник времени
    {
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "DAY"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "NIGHT"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "MORNING"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "AFTERNOON"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "EVENING"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "LATER"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "MOMENTS LATER"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "CONTINUOUS"))
                    );
        q_creator.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "THE NEXT DAY"))
                    );
    }

    // Справочник состояний персонажей
    {
        //: Voice over
        q_creator.exec(
                    QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "V.O."))
                    );
        //: Off-screen
        q_creator.exec(
                    QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "O.S."))
                    );
        //: Off-camera
        q_creator.exec(
                    QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "O.C."))
                    );
        //: Subtitle
        q_creator.exec(
                    QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "SUBTITLE"))
                    );
        //: Continued
        q_creator.exec(
                    QString("INSERT INTO character_states (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "CONT'D"))
                    );
    }

    // Справочник переходов
    {
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "CUT TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "FADE IN:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "FADE OUT"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "FADE TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "DISSOLVE TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "BACK TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "MATCH CUT TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "JUMP CUT TO:"))
                    );
        q_creator.exec(
                    QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "FADE TO BLACK"))
                    );
    }

    _database.commit();
}

void Database::updateDatabase(QSqlDatabase& _database)
{
    QSqlQuery q_checker(_database);

    //
    // Определим версию базы данных
    //
    q_checker.prepare("SELECT value as version FROM system_variables WHERE variable = ? ");
    q_checker.addBindValue(applicationVersionKey());
    q_checker.exec();
    q_checker.next();
    QString databaseVersion = q_checker.record().value("version").toString();
    //
    // ... если версии нет (файл пришёл из другой версии мобильная <-> десктоп), то создадим её
    //
    if (databaseVersion.isEmpty()) {
        q_checker.addBindValue(invertedApplicationVersionKey());
        q_checker.exec();
        q_checker.next();
        databaseVersion = q_checker.record().value("version").toString();
    }

    //
    // Некоторые версии выходили с ошибками, их заменяем на предыдущие
    //
    {
        if (databaseVersion.startsWith("0.7.0 beta")) {
            databaseVersion = "0.6.2";
        }
        if (databaseVersion.startsWith("0.7.2 beta")) {
            databaseVersion = "0.7.1";
        }
    }
    const int versionMajor = databaseVersion.split(".").value(0, "0").toInt();
    const int versionMinor = databaseVersion.split(".").value(1, "0").toInt();
    const int versionBuild = databaseVersion.split(".").value(2, "1").split(" ").value(0, "1").toInt();

    //
    // Вызываются необходимые процедуры обновления БД в зависимости от её версии
    //
    // 0.X.X
    //
    if (versionMajor <= 0) {
        //
        // 0.0.X
        //
        if (versionMinor <= 0) {
            if (versionMinor < 0
                || versionBuild <= 1) {
                updateDatabaseTo_0_0_2(_database);
            }
            if (versionMinor < 0
                || versionBuild <= 4) {
                updateDatabaseTo_0_0_5(_database);
            }
        }
        //
        // 0.1.X
        //
        if (versionMinor <= 1) {
            if (versionMinor < 1
                || versionBuild <= 0) {
                updateDatabaseTo_0_1_0(_database);
            }
        }
        //
        // 0.2.X
        //
        if (versionMinor <= 2) {
            if (versionMinor < 2
                || versionBuild <= 7) {
                updateDatabaseTo_0_2_8(_database);
            }
        }
        //
        // 0.3.X
        //
        if (versionMinor <= 3) {
            if (versionMinor < 3
                || versionBuild <= 2) {
                updateDatabaseTo_0_3_3(_database);
            }
        }
        //
        // 0.4.X
        //
        if (versionMinor <= 4) {
            if (versionMinor < 4
                || versionBuild <= 4) {
                updateDatabaseTo_0_4_5(_database);
            }
            if (versionMinor < 4
                || versionBuild <= 9) {
                updateDatabaseTo_0_5_0(_database);
            }
        }
        //
        // 0.5.x
        //
        if (versionMinor <= 5) {
            if (versionMinor < 5
                || versionBuild <= 5) {
                updateDatabaseTo_0_5_6(_database);
            }
            if (versionMinor < 5
                || versionBuild <= 7) {
                updateDatabaseTo_0_5_8(_database);
            }
            if (versionMinor < 5
                || versionBuild <= 8) {
                updateDatabaseTo_0_5_9(_database);
            }
        }
        //
        // 0.6.x
        //
        if (versionMinor <= 6) {
            if (versionMinor < 6
                || versionBuild <= 2) {
                updateDatabaseTo_0_7_0(_database);
            }
        }
        //
        // 0.7.x
        //
        if (versionMinor <= 7) {
            if (versionMinor < 7
                || versionBuild <= 0) {
                updateDatabaseTo_0_7_1(_database);
            }
            if (versionMinor < 7
                || versionBuild <= 1) {
                updateDatabaseTo_0_7_2(_database);
            }
        }
    }

    //
    // Сохраняем информацию об обновлении версии
    //
    q_checker.exec(
        QString("INSERT INTO system_variables VALUES ('application-updated-to-version-%1', '%2')")
               .arg(QApplication::applicationVersion())
               .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
        );

    //
    // Обновляется версия программы
    //
    q_checker.exec(
                QString("INSERT INTO system_variables VALUES ('%1', '%2')")
                .arg(::applicationVersionKey())
                .arg(QApplication::applicationVersion())
                );
}

void Database::updateDatabaseTo_0_0_2(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        q_updater.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "MORNING"))
                    );
        q_updater.exec(
                    QString("INSERT INTO times (id, name) VALUES (null, '%1');")
                    .arg(QApplication::translate("DatabaseLayer::Database", "EVENING"))
                    );
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_0_5(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Обновление таблицы локаций
        //
        q_updater.exec("ALTER TABLE locations ADD COLUMN description TEXT DEFAULT(NULL)");

        // Таблица "Фотографии локаций"
        q_updater.exec("CREATE TABLE locations_photo "
                       "( "
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "fk_location_id INTEGER NOT NULL, "
                       "photo BLOB NOT NULL, "
                       "sort_order INTEGER NOT NULL DEFAULT(0) "
                       ")"
                       );

        //
        // Обновление таблицы персонажей
        //
        q_updater.exec("ALTER TABLE characters ADD COLUMN real_name TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE characters ADD COLUMN description TEXT DEFAULT(NULL)");

        // Таблица "Фотографии персонажей"
        q_updater.exec("CREATE TABLE characters_photo "
                       "( "
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "fk_character_id INTEGER NOT NULL, "
                       "photo BLOB NOT NULL, "
                       "sort_order INTEGER NOT NULL DEFAULT(0) "
                       ")"
                       );
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_1_0(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Обновление таблицы сценария
        //
        q_updater.exec("ALTER TABLE scenario ADD COLUMN name TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE scenario ADD COLUMN synopsis TEXT DEFAULT(NULL)");
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_2_8(QSqlDatabase& _database)
{
    //
    // Заменить при помощи регулярок все
    //       font-family:'*';
    //		 font-size:*;
    //
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        const QRegularExpression rx_fontFamilyCleaner("font-family:([^;]*);");
        const QRegularExpression rx_fontSizeCleaner("font-size:([^;]*);");

        //
        // Персонажи
        //
        // ... очистим данные
        //
        q_updater.exec("SELECT id, description FROM characters");
        QMap<int, QString> charactersDescriptions;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString description = q_updater.record().value("description").toString();
            description = description.remove(rx_fontFamilyCleaner);
            description = description.remove(rx_fontSizeCleaner);
            charactersDescriptions.insert(id, description);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE characters SET description = ? WHERE id = ?");
        foreach (int id, charactersDescriptions.keys()) {
            q_updater.addBindValue(charactersDescriptions.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }

        //
        // Локации
        //
        // ... очистим данные
        //
        q_updater.exec("SELECT id, description FROM locations");
        QMap<int, QString> locationsDescriptions;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString description = q_updater.record().value("description").toString();
            description = description.remove(rx_fontFamilyCleaner);
            description = description.remove(rx_fontSizeCleaner);
            locationsDescriptions.insert(id, description);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE locations SET description = ? WHERE id = ?");
        foreach (int id, locationsDescriptions.keys()) {
            q_updater.addBindValue(locationsDescriptions.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }

        //
        // Синопсис сценария
        //
        // ... очистим данные
        //
        q_updater.exec("SELECT id, synopsis FROM scenario");
        QMap<int, QString> scenarioSynopsis;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString synopsis = q_updater.record().value("synopsis").toString();
            synopsis = synopsis.remove(rx_fontFamilyCleaner);
            synopsis = synopsis.remove(rx_fontSizeCleaner);
            scenarioSynopsis.insert(id, synopsis);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE scenario SET synopsis = ? WHERE id = ?");
        foreach (int id, scenarioSynopsis.keys()) {
            q_updater.addBindValue(scenarioSynopsis.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_3_3(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Создать таблицу состояний
        //
        q_updater.exec("CREATE TABLE character_states "
                       "( "
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "name TEXT UNIQUE NOT NULL "
                       "); "
                       );

        //
        // Добавление полей в таблицу сценария
        //
        q_updater.exec("ALTER TABLE scenario ADD COLUMN additional_info TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE scenario ADD COLUMN genre TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE scenario ADD COLUMN author TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE scenario ADD COLUMN contacts TEXT DEFAULT(NULL)");
        q_updater.exec("ALTER TABLE scenario ADD COLUMN year TEXT DEFAULT(NULL)");
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_4_5(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Добавление поля в таблицу сценария
        //
        q_updater.exec("ALTER TABLE scenario ADD COLUMN is_draft INTEGER NOT NULL DEFAULT(0)");

        //
        // Добавление самого черновика
        //
        q_updater.exec("INSERT INTO scenario (id, text, is_draft) VALUES(null, '', 1)");
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_5_0(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Создаём таблицу для хранения всех запросов
        //
        q_updater.exec("CREATE TABLE _database_history "
                       "( "
                       "id TEXT PRIMARY KEY, " // uuid
                       "query TEXT NOT NULL, "
                       "query_values TEXT NOT NULL, "
                       "datetime TEXT NOT NULL "
                       "); "
                       );

        //
        // Создаём таблицу изменений сценария
        //
        q_updater.exec("CREATE TABLE scenario_changes "
                       "("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "uuid TEXT NOT NULL, "
                       "datetime TEXT NOT NULL, "
                       "username TEXT NOT NULL, "
                       "undo_patch TEXT NOT NULL, " // отмена изменения
                       "redo_patch TEXT NOT NULL, " // повтор изменения (наложение для соавторов)
                       "is_draft INTEGER NOT NULL DEFAULT(0) "
                       ")"
                       );

        //
        // Таблица с данными сценария
        //
        q_updater.exec("CREATE TABLE scenario_data "
                       "("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "data_name TEXT NOT NULL UNIQUE, "
                       "data_value TEXT DEFAULT(NULL) "
                       ")"
                       );

        //
        // Перенесём данные о сценарии в новую таблицу
        //
        q_updater.exec("SELECT name, additional_info, genre, author, contacts, year, synopsis "
                       "FROM scenario WHERE is_draft = 0");
        if (q_updater.next()) {
            QSqlQuery q_transporter(_database);
            q_transporter.prepare("INSERT INTO scenario_data (data_name, data_value) VALUES(?, ?)");

            const QString name = "name";
            q_transporter.addBindValue(name);
            q_transporter.addBindValue(q_updater.record().value(name));
            q_transporter.exec();

            const QString additionalInfo = "additional_info";
            q_transporter.addBindValue(additionalInfo);
            q_transporter.addBindValue(q_updater.record().value(additionalInfo));
            q_transporter.exec();

            const QString genre = "genre";
            q_transporter.addBindValue(genre);
            q_transporter.addBindValue(q_updater.record().value(genre));
            q_transporter.exec();

            const QString author = "author";
            q_transporter.addBindValue(author);
            q_transporter.addBindValue(q_updater.record().value(author));
            q_transporter.exec();

            const QString contacts = "contacts";
            q_transporter.addBindValue(contacts);
            q_transporter.addBindValue(q_updater.record().value(contacts));
            q_transporter.exec();

            const QString year = "year";
            q_transporter.addBindValue(year);
            q_transporter.addBindValue(q_updater.record().value(year));
            q_transporter.exec();

            const QString synopsis = "synopsis";
            q_transporter.addBindValue(synopsis);
            q_transporter.addBindValue(q_updater.record().value(synopsis));
            q_transporter.exec();
        }

        //
        // Сам сценарий
        //
        q_updater.exec("SELECT text FROM scenario WHERE is_draft = 0");
        if (q_updater.next()) {
            const QString defaultScenarioXml = BusinessLogic::ScenarioXml::defaultTextXml();
            const QString scenarioXml = q_updater.record().value("text").toString();
            const QString undoPatch = DiffMatchPatchHelper::makePatchXml(scenarioXml, defaultScenarioXml);
            const QString redoPatch = DiffMatchPatchHelper::makePatchXml(defaultScenarioXml, scenarioXml);
            q_updater.prepare("INSERT INTO scenario_changes (uuid, datetime, username, undo_patch, redo_patch) "
                              "VALUES(?, ?, ?, ?, ?)");
            q_updater.addBindValue(QUuid::createUuid().toString());
            q_updater.addBindValue(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss:zzz"));
            q_updater.addBindValue("user");
            q_updater.addBindValue(undoPatch);
            q_updater.addBindValue(redoPatch);
            q_updater.exec();
        }

    }

    _database.commit();
}

void Database::updateDatabaseTo_0_5_6(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Извлекаем текст сценария
        //
        q_updater.exec("SELECT id, text FROM scenario");
        QMap<int, QString> scenarioTexts;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString text = q_updater.record().value("text").toString();
            //
            // Заменяем старые теги на новые
            //
            text = text.replace("time_and_place", "scene_heading");
            scenarioTexts.insert(id, text);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE scenario SET text = ? WHERE id = ?");
        foreach (int id, scenarioTexts.keys()) {
            q_updater.addBindValue(scenarioTexts.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_5_8(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Добавляем таблицу "Разработка"
        //
        q_updater.exec("CREATE TABLE research "
                       "("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "parent_id INTEGER DEFAULT(NULL), "
                       "type INTEGER NOT NULL DEFAULT(0), "
                       "name TEXT NOT NULL, "
                       "description TEXT DEFAULT(NULL), "
                       "url TEXT DEFAULT(NULL), "
                       "image BLOB DEFAULT(NULL), "
                       "sort_order INTEGER NOT NULL DEFAULT(0) "
                       ")"
                       );
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_5_9(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Извлекаем текст сценария
        //
        q_updater.exec("SELECT id, text FROM scenario");
        QMap<int, QString> scenarioTexts;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString text = q_updater.record().value("text").toString();

            //
            // Вытаскиваем описание папок/групп/сцен и вставляем после заголовков или персонажей сцен
            //

            QStringList textLines = text.split("\n");
            QString lastDescription;
            const QString folderPrefix = "<folder_header description=\"";
            const QString sceneGroupPrefix = "<scene_group_header description=\"";
            const QString scenePrefix = "<scene_heading description=\"";
            const QString postfix = "\">";

            for (int lineNumber = 0; lineNumber < textLines.size(); ++lineNumber) {
                const QString& lineText = textLines.at(lineNumber);
                QString prefix;
                if (lineText.startsWith(scenePrefix)) {
                    prefix = scenePrefix;
                } else if (lineText.startsWith(folderPrefix)) {
                    prefix = folderPrefix;
                } else if (lineText.startsWith(sceneGroupPrefix)) {
                    prefix = sceneGroupPrefix;
                }
                //
                // ... если в блоке есть описание, вытаскиваем его
                //
                if (!prefix.isEmpty()) {
                    lastDescription = lineText.mid(prefix.length());
                    lastDescription = lastDescription.left(lastDescription.length() - postfix.length());
                    //
                    // ... если для элемента задан цвет, нужно и его отрезать от описания
                    //	   там получается вот такой текст [" color="#cc0000]
                    //
                    if (lastDescription.right(16).left(10) == "\" color=\"#") {
                        lastDescription = lastDescription.left(lastDescription.length() - 16);
                    }
                }
                //
                // ... проверяем не пора ли вставить описание
                //
                else if (!lastDescription.isEmpty()) {
                    bool needInsert = false;
                    //
                    // ... после сцены
                    //
                    if (lineText == "</scene_heading>") {
                        if (textLines.at(lineNumber + 1) == "<scene_characters>") {
                            while (textLines.at(++lineNumber) != "</scene_characters>");
                        }
                        needInsert = true;
                    }
                    //
                    // ... после папки или группы
                    //
                    else if (lineText == "</folder_header>"
                             || lineText == "</scene_group_header>") {
                        needInsert = true;
                    }

                    //
                    // ... вставляем, если нужно
                    //
                    if (needInsert) {
                        //
                        // ... убираем спецсимволы и тэги, пока не дойдём до содержимого
                        //
                        QTextDocument descriptionDoc;
                        descriptionDoc.setHtml(lastDescription);
                        descriptionDoc.setHtml(descriptionDoc.toPlainText());
                        descriptionDoc.setHtml(descriptionDoc.toPlainText());
                        //
                        QStringList descriptionLines = descriptionDoc.toPlainText().split("\n", QString::SkipEmptyParts);

                        foreach (const QString& descriptionLine, descriptionLines) {
                            textLines.insert(++lineNumber, "<scene_description>");
                            textLines.insert(++lineNumber, QString("<v><![CDATA[%1]]></v>").arg(descriptionLine));
                            textLines.insert(++lineNumber, "</scene_description>");
                        }

                        //
                        // ... очищаем, чтобы не вставлять его же в следующие сцены
                        //
                        lastDescription.clear();
                    }
                }
            }
            text = textLines.join("\n");
            scenarioTexts.insert(id, text);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE scenario SET text = ? WHERE id = ?");
        foreach (int id, scenarioTexts.keys()) {
            q_updater.addBindValue(scenarioTexts.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_7_0(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Добавление поля в таблицу сценария
        //
        q_updater.exec("ALTER TABLE scenario ADD COLUMN scheme TEXT NOT NULL DEFAULT('')");
    }

    _database.commit();
}

void Database::updateDatabaseTo_0_7_1(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    {
        //
        // Преобразуем группы сцен в папки
        //
        // ... извлекаем текст сценария
        //
        q_updater.exec("SELECT id, text FROM scenario");
        QMap<int, QString> scenarioTexts;
        while (q_updater.next()) {
            const int id = q_updater.record().value("id").toInt();
            QString text = q_updater.record().value("text").toString();
            //
            // Заменяем старые теги на новые
            //
            text = text.replace("scene_group", "folder");
            scenarioTexts.insert(id, text);
        }
        //
        // ... обновим данные
        //
        q_updater.prepare("UPDATE scenario SET text = ? WHERE id = ?");
        foreach (int id, scenarioTexts.keys()) {
            q_updater.addBindValue(scenarioTexts.value(id));
            q_updater.addBindValue(id);
            q_updater.exec();
        }

        //
        // Очищаем схемы карточек из прошлой версии
        //
        q_updater.exec("UPDATE scenario SET scheme = ''");

        //
        // Добавление полей с именем пользователя и временем в таблицу истории изменений данных
        //
        q_updater.exec("ALTER TABLE _database_history ADD COLUMN username TEXT NOT NULL DEFAULT('')");
        q_updater.exec("ALTER TABLE _database_history ADD COLUMN datetime TEXT NOT NULL DEFAULT('')");

        //
        // Удаляем таблицы персонажей, перенося данные в разработку
        //
        {
            QMap<int, QPair<QString, QString>> characters;
            q_updater.exec("SELECT id, name, real_name, description FROM characters");
            while (q_updater.next()) {
                const int id = q_updater.record().value("id").toInt();
                const QString name = q_updater.record().value("name").toString();
                const QString realName = q_updater.record().value("real_name").toString();
                const QString description = q_updater.record().value("description").toString();
                //
                // формируем данные для сохранения в новом хранилище
                //
                QString newDescription;
                QXmlStreamWriter writer(&newDescription);
                writer.writeStartDocument();
                writer.writeStartElement("character");
                writer.writeTextElement("real_name", realName);
                writer.writeStartElement("description");
                writer.writeCDATA(TextEditHelper::toHtmlEscaped(description));
                writer.writeEndElement();
                writer.writeEndElement();
                writer.writeEndDocument();
                //
                // запомним персонажа
                //
                characters.insert(id, qMakePair(name, newDescription));
            }
            //
            // ... проходим всех персонажей
            //
            for (const int& oldId : characters.keys()) {
                //
                // ... переносим персонажа в разработку
                //
                q_updater.prepare("INSERT INTO research (type, name, description) VALUES(?,?,?)");
                q_updater.addBindValue(Domain::Research::Character);
                q_updater.addBindValue(characters[oldId].first);
                q_updater.addBindValue(characters[oldId].second);
                q_updater.exec();
                const int newId = q_updater.lastInsertId().toInt();

                //
                // ... если у него есть фотки
                //
                q_updater.prepare("SELECT photo FROM characters_photo WHERE fk_character_id = ?");
                q_updater.addBindValue(oldId);
                q_updater.exec();
                QVector<QByteArray> photos;
                while (q_updater.next()) {
                    photos.append(q_updater.record().value("photo").toByteArray());
                }
                if (!photos.isEmpty()) {
                    //
                    // ... создаём вложенный элемент с фотками
                    //
                    q_updater.prepare("INSERT INTO research (parent_id, type, name) VALUES(?,?,?)");
                    q_updater.addBindValue(newId);
                    q_updater.addBindValue(Domain::Research::ImagesGallery);
                    q_updater.addBindValue(QApplication::translate("DatabaseLayer::Database", "Photos"));
                    q_updater.exec();
                    const int photosId = q_updater.lastInsertId().toInt();

                    //
                    // ...  переносим фотки в него
                    //
                    for (const QByteArray& photo : photos) {
                        q_updater.prepare("INSERT INTO research (parent_id, type, name, image) VALUES(?,?,?,?)");
                        q_updater.addBindValue(photosId);
                        q_updater.addBindValue(Domain::Research::Image);
                        q_updater.addBindValue(QApplication::translate("DatabaseLayer::Database", "Unnamed image"));
                        q_updater.addBindValue(photo);
                        q_updater.exec();
                    }
                }
            }

            //
            // ... удаляем данные из БД
            //
            q_updater.exec("DROP TABLE characters");
            q_updater.exec("DROP TABLE characters_photo");
        }

        //
        // Аналогично и для локаций
        //
        {
            QMap<int, QPair<QString, QString>> locations;
            q_updater.exec("SELECT id, name, description FROM locations");
            while (q_updater.next()) {
                const int id = q_updater.record().value("id").toInt();
                const QString name = q_updater.record().value("name").toString();
                const QString description = q_updater.record().value("description").toString();
                //
                // запомним локацию
                //
                locations.insert(id, qMakePair(name, description));
            }
            //
            // ... проходим все локации
            //
            for (const int& oldId : locations.keys()) {
                //
                // ... переносим локацию в разработку
                //
                q_updater.prepare("INSERT INTO research (type, name, description) VALUES(?,?,?)");
                q_updater.addBindValue(Domain::Research::Location);
                q_updater.addBindValue(locations[oldId].first);
                q_updater.addBindValue(locations[oldId].second);
                q_updater.exec();
                const int newId = q_updater.lastInsertId().toInt();

                //
                // ... если у неё есть фотки
                //
                q_updater.prepare("SELECT photo FROM locations_photo WHERE fk_location_id = ?");
                q_updater.addBindValue(oldId);
                q_updater.exec();
                QVector<QByteArray> photos;
                while (q_updater.next()) {
                    photos.append(q_updater.record().value("photo").toByteArray());
                }
                if (!photos.isEmpty()) {
                    //
                    // ... создаём вложенный элемент с фотками
                    //
                    q_updater.prepare("INSERT INTO research (parent_id, type, name) VALUES(?,?,?)");
                    q_updater.addBindValue(newId);
                    q_updater.addBindValue(Domain::Research::ImagesGallery);
                    q_updater.addBindValue(QApplication::translate("DatabaseLayer::Database", "Photos"));
                    q_updater.exec();
                    const int photosId = q_updater.lastInsertId().toInt();

                    //
                    // ...  переносим фотки в него
                    //
                    for (const QByteArray& photo : photos) {
                        q_updater.prepare("INSERT INTO research (parent_id, type, name, image) VALUES(?,?,?,?)");
                        q_updater.addBindValue(photosId);
                        q_updater.addBindValue(Domain::Research::Image);
                        q_updater.addBindValue(QApplication::translate("DatabaseLayer::Database", "Unnamed image"));
                        q_updater.addBindValue(photo);
                        q_updater.exec();
                    }
                }
            }

            //
            // ... удаляем данные из БД
            //
            q_updater.exec("DROP TABLE locations");
            q_updater.exec("DROP TABLE locations_photo");
        }
    }

    _database.commit();

    //
    // Уменьшим размер файла
    //
    q_updater.exec("VACUUM");
}

void Database::updateDatabaseTo_0_7_2(QSqlDatabase& _database)
{
    QSqlQuery q_updater(_database);

    _database.transaction();

    // Таблица "Переходы"
    q_updater.exec("CREATE TABLE transitions "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT UNIQUE NOT NULL "
                   "); "
                   );

    // Данные таблицы
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "CUT TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "FADE IN:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "FADE OUT"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "FADE TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "DISSOLVE TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "BACK TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "MATCH CUT TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "JUMP CUT TO:"))
                );
    q_updater.exec(
                QString("INSERT INTO transitions (id, name) VALUES (null, '%1');")
                .arg(QApplication::translate("DatabaseLayer::Database", "FADE TO BLACK"))
                );

    //
    // Добавляем колонку цветов в разработку
    //
    q_updater.exec("ALTER TABLE research ADD COLUMN color TEXT DEFAULT(NULL)");

    //
    // Таблица "Версии сценария"
    //
    q_updater.exec("CREATE TABLE script_versions "
                   "( "
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "fk_script_id INTEGER NOT NULL, "
                   "username TEXT NOT NULL, "
                   "datetime TEXT NOT NULL, "
                   "color TEXT DEFAULT(NULL), "
                   "name TEXT UNIQUE NOT NULL, "
                   "description TEXT DEFAULT(NULL), "
                   "script_text TEXT NOT NULL"
                   "); "
                   );
    //
    // ... и первая версия
    //
    q_updater.exec(
                QString("INSERT INTO script_versions (id, fk_script_id, username, datetime, color, name, script_text) "
                        "VALUES (null, 0, '', '%1', '#ffffff', '%2', '')")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz"))
                .arg(QApplication::translate("DatabaseLayer::Database", "First draft"))
                );

    _database.commit();
}
