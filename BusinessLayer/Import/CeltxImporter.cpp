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
    const QString kNodeScript = "scenario";
    const QString kNodeValue = "v";
    const QString kNodeReviewGroup = "reviews";
    const QString kNodeReview = "review";
    const QString kNodeReviewComment = "review_comment";
    const QString kNodeFormatGroup = "formatting";
    const QString kNodeFormat = "format";

    const QString ATTRIBUTE_REVIEW_FROM = "from";
    const QString ATTRIBUTE_REVIEW_LENGTH = "length";
    const QString ATTRIBUTE_REVIEW_COLOR = "color";
    const QString ATTRIBUTE_REVIEW_BGCOLOR = "bgcolor";
    const QString ATTRIBUTE_REVIEW_IS_HIGHLIGHT = "is_highlight";
    const QString ATTRIBUTE_REVIEW_COMMENT = "comment";
    const QString ATTRIBUTE_REVIEW_AUTHOR = "author";
    const QString ATTRIBUTE_REVIEW_DATE = "date";
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
    const QMap<QString, QString> kTitleKeys({std::make_pair(QString("Author"), QString("author")),
                                             std::make_pair(QString("DC.rights"), QString("year")),
                                             std::make_pair(QString("CX.contact"), QString("contacts")),
                                             std::make_pair(QString("CX.byline"), QString("genre")),
                                             std::make_pair(QString("DC.source"), QString("additional_info"))});
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
        writer.writeStartElement(kNodeScript);
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
            writer.writeStartElement(kNodeValue);
            //
            // Сформируем текст
            //
            QString paragraphText = pNode.innerText();
            QVector<TextFormat> formatting;
            QVector<TextNote> notes;
            for (const auto& child : pNode.children()) {
                if (child.tag() != HtmlTag::SPAN) {
                    continue;
                }

                //
                // Обработать заметку
                //
                if (child.hasAttribute("text")) {
                    TextNote note;
                    note.isFullBlock = true;
                    note.comment = child.getAttribute("text");
                    QString colorText = child.getAttribute("style");
                    colorText = colorText.remove(0, colorText.indexOf("(") + 1);
                    colorText = colorText.left(colorText.indexOf(")"));
                    colorText = colorText.remove(" ");
                    const QStringList colorComponents = colorText.split(",");
                    if (colorComponents.size() == 3) {
                        note.color = QColor(colorComponents[0].toInt(), colorComponents[1].toInt(), colorComponents[2].toInt());
                    }
                    if (note.isValid()) {
                        notes.append(note);
                    }
                }
                //
                // Обработать форматированный текст
                //
                else {
                    const QString childText = child.innerText();
                    const int childStartPosition = pNode.childStartPosition(child);
                    paragraphText.insert(childStartPosition, childText);

                    const QString style = child.getAttribute("style");
                    TextFormat format;
                    format.start = childStartPosition;
                    format.length = childText.length();
                    format.bold = style.contains("font-weight: bold;");
                    format.italic = style.contains("font-style: italic;");
                    format.underline = style.contains("text-decoration: underline;");
                    if (format.isValid()) {
                        formatting.append(format);
                    }
                }
            }
            writer.writeCDATA(paragraphText);
            writer.writeEndElement();

            //
            // Добавляем форматирование
            //
            if (!formatting.isEmpty()) {
                writer.writeStartElement(kNodeFormatGroup);
                for (const TextFormat& format : formatting){
                    writer.writeStartElement(kNodeFormat);
                    //
                    // Данные пользовательского форматирования
                    //
                    writer.writeAttribute(ATTRIBUTE_FORMAT_FROM, QString::number(format.start));
                    writer.writeAttribute(ATTRIBUTE_FORMAT_LENGTH, QString::number(format.length));
                    writer.writeAttribute(ATTRIBUTE_FORMAT_BOLD, format.bold ? "true" : "false");
                    writer.writeAttribute(ATTRIBUTE_FORMAT_ITALIC, format.italic? "true" : "false");
                    writer.writeAttribute(ATTRIBUTE_FORMAT_UNDERLINE, format.underline ? "true" : "false");
                    //
                    writer.writeEndElement();
                }
                writer.writeEndElement();
            }

            //
            // Добавляем заметки
            //
            if (!notes.isEmpty()) {
                writer.writeStartElement(kNodeReviewGroup);
                for (const TextNote& note : notes) {
                    writer.writeStartElement(kNodeReview);
                    writer.writeAttribute(ATTRIBUTE_REVIEW_FROM, "0");
                    writer.writeAttribute(ATTRIBUTE_REVIEW_LENGTH, QString::number(paragraphText.length()));
                    writer.writeAttribute(ATTRIBUTE_REVIEW_COLOR, "#000000");
                    writer.writeAttribute(ATTRIBUTE_REVIEW_BGCOLOR, note.color.name());
                    writer.writeAttribute(ATTRIBUTE_REVIEW_IS_HIGHLIGHT, "false");

                    writer.writeEmptyElement(kNodeReviewComment);
                    writer.writeAttribute(ATTRIBUTE_REVIEW_COMMENT, note.comment);

                    writer.writeEndElement();
                }
                writer.writeEndElement();
            }

            writer.writeEndElement(); // NODE_VALUE
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
