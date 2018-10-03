#include "FdxExporter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QFile>
#include <QTextBlock>
#include <QTextCursor>
#include <QXmlStreamWriter>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Стиль экспорта
     */
    static ScenarioTemplate exportStyle() {
        return ScenarioTemplateFacade::getTemplate(
                    DataStorageLayer::StorageFacade::settingsStorage()->value(
                        "export/style",
                        DataStorageLayer::SettingsStorage::ApplicationSettings)
                    );
    }
}


FdxExporter::FdxExporter() :
    AbstractExporter()
{

}

void FdxExporter::exportTo(ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const
{
    //
    // Открываем документ на запись
    //
    QFile fdxFile(_exportParameters.filePath);
    if (fdxFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QXmlStreamWriter writer(&fdxFile);
        //
        // Начало документа
        //
        writer.writeStartDocument();
        writer.writeStartElement("FinalDraft");
        writer.writeAttribute("DocumentType", "Script");
        writer.writeAttribute("Template", "No");
        writer.writeAttribute("Version", "3");
        //
        // Текст сценария
        //
        writeContent(writer, _scenario, _exportParameters);
        //
        // Параметры сценария
        //
        writeSettings(writer);
        //
        // Титульная страница
        //
        writeTitlePage(writer, _exportParameters);
        //
        // Конец документа
        //
        writer.writeEndDocument();

        fdxFile.close();
    }
}

void FdxExporter::exportTo(const ResearchModelCheckableProxy* _researchModel, const ExportParameters& _exportParameters) const
{
    Q_UNUSED(_researchModel);
    Q_UNUSED(_exportParameters);
}

void FdxExporter::writeContent(QXmlStreamWriter& _writer, ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const
{
    _writer.writeStartElement("Content");

    //
    // Используем ненастоящие параметры экспорта, если надо, то обрабатываем их вручную
    //
    ExportParameters fakeParameters;
    fakeParameters.isOutline = true;
    fakeParameters.isScript = _exportParameters.isScript;
    fakeParameters.printScenesNumbers = false;

    //
    // Сформируем документ
    //
    QTextDocument* preparedDocument = prepareDocument(_scenario, fakeParameters);

    //
    // Данные считываются из исходного документа, определяется тип блока
    // и записываются прямо в файл
    //
    QTextCursor documentCursor(preparedDocument);
    while (!documentCursor.atEnd()) {
        if (!documentCursor.block().text().isEmpty()) {
            QString paragraphType;
            QString sceneNumber;
            switch (ScenarioBlockStyle::forBlock(documentCursor.block())) {
                case ScenarioBlockStyle::SceneHeading: {
                    paragraphType = "Scene Heading";

                    //
                    // ... если надо, то выводим номера сцен
                    //
                    if (_exportParameters.printScenesNumbers) {
                        if (SceneHeadingBlockInfo* sceneInfo = dynamic_cast<SceneHeadingBlockInfo*>(documentCursor.block().userData())) {
                            sceneNumber = QString("%1%2").arg(_exportParameters.scenesPrefix).arg(sceneInfo->sceneNumber());
                        }
                    }

                    break;
                }

                case ScenarioBlockStyle::SceneDescription:
                case ScenarioBlockStyle::Action:
                case ScenarioBlockStyle::TitleHeader:{
                    paragraphType = "Action";
                    break;
                }

                case ScenarioBlockStyle::Character: {
                    paragraphType = "Character";
                    break;
                }

                case ScenarioBlockStyle::Parenthetical:
                case ScenarioBlockStyle::Title: {
                    paragraphType = "Parenthetical";
                    break;
                }

                case ScenarioBlockStyle::Dialogue: {
                    paragraphType = "Dialogue";
                    break;
                }

                case ScenarioBlockStyle::Transition: {
                    paragraphType = "Transition";
                    break;
                }

                case ScenarioBlockStyle::Note: {
                    paragraphType = "Shot";
                    break;
                }

                case ScenarioBlockStyle::SceneCharacters: {
                    paragraphType = "Cast List";
                    break;
                }

                case ScenarioBlockStyle::Lyrics: {
                    paragraphType = "Lyrics";
                    break;
                }

                default: {
                    paragraphType = "General";
                    break;
                }
            }

            _writer.writeStartElement("Paragraph");
            if (!sceneNumber.isEmpty()) {
                _writer.writeAttribute("Number", sceneNumber);
            }
            _writer.writeAttribute("Type", paragraphType);
            for (const QTextLayout::FormatRange& range : documentCursor.block().textFormats()) {
                _writer.writeStartElement("Text");
                //
                // Пишем стиль блока
                //
                QString style;
                QString styleDivider;
                if (range.format.fontWeight() == QFont::Bold) {
                    style = "Bold";
                    styleDivider = "+";
                }
                if (range.format.fontItalic()) {
                    style += styleDivider + "Italic";
                    styleDivider = "+";
                }
                if (range.format.fontUnderline()) {
                    style += styleDivider + "Underline";
                }
                //
                if (!style.isEmpty()) {
                    _writer.writeAttribute("Style", style);
                }
                //
                // Пишем текст
                //
                _writer.writeCharacters(documentCursor.block().text().mid(range.start, range.length));
                //
                _writer.writeEndElement();
            }
            _writer.writeEndElement(); // Paragraph
        }

        //
        // Переходим к следующему параграфу
        //
        documentCursor.movePosition(QTextCursor::EndOfBlock);
        documentCursor.movePosition(QTextCursor::NextBlock);
    }

    _writer.writeEndElement(); // Content
}

void FdxExporter::writeSettings(QXmlStreamWriter& _writer) const
{
    //
    // Информация о параметрах страницы
    //
    ScenarioTemplate exportStyle = ::exportStyle();
    QString pageHeight, pageWidth;
    switch (exportStyle.pageSizeId()) {
        case QPageSize::A4: {
            pageHeight = "11.69";
            pageWidth = "8.26";
            break;
        }

        case QPageSize::Letter: {
            pageHeight = "11.00";
            pageWidth = "8.50";
            break;
        }

        default: {
            qWarning("Unknown page size");
            break;
        }
    }

    //
    // Пишем параметры страницы
    //
    _writer.writeStartElement("PageLayout");
    _writer.writeAttribute("BackgroundColor", "#FFFFFFFFFFFF");
    _writer.writeAttribute("BottomMargin", "72"); // 1.000"
    _writer.writeAttribute("BreakDialogueAndActionAtSentences", "Yes");
    _writer.writeAttribute("DocumentLeading", "Normal");
    _writer.writeAttribute("FooterMargin", "36"); // 0.500"
    _writer.writeAttribute("ForegroundColor", "#000000000000");
    _writer.writeAttribute("HeaderMargin", "36"); // 0.500"
    _writer.writeAttribute("InvisiblesColor", "#808080808080");
    _writer.writeAttribute("TopMargin", "72"); // 1.000"
    _writer.writeAttribute("UsesSmartQuotes", "Yes");
    //
    _writer.writeEmptyElement("PageSize");
    _writer.writeAttribute("Height", pageHeight);
    _writer.writeAttribute("Width", pageWidth);
    //
    _writer.writeEmptyElement("AutoCastList");
    _writer.writeAttribute("AddParentheses", "Yes");
    _writer.writeAttribute("AutomaticallyGenerate", "Yes");
    _writer.writeAttribute("CastListElement", "Cast List");
    //
    _writer.writeEndElement(); // PageLayout
}

void FdxExporter::writeTitlePage(QXmlStreamWriter& _writer, const ExportParameters& _exportParameters) const
{
    if (!_exportParameters.printTilte) {
        return;
    }

    auto writeLine =
            [&_writer] (const QString& _alignment,
                        const QString& _content,
                        const QString& _style) {
        _writer.writeStartElement("Paragraph");
        _writer.writeAttribute("Type", "General");
        _writer.writeAttribute("Alignment", _alignment);
        _writer.writeAttribute("LeftIndent", "1.25");
        _writer.writeAttribute("RightIndent", "7.25");
        _writer.writeAttribute("SpaceBefore", "0");
        _writer.writeAttribute("Spacing", "1.0");
        _writer.writeAttribute("StartsNewPage", "No");
        if (!_content.isEmpty()) {
            _writer.writeStartElement("Text");
            if (!_style.isEmpty()) {
                _writer.writeAttribute("Style", _style);
            }
            _writer.writeCharacters(_content);
            _writer.writeEndElement(); // Text
        }
        _writer.writeEndElement(); // Paragraph
    };
    auto writeEmptyLines = [&writeLine] (int _count) {
        for (int i = 0; i < _count; ++i) {
            writeLine(QString("Left"), QString{}, QString{});
        }
    };

    _writer.writeStartElement("TitlePage");
    _writer.writeStartElement("Content");
    //
    writeEmptyLines(16);
    writeLine("Center", _exportParameters.scriptName, "Underline");
    writeEmptyLines(3);
    writeLine("Center", "Written by", QString{});
    writeEmptyLines(1);
    writeLine("Center", _exportParameters.scriptAuthor, QString{});
    writeEmptyLines(20);
    writeLine("Left", _exportParameters.scriptYear, QString{});
    writeEmptyLines(1);
    writeLine("Left", _exportParameters.scriptContacts, QString{});

    _writer.writeEndElement(); // Content
    _writer.writeEndElement(); // Title Page
}
