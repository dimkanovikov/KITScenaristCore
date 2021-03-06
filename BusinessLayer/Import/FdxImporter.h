#ifndef FDXIMPORTER_H
#define FDXIMPORTER_H

#include "AbstractImporter.h"


namespace BusinessLogic
{
    /**
     * @brief Импортер FDX-документов
     */
    class FdxImporter : public AbstractImporter
    {
    public:
        FdxImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScript(const ImportParameters& _importParameters) const override;
    };
}

#endif // FDXIMPORTER_H
