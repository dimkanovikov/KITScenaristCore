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

#include "CeltxImporter.h"

#include "qtzip/QtZipReader"

#include <qgumbodocument.h>
#include <qgumbonode.h>

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <QFileInfo>
#include <QXmlStreamWriter>

using BusinessLogic::AbstractImporter;
using BusinessLogic::CeltxImporter;
using BusinessLogic::ImportParameters;
using BusinessLogic::ScenarioBlockStyle;

namespace {
    /**
     * @brief Ключ для формирования xml из импортируемого документа
     */
    /** @{ */
    const QString NODE_SCENARIO = "scenario";
    const QString NODE_VALUE = "v";
    const QString NODE_FORMAT_GROUP = "formatting";
    const QString NODE_FORMAT = "format";

    const QString ATTRIBUTE_FORMAT_FROM = "from";
    const QString ATTRIBUTE_FORMAT_LENGTH = "length";
    const QString ATTRIBUTE_FORMAT_BOLD = "bold";
    const QString ATTRIBUTE_FORMAT_ITALIC = "italic";
    const QString ATTRIBUTE_FORMAT_UNDERLINE = "underline";

    const QString ATTRIBUTE_VERSION = "version";
    /** @} */

    /**
     * @brief Карта соответствия ключей титульной страницы
     */
    const QMap<QString, QString> kTitleKeys({{"Author", "author"},
                                             {"DC.rights", "year"},
                                             {"CX.contact", "contacts"},
                                             {"CX.byline", "genre"},
                                             {"DC.source", "additional_info"}});
}


CeltxImporter::CeltxImporter() :
    AbstractImporter()
{
}

QString CeltxImporter::importScript(const BusinessLogic::ImportParameters& _importParameters) const
{
    QString scriptXml;

    const QString scriptHtml = readScript(_importParameters.filePath);
    if (!scriptHtml.isEmpty()) {
        //
        // Читаем html text
        //
        // ... и пишем в сценарий
        //
        QXmlStreamWriter writer(&scriptXml);
        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(true);
        writer.writeStartDocument();
        writer.writeStartElement(NODE_SCENARIO);
        writer.writeAttribute(ATTRIBUTE_VERSION, "1.0");

        //
        // Получим список строк текста
        //
        const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
        const auto rootNode = scriptDocument.rootNode();
        const auto pNodes = rootNode.getElementsByTagName(HtmlTag::P);
        for (const auto& pNode : pNodes) {
            const QString pNodeClass = pNode.getAttribute("class");
            ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::Action;
            if (pNodeClass == "sceneheading") {
                blockType = ScenarioBlockStyle::SceneHeading;
            } else if (pNodeClass == "character") {
                blockType = ScenarioBlockStyle::Character;
            } else if (pNodeClass == "parenthetical") {
                blockType = ScenarioBlockStyle::Parenthetical;
            } else if (pNodeClass == "dialog") {
                blockType = ScenarioBlockStyle::Dialogue;
            } else if (pNodeClass == "transition") {
                blockType = ScenarioBlockStyle::Transition;
            }

            const QString& blockTypeName = ScenarioBlockStyle::typeName(blockType);
            writer.writeStartElement(blockTypeName);
            writer.writeStartElement(NODE_VALUE);
            writer.writeCDATA(pNode.innerText());
            writer.writeEndElement();
        }

        writer.writeEndDocument();
    }

    return scriptXml;
}

QVariantMap CeltxImporter::importResearch(const BusinessLogic::ImportParameters& _importParameters) const
{
    const QString scriptHtml = readScript(_importParameters.filePath);

    //
    // Прочитать содержимое файла
    //
    QVariantMap titlePage;
    if (!scriptHtml.isEmpty()) {
        const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
        const auto rootNode = scriptDocument.rootNode();
        //
        // Название
        //
        const auto titleNodes = rootNode.getElementsByTagName(HtmlTag::TITLE);
        if (!titleNodes.empty()) {
            titlePage["name"] = titleNodes.front().innerText();
        }
        //
        // Другие поля титульной страницы
        //
        const auto metaNodes = rootNode.getElementsByTagName(HtmlTag::META);
        for (const auto& metaNode : metaNodes) {
            const QString metaNodeName = metaNode.getAttribute("name");
            if (kTitleKeys.contains(metaNodeName)) {
                const QString metaNodeValue = metaNode.getAttribute("content");
                titlePage[kTitleKeys[metaNodeName]] = metaNodeValue;
            }
        }
    }

    QVariantMap result;
    result["script"] = titlePage;
    return result;
}

QString CeltxImporter::readScript(const QString& _filePath) const
{
    //
    // Открыть архив и извлечь содержимое файла, название которого начинается со "script"
    //
    QFile celtxFile(_filePath);
    QString scriptHtml;
    if (celtxFile.open(QIODevice::ReadOnly)) {
        QtZipReader zip(&celtxFile);
        if (zip.isReadable()) {
            for (const QString& fileName : zip.fileList()) {
                if (fileName.startsWith("script")) {
                    scriptHtml = zip.fileData(fileName);
                    break;
                }
            }
        }
        zip.close();
    }
    celtxFile.close();

    return scriptHtml;
}
