#ifndef TRELBYIMPORTER_H
#define TRELBYIMPORTER_H

#include "AbstractImporter.h"


namespace BusinessLogic
{
    /**
     * @brief Импортер Trelby-документов
     */
    class TrelbyImporter : public AbstractImporter
    {
    public:
        TrelbyImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScript(const ImportParameters& _importParameters) const override;
    };
}

#endif // TRELBYIMPORTER_H
