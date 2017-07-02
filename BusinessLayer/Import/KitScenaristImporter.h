#ifndef KITSCENARISTIMPORTER_H
#define KITSCENARISTIMPORTER_H

#include "AbstractImporter.h"


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
    };
}

#endif // KITSCENARISTIMPORTER_H
