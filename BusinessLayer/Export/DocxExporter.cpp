#include "DocxExporter.h"

#include "qtzip/QtZipWriter"

#include "format_helpers.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <Domain/Scenario.h>

#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Widgets/PagesTextEdit/PageMetrics.h>

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QFile>
#include <QChar>

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

    /**
     * @brief Перевести миллиметры в твипсы (мера длины в формате RTF)
     */
    static int mmToTwips(qreal _mm) {
        return 56.692913386 * _mm;
    }

    /**
     * @brief Пронумерованный список типов блоков
     */
    static QMap<int, ScenarioBlockStyle::Type> blockTypes() {
        static QMap<int, ScenarioBlockStyle::Type> s_types;
        if (s_types.isEmpty()) {
            s_types.insert(0, ScenarioBlockStyle::Undefined);
            s_types.insert(1, ScenarioBlockStyle::SceneHeading);
            s_types.insert(2, ScenarioBlockStyle::SceneCharacters);
            s_types.insert(3, ScenarioBlockStyle::Action);
            s_types.insert(4, ScenarioBlockStyle::Character);
            s_types.insert(5, ScenarioBlockStyle::Parenthetical);
            s_types.insert(6, ScenarioBlockStyle::Dialogue);
            s_types.insert(7, ScenarioBlockStyle::Transition);
            s_types.insert(8, ScenarioBlockStyle::Note);
            s_types.insert(9, ScenarioBlockStyle::TitleHeader);
            s_types.insert(10, ScenarioBlockStyle::Title);;
            s_types.insert(11, ScenarioBlockStyle::SceneDescription);
            s_types.insert(12, ScenarioBlockStyle::Lyrics);
        }
        return s_types;
    }

    /**
     * @brief Нужно ли записывать шрифты в файл
     */
    static bool needWriteFonts() {
        const ScenarioTemplate exportTemplate = exportStyle();
        for (const ScenarioBlockStyle::Type type : blockTypes().values()) {
            if (exportTemplate.blockStyle(type).font().family() == "Courier Prime") {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Необходимо ли писать верхний колонтитул
     */
    static bool needWriteHeader(const ExportParameters& _exportParameters) {
        return (_exportParameters.printPagesNumbers
                && exportStyle().numberingAlignment().testFlag(Qt::AlignTop))
                || !_exportParameters.scriptHeader.isEmpty();
    }

    /**
     * @brief Необходимо ли писать нижний колонтитул
     */
    static bool needWriteFooter(const ExportParameters& _exportParameters) {
        return (_exportParameters.printPagesNumbers
                && exportStyle().numberingAlignment().testFlag(Qt::AlignBottom))
                || !_exportParameters.scriptFooter.isEmpty();
    }

    /**
     * @brief Сформировать строку DOCX-стиля из стиля блока
     */
    static QString docxBlockStyle(const ScenarioBlockStyle& _style, const QString& _defaultFontFamily) {
        QString blockStyle;

        if (_style.type() != ScenarioBlockStyle::Undefined) {
            //
            // Стиль
            //
            blockStyle.append(
                QString("<w:style w:type=\"paragraph\" w:styleId=\"%1\">")
                .arg(ScenarioBlockStyle::typeName(_style.type()).toUpper().replace("_", ""))
                );

            //
            // Наименование
            //
            blockStyle.append(
                QString("<w:name w:val=\"%1\"/>")
                .arg(ScenarioBlockStyle::typeName(_style.type()).toUpper().replace("_", " "))
                );

            //
            // Свойства
            //
            blockStyle.append("<w:pPr>");
            //
            // ... разрешаем перенос абзацев между страницами и висячие строки
            //
            blockStyle.append("<w:widowControl w:val=\"0\"/><w:autoSpaceDE w:val=\"0\"/><w:autoSpaceDN w:val=\"0\"/><w:adjustRightInd w:val=\"0\"/>");
            //
            // ... отступы
            //
            if (QLocale().textDirection() == Qt::LeftToRight) {
                blockStyle.append(
                            QString("<w:ind w:left=\"%1\" w:right=\"%2\"/>")
                            .arg(mmToTwips(_style.leftMargin()))
                            .arg(mmToTwips(_style.rightMargin()))
                            );
            } else {
                blockStyle.append(
                            QString("<w:ind w:left=\"%1\" w:right=\"%2\"/>")
                            .arg(mmToTwips(_style.rightMargin()))
                            .arg(mmToTwips(_style.leftMargin()))
                    );
            }
            //
            // ... интервалы
            //
            blockStyle.append(
                QString("<w:spacing w:before=\"%1\" w:after=\"%2\" ")
                .arg(mmToTwips(_style.topMargin()))
                .arg(mmToTwips(_style.bottomMargin()))
                );
            // ... межстрочный
            int lineSpacing = 240;
            QString lineSpacingType = "auto";
            switch (_style.lineSpacing()) {
                default:
                case ScenarioBlockStyle::SingleLineSpacing: {
                    break;
                }

                case ScenarioBlockStyle::OneAndHalfLineSpacing: {
                    lineSpacing = 360;
                    break;
                }

                case ScenarioBlockStyle::DoubleLineSpacing: {
                    lineSpacing = 480;
                    break;
                }

                case ScenarioBlockStyle::FixedLineSpacing: {
                    lineSpacing = mmToTwips(_style.lineSpacingValue());
                    lineSpacingType = "exact";
                    break;
                }
            }
            blockStyle.append(
                QString("w:line=\"%1\" w:lineRule=\"%2\"/>")
                .arg(lineSpacing)
                .arg(lineSpacingType)
                );
            //
            // ... выравнивание
            //
            blockStyle.append("<w:jc w:val=\"");
            switch (_style.blockFormat().alignment()) {
                case Qt::AlignRight: {
                    blockStyle.append("right");
                    break;
                }

                case Qt::AlignCenter:
                case Qt::AlignHCenter: {
                    blockStyle.append("center");
                    break;
                }

                case Qt::AlignJustify: {
                    blockStyle.append("both");
                    break;
                }

                default: {
                    blockStyle.append("left");
                    break;
                }
            }
            blockStyle.append("\"/>");
            //
            // ... направление письма
            //
            blockStyle.append(
                QString("<w:bidi w:val=\"%1\"/>")
                .arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
            //
            // ... конец свойств
            //
            blockStyle.append("</w:pPr>");

            //
            // Параметры шрифта
            //
            blockStyle.append("<w:rPr>");
            blockStyle.append(
                QString("<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>")
                .arg(_style.font().family())
                );
            //
            // ... размер
            //
            blockStyle.append(
                QString("<w:sz w:val=\"%1\"/><w:szCs w:val=\"%1\"/>")
                .arg(_style.font().pointSize() * 2)
                );
            //
            // ... начертание
            //
            if (_style.font().bold()) {
                blockStyle.append("<w:b/><w:bCs/>");
            }
            if (_style.font().italic()) {
                blockStyle.append("<w:i/><w:iCs/>");
            }
            if (_style.font().underline()) {
                blockStyle.append("<w:u  w:val=\"single\"/>");
            }
            //
            // ... регистр
            //
            if (_style.font().capitalization() == QFont::AllUppercase) {
                blockStyle.append("<w:caps/>");
            }
            //
            // ... конец параметров шрифта
            //
            blockStyle.append("</w:rPr>");

            //
            // Конец стиля
            //
            blockStyle.append("</w:style>");
        } else {
            //
            // Для неопределённого стиля формируется простая заглушка
            //
            blockStyle =
                    QString(
                        "<w:style w:type=\"paragraph\" w:styleId=\"Normal\">"
                            "<w:name w:val=\"Normal\"/>"
                            "<w:pPr>"
                                "<w:widowControl w:val=\"0\"/><w:autoSpaceDE w:val=\"0\"/><w:autoSpaceDN w:val=\"0\"/><w:adjustRightInd w:val=\"0\"/><w:spacing w:after=\"0\" w:line=\"240\" w:lineRule=\"auto\"/>"
                            "</w:pPr>"
                            "<w:rPr>"
                                "<w:rFonts w:ascii=\"%1\" w:hAnsi=\"%1\" w:cs=\"%1\"/>"
                                "<w:sz w:val=\"24\"/>"
                            "</w:rPr>"
                        "</w:style>")
                    .arg(_defaultFontFamily);
        }

        return blockStyle;
    }

    /**
     * @brief Является ли заданный формат открывающим комментарий
     */
    static bool isCommentsRangeStart(const QTextBlock& _block, const QTextLayout::FormatRange& _range) {
        //
        // Ищем предыдущий блок, до тех пор, пока не дойдём до текста или начала
        //
        QTextBlock prevBlock = _block.previous();
        while (prevBlock.isValid() && prevBlock.text().isEmpty()) {
            prevBlock = prevBlock.previous();
        }

        bool isStart = true;
        if (prevBlock.isValid()) {
            foreach (const QTextLayout::FormatRange& range, prevBlock.textFormats()) {
                if (range.format == _range.format) {
                    isStart = false;
                    break;
                }
            }
        }
        return isStart;
    }

    /**
     * @brief Является ли заданный формат закрывающим комментарий
     */
    static bool isCommentsRangeEnd(const QTextBlock& _block, const QTextLayout::FormatRange& _range) {
        //
        // Ищем следующий блок, до тех пор, пока не дойдём до текста или конца
        //
        QTextBlock nextBlock = _block.next();
        while (nextBlock.isValid() && nextBlock.text().isEmpty()) {
            nextBlock = nextBlock.next();
        }

        bool isEnd = true;
        if (nextBlock.isValid()) {
            foreach (const QTextLayout::FormatRange& range, nextBlock.textFormats()) {
                if (range.format == _range.format) {
                    isEnd = false;
                    break;
                }
            }
        }
        return isEnd;
    }

    /**
     * @brief Сформировать текст блока документа в зависимости от его стиля и оформления
     */
    static QString docxText(QMap<int, QStringList>& _comments, const QTextCursor& _cursor,
        const ExportParameters& _exportParameters) {

        QString documentXml;

        //
        // Получим стиль параграфа
        //
        ScenarioBlockStyle::Type currentBlockType = ScenarioBlockStyle::forBlock(_cursor.block());

        //
        // Запишем параграф в документ
        //
        if (currentBlockType != ScenarioBlockStyle::Undefined) {
            documentXml =
                QString("<w:p><w:pPr><w:pStyle w:val=\"%1\"/>")
                .arg(ScenarioBlockStyle::typeName(currentBlockType).toUpper().replace("_", ""));

            if (currentBlockType == ScenarioBlockStyle::SceneHeading
                && _exportParameters.printScenesNumbers) {
                if (SceneHeadingBlockInfo* sceneInfo = dynamic_cast<SceneHeadingBlockInfo*>(_cursor.block().userData())) {
                    const QString sceneNumber = QString("%1%2. ").arg(_exportParameters.scenesPrefix)
                                                                 .arg(sceneInfo->sceneNumber());
                    const QFontMetrics fontMetrics(exportStyle().blockStyle(currentBlockType).font());
                    documentXml.append(
                                QString("<w:ind w:left=\"-%1\" w:right=\"0\" w:hanging=\"0\" />")
                                .arg(mmToTwips(PageMetrics::pxToMm(fontMetrics.width(sceneNumber))))
                                );
                }
            }

            //
            //  ... текст абзаца
            //
            const QTextBlock block = _cursor.block();
            const QString blockText = block.text();
            documentXml.append("<w:rPr/></w:pPr>");
            foreach (const QTextLayout::FormatRange& range, block.textFormats()) {
                //
                // ... не является редакторской заметкой
                //
                if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark) == false) {
                    //
                    // ... стандартный для абзаца
                    //
                    if (range.format == block.charFormat()) {
                        documentXml.append("<w:r><w:rPr>");
                        documentXml.append(QString("<w:rtl w:val=\"%1\"/>").arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                        documentXml.append(
                                    QString("</w:rPr><w:t xml:space=\"preserve\">%2</w:t></w:r>")
                                    .arg(TextEditHelper::toHtmlEscaped(blockText.mid(range.start, range.length)))
                                    );
                    }
                    //
                    // ... не стандартный
                    //
                    else {
                        documentXml.append("<w:r>");
                        documentXml.append("<w:rPr>");
                        documentXml.append(QString("<w:b w:val=\"%1\"/>").arg(range.format.font().bold() ? "true" : "false"));
                        documentXml.append(QString("<w:i w:val=\"%1\"/>").arg(range.format.font().italic() ? "true" : "false"));
                        documentXml.append(QString("<w:u w:val=\"%1\"/>").arg(range.format.font().underline() ? "single" : "none"));
                        documentXml.append(QString("<w:rtl w:val=\"%1\"/>").arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                        documentXml.append("</w:rPr>");
                        //
                        // Сам текст
                        //
                        documentXml.append(
                            QString("<w:t xml:space=\"preserve\">%2</w:t>")
                            .arg(TextEditHelper::toHtmlEscaped(blockText.mid(range.start, range.length)))
                            );
                        documentXml.append("</w:r>");
                    }
                }
                //
                // ... редакторская заметка
                //
                else {
                    const QStringList comments = range.format.property(ScenarioBlockStyle::PropertyComments).toStringList();
                    const bool hasComments = !comments.isEmpty() && !comments.first().isEmpty();
                    int lastCommentIndex = _comments.isEmpty() ? 0 : _comments.lastKey();
                    //
                    // Комментарий
                    //
                    if (hasComments
                        && isCommentsRangeStart(block, range)) {
                        const QStringList authors = range.format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
                        const QStringList dates = range.format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();

                        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                            if (!_comments.isEmpty()) {
                                lastCommentIndex = _comments.lastKey() + 1;
                            }
                            _comments.insert(lastCommentIndex,
                                QStringList() << comments.at(commentIndex)
                                              << authors.at(commentIndex)
                                              << dates.at(commentIndex));

                            documentXml.append(
                                QString("<w:commentRangeStart w:id=\"%1\"/>").arg(lastCommentIndex));
                        }
                    }
                    documentXml.append("<w:r>");
                    documentXml.append("<w:rPr>");
                    //
                    // Заливка
                    //
                    if (!hasComments
                        && range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                        if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsHighlight)) {
                            documentXml.append(
                                        QString("<w:highlight w:val=\"%1\"/>")
                                        // код цвета без решётки
                                        .arg(Docx::highlightColorName(range.format.background().color())));
                        } else {
                            documentXml.append(
                                        QString("<w:shd w:fill=\"%1\" w:val=\"clear\"/>")
                                        // код цвета без решётки
                                        .arg(range.format.background().color().name().mid(1)));
                        }
                    }
                    //
                    // Цвет текста
                    //
                    if (!hasComments
                        && range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                        documentXml.append(
                            QString("<w:color w:val=\"%1\"/>")
                                    // код цвета без решётки
                                    .arg(range.format.foreground().color().name().mid(1)));
                    }
                    documentXml.append(QString("<w:b w:val=\"%1\"/>").arg(range.format.font().bold() ? "true" : "false"));
                    documentXml.append(QString("<w:i w:val=\"%1\"/>").arg(range.format.font().italic() ? "true" : "false"));
                    documentXml.append(QString("<w:u w:val=\"%1\"/>").arg(range.format.font().underline() ? "single" : "none"));
                    documentXml.append(QString("<w:rtl w:val=\"%1\"/>").arg(QLocale().textDirection() == Qt::RightToLeft ? "true" : "false"));
                    documentXml.append("</w:rPr>");
                    //
                    // Сам текст
                    //
                    documentXml.append(
                        QString("<w:t xml:space=\"preserve\">%2</w:t>")
                        .arg(TextEditHelper::toHtmlEscaped(blockText.mid(range.start, range.length)))
                        );
                    documentXml.append("</w:r>");
                    //
                    // Текст комментария
                    //
                    if (hasComments
                        && isCommentsRangeEnd(block, range)) {
                        for (int commentIndex = lastCommentIndex - comments.size() + 1;
                             commentIndex <= lastCommentIndex; ++commentIndex) {
                            documentXml.append(
                                        QString("<w:commentRangeEnd w:id=\"%1\"/>"
                                                "<w:r><w:rPr/><w:commentReference w:id=\"%1\"/></w:r>")
                                        .arg(commentIndex));
                        }
                    }
                }
            }
            //
            // ... закрываем абзац
            //
            documentXml.append("</w:p>");


        } else {
            //
            // ... настройки абзаца
            //
            documentXml = "<w:p><w:pPr><w:pStyle w:val=\"Normal\"/>";
            switch (_cursor.blockFormat().alignment()) {
                case Qt::AlignCenter:
                case Qt::AlignHCenter: {
                    documentXml.append("<w:jc w:val=\"center\"/>");
                    break;
                }

                case Qt::AlignRight: {
                    documentXml.append("<w:jc w:val=\"right\"/>");
                    break;
                }

                case Qt::AlignJustify: {
                    documentXml.append("<w:jc w:val=\"both\"/>");
                    break;
                }

                default: {
                    break;
                }
            }
            if (_cursor.blockFormat().rightMargin() > 0
                || _cursor.blockFormat().leftMargin() > 0) {
                documentXml.append(
                            QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"0\" />")
                            .arg(mmToTwips(PageMetrics::pxToMm(_cursor.blockFormat().leftMargin())))
                            .arg(mmToTwips(PageMetrics::pxToMm(_cursor.blockFormat().rightMargin())))
                            );
            }
            documentXml.append(
                QString("<w:rPr/></w:pPr><w:r><w:rPr/><w:t>%1</w:t></w:r></w:p>")
                .arg(TextEditHelper::toHtmlEscaped(_cursor.block().text()))
                );
        }

        return documentXml;
    }
}

DocxExporter::DocxExporter() :
    AbstractExporter()
{
}

void DocxExporter::exportTo(ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const
{
    //
    // Открываем документ на запись
    //
    QFile docxFile(_exportParameters.filePath);
    if (docxFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        //
        // Пишем все статичные данные в файл
        //
        QtZipWriter zip(&docxFile);
        if (zip.status() == QtZipWriter::NoError) {
            //
            // Конвертируем документ в DOCX
            //
            // ... статичные данные
            //
            writeStaticData(&zip, _exportParameters);
            //
            // ... стили
            //
            writeStyles(&zip);
            //
            // ... шрифты
            //
            if (needWriteFonts()) {
                writeFonts(&zip);
            }
            //
            // ... колонтитулы
            //
            writeHeader(&zip, _exportParameters);
            writeFooter(&zip, _exportParameters);
            //
            // ... документ
            //
            QMap<int, QStringList> comments;
            writeDocument(&zip, _scenario, comments, _exportParameters);
            //
            // ... комментарии
            //
            writeComments(&zip, comments);
        }
        zip.close();
        docxFile.close();
    }
}

void DocxExporter::exportTo(const ResearchModelCheckableProxy* _researchModel, const ExportParameters& _exportParameters) const
{
    Q_UNUSED(_researchModel);
    Q_UNUSED(_exportParameters);
}

void DocxExporter::writeStaticData(QtZipWriter* _zip, const ExportParameters& _exportParameters) const
{
    //
    // Перечисление всех компонентов архива
    //
    QString contentTypesXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
            "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
            "<Override PartName=\"/_rels/.rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Override PartName=\"/word/_rels/document.xml.rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
            "<Override PartName=\"/word/comments.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml\"/>";
    if (needWriteFonts()) {
        contentTypesXml.append(
            "<Default Extension=\"odttf\" ContentType=\"application/vnd.openxmlformats-officedocument.obfuscatedFont\"/>"
            "<Override PartName=\"/word/fontTable.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.fontTable+xml\"/>");
    }
    //
    // ... необходимы ли колонтитулы
    //
    if (needWriteHeader(_exportParameters)) {
        contentTypesXml.append("<Override PartName=\"/word/header1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml\"/>");
    }
    if (needWriteFooter(_exportParameters)) {
        contentTypesXml.append("<Override PartName=\"/word/footer1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml\"/>");
    }
    contentTypesXml.append(
            "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
            "<Override PartName=\"/word/settings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.settings+xml\"/>"
            "</Types>");
    _zip->addFile(QString::fromLatin1("[Content_Types].xml"), contentTypesXml.toUtf8());

    //
    // Связи пакета
    //
    _zip->addFile(QString::fromLatin1("_rels/.rels"),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Target=\"word/document.xml\" Id=\"pkgRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\"/>"
        "</Relationships>");

    //
    // Связи документа
    //
    QString documentXmlRels =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"docRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
            "<Relationship Id=\"docRId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments\" Target=\"comments.xml\"/>";
    //
    // ... необходимы ли колонтитулы
    //
    if (needWriteHeader(_exportParameters)) {
        documentXmlRels.append("<Relationship Id=\"docRId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/header\" Target=\"header1.xml\"/>");
    }
    if (needWriteFooter(_exportParameters)) {
        documentXmlRels.append("<Relationship Id=\"docRId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer\" Target=\"footer1.xml\"/>");
    }
    documentXmlRels.append("<Relationship Id=\"docRId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/settings\" Target=\"settings.xml\"/>");
    //
    // ... необходимы ли шрифты
    //
    if (needWriteFonts()) {
        documentXmlRels.append("<Relationship Id=\"docRId5\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/fontTable\" Target=\"fontTable.xml\"/>");
    }
    documentXmlRels.append("</Relationships>");
    _zip->addFile(QString::fromLatin1("word/_rels/document.xml.rels"), documentXmlRels.toUtf8());

    //
    // Связи шрифтов
    //
    if (needWriteFonts()) {
        _zip->addFile(QString::fromLatin1("word/_rels/fontTable.xml.rels"),
                      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                      "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
                      "<Relationship Id=\"fontRId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/font\" Target=\"fonts/font1.odttf\"/>"
                      "<Relationship Id=\"fontRId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/font\" Target=\"fonts/font2.odttf\"/>"
                      "<Relationship Id=\"fontRId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/font\" Target=\"fonts/font3.odttf\"/>"
                      "<Relationship Id=\"fontRId4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/font\" Target=\"fonts/font4.odttf\"/>"
                      "</Relationships>");
    }

    //
    // Настройки документа
    //
    QString wordSettings =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:settings xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:sl=\"http://schemas.openxmlformats.org/schemaLibrary/2006/main\">";
    if (needWriteFonts()) {
        wordSettings += "<w:embedTrueTypeFonts w:val=\"true\"/><w:embedSystemFonts w:val=\"false\"/>";
    }
    wordSettings +=
            "<w:characterSpacingControl w:val=\"doNotCompress\"/>"
            "<w:footnotePr></w:footnotePr>"
            "<w:endnotePr></w:endnotePr>"
            "</w:settings>";
    _zip->addFile(QString::fromLatin1("word/settings.xml"), wordSettings.toUtf8());
}

void DocxExporter::writeStyles(QtZipWriter* _zip) const
{
    //
    // Сформируем xml стилей
    //
    const QString languageCode = QLocale().uiLanguages().value(1, "en-US");
    QString styleXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
            "<w:docDefaults>"
            "<w:rPrDefault>"
            "<w:rPr>"
            "<w:rFonts w:asciiTheme=\"minorHAnsi\" w:eastAsiaTheme=\"minorEastAsia\" w:hAnsiTheme=\"minorHAnsi\" w:cstheme=\"minorBidi\"/>"
            "<w:sz w:val=\"22\"/>"
            "<w:szCs w:val=\"22\"/>"
            "<w:lang w:val=\"" + languageCode + "\" w:eastAsia=\"" + languageCode + "\" w:bidi=\"" + (QLocale().language() == QLocale::Hebrew ? "he-IL" : "ar-SA") + "\"/>"
            "</w:rPr>"
            "</w:rPrDefault>"
            "<w:pPrDefault><w:pPr><w:spacing w:after=\"200\" w:line=\"276\" w:lineRule=\"auto\"/></w:pPr></w:pPrDefault>"
            "</w:docDefaults>";

    //
    // Настройки в соответсвии со стилем
    //
    ScenarioTemplate style = ::exportStyle();
    const QString defaultFontFamily = style.blockStyle(ScenarioBlockStyle::Action).font().family();
    foreach (int blockNumber, ::blockTypes().keys()) {
        ScenarioBlockStyle blockStyle = style.blockStyle(blockTypes().value(blockNumber));
        styleXml.append(docxBlockStyle(blockStyle, defaultFontFamily));
    }

    styleXml.append("</w:styles>");

    //
    // Запишем стили в архив
    //
    _zip->addFile(QString::fromLatin1("word/styles.xml"), styleXml.toUtf8());
}

void DocxExporter::writeFonts(QtZipWriter* _zip) const
{
    //
    // Сформируем xml шрифтов
    //
    QString fontsXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:fonts xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
            "<w:font w:name=\"Courier Prime\">"
            "<w:altName w:val=\"Courier New\"/>"
            "<w:panose1 w:val=\"02000409000000000000\"/>"
            "<w:charset w:val=\"CC\"/>"
            "<w:family w:val=\"modern\"/>"
            "<w:pitch w:val=\"variable\"/>"
            "<w:sig w:usb0=\"A000022F\" w:usb1=\"5000004B\" w:usb2=\"00000000\" w:usb3=\"00000000\" w:csb0=\"00000097\" w:csb1=\"00000000\"/>"
            "<w:embedRegular r:id=\"fontRId1\" w:fontKey=\"{E986E054-5FE3-4CE1-AC06-D0F4C1EA288D}\"/>"
            "<w:embedBold r:id=\"fontRId2\" w:fontKey=\"{E74E9704-F3D4-4C5A-A0F9-6D5CA0A87C03}\"/>"
            "<w:embedItalic r:id=\"fontRId3\" w:fontKey=\"{3F8FC19D-124E-49D8-8A3F-F9B677D4F9DD}\"/>"
            "<w:embedBoldItalic r:id=\"fontRId4\" w:fontKey=\"{ECA70A3C-24DC-43CB-AC33-333B6A6DDC0B}\"/>"
            "</w:font>"
            "</w:fonts>";

    //
    // Запишем таблицу шрифтов в архив
    //
    _zip->addFile(QString::fromLatin1("word/fontTable.xml"), fontsXml.toUtf8());

    //
    // Запишем сами шрифты в архив
    //
    auto readFontData = [=] (const QString& _path) {
        QFile file(_path);
        file.open(QIODevice::ReadOnly);
        return file.readAll();
    };
    _zip->addFile(QString::fromLatin1("word/fonts/font1.odttf"), readFontData(":/Fonts/Fonts/embed/courierprime-regular.odttf"));
    _zip->addFile(QString::fromLatin1("word/fonts/font2.odttf"), readFontData(":/Fonts/Fonts/embed/courierprime-bold.odttf"));
    _zip->addFile(QString::fromLatin1("word/fonts/font3.odttf"), readFontData(":/Fonts/Fonts/embed/courierprime-italic.odttf"));
    _zip->addFile(QString::fromLatin1("word/fonts/font4.odttf"), readFontData(":/Fonts/Fonts/embed/courierprime-bold-italic.odttf"));
}

void DocxExporter::writeHeader(QtZipWriter* _zip, const ExportParameters& _exportParameters) const
{
    //
    // Если нужна нумерация вверху
    //
    const bool needPrintPageNumbers = _exportParameters.printPagesNumbers
                                      && exportStyle().numberingAlignment().testFlag(Qt::AlignTop);
    if (needPrintPageNumbers
        || !_exportParameters.scriptHeader.isEmpty()) {
        QString headerXml =
                "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                "<w:hdr xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">";


        headerXml.append("<w:p><w:pPr><w:jc w:val=\"");
        if (exportStyle().numberingAlignment().testFlag(Qt::AlignLeft) || !needPrintPageNumbers) {
            headerXml.append("left");
        } else if (exportStyle().numberingAlignment().testFlag(Qt::AlignCenter)) {
            headerXml.append("center");
        } else {
            headerXml.append("right");
        }
        headerXml.append("\"/><w:rPr/></w:pPr>");

        const QString pageNumbersXml = needPrintPageNumbers
                                       ? "<w:r><w:rPr/><w:fldChar w:fldCharType=\"begin\"/></w:r><w:r><w:instrText> PAGE </w:instrText></w:r><w:r><w:fldChar w:fldCharType=\"separate\"/></w:r><w:r><w:t>0</w:t></w:r><w:r><w:fldChar w:fldCharType=\"end\"/></w:r>"
                                       : "";
        const QString pageHeaderXml = QString("<w:r><w:rPr/><w:t>%1</w:t></w:r>").arg(_exportParameters.scriptHeader);
        const QString pageHeaderSeparatorXml = "<w:r><w:rPr/><w:t> </w:t></w:r>";
        if (exportStyle().numberingAlignment().testFlag(Qt::AlignRight)) {
            headerXml.append(pageHeaderXml);
            headerXml.append(pageHeaderSeparatorXml);
            headerXml.append(pageNumbersXml);
        } else {
            headerXml.append(pageNumbersXml);
            headerXml.append(pageHeaderSeparatorXml);
            headerXml.append(pageHeaderXml);
        }

        headerXml.append("</w:p></w:hdr>");

        //
        // Запишем верхний колонтитул в архив
        //
        _zip->addFile(QString::fromLatin1("word/header1.xml"), headerXml.toUtf8());
    }
}

void DocxExporter::writeFooter(QtZipWriter* _zip, const ExportParameters& _exportParameters) const
{
    //
    // Если нужна нумерация внизу
    //
    const bool needPrintPageNumbers = _exportParameters.printPagesNumbers
                                      && exportStyle().numberingAlignment().testFlag(Qt::AlignBottom);
    if (needPrintPageNumbers
        || !_exportParameters.scriptFooter.isEmpty()) {
        QString footerXml =
                "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                "<w:ftr xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">";

        footerXml.append("<w:p><w:pPr><w:jc w:val=\"");
        if (exportStyle().numberingAlignment().testFlag(Qt::AlignLeft)
            || !needPrintPageNumbers) {
            footerXml.append("left");
        } else if (exportStyle().numberingAlignment().testFlag(Qt::AlignCenter)) {
            footerXml.append("center");
        } else {
            footerXml.append("right");
        }
        footerXml.append("\"/><w:rPr/></w:pPr>");

        const QString pageNumbersXml = needPrintPageNumbers
                                       ? "<w:r><w:rPr/><w:fldChar w:fldCharType=\"begin\"/></w:r><w:r><w:instrText> PAGE </w:instrText></w:r><w:r><w:fldChar w:fldCharType=\"separate\"/></w:r><w:r><w:t>0</w:t></w:r><w:r><w:fldChar w:fldCharType=\"end\"/></w:r>"
                                       : "";
        const QString pageFooterXml = QString("<w:r><w:rPr/><w:t>%1</w:t></w:r>").arg(_exportParameters.scriptFooter);
        const QString pageFooterSeparatorXml = "<w:r><w:rPr/><w:t> </w:t></w:r>";
        if (exportStyle().numberingAlignment().testFlag(Qt::AlignRight)) {
            footerXml.append(pageFooterXml);
            footerXml.append(pageFooterSeparatorXml);
            footerXml.append(pageNumbersXml);
        } else {
            footerXml.append(pageNumbersXml);
            footerXml.append(pageFooterSeparatorXml);
            footerXml.append(pageFooterXml);
        }

        footerXml.append("</w:p></w:ftr>");

        //
        // Запишем верхний колонтитул в архив
        //
        _zip->addFile(QString::fromLatin1("word/footer1.xml"), footerXml.toUtf8());
    }
}

void DocxExporter::writeDocument(QtZipWriter* _zip, ScenarioDocument* _scenario,
    QMap<int, QStringList>& _comments, const ExportParameters& _exportParameters) const
{
    QString documentXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:document xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">"
            "<w:body>";

    //
    // Сформируем документ
    //
    QTextDocument* preparedDocument = prepareDocument(_scenario, _exportParameters);

    //
    // Данные считываются из исходного документа, определяется тип блока
    // и записываются прямо в файл
    //
    QTextCursor documentCursor(preparedDocument);
    while (!documentCursor.atEnd()) {
        documentXml.append(docxText(_comments, documentCursor, _exportParameters));

        //
        // Переходим к следующему параграфу
        //
        documentCursor.movePosition(QTextCursor::EndOfBlock);
        documentCursor.movePosition(QTextCursor::NextBlock);
    }

    //
    // В конце идёт блок настроек страницы
    //
    ScenarioTemplate style = ::exportStyle();
    documentXml.append("<w:sectPr>");
    //
    // ... колонтитулы
    //
    if (needWriteHeader(_exportParameters)) {
            documentXml.append("<w:headerReference w:type=\"default\" r:id=\"docRId2\"/>");
        }
    if (needWriteFooter(_exportParameters)) {
        documentXml.append("<w:footerReference w:type=\"default\" r:id=\"docRId3\"/>");
    }
    //
    // ... размер страницы
    //
    QSizeF paperSize = QPageSize(style.pageSizeId()).size(QPageSize::Millimeter);
    documentXml.append(
        QString("<w:pgSz w:w=\"%1\" w:h=\"%2\"/>")
        .arg(mmToTwips(paperSize.width()))
        .arg(mmToTwips(paperSize.height()))
        );
    //
    // ... поля документа
    //
    documentXml.append(
        QString("<w:pgMar w:left=\"%1\" w:right=\"%2\" w:top=\"%3\" w:bottom=\"%4\" w:header=\"%5\" w:footer=\"%6\" w:gutter=\"0\"/>")
        .arg(mmToTwips(style.pageMargins().left()))
        .arg(mmToTwips(style.pageMargins().right()))
        .arg(mmToTwips(style.pageMargins().top()))
        .arg(mmToTwips(style.pageMargins().bottom()))
        .arg(mmToTwips(style.pageMargins().top() / 2))
        .arg(mmToTwips(style.pageMargins().bottom() / 2))
        );
    //
    // ... нужна ли титульная страница
    //
    if (_exportParameters.printTilte) {
        documentXml.append("<w:titlePg/>");
    }
    //
    // ... нумерация страниц
    //
    int pageNumbersStartFrom = _exportParameters.printTilte ? 0 : 1;
    documentXml.append(
        QString("<w:pgNumType w:fmt=\"decimal\" w:start=\"%1\"/>").arg(pageNumbersStartFrom));
    //
    // ... конец блока настроек страницы
    //
    documentXml.append(
            "<w:textDirection w:val=\"lrTb\"/>"
        "</w:sectPr>"
        );

    documentXml.append("</w:body></w:document>");

    //
    // Запишем документ в архив
    //
    _zip->addFile(QString::fromLatin1("word/document.xml"), documentXml.toUtf8());
}

void DocxExporter::writeComments(QtZipWriter* _zip, const QMap<int, QStringList>& _comments) const
{
    const int COMMENT_AUTHOR = 1;
    const int COMMENT_DATE = 2;
    const int COMMENT_TEXT = 0;

    QString headerXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<w:comments xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:w10=\"urn:schemas-microsoft-com:office:word\" xmlns:wp=\"http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing\">";

    foreach (const int commentIndex, _comments.keys()) {
        const QStringList comment = _comments.value(commentIndex);
        headerXml.append(
            QString("<w:comment w:id=\"%1\" w:author=\"%2\" w:date=\"%3\" w:initials=\"\">"
                    "<w:p><w:r><w:rPr></w:rPr><w:t>%4</w:t></w:r></w:p>"
                    "</w:comment>")
                    .arg(commentIndex)
                    .arg(comment.at(COMMENT_AUTHOR))
                    .arg(comment.at(COMMENT_DATE))
                    .arg(TextEditHelper::toHtmlEscaped(comment.at(COMMENT_TEXT)))
                    );
    }

    headerXml.append("</w:comments>");

    //
    // Запишем комментарии в архив
    //
    _zip->addFile(QString::fromLatin1("word/comments.xml"), headerXml.toUtf8());
}
