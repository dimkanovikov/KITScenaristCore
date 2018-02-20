#ifndef CELTXIMPORTER_H
#define CELTXIMPORTER_H

#include "AbstractImporter.h"


namespace BusinessLogic
{
    /**
     * @brief Импортер Celtx-документов
     */
    class CeltxImporter : public AbstractImporter
    {
    public:
        CeltxImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScript(const ImportParameters& _importParameters) const override;
    };
}

#endif // CELTXIMPORTER_H
