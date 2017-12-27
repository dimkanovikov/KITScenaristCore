#include "AbstractExporter.h"

#include <BusinessLayer/Research/ResearchModel.h>
#include <BusinessLayer/Research/ResearchModelItem.h>
#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>
#include <BusinessLayer/ScenarioDocument/ScriptTextCorrector.h>

#include <Domain/Research.h>
#include <Domain/Scenario.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Widgets/PagesTextEdit/PageMetrics.h>
#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>
#include <3rd_party/Widgets/QtMindMap/include/graphwidget.h>

#include <QApplication>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

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
     * @brief Определить размер страницы документа
     */
    static QSizeF documentSize() {
        QSizeF pageSize = QPageSize(exportStyle().pageSizeId()).size(QPageSize::Millimeter);
        QMarginsF pageMargins = exportStyle().pageMargins();

        return QSizeF(
                    PageMetrics::mmToPx(pageSize.width() - pageMargins.left() - pageMargins.right()),
                    PageMetrics::mmToPx(pageSize.height() - pageMargins.top() - pageMargins.bottom(), false)
                    );
    }

    /**
     * @brief Получить стиль оформления символов для заданного типа
     */
    static QTextCharFormat charFormatForType(ScenarioBlockStyle::Type _type) {
        QTextCharFormat format = exportStyle().blockStyle(_type).charFormat();

        //
        // Очищаем цвета
        //
        format.setForeground(Qt::black);

        return format;
    }

    /**
     * @brief Получить стиль оформления абзаца для заданного типа
     */
    static QTextBlockFormat blockFormatForType(ScenarioBlockStyle::Type _type) {
        ScenarioBlockStyle style = exportStyle().blockStyle(_type);
        QTextBlockFormat format = style.blockFormat();

        format.setProperty(ScenarioBlockStyle::PropertyType, _type);

        //
        // Очищаем цвета
        //
        format.setBackground(Qt::white);

        //
        // Оставляем только реальные отступы (отступы в строках будут преобразованы в пустые строки)
        //
        int topMargin = 0;
        int bottomMargin = 0;
        if (style.hasVerticalSpacingInMM()) {
            topMargin = PageMetrics::mmToPx(style.topMargin());
            bottomMargin = PageMetrics::mmToPx(style.bottomMargin());
        }
        format.setTopMargin(topMargin);
        format.setBottomMargin(bottomMargin);

        return format;
    }

    /**
     * @brief Определить, нужно ли записать блок с заданным типом в результирующий файл
     */
    static bool needPrintBlock(ScenarioBlockStyle::Type _blockType, bool _outline, bool _showInvisible) {
        static QList<ScenarioBlockStyle::Type> s_outlinePrintableBlocksTypes =
            QList<ScenarioBlockStyle::Type>()
            << ScenarioBlockStyle::SceneHeading
            << ScenarioBlockStyle::SceneHeadingShadow
            << ScenarioBlockStyle::SceneCharacters
            << ScenarioBlockStyle::SceneDescription;

        static QList<ScenarioBlockStyle::Type> s_scenarioPrintableBlocksTypes =
            QList<ScenarioBlockStyle::Type>()
            << ScenarioBlockStyle::SceneHeading
            << ScenarioBlockStyle::SceneHeadingShadow
            << ScenarioBlockStyle::SceneCharacters
            << ScenarioBlockStyle::Action
            << ScenarioBlockStyle::Character
            << ScenarioBlockStyle::Dialogue
            << ScenarioBlockStyle::Parenthetical
            << ScenarioBlockStyle::TitleHeader
            << ScenarioBlockStyle::Title
            << ScenarioBlockStyle::Note
            << ScenarioBlockStyle::Transition
            << ScenarioBlockStyle::Lyrics;

        return
                (_outline
                 ? s_outlinePrintableBlocksTypes.contains(_blockType)
                 : s_scenarioPrintableBlocksTypes.contains(_blockType))
                || (_showInvisible
                    && (_blockType == ScenarioBlockStyle::NoprintableText
                        || _blockType == ScenarioBlockStyle::FolderFooter
                        || _blockType == ScenarioBlockStyle::FolderHeader));
    }

    /**
     * @brief Типы строк документа
     */
    enum LineType {
        UndefinedLine,
        FirstDocumentLine,
        MiddlePageLine,
        LastPageLine
    };

    /**
     * @brief Вставить строку с заданным форматированием
     */
    static void insertLine(QTextCursor& _cursor, const QTextBlockFormat& _blockFormat,
        const QTextCharFormat& _charFormat) {
        _cursor.insertBlock(_blockFormat, _charFormat);
    }

    /**
     * @brief Определить тип следующей строки документа
     */
    static LineType currentLine(QTextDocument* _inDocument, const QTextBlockFormat& _blockFormat,
        const QTextCharFormat& _charFormat) {
        LineType type = UndefinedLine;

        if (_inDocument->isEmpty()) {
            type = FirstDocumentLine;
        } else {
            //
            // Определяем конец страницы или середина при помощи проверки
            // на количество страниц, после вставки новой строки
            //
            const int documentPagesCount = _inDocument->pageCount();
            QTextCursor cursor(_inDocument);
            cursor.movePosition(QTextCursor::End);
            insertLine(cursor, _blockFormat, _charFormat);
            const int documentPagesCountWithNextLine = _inDocument->pageCount();
            cursor.deletePreviousChar();
            if (documentPagesCount == documentPagesCountWithNextLine) {
                type = MiddlePageLine;
            } else {
                type = LastPageLine;
            }
        }

        return type;
    }

    // **********
    // Экспорт разработки

    /**
     * @brief Шрифт разработки по умолчанию
     */
    static QString defaultResearchFont() {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "research/default-font/family", DataStorageLayer::SettingsStorage::ApplicationSettings);
    }

    /**
     * @brief Размер шрифта разработки по умолчанию
     */
    static int defaultResearchFontSize() {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "research/default-font/size", DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();
    }

    /**
     * @brief Экспортировать заданный элемент разработки с заданными параметрами экспорта и на заданном уровне иерархии
     */
    static void exportResearchItem(const BusinessLogic::ResearchModelItem* _item,
        const ExportParameters& _exportParameters, int _level, QTextCursor& _cursor) {

        //
        // Сформируем эталонные стили для блоков
        //
        QTextCharFormat titleCharFormat;
        titleCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 4));
        titleCharFormat.setFontWeight(QFont::Bold);
        QTextBlockFormat titleBlockFormat;
        titleBlockFormat.setLineHeight(QFontMetricsF(titleCharFormat.font()).height() / 1.2, QTextBlockFormat::FixedHeight);
        QTextCharFormat subtitleCharFormat;
        subtitleCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 2));
        subtitleCharFormat.setForeground(Qt::darkGray);
        QTextBlockFormat subtitleBlockFormat;
        //
        QTextBlockFormat headerBlockFormat;
        QTextCharFormat headerCharFormat;
        switch (_level) {
            case 0: {
                headerCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 3));
                headerBlockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
                break;
            }

            case 1: {
                headerCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 2));
                headerBlockFormat.setTopMargin(defaultResearchFontSize() * 1.5);
                break;
            }

            case 2: {
                headerCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 1.5));
                headerBlockFormat.setTopMargin(defaultResearchFontSize() * 1.2);
                break;
            }

            case 3: {
                headerCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize() * 1.2));
                headerBlockFormat.setTopMargin(defaultResearchFontSize());
                break;
            }

            default: {
                headerCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize()));
                headerBlockFormat.setTopMargin(defaultResearchFontSize());
                break;
            }
        }
        headerCharFormat.setFontWeight(QFont::Bold);
        //
        QTextBlockFormat textBlockFormat;
        QTextCharFormat textCharFormat;
        textCharFormat.setFont(QFont(defaultResearchFont(), defaultResearchFontSize()));

        //
        // Если это первый блок документа, то убираем флаг разрыва страниц
        //
        if (_cursor.document()->isEmpty()) {
            headerBlockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
        }

        //
        // Выводим элемент разработки в документ
        //
        const Domain::Research* research = _item->research();
        if (research != nullptr) {
            switch (research->type()) {
                case Domain::Research::Scenario: {
                    _cursor.setBlockFormat(titleBlockFormat);
                    _cursor.setCharFormat(titleCharFormat);
                    _cursor.setBlockCharFormat(titleCharFormat);
                    _cursor.insertText(_exportParameters.scriptName);
                    //
                    _cursor.insertBlock(subtitleBlockFormat, subtitleCharFormat);
                    QTextDocument loglineDocument;
                    loglineDocument.setHtml(_exportParameters.logline);
                    _cursor.insertText(loglineDocument.toPlainText());
                    break;
                }

                case Domain::Research::TitlePage: {
                    for (int i = 0; i < 8; ++i) {
                        _cursor.insertBlock(textBlockFormat, textCharFormat);
                    }
                    _cursor.insertText(_exportParameters.scriptAdditionalInfo);
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertText(_exportParameters.scriptGenre);
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertText(_exportParameters.scriptAuthor);
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertText(_exportParameters.scriptContacts);
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertText(_exportParameters.scriptYear);
                    break;
                }

                case Domain::Research::Synopsis: {
                    headerBlockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(_exportParameters.synopsis);
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    break;
                }

                case Domain::Research::CharactersRoot:
                case Domain::Research::LocationsRoot:
                case Domain::Research::ResearchRoot:
                case Domain::Research::ImagesGallery: {
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    break;
                }

                case Domain::Research::Folder: {
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(research->description());
                    break;
                }

                case Domain::Research::Text: {
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(research->description());
                    break;
                }

                case Domain::Research::Url: {
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(QString("<a href='%1'>%2</a>").arg(research->name(), research->url()));
                    break;
                }

                case Domain::Research::Image: {
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    QImage image = research->image().toImage();
                    if (image.size().width() > ::documentSize().width()
                        || image.size().height() > ::documentSize().height()) {
                        image = image.scaled(::documentSize().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    _cursor.insertImage(image);
                    break;
                }

                case Domain::Research::MindMap: {
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    GraphWidget mindMap;
                    mindMap.load(research->description());
                    QImage image = mindMap.saveToImage();
                    if (image.size().width() > ::documentSize().width()
                        || image.size().height() > ::documentSize().height()) {
                        image = image.scaled(::documentSize().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    _cursor.insertImage(image);
                    break;
                }

                case Domain::Research::Character: {
                    const Domain::ResearchCharacter* character = dynamic_cast<const Domain::ResearchCharacter*>(research);
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(character->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertText(QApplication::translate("BusinessLayer::AbstractExporter", "Real name: ") + character->realName());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(character->descriptionText());
                    break;
                }

                case Domain::Research::Location: {
                    _cursor.insertBlock(headerBlockFormat, headerCharFormat);
                    _cursor.insertText(research->name());
                    _cursor.insertBlock(textBlockFormat, textCharFormat);
                    _cursor.insertHtml(research->description());
                    break;
                }
            }
        }
    }

    /**
     * @brief Экспортировать заданный уровень иерархии разработки целиком
     */
    static void exportResearchLevel(const BusinessLogic::ResearchModelCheckableProxy* _researchModel,
        const ExportParameters& _exportParameters, const QModelIndex& _parentIndex, int _level, QTextCursor& cursor) {
        for (int row = 0; row < _researchModel->rowCount(_parentIndex); ++row) {
            const QModelIndex index = _researchModel->index(row, 0, _parentIndex);
            if (index.data(Qt::CheckStateRole).toInt() != Qt::Unchecked) {
                ::exportResearchItem(_researchModel->researchItem(index), _exportParameters, _level, cursor);
                ::exportResearchLevel(_researchModel, _exportParameters, index, _level + 1, cursor);
            }
        }
    }

} // namesapce



QTextDocument* AbstractExporter::prepareDocument(const BusinessLogic::ScenarioDocument* _scenario,
        const ExportParameters& _exportParameters)
{
    ScenarioTemplate exportStyle = ::exportStyle();

    //
    // Формируем копию сценария, в экпортируемом стиле
    //
    QTextDocument* scenarioDocument = new QTextDocument;
    {
        //
        // Используем тексовый редактор, чтобы в последствии корректно сформировать переносы блоков
        //
        PageTextEdit edit;
        edit.setUsePageMode(true);
        edit.setPageFormat(exportStyle.pageSizeId());
        edit.setPageMargins(exportStyle.pageMargins());
        edit.setDocument(scenarioDocument);

        //
        // Копируем содержимое
        //
        QTextCursor sourceCursor(_scenario->document());
        QTextCursor destCursor(scenarioDocument);
        ScenarioBlockStyle::Type currentBlockType = ScenarioBlockStyle::Undefined;
        while (!sourceCursor.atEnd()) {
            //
            // Копируем содержимое блока, если нужно
            //
            currentBlockType = ScenarioBlockStyle::forBlock(sourceCursor.block());
            if (::needPrintBlock(currentBlockType, _exportParameters.isOutline, _exportParameters.saveInvisible)) {
                //
                // ... настраиваем стиль блока
                //
                ScenarioBlockStyle blockStyle = exportStyle.blockStyle(currentBlockType);
                if (sourceCursor.atStart()) {
                    destCursor.setBlockFormat(sourceCursor.blockFormat());
                    destCursor.setCharFormat(sourceCursor.blockCharFormat());
                } else {
                    destCursor.insertBlock(sourceCursor.blockFormat(), sourceCursor.blockCharFormat());
                }
                destCursor.mergeBlockFormat(blockStyle.blockFormat());
                destCursor.mergeBlockCharFormat(blockStyle.charFormat());
                //
                // ... копируем текст
                //
                destCursor.insertText(sourceCursor.block().text());
                //
                // ... выделения и форматирование
                //
                if (!sourceCursor.block().textFormats().isEmpty()) {
                    const int startBlockPosition = destCursor.block().position();
                    foreach (const QTextLayout::FormatRange& range, sourceCursor.block().textFormats()) {
                        const bool isReviewMark = range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark);
                        const bool isFormatting = range.format != sourceCursor.blockCharFormat();
                        if (isReviewMark || isFormatting) {
                            destCursor.setPosition(startBlockPosition + range.start);
                            destCursor.setPosition(destCursor.position() + range.length, QTextCursor::KeepAnchor);
                            QTextCharFormat format = range.format;
                            //
                            // ... выделение красим в чёрный, чтобы случайно не выгрузить
                            //     текст с пользовательским цветом
                            //
                            if (!isReviewMark) {
                                format.setForeground(Qt::black);
                            }
                            destCursor.mergeCharFormat(format);
                        }
                    }
                    destCursor.movePosition(QTextCursor::EndOfBlock);
                }
                //
                // ... пользовательские данные
                //
                if (SceneHeadingBlockInfo* sceneInfo = dynamic_cast<SceneHeadingBlockInfo*>(sourceCursor.block().userData())) {
                    destCursor.block().setUserData(sceneInfo->clone());
                } else if (CharacterBlockInfo* sceneInfo = dynamic_cast<CharacterBlockInfo*>(sourceCursor.block().userData())) {
                    destCursor.block().setUserData(sceneInfo->clone());
                }
            }

            sourceCursor.movePosition(QTextCursor::EndOfBlock);
            sourceCursor.movePosition(QTextCursor::NextCharacter);
        }

        //
        // Cкорректируем сценарий
        //
        ScriptTextCorrector corrector(scenarioDocument, exportStyle.name());
        corrector.setNeedToCorrectPageBreaks(_exportParameters.checkPageBreaks);
        corrector.correct();
    }


    //
    // Настроим новый документ
    //
    QTextDocument* preparedDocument = prepareDocument();
    //
    // и подготовим курсоры для копирования текста из исходного в новый
    //
    QTextCursor sourceDocumentCursor(scenarioDocument);
    QTextCursor destDocumentCursor(preparedDocument);


    //
    // Формирование титульной страницы
    //
    if (_exportParameters.printTilte) {
        QTextCharFormat titleFormat;
        titleFormat.setFont(exportStyle.blockStyle(ScenarioBlockStyle::Action).font());
        QTextCharFormat headerFormat;
        headerFormat.setFont(exportStyle.blockStyle(ScenarioBlockStyle::Action).font());
        headerFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QTextBlockFormat leftFormat;
        leftFormat.setAlignment(Qt::AlignLeft);
        leftFormat.setLineHeight(
                    TextEditHelper::fontLineHeight(titleFormat.font()),
                    QTextBlockFormat::FixedHeight);
        QTextBlockFormat centerFormat;
        centerFormat.setAlignment(Qt::AlignCenter);
        centerFormat.setLineHeight(
                    TextEditHelper::fontLineHeight(titleFormat.font()),
                    QTextBlockFormat::FixedHeight);
        QTextBlockFormat rightFormat;
        rightFormat.setAlignment(Qt::AlignRight);
        rightFormat.setLineHeight(
                    TextEditHelper::fontLineHeight(titleFormat.font()),
                    QTextBlockFormat::FixedHeight);
        //
        // Номер текущей строки
        //
        int currentLineNumber = 1;
        destDocumentCursor.setBlockFormat(rightFormat);
        destDocumentCursor.setCharFormat(titleFormat);

        //
        // Название [17 строка]
        //
        while ((currentLineNumber++) < 16) {
            ::insertLine(destDocumentCursor, centerFormat, headerFormat);
        }
        ::insertLine(destDocumentCursor, centerFormat, headerFormat);
        destDocumentCursor.insertText(_exportParameters.scriptName.toUpper());
        //
        // Две строки отступа от заголовка
        //
        ::insertLine(destDocumentCursor, centerFormat, titleFormat);
        ::insertLine(destDocumentCursor, centerFormat, titleFormat);
        //
        // Жанр [через одну под предыдущим]
        //
        if (!_exportParameters.scriptGenre.isEmpty()) {
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            destDocumentCursor.insertText(_exportParameters.scriptGenre);
            currentLineNumber += 2;
        }
        //
        // Автор [через одну под предыдущим]
        //
        if (!_exportParameters.scriptAuthor.isEmpty()) {
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            destDocumentCursor.insertText(_exportParameters.scriptAuthor);
            currentLineNumber += 2;
        }
        //
        // Доп. инфо [через одну под предыдущим]
        //
        if (!_exportParameters.scriptAdditionalInfo.isEmpty()) {
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            destDocumentCursor.insertText(_exportParameters.scriptAdditionalInfo);
            currentLineNumber += 2;
        }
        //
        // необходимое количество пустых строк до 30ой
        //
        while ((currentLineNumber++) < 44) {
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
        }
        //
        // Контакты [45 строка]
        //
        ::insertLine(destDocumentCursor, leftFormat, titleFormat);
        destDocumentCursor.insertText(_exportParameters.scriptContacts);

        //
        // Год печатается на последней строке документа
        //
        LineType currentLineType = ::currentLine(preparedDocument, centerFormat, titleFormat);
        while (currentLineType != LastPageLine) {
            ++currentLineNumber;
            ::insertLine(destDocumentCursor, centerFormat, titleFormat);
            currentLineType = ::currentLine(preparedDocument, centerFormat, titleFormat);
        }
        destDocumentCursor.insertText(_exportParameters.scriptYear);
    }


    //
    // Запись текста документа
    //
    ScenarioBlockStyle::Type currentBlockType = ScenarioBlockStyle::Undefined;
    //
    // Кол-во пустых строк после предыдущего блока с текстом. Отслеживаем их, для случая, когда
    // подряд идут два блока у первого из которых есть отступ снизу, а у второго сверху. В таком
    // случае отступ должен не суммироваться, а стать наибольшим из имеющихся. Например у верхнего
    // отступ снизу 2 строки, а у нижнего отступ сверху 1 строка, отступ между ними должен быть
    // 2 строки.
    //
    int lastEmptyLines = 0;
    while (!sourceDocumentCursor.atEnd()) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        //
        // Получим тип текущего блока под курсором
        //
        currentBlockType = ScenarioBlockStyle::forBlock(sourceDocumentCursor.block());

        //
        // Если блок содержит текст, который необходимо вывести на печать
        //
        if (::needPrintBlock(currentBlockType, _exportParameters.isOutline,
                             _exportParameters.saveInvisible)) {

            //
            // Определим стили и настроим курсор
            //
            QTextBlockFormat blockFormat = ::blockFormatForType(currentBlockType);
            QTextCharFormat charFormat = ::charFormatForType(currentBlockType);

            //
            // Если вставляется не первый блок текста, возможно следует сделать отступы
            //
            {
                LineType currentLineType = ::currentLine(preparedDocument, blockFormat, charFormat);
                if (currentLineType == MiddlePageLine) {
                    int emptyLines = exportStyle.blockStyle(currentBlockType).topSpace();
                    //
                    // Корректируем кол-во вставляемых строк в зависимости от предыдущего блока
                    //
                    if (lastEmptyLines > 0) {
                        if (lastEmptyLines > emptyLines) {
                            emptyLines = 0;
                        } else {
                            emptyLines -= lastEmptyLines;
                        }
                    }

                    //
                    // ... выполняется до тех пор, пока не будут вставлены все строки,
                    //	   или не будет достигнут конец страницы
                    //
                    while (emptyLines-- > 0
                           && currentLineType == MiddlePageLine) {
                        //
                        // ... вставим линию и настроим её стиль
                        //
                        ::insertLine(destDocumentCursor, blockFormat, charFormat);
                        currentLineType = ::currentLine(preparedDocument, blockFormat, charFormat);
                    }
                }
            }

            //
            // Вставляется текст блока
            //
            {
                //
                // ... если вставляется не первый блок текста
                //
                LineType currentLineType = ::currentLine(preparedDocument, blockFormat, charFormat);
                if (currentLineType != FirstDocumentLine) {
                    //
                    // ... вставим новый абзац для наполнения текстом
                    //
                    destDocumentCursor.insertBlock();
                }

                //
                // Настроим стиль нового абзаца
                //
                destDocumentCursor.setBlockFormat(blockFormat);
                destDocumentCursor.setCharFormat(charFormat);

                //
                // Дополнительное место под номер вписываемый в текст блока
                //
                int additionalSpaceForNumber = 0;

                //
                // Для блока "Время и место" добавочная информация
                //
                if (currentBlockType == ScenarioBlockStyle::SceneHeading) {
                    //
                    // Данные сцены
                    //
                    QTextBlockUserData* textBlockData = sourceDocumentCursor.block().userData();
                    SceneHeadingBlockInfo* sceneInfo = dynamic_cast<SceneHeadingBlockInfo*>(textBlockData);
                    if (sceneInfo != nullptr) {
                        destDocumentCursor.block().setUserData(sceneInfo->clone());
                    }

                    //
                    // Префикс экспорта
                    //
                    destDocumentCursor.insertText(_exportParameters.scenesPrefix);
                    //
                    // Номер сцены, если необходимо
                    //
                    if (_exportParameters.printScenesNumbers
                        && sceneInfo != nullptr) {
                        const QString sceneNumber = QString("%1. ").arg(sceneInfo->sceneNumber());
                        destDocumentCursor.insertText(sceneNumber);
                        additionalSpaceForNumber = sceneNumber.length();
                    }
                }
                //
                // Для блока реплики добавляем номер реплики, если необходимо
                //
                else if (currentBlockType == ScenarioBlockStyle::Character) {
                    //
                    // Данные реплики
                    //
                    QTextBlockUserData* textBlockData = sourceDocumentCursor.block().userData();
                    CharacterBlockInfo* characterInfo = dynamic_cast<CharacterBlockInfo*>(textBlockData);
                    if (characterInfo != nullptr) {
                        destDocumentCursor.block().setUserData(characterInfo->clone());
                    }

                    //
                    // Номер реплики, если необходимо
                    //
                    if (_exportParameters.printDialoguesNumbers
                        && characterInfo != nullptr) {
                        const QString dialogueNumber = QString("%1: ").arg(characterInfo->dialogueNumbder());
                        destDocumentCursor.insertText(dialogueNumber);
                        additionalSpaceForNumber = dialogueNumber.length();
                    }
                }

                //
                // Вставить текст
                //
                // Приходится вручную устанавливать верхний регистр для текста,
                // т.к. при выводе в диалог предварительного просмотра эта
                // настройка не учитывается...
                //
                if (charFormatForType(currentBlockType).fontCapitalization() == QFont::AllUppercase) {
                    destDocumentCursor.insertText(sourceDocumentCursor.block().text().toUpper());
                } else {
                    destDocumentCursor.insertText(sourceDocumentCursor.block().text());
                }

                //
                // Добавляем редакторские пометки, если необходимо
                //
                if (_exportParameters.saveReviewMarks) {
                    //
                    // Если в блоке есть пометки, добавляем их
                    //
                    if (!sourceDocumentCursor.block().textFormats().isEmpty()) {
                        const int startBlockPosition = destDocumentCursor.block().position();
                        foreach (const QTextLayout::FormatRange& range, sourceDocumentCursor.block().textFormats()) {
                            if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                                destDocumentCursor.setPosition(startBlockPosition + range.start + additionalSpaceForNumber);
                                destDocumentCursor.setPosition(destDocumentCursor.position() + range.length, QTextCursor::KeepAnchor);
                                destDocumentCursor.mergeCharFormat(range.format);
                            }
                        }
                        destDocumentCursor.movePosition(QTextCursor::EndOfBlock);
                    }
                }

                //
                // Если в блоке есть форматирование отличное от стандартного, добавляем его
                //
                if (!sourceDocumentCursor.block().textFormats().isEmpty()) {
                    const int startBlockPosition = destDocumentCursor.block().position();
                    foreach (const QTextLayout::FormatRange& range, sourceDocumentCursor.block().textFormats()) {
                        if (range.format == sourceDocumentCursor.blockCharFormat()
                            || range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                            continue;
                        }

                        destDocumentCursor.setPosition(startBlockPosition + range.start + additionalSpaceForNumber);
                        destDocumentCursor.setPosition(destDocumentCursor.position() + range.length, QTextCursor::KeepAnchor);
                        QTextCharFormat format = range.format;
                        format.setForeground(Qt::black);
                        destDocumentCursor.mergeCharFormat(format);
                    }
                    destDocumentCursor.movePosition(QTextCursor::EndOfBlock);
                }
            }

            //
            // После текста, так же возможно следует сделать отступы
            //
            {
                LineType currentLineType = ::currentLine(preparedDocument, blockFormat, charFormat);
                if (currentLineType == MiddlePageLine) {
                    int emptyLines = exportStyle.blockStyle(currentBlockType).bottomSpace();
                    //
                    // ... сохраним кол-во пустых строк в последнем блоке
                    //
                    lastEmptyLines = emptyLines;
                    //
                    // ... выполняется до тех пор, пока не будут вставлены все строки,
                    //	   или не будет достигнут конец страницы
                    //
                    while (emptyLines-- > 0
                           && currentLineType == MiddlePageLine) {
                        //
                        // ... вставим линию и настроим её стиль
                        //
                        ::insertLine(destDocumentCursor, blockFormat, charFormat);
                        currentLineType = ::currentLine(preparedDocument, blockFormat, charFormat);
                    }
                }
            }
        }

        //
        // Переходим к следующему блоку
        //
        sourceDocumentCursor.movePosition(QTextCursor::EndOfBlock);
        sourceDocumentCursor.movePosition(QTextCursor::NextBlock);
    }

    //
    // Удаляем копию документа с текстом сценария
    //
    delete scenarioDocument;
    scenarioDocument = 0;

    return preparedDocument;
}

QTextDocument* AbstractExporter::prepareDocument(const ResearchModelCheckableProxy* _researchModel,
    const ExportParameters& _exportParameters)
{
    //
    // Настроим новый документ
    //
    QTextDocument* preparedDocument = prepareDocument();
    preparedDocument->setDefaultFont(QFont(defaultResearchFont(), defaultResearchFontSize()));

    //
    // Проходим все выбранные для экспорта документы разработки и помещаем их в документ
    //
    QTextCursor cursor(preparedDocument);
    ::exportResearchLevel(_researchModel, _exportParameters, QModelIndex(), 0, cursor);

    return preparedDocument;
}

QTextDocument* AbstractExporter::prepareDocument()
{
    //
    // Настроим новый документ
    //
    QTextDocument* preparedDocument = new QTextDocument;
    preparedDocument->setDocumentMargin(0);
    preparedDocument->setIndentWidth(0);

    //
    // Настроим размер страниц
    //
    preparedDocument->setPageSize(::documentSize());

    return preparedDocument;
}
