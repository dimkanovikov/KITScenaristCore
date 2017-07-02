#include "KitScenaristImporter.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>

using namespace BusinessLogic;

namespace {
    const QString SQL_DRIVER = "QSQLITE";
    const QString CONNECTION_NAME = "import_database";
}


KitScenaristImporter::KitScenaristImporter() :
    AbstractImporter()
{

}

QString KitScenaristImporter::importScenario(const ImportParameters& _importParameters) const
{
    QString result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(SQL_DRIVER, CONNECTION_NAME);
        database.setDatabaseName(_importParameters.filePath);
        if (database.open()) {
            QSqlQuery query(database);
            query.exec("SELECT text FROM scenario WHERE is_draft = 0");
            query.next();
            result = query.record().value("text").toString();
        }
    }

    QSqlDatabase::removeDatabase(CONNECTION_NAME);

    return result;
}

QVariantMap KitScenaristImporter::importResearch(const ImportParameters& _importParameters) const
{
    QVariantMap research;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(SQL_DRIVER, CONNECTION_NAME);
        database.setDatabaseName(_importParameters.filePath);
        if (database.open()) {
            QSqlQuery query(database);

            //
            // Загрузить информацию о сценарии
            //
            QVariantMap script;
            query.exec("SELECT * FROM scenario_data");
            while (query.next()) {
                const QString key = query.record().value("data_name").toString();
                const QString value = query.record().value("data_value").toString();
                script[key] = value;
            }
            research["script"] = script;

            //
            // Выгрузить данные о персонажах
            //
            {
                //
                // Получить список персонажей
                //
                {
                    //
                    // Для каждого персонажа получить список вложенных документов
                    //
                    {
                        //
                        // Для каждого вложенного документа получить список вложенных документов | зациклить
                        //
                    }
                }
            }

            //
            // Выгрузить данные о локациях
            //

            //
            // Выгрузить документы разработки
            //
        }
    }

    QSqlDatabase::removeDatabase(CONNECTION_NAME);

    return research;
}
