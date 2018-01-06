#include "FdxImporter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QXmlStreamWriter>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Ключи для формирования xml из импортируемого документа
     */
    /** @{ */
    const QString NODE_SCENARIO = "scenario";
    const QString NODE_VALUE = "v";
    const QString NODE_FORMAT_GROUP = "formatting";
    const QString NODE_FORMAT = "format";

    const QString ATTRIBUTE_VERSION = "version";
    const QString ATTRIBUTE_DESCRIPTION = "description";
    const QString ATTRIBUTE_COLOR = "color";
    const QString ATTRIBUTE_TITLE = "title";
    const QString ATTRIBUTE_FORMAT_FROM = "from";
    const QString ATTRIBUTE_FORMAT_LENGTH = "length";
    const QString ATTRIBUTE_FORMAT_BOLD = "bold";
    const QString ATTRIBUTE_FORMAT_ITALIC = "italic";
    const QString ATTRIBUTE_FORMAT_UNDERLINE = "underline";
    /** @} */
}


FdxImporter::FdxImporter() :
    AbstractImporter()
{

}

QString FdxImporter::importScript(const ImportParameters& _importParameters) const
{
    QString scenarioXml;

    //
    // Открываем файл
    //
    QFile fdxFile(_importParameters.filePath);
    if (fdxFile.open(QIODevice::ReadOnly)) {
        //
        // Читаем XML
        //
        QDomDocument fdxDocument;
        fdxDocument.setContent(&fdxFile);
        //
        // ... и пишем в сценарий
        //
        QXmlStreamWriter writer(&scenarioXml);
        writer.writeStartDocument();
        writer.writeStartElement(NODE_SCENARIO);
        writer.writeAttribute(ATTRIBUTE_VERSION, "1.0");

        //
        // Content - текст сценария
        //
        QDomElement rootElement = fdxDocument.documentElement();
        QDomElement content = rootElement.firstChildElement("Content");
        QDomNode paragraph = content.firstChild();
        while (!paragraph.isNull()) {
            //
            // Определим тип блока
            //
            const QString paragraphType = paragraph.attributes().namedItem("Type").nodeValue();
            ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::Action;
            if (paragraphType == "Scene Heading") {
                blockType = ScenarioBlockStyle::SceneHeading;
            } else if (paragraphType == "Action") {
                blockType = ScenarioBlockStyle::Action;
            } else if (paragraphType == "Character") {
                blockType = ScenarioBlockStyle::Character;
            } else if (paragraphType == "Parenthetical") {
                blockType = ScenarioBlockStyle::Parenthetical;
            } else if (paragraphType == "Dialogue") {
                blockType = ScenarioBlockStyle::Dialogue;
            } else if (paragraphType == "Transition") {
                blockType = ScenarioBlockStyle::Transition;
            } else if (paragraphType == "Shot") {
                blockType = ScenarioBlockStyle::Note;
            } else if (paragraphType == "Cast List") {
                blockType = ScenarioBlockStyle::SceneCharacters;
            } else if (paragraphType == "Lyrics") {
                blockType = ScenarioBlockStyle::Lyrics;
            }

            //
            // Получим текст блока
            //
            QString paragraphText;
            QVector<TextFormat> formatting;
            {
                QDomElement textNode = paragraph.firstChildElement("Text");
                while (!textNode.isNull()) {
                    //
                    // ... читаем форматирование
                    //
                    if (textNode.hasAttribute("Style")) {
                        const QString style = textNode.attribute("Style");
                        TextFormat format;
                        format.start = paragraphText.length();
                        format.length = textNode.text().length();
                        format.bold = style.contains("Bold");
                        format.italic = style.contains("Italic");
                        format.underline = style.contains("Underline");
                        if (format.isValid()) {
                            formatting.append(format);
                        }
                    }
                    //
                    // ... читаем текст
                    //
                    paragraphText.append(textNode.text());

                    textNode = textNode.nextSiblingElement("Text");
                }
            }

            //
            // По возможности получим цвет, заголовок и описание сцены
            //
            QString sceneColor;
            QString sceneTitle;
            QString sceneDescription;
            QDomElement sceneProperties = paragraph.firstChildElement("SceneProperties");
            if (!sceneProperties.isNull()) {
                if (sceneProperties.hasAttribute("Color")) {
                    sceneColor = QColor(sceneProperties.attribute("Color")).name();
                }

                if (sceneProperties.hasAttribute("Title")) {
                    sceneTitle = sceneProperties.attribute("Title");
                }

                QDomElement summary = sceneProperties.firstChildElement("Summary");
                if (!summary.isNull()) {
                    QDomElement summaryParagraph = summary.firstChildElement("Paragraph");
                    while (!summaryParagraph.isNull()) {
                        QDomElement textNode = summaryParagraph.firstChildElement("Text");
                        while (!textNode.isNull()) {
                            sceneDescription.append(textNode.text());
                            textNode = textNode.nextSiblingElement("Text");
                        }
                        summaryParagraph = summaryParagraph.nextSiblingElement("Paragraph");
                    }
                }
            }

            //
            // Формируем блок сценария
            //
            const QString blockTypeName = ScenarioBlockStyle::typeName(blockType);
            writer.writeStartElement(blockTypeName);
            if (blockType == ScenarioBlockStyle::SceneHeading) {
                if (!sceneColor.isEmpty()) {
                    writer.writeAttribute(ATTRIBUTE_COLOR, sceneColor);
                }
                if (!sceneTitle.isEmpty()) {
                    writer.writeAttribute(ATTRIBUTE_TITLE, sceneTitle);
                }
            }
            writer.writeStartElement(NODE_VALUE);
            writer.writeCDATA(paragraphText);
            writer.writeEndElement();
            if (!formatting.isEmpty()) {
                writer.writeStartElement(NODE_FORMAT_GROUP);
                for (const TextFormat& format : formatting){
                    writer.writeStartElement(NODE_FORMAT);
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
            writer.writeEndElement();

            //
            // Если есть описание, то добавляем ещё один блок с ним
            //
            if (!sceneDescription.isEmpty()) {
                writer.writeStartElement(ScenarioBlockStyle::typeName(ScenarioBlockStyle::SceneDescription));
                writer.writeStartElement(NODE_VALUE);
                writer.writeCDATA(sceneDescription);
                writer.writeEndElement();
                writer.writeEndElement();
            }

            //
            // Переходим к следующему
            //
            paragraph = paragraph.nextSibling();
        }
    }

    return scenarioXml;
}
