/*
 * Copyright (C) 2018  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: http://dimkanovikov.pro/license/LGPLv3
 */

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

        /**
         * @brief Импорт данных разработки
         */
        QVariantMap importResearch(const ImportParameters& _importParameters) const override;

    private:
        /**
         * @brief Прочитать содержимое сценария из заданного файла
         */
        QString readScript(const QString& _filePath) const;
    };
}

#endif // CELTXIMPORTER_H
