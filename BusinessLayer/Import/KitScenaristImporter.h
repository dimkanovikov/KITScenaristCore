#ifndef KITSCENARISTIMPORTER_H
#define KITSCENARISTIMPORTER_H

#include "AbstractImporter.h"

class QSqlDatabase;
class QSqlQuery;


namespace BusinessLogic
{
    /**
     * @brief Импортер FDX-документов
     */
    class KitScenaristImporter : public AbstractImporter
    {
    public:
        KitScenaristImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScenario(const ImportParameters& _importParameters) const override;

        /**
         * @brief Импорт данных разработки из документа
         */
        QVariantMap importResearch(const ImportParameters &_importParameters) const override;

    private:
        /**
         * @brief Загружить документ разработки из заданного запроса
         */
        QVariantMap loadResearchDocument(const QSqlQuery& _query, QSqlDatabase& _database) const;

        /**
         * @brief Загрузить документы разработки для указанного элемента в заданной базе данных
         */
        QVariantList loadResearchDocuments(int _parentId, QSqlDatabase& _database) const;
    };
}

#endif // KITSCENARISTIMPORTER_H
