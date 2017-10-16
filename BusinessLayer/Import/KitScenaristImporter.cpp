#include "KitScenaristImporter.h"

#include <Domain/Research.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>

using namespace BusinessLogic;

namespace {
    const QString SQL_DRIVER = "QSQLITE";
    const QString CONNECTION_NAME = "import_database";

    /**
     * @brief Добавить данные из sql-записи в карту по заданному ключу
     */
    static void storeField(const QString& _key, const QSqlQuery& _query, QVariantMap& _data) {
        _data[_key] = _query.value(_key);
    }
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
            //
            // Загрузить информацию о сценарии
            //
            {
                QSqlQuery scripQuery(database);
                scripQuery.exec("SELECT * FROM scenario_data");
                QVariantMap script;
                while (scripQuery.next()) {
                    const QString key = scripQuery.value("data_name").toString();
                    const QString value = scripQuery.value("data_value").toString();
                    script[key] = value;
                }
                research["script"] = script;
            }

            //
            // Выгрузить данные о персонажах
            //
            {
                QSqlQuery charactersQuery(database);
                charactersQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                charactersQuery.addBindValue(Domain::Research::Character);
                charactersQuery.exec();
                QVariantList characters;
                while (charactersQuery.next()) {
                    const QVariantMap character = loadResearchDocument(charactersQuery, database);
                    characters.append(character);
                }
                research["characters"] = characters;
            }

            //
            // Выгрузить данные о локациях
            //
            {
                QSqlQuery locationsQuery(database);
                locationsQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                locationsQuery.addBindValue(Domain::Research::Location);
                locationsQuery.exec();
                QVariantList locations;
                while (locationsQuery.next()) {
                    const QVariantMap location = loadResearchDocument(locationsQuery, database);
                    locations.append(location);
                }
                research["locations"] = locations;
            }

            //
            // Выгрузить документы разработки
            //
            {
                QSqlQuery documentsQuery(database);
                documentsQuery.prepare("SELECT * FROM research WHERE type NOT IN (?, ?) AND parent_id IS NULL ORDER by sort_order");
                documentsQuery.addBindValue(Domain::Research::Character);
                documentsQuery.addBindValue(Domain::Research::Location);
                documentsQuery.exec();
                QVariantList documents;
                while (documentsQuery.next()) {
                    const QVariantMap document = loadResearchDocument(documentsQuery, database);
                    documents.append(document);
                }
                research["documents"] = documents;
            }
        }
    }

    QSqlDatabase::removeDatabase(CONNECTION_NAME);

    return research;
}

QVariantMap KitScenaristImporter::loadResearchDocument(const QSqlQuery& _query, QSqlDatabase& _database) const
{
    //
    // Т.к. разработка может быть довольно большой, то нужно давать выполняться гуи-потоку,
    // чтобы не было ощущения зависания
    //
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    //
    // Извлечём общие данные для документа
    //
    QVariantMap document;
    ::storeField("type", _query, document);
    ::storeField("name", _query, document);
    ::storeField("description", _query, document);
    ::storeField("sort_order", _query, document);
    const int documentType = document["type"].toInt();
    //
    // Извлечём данные зависящие от типа
    //
    switch (documentType) {
        case Domain::Research::Character:
        case Domain::Research::Location:
        case Domain::Research::Folder:
        case Domain::Research::ImagesGallery: {
            //
            // Найдём все вложенные документы
            //
            const int folderId = _query.value("id").toInt();
            document["childs"] = loadResearchDocuments(folderId, _database);
            break;
        }

        case Domain::Research::Url: {
            ::storeField("url", _query, document);
            break;
        }

        case Domain::Research::Image: {
            ::storeField("image", _query, document);
            break;
        }

        default: break;
    }

    return document;
}

QVariantList KitScenaristImporter::loadResearchDocuments(int _parentId, QSqlDatabase& _database) const
{
    QVariantList documents;

    QSqlQuery documentsQuery(_database);
    documentsQuery.prepare("SELECT * FROM research WHERE parent_id = ? ORDER by sort_order");
    documentsQuery.addBindValue(_parentId);
    documentsQuery.exec();
    while (documentsQuery.next()) {
        //
        // Извлечём общие данные для документа
        //
        const QVariantMap document = loadResearchDocument(documentsQuery, _database);

        //
        // Сохраним документ
        //
        documents.append(document);
    }

    return documents;
}
