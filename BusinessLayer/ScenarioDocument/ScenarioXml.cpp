#include "ScenarioXml.h"

#include "ScenarioDocument.h"
#include "ScenarioTextDocument.h"
#include "ScenarioModelItem.h"
#include "ScenarioTemplate.h"
#include "ScenarioTextBlockInfo.h"
#include "ScriptTextCursor.h"

#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QApplication>
#include <QStringBuilder>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextTable>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Шаг увеличения размера кэша при его заполнении
     */
    const int kCostIncreaseStep = 100;

    /**
     * @brief Начальное значение размера кэша
     */
    const int kInitialCost  = 3000;

    const QString kNodeScript = "scenario";
    const QString kNodeValue = "v";
    const QString kNodeReviewGroup = "reviews";
    const QString kNodeReview = "review";
    const QString kNodeReviewComment = "review_comment";
    const QString kNodeFormatGroup = "formatting";
    const QString kNodeFormat = "format";
    const QString kNodeSplitterStart = "splitter_start";
    const QString kNodeSplitter = "splitter";
    const QString kNodeSplitterEnd = "splitter_end";

    const QString ATTRIBUTE_VERSION = "version";
    const QString ATTRIBUTE_DESCRIPTION = "description";
    const QString ATTRIBUTE_UUID = "uuid";
    const QString ATTRIBUTE_COLOR = "color";
    const QString ATTRIBUTE_STAMP = "stamp";
    const QString ATTRIBUTE_TITLE = "title";
    const QString ATTRIBUTE_BOOKMARK = "bookmark";
    const QString ATTRIBUTE_BOOKMARK_COLOR = "bookmark_color";
    const QString ATTRIBUTE_REVIEW_FROM = "from";
    const QString ATTRIBUTE_REVIEW_LENGTH = "length";
    const QString ATTRIBUTE_REVIEW_COLOR = "color";
    const QString ATTRIBUTE_REVIEW_BGCOLOR = "bgcolor";
    const QString ATTRIBUTE_REVIEW_IS_HIGHLIGHT = "is_highlight";
    const QString ATTRIBUTE_REVIEW_DONE = "done";
    const QString ATTRIBUTE_REVIEW_COMMENT = "comment";
    const QString ATTRIBUTE_REVIEW_AUTHOR = "author";
    const QString ATTRIBUTE_REVIEW_DATE = "date";
    const QString ATTRIBUTE_FORMAT_FROM = "from";
    const QString ATTRIBUTE_FORMAT_LENGTH = "length";
    const QString ATTRIBUTE_FORMAT_BOLD = "bold";
    const QString ATTRIBUTE_FORMAT_ITALIC = "italic";
    const QString ATTRIBUTE_FORMAT_UNDERLINE = "underline";
    const QString ATTRIBUTE_SCENE_NUMBER = "number";
    const QString ATTRIBUTE_SCENE_NUMBER_FIX_NESTING = "number_fix_nesting";
    const QString ATTRIBUTE_SCENE_NUMBER_SUFFIX = "number_suffix";
    const QString ATTRIBUTE_DIFF_ADDED = "diff_added";
    const QString ATTRIBUTE_DIFF_REMOVED = "diff_removed";

    const QString SCENARIO_XML_VERSION = "1.0";

    /**
     * @brief Сформировать хэш для текстового блока
     */
    static inline quint64 blockHash(const QTextBlock& _block)
    {
        //
        // TODO: Нужно оптимизировать формирование хэша, т.к. от него напрямую зависит
        //       скорость формирования xml-документа сценария
        //
        TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(_block.userData());
        if (blockInfo != nullptr) {
            return blockInfo->id();
        }

        //
        // Формируем уникальную строку, главное, чтобы два разных блока не имели одинакового хэша,
        // но в то же время, нельзя опираться на позицию блока, т.к. при смещение текста на абзац
        // вниз, придётся пересчитывать хэши всех остальных блоков
        //
        QString hash =
                _block.text()
                % "#"
                % QString::number(_block.revision())
                % "#"
                % QString::number(ScenarioBlockStyle::forBlock(_block));

        if (TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(_block.userData())) {
            if (SceneHeadingBlockInfo* shBlockInfo = dynamic_cast<SceneHeadingBlockInfo*>(blockInfo)) {
                hash = hash
                       % "#"
                       % shBlockInfo->uuid()
                       % "#"
                       % shBlockInfo->sceneNumber()
                       % "#"
                       % QString::number(shBlockInfo->isSceneNumberFixed())
                       % "#"
                       % QString::number(shBlockInfo->sceneNumberFixNesting())
                       % "#"
                       % shBlockInfo->colors()
                       % "#"
                       % shBlockInfo->stamp()
                       % "#"
                       % shBlockInfo->name()
                       % "#"
                       % shBlockInfo->description()
                       % "#"
                       % (shBlockInfo->hasBookmark() ? "1" : "0")
                       % "#"
                       % shBlockInfo->bookmark();
            } else {
                hash = hash
                       % "#"
                       % (blockInfo->hasBookmark() ? "1" : "0")
                       % "#"
                       % blockInfo->bookmark();
            }
        }

        for (const QTextLayout::FormatRange& range : _block.textFormats()) {
            if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                hash = hash
                        % "#"
                        % QString::number(range.start)
                        % "#"
                        % QString::number(range.length)
                        % "#"
                        % range.format.foreground().color().name()
                        % "#"
                        % range.format.background().color().name()
                        % "#"
                        % (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark) ? "1" : "0")
                        % "#"
                        % (range.format.boolProperty(ScenarioBlockStyle::PropertyIsHighlight) ? "1" : "0")
                        % "#"
                        % (range.format.boolProperty(ScenarioBlockStyle::PropertyIsDone) ? "1" : "0")
                        % "#"
                        % range.format.property(ScenarioBlockStyle::PropertyComments).toStringList().join("#")
                        % "#"
                        % range.format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList().join("#")
                        % "#"
                        % range.format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList().join("#");
            }
            if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsFormatting)) {
                hash = hash
                        % "#"
                        % QString::number(range.start)
                        % "#"
                        % QString::number(range.length)
                        % "#"
                        % (range.format.font().bold() ? "1" : "0")
                        % "#"
                        % (range.format.font().italic() ? "1" : "0")
                        % "#"
                        % (range.format.font().underline() ? "1" : "0");
            }
        }

        return qHash(hash);
    }

    /**
     * @brief Одинаковы ли форматы редакторских заметок
     */
    static bool isReviewFormatEquals(const QTextCharFormat& _lhs, const QTextCharFormat& _rhs) {
        return
                _lhs.foreground().color() == _rhs.foreground().color()
                && _lhs.background().color() == _rhs.background().color()
                && _lhs.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark) == _rhs.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)
                && _lhs.boolProperty(ScenarioBlockStyle::PropertyIsHighlight) == _rhs.boolProperty(ScenarioBlockStyle::PropertyIsHighlight)
                && _lhs.boolProperty(ScenarioBlockStyle::PropertyIsDone) == _rhs.boolProperty(ScenarioBlockStyle::PropertyIsDone)
                && _lhs.property(ScenarioBlockStyle::PropertyComments) == _rhs.property(ScenarioBlockStyle::PropertyComments)
                && _lhs.property(ScenarioBlockStyle::PropertyCommentsAuthors) == _rhs.property(ScenarioBlockStyle::PropertyCommentsAuthors)
                && _lhs.property(ScenarioBlockStyle::PropertyCommentsDates) == _rhs.property(ScenarioBlockStyle::PropertyCommentsDates);
    }

    /**
     * @brief Есть ли в блоке редакторские заметки
     */
    static bool hasReviewMarks(const QTextBlock& _block) {
        bool hasMarks = false;
        foreach (const QTextLayout::FormatRange& range, _block.textFormats()) {
            if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                hasMarks = true;
                break;
            }
        }
        return hasMarks;
    }

    /**
     * @brief Одинаково ли форматирование блоков (сравниваем только ж/к/п)
     */
    static bool isFormattingEquals(const QTextCharFormat& _lhs, const QTextCharFormat& _rhs) {
        return
                _lhs.fontWeight() == _rhs.fontWeight()
                && _lhs.fontItalic() == _rhs.fontItalic()
                && _lhs.fontUnderline() == _rhs.fontUnderline();
    }

    /**
     * @brief Есть ли в блоке пользовательское форматирование
     */
    static bool hasFormatting(const QTextBlock& _block) {
        bool hasFormatting = false;
        foreach (const QTextLayout::FormatRange& range, _block.textFormats()) {
            if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsFormatting)) {
                hasFormatting = true;
                break;
            }
        }
        return hasFormatting;
    }
}


QString ScenarioXml::defaultCardsXml()
{
    return "<?xml version=\"1.0\"?>\n"
           "<cards x=\"0\" y=\"0\" width=\"0\" height=\"0\" scale=\"1\" >\n"
           "<Card id=\"{000000-0000000-000000}\" is_folder=\"false\" title=\"\" description=\"\" stamp=\"\" colors=\"\" x=\"0\" y=\"0\" />\n"
           "</cards_xml>";
}

QString ScenarioXml::defaultTextXml()
{
    return makeMimeFromXml(
            "<scene_heading uuid=\"{000000-0000000-000000}\">\n"
            "<v><![CDATA[]]></v>\n"
            "</scene_heading>\n");
}

QString ScenarioXml::makeMimeFromXml(const QString& _xml)
{
    const QString XML_HEADER = "<?xml version=\"1.0\"?>\n";
    const QString SCENARIO_HEADER = "<scenario version=\"1.0\">\n";
    const QString SCENARIO_FOOTER = "</scenario>\n";

    QString mimeXml = _xml;
    if (!mimeXml.contains(XML_HEADER)) {
        if (!mimeXml.contains(SCENARIO_HEADER)) {
            mimeXml.prepend(SCENARIO_HEADER);
        }
        mimeXml.prepend(XML_HEADER);
    }
    if (!mimeXml.endsWith(SCENARIO_FOOTER)) {
        mimeXml.append(SCENARIO_FOOTER);
    }
    return mimeXml;
}

ScenarioXml::ScenarioXml(ScenarioDocument* _scenario) :
    m_scenario(_scenario),
    m_lastMimeFrom(0),
    m_lastMimeTo(0)
{
    Q_ASSERT(m_scenario);

    m_xmlCache.setMaxCost(kInitialCost);
}

QString ScenarioXml::scenarioToXml()
{
    //
    // Проверим, чтобы ёмкость кэша была достаточной
    //
    {
        const int maxCost = m_xmlCache.maxCost();
        const int currentCost = m_scenario->document()->blockCount();
        if (currentCost > kInitialCost
            && (maxCost < currentCost
                || maxCost > currentCost * 2)) {
            m_xmlCache.setMaxCost(std::max(kInitialCost, currentCost + kCostIncreaseStep));
        }
    }


    //
    // Для формирования xml не используем QXmlStreamWriter, т.к. нам нужно хранить по отдельности
    // xml каждого блока, а QXmlStreamWriter не всегда закрывает последний записанный тэг,
    // оставляя место для записи атрибутов. В результате это приводит к появлению в xml
    // странных последовательностей, наподобии ">>" или ">/>"
    //

    QString resultXml;

    QTextBlock currentBlock = m_scenario->document()->begin();
    QString currentBlockXml;
    bool isInTable = false;
    bool isSecondColumn = false;
    do {
        currentBlockXml.clear();
        const quint64 currentBlockHash = blockHash(currentBlock);

        //
        // Определим тип текущего блока
        //
        ScenarioBlockStyle::Type currentType = ScenarioBlockStyle::forBlock(currentBlock);

        //
        // Выполним проверки необходимые для корректной обработки таблиц
        //
        {
            //
            // Собственно проверяем, что попали или вышли из таблицы
            //
            if (currentType == ScenarioBlockStyle::PageSplitter) {
                isInTable = !isInTable;
                isSecondColumn = false;
            }
            //
            // Если начало второй колонки, запишем разделитель
            //
            if (isInTable
                && !isSecondColumn) {
                QTextCursor cursor(currentBlock);
                if (cursor.currentTable() != nullptr
                    && cursor.currentTable()->cellAt(cursor).column() == 1) { // вторая колонка
                    isSecondColumn = true;
                    resultXml.append(QString("<%1/>\n").arg(kNodeSplitter));
                }
            }
        }

        //
        // Используем кэшированное значение, если
        // - для блока есть кэш
        // - блок не разорван по середине
        // - блок не является границей таблицы
        //
        if (m_xmlCache.contains(currentBlockHash)
            && !currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)
            && !currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)
            && currentType != ScenarioBlockStyle::PageSplitter) {
            resultXml.append(m_xmlCache[currentBlockHash]);
        }
        //
        // В противном случае формируем xml
        //
        else {
            //
            // Получить текст под курсором
            //
            QString textToSave = TextEditHelper::toHtmlEscaped(currentBlock.text());

            //
            // Определить параметры текущего абзаца
            //
            bool needWrite = true; // пишем абзац?
            QString currentNode = ScenarioBlockStyle::typeName(currentType); // имя текущей ячейки
            bool canHaveColors = false; // может иметь цвета
            switch (currentType) {
                case ScenarioBlockStyle::SceneHeading: {
                    canHaveColors = true;
                    break;
                }

                case ScenarioBlockStyle::Parenthetical: {
                    needWrite = !textToSave.isEmpty();
                    break;
                }

                case ScenarioBlockStyle::FolderHeader: {
                    canHaveColors = true;
                    break;
                }

                case ScenarioBlockStyle::PageSplitter: {
                    needWrite = false;
                    break;
                }

                default: {
                    break;
                }
            }

            //
            // Если это декорация, не сохраняем
            //
            if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                needWrite = false;
            }

            //
            // Если разрыв, пробуем сшить
            //
            QTextBlock previousBlock;
            if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
                //
                // ... сохраняем начальный блок разрыва абзаца
                //
                previousBlock = currentBlock;
                do {
                    currentBlock = currentBlock.next();
                } while (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection));
                //
                // ... если дошли до конца разрыва, то сшиваем его
                //
                if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    textToSave += " " + TextEditHelper::toHtmlEscaped(currentBlock.text());
                }
            }

            //
            // Дописать xml
            //
            if (needWrite) {
                //
                // NOTE: все данные параграфа остаются в первом блоке, если есть разрыв, так что в
                //       этом случае нужно смотреть по первому блоку
                //
                auto blockUserData = previousBlock.isValid()
                                     ? previousBlock.userData()
                                     : currentBlock.userData();

                //
                // Если возможно, сохраним uuid, цвета элемента, штамп и его заголовок
                //
                QString uuidColorsAndTitle;
                if (canHaveColors) {
                    SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(blockUserData);
                    if (info == nullptr) {
                        info = new SceneHeadingBlockInfo;
                        if (previousBlock.isValid()) {
                            previousBlock.setUserData(info);
                        } else {
                            currentBlock.setUserData(info);
                        }
                        blockUserData = info;
                    }
                    //
                    if (!info->uuid().isEmpty()) {
                        uuidColorsAndTitle = QString(" %1=\"%2\"").arg(ATTRIBUTE_UUID, info->uuid());
                    }
                    if (!info->colors().isEmpty()) {
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_COLOR, info->colors());
                    }
                    if (!info->stamp().isEmpty()) {
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_STAMP, info->stamp());
                    }
                    if (!info->name().isEmpty()) {
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_TITLE, TextEditHelper::toHtmlEscaped(info->name()));
                    }
                    if (!info->sceneNumber().isEmpty() && info->isSceneNumberFixed()) {
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_SCENE_NUMBER, TextEditHelper::toHtmlEscaped(info->sceneNumber()));
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_SCENE_NUMBER_FIX_NESTING, QString::number(info->sceneNumberFixNesting()));
                        uuidColorsAndTitle += QString(" %1=\"%2\"").arg(ATTRIBUTE_SCENE_NUMBER_SUFFIX, QString::number(info->sceneNumberSuffix()));
                    }
                }

                //
                // Запишем закладку, если установлена для блока
                // NOTE: все данные параграфа остаются в первом блоке, если есть разрыв, так что в
                //       этом случае нужно смотреть по первому блоку
                //
                QString bookmark;
                {
                    TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(blockUserData);
                    if (blockInfo != nullptr
                        && blockInfo->hasBookmark()) {
                        bookmark = QString(" %1=\"%2\"").arg(ATTRIBUTE_BOOKMARK).arg(TextEditHelper::toHtmlEscaped(blockInfo->bookmark()));
                        bookmark += QString(" %1=\"%2\"").arg(ATTRIBUTE_BOOKMARK_COLOR).arg(blockInfo->bookmarkColor().name());
                    }
                }

                //
                // Открыть ячейку текущего элемента
                //
                currentBlockXml.append(QString("<%1%2%3>\n").arg(currentNode, uuidColorsAndTitle, bookmark));

                //
                // Пишем текст текущего элемента
                //
                currentBlockXml.append(QString("<%1><![CDATA[%2]]></%1>\n").arg(kNodeValue, textToSave));

                //
                // Пишем редакторские комментарии, если они есть в блоке
                //
                if (hasReviewMarks(previousBlock) || hasReviewMarks(currentBlock)) {
                    auto writeReviewMark = [&currentBlockXml] (const QTextLayout::FormatRange& _range) {
                        currentBlockXml.append(QString("<%1").arg(kNodeReview));
                        //
                        // Данные редакторского выделения
                        //
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_FROM, QString::number(_range.start)));
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_LENGTH, QString::number(_range.length)));
                        if (_range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_COLOR, _range.format.foreground().color().name()));
                        }
                        if (_range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_BGCOLOR, _range.format.background().color().name()));
                        }
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_IS_HIGHLIGHT,
                            _range.format.boolProperty(ScenarioBlockStyle::PropertyIsHighlight) ? "true" : "false"));
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_DONE,
                            _range.format.boolProperty(ScenarioBlockStyle::PropertyIsDone) ? "true" : "false"));
                        currentBlockXml.append(">\n");
                        //
                        // ... сами комментарии
                        //
                        const QStringList comments = _range.format.property(ScenarioBlockStyle::PropertyComments).toStringList();
                        const QStringList authors = _range.format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
                        const QStringList dates = _range.format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();
                        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                            currentBlockXml.append(QString("<%1").arg(kNodeReviewComment));
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_COMMENT,
                                TextEditHelper::toHtmlEscaped(comments.at(commentIndex))));
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_AUTHOR, authors.at(commentIndex)));
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_REVIEW_DATE, dates.at(commentIndex)));
                            currentBlockXml.append("/>\n");
                        }
                        //
                        currentBlockXml.append(QString("</%1>\n").arg(kNodeReview));
                    };

                    //
                    // Пишем начало
                    //
                    currentBlockXml.append(QString("<%1>\n").arg(kNodeReviewGroup));
                    //
                    // Пишем тело
                    //
                    // ... но сперва сформируем список форматов, скорректировав его, если был разрыв
                    //
                    auto ranges = [previousBlock, currentBlock] {
                        if (!previousBlock.isValid()) {
                            return currentBlock.textFormats();
                        }

                        auto ranges = previousBlock.textFormats();
                        for (QTextLayout::FormatRange range : currentBlock.textFormats()) {
                            //
                            // ... если в самом начале разрыва находится часть выделения
                            //     предыдущего блока, объединяем их
                            //
                            if (range.start == 0
                                && isReviewFormatEquals(ranges.last().format, range.format)) {
                                ranges.last().length += range.length + 1;
                            }
                            //
                            // ... в противном случае просто сохраняем выделение,
                            //     корректируя стартовую позицию
                            //
                            else {
                                range.start += previousBlock.length();
                                ranges.append(range);
                            }
                        }
                        return ranges;
                    } ();
                    QTextLayout::FormatRange lastFormatRange{0, 0, QTextCharFormat()};
                    for (const QTextLayout::FormatRange& range : ranges) {
                        if (!range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                            continue;
                        }

                        //
                        // Если следующий формат равен предыдущему и он является продолжает предыдущего
                        //
                        if (isReviewFormatEquals(lastFormatRange.format, range.format)
                            && ((lastFormatRange.start + lastFormatRange.length) == range.start)) {
                            //
                            // Объединяем их в одну сущность
                            //
                            lastFormatRange.length += range.length;
                        }
                        //
                        // В противном случае
                        //
                        else {
                            //
                            // Если предыдущий формат был задан, запишем его
                            //
                            if (lastFormatRange.length > 0) {
                                writeReviewMark(lastFormatRange);
                            }
                            //
                            // и перейдём к обработке следующего формата
                            //
                            lastFormatRange = range;
                        }
                    }
                    //
                    // Запишем последний формат
                    //
                    writeReviewMark(lastFormatRange);
                    //
                    // Пишем конец
                    //
                    currentBlockXml.append(QString("</%1>\n").arg(kNodeReviewGroup));
                }

                //
                // Пишем форматирование текста блока
                //
                if (hasFormatting(previousBlock) || hasFormatting(currentBlock)) {
                    auto writeFormatting = [&currentBlockXml] (const QTextLayout::FormatRange& _range) {
                        currentBlockXml.append(QString("<%1").arg(kNodeFormat));
                        //
                        // Данные пользовательского форматирования
                        //
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_FORMAT_FROM, QString::number(_range.start)));
                        currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_FORMAT_LENGTH, QString::number(_range.length)));
                        if (_range.format.hasProperty(QTextFormat::FontWeight)) {
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_FORMAT_BOLD,
                                                                             _range.format.font().bold() ? "true" : "false"));
                        }
                        if (_range.format.hasProperty(QTextFormat::FontItalic)) {
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_FORMAT_ITALIC,
                                                                             _range.format.font().italic() ? "true" : "false"));
                        }
                        if (_range.format.hasProperty(QTextFormat::TextUnderlineStyle)) {
                            currentBlockXml.append(QString(" %1=\"%2\"").arg(ATTRIBUTE_FORMAT_UNDERLINE,
                                                                             _range.format.font().underline() ? "true" : "false"));
                        }
                        //
                        currentBlockXml.append("/>\n");
                    };

                    //
                    // Пишем начало
                    //
                    currentBlockXml.append(QString("<%1>\n").arg(kNodeFormatGroup));
                    //
                    // Пишем тело
                    //
                    // ... но сперва сформируем список форматов, скорректировав его, если был разрыв
                    //
                    auto ranges = [previousBlock, currentBlock] {
                        if (!previousBlock.isValid()) {
                            return currentBlock.textFormats();
                        }

                        auto ranges = previousBlock.textFormats();
                        for (QTextLayout::FormatRange range : currentBlock.textFormats()) {
                            //
                            // ... если в самом начале разрыва находится часть выделения
                            //     предыдущего блока, объединяем их
                            //
                            if (range.start == 0
                                && isFormattingEquals(ranges.last().format, range.format)) {
                                ranges.last().length += range.length + 1;
                            }
                            //
                            // ... в противном случае просто сохраняем выделение,
                            //     корректируя стартовую позицию
                            //
                            else {
                                range.start += previousBlock.length();
                                ranges.append(range);
                            }
                        }
                        return ranges;
                    } ();
                    QTextLayout::FormatRange lastFormatRange{0, 0, QTextCharFormat()};
                    for (const QTextLayout::FormatRange& range : ranges) {
                        if (!range.format.boolProperty(ScenarioBlockStyle::PropertyIsFormatting)) {
                            continue;
                        }

                        //
                        // Если следующий формат равен предыдущему и он является продолжает предыдущего
                        //
                        if (isFormattingEquals(lastFormatRange.format, range.format)
                            && ((lastFormatRange.start + lastFormatRange.length) == range.start)) {
                            //
                            // Объединяем их в одну сущность
                            //
                            lastFormatRange.length += range.length;
                        }
                        //
                        // В противном случае
                        //
                        else {
                            //
                            // Если предыдущий формат был задан, запишем его
                            //
                            if (lastFormatRange.length > 0) {
                                writeFormatting(lastFormatRange);
                            }
                            //
                            // и перейдём к обработке следующего формата
                            //
                            lastFormatRange = range;
                        }
                    }
                    //
                    // Запишем последний формат
                    //
                    writeFormatting(lastFormatRange);
                    //
                    // Пишем конец
                    //
                    currentBlockXml.append(QString("</%1>\n").arg(kNodeFormatGroup));
                }

                //
                // Закрываем текущий элемент
                //
                currentBlockXml.append(QString("</%1>\n").arg(currentNode));
            }

            else if (currentType == ScenarioBlockStyle::PageSplitter) {
                currentBlockXml.append(QString("<%1/>\n").arg(isInTable
                                                            ? kNodeSplitterStart
                                                            : kNodeSplitterEnd));
            }

            m_xmlCache.insert(currentBlockHash, new QString(currentBlockXml));
            resultXml.append(currentBlockXml);
        }
        currentBlock = currentBlock.next();


    } while (currentBlock.isValid());

    return makeMimeFromXml(resultXml);
}

QString ScenarioXml::scenarioToXml(int _startPosition, int _endPosition, bool _correctLastMime)
{
    QString resultXml;

    //
    // Если необходимо обработать весь текст
    //
    bool isFullDocumentSave = false;
    if (_startPosition == 0
        && _endPosition == 0) {
        isFullDocumentSave = true;
        _endPosition = m_scenario->document()->characterCount();
    }

    //
    // Сохраним позиции
    //
    if (_correctLastMime) {
        m_lastMimeFrom = _startPosition;
        m_lastMimeTo = _endPosition;
    }

    //
    // Получим курсор для редактирования
    //
    QTextCursor cursor(m_scenario->document());

    //
    // Переместимся к началу текста для формирования xml
    //
    cursor.setPosition(_startPosition);

    //
    // Подсчитаем кол-во незакрытых папок, и закроем, если необходимо
    //
    int openedFolders = 0;

    QXmlStreamWriter writer(&resultXml);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(0);
    writer.writeStartDocument();
    writer.writeStartElement(kNodeScript);
    writer.writeAttribute(ATTRIBUTE_VERSION, "1.0");
    bool isFirstBlock = true;
    bool isInTable = false;
    bool isSecondColumn = false;
    do {
        //
        // Для всего документа сохраняем блоками
        //
        if (isFullDocumentSave) {
            //
            // Если не первый блок, перейдём к следующему
            //
            if (!isFirstBlock) {
                cursor.movePosition(QTextCursor::NextBlock);
            } else {
                isFirstBlock = false;
            }
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }
        //
        // Для некоторого текста, посимвольно
        //
        else {
            //
            // ... перебегаем на следующий блок, чтобы не захватыывать символ переноса строки
            //
            if (!cursor.hasSelection()
                && cursor.atBlockEnd()) {
                cursor.movePosition(QTextCursor::NextCharacter);
            }
            //
            // ... если текущий блок не пуст, выделяем его текст
            //
            if (!cursor.block().text().isEmpty()) {
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            }
        }

        //
        // Курсор в конце текущего блока
        // или в конце выделения
        //
        if (cursor.atBlockEnd()
            || cursor.position() == _endPosition) {
            //
            // Текущий блок
            //
            QTextBlock currentBlock = cursor.block();

            //
            // Определим тип текущего блока
            //
            ScenarioBlockStyle::Type currentType = ScenarioBlockStyle::forBlock(currentBlock);

            //
            // Получить текст под курсором
            //
            QString textToSave = TextEditHelper::toHtmlEscaped(cursor.selectedText());

            //
            // Определить параметры текущего абзаца
            //
            bool needWrite = true; // пишем абзац?
            QString currentNode = ScenarioBlockStyle::typeName(currentType); // имя текущей ячейки
            bool canHaveUuidColorsAndTitle = false; // может иметь цвета
            switch (currentType) {
                case ScenarioBlockStyle::SceneHeading: {
                    canHaveUuidColorsAndTitle = true;
                    break;
                }

                case ScenarioBlockStyle::Parenthetical: {
                    needWrite = !textToSave.isEmpty();
                    break;
                }

                case ScenarioBlockStyle::FolderHeader: {
                    canHaveUuidColorsAndTitle = true;
                    ++openedFolders;
                    break;
                }

                case ScenarioBlockStyle::FolderFooter: {
                    //
                    // Закрываем папки, если были открыты, то просто корректируем счётчик,
                    // а если открытых нет, то не записываем и конец
                    //
                    if (openedFolders > 0) {
                        --openedFolders;
                    } else {
                        needWrite = false;
                    }
                    break;
                }

                case ScenarioBlockStyle::PageSplitter: {
                    isInTable = !isInTable;
                    needWrite = false;
                    break;
                }

                default: {
                    break;
                }
            }

            //
            // Если это декорация, не сохраняем
            //
            if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                needWrite = false;
            }

            //
            // Если разрыв, пробуем сшить
            //
            QTextBlock previousBlock;
            if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
                //
                // ... сохраняем начальный блок разрыва абзаца
                //
                previousBlock = currentBlock;
                QTextCursor breakCursor = cursor;
                do {
                    breakCursor.movePosition(QTextCursor::NextBlock);
                } while (breakCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection));
                //
                // ... если дошли до конца разрыва, то сшиваем его
                //
                if (breakCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    textToSave += " " + breakCursor.block().text();
                    breakCursor.movePosition(QTextCursor::EndOfBlock);
                    cursor = breakCursor;
                    currentBlock = cursor.block();
                }
            }

            //
            // Дописать xml
            //
            if (needWrite) {
                //
                // Если начало второй колонки, запишем разделитель
                //
                if (isInTable
                    && !isSecondColumn
                    && cursor.currentTable() != nullptr
                    && cursor.currentTable()->cellAt(cursor).column() == 1) { // вторая колонка
                    isSecondColumn = true;
                    writer.writeEmptyElement(kNodeSplitter);
                }

                //
                // Открыть ячейку текущего элемента
                //
                writer.writeStartElement(currentNode);

                //
                // NOTE: все данные параграфа остаются в первом блоке, если есть разрыв, так что в
                //       этом случае нужно смотреть по первому блоку
                //
                auto blockUserData = previousBlock.isValid()
                                     ? previousBlock.userData()
                                     : currentBlock.userData();

                //
                // Если возможно, сохраним uuid, цвета элемента и его название
                //
                if (canHaveUuidColorsAndTitle) {
                    SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(blockUserData);
                    if (info == nullptr) {
                        SceneHeadingBlockInfo* info = new SceneHeadingBlockInfo;
                        if (previousBlock.isValid()) {
                            previousBlock.setUserData(info);
                        } else {
                            currentBlock.setUserData(info);
                        }
                        blockUserData = info;
                    }
                    //
                    if (!info->uuid().isEmpty()) {
                        writer.writeAttribute(ATTRIBUTE_UUID, info->uuid());
                    }
                    if (!info->colors().isEmpty()) {
                        writer.writeAttribute(ATTRIBUTE_COLOR, info->colors());
                    }
                    if (!info->stamp().isEmpty()) {
                        writer.writeAttribute(ATTRIBUTE_STAMP, info->stamp());
                    }
                    if (!info->name().isEmpty()) {
                        writer.writeAttribute(ATTRIBUTE_TITLE, TextEditHelper::toHtmlEscaped(info->name()));
                    }
                    if (!info->sceneNumber().isEmpty() && info->isSceneNumberFixed()) {
                        writer.writeAttribute(ATTRIBUTE_SCENE_NUMBER, TextEditHelper::toHtmlEscaped(info->sceneNumber()));
                        writer.writeAttribute(ATTRIBUTE_SCENE_NUMBER_FIX_NESTING, QString::number(info->sceneNumberFixNesting()));
                        writer.writeAttribute(ATTRIBUTE_SCENE_NUMBER_SUFFIX, QString::number(info->sceneNumberSuffix()));
                    }
                }

                //
                // Запишем закладку, если установлена для блока
                //
                if (blockUserData != nullptr) {
                    TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(blockUserData);
                    if (blockInfo->hasBookmark()) {
                        writer.writeAttribute(ATTRIBUTE_BOOKMARK, TextEditHelper::toHtmlEscaped(blockInfo->bookmark()));
                        writer.writeAttribute(ATTRIBUTE_BOOKMARK_COLOR, blockInfo->bookmarkColor().name());
                    }
                }


                //
                // Пишем текст текущего элемента
                //
                writer.writeStartElement(kNodeValue);
                writer.writeCDATA(textToSave);
                writer.writeEndElement();


                //
                // Пишем редакторские комментарии, если они есть в блоке
                //
                if (hasReviewMarks(previousBlock) || hasReviewMarks(currentBlock)) {
                    auto writeReviewMark = [&writer] (const QTextLayout::FormatRange& _range) {
                        writer.writeStartElement(kNodeReview);
                        //
                        // Данные редакторского выделения
                        //
                        writer.writeAttribute(ATTRIBUTE_REVIEW_FROM, QString::number(_range.start));
                        writer.writeAttribute(ATTRIBUTE_REVIEW_LENGTH, QString::number(_range.length));
                        if (_range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                            writer.writeAttribute(ATTRIBUTE_REVIEW_COLOR, _range.format.foreground().color().name());
                        }
                        if (_range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                            writer.writeAttribute(ATTRIBUTE_REVIEW_BGCOLOR, _range.format.background().color().name());
                        }
                        writer.writeAttribute(ATTRIBUTE_REVIEW_IS_HIGHLIGHT,
                            _range.format.boolProperty(ScenarioBlockStyle::PropertyIsHighlight) ? "true" : "false");
                        writer.writeAttribute(ATTRIBUTE_REVIEW_DONE,
                            _range.format.boolProperty(ScenarioBlockStyle::PropertyIsDone) ? "true" : "false");
                        //
                        // ... комментарии
                        //
                        const QStringList comments = _range.format.property(ScenarioBlockStyle::PropertyComments).toStringList();
                        const QStringList authors = _range.format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
                        const QStringList dates = _range.format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();
                        for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                            writer.writeEmptyElement(kNodeReviewComment);
                            writer.writeAttribute(ATTRIBUTE_REVIEW_COMMENT, TextEditHelper::toHtmlEscaped(comments.at(commentIndex)));
                            writer.writeAttribute(ATTRIBUTE_REVIEW_AUTHOR, authors.at(commentIndex));
                            writer.writeAttribute(ATTRIBUTE_REVIEW_DATE, dates.at(commentIndex));
                        }
                        //
                        writer.writeEndElement();
                    };

                    //
                    // Пишем начало
                    //
                    writer.writeStartElement(kNodeReviewGroup);
                    //
                    // Пишем тело
                    //
                    // ... но сперва сформируем список форматов, скорректировав его, если был разрыв
                    //
                    auto ranges = [previousBlock, currentBlock] {
                        if (!previousBlock.isValid()) {
                            return currentBlock.textFormats();
                        }

                        auto ranges = previousBlock.textFormats();
                        for (QTextLayout::FormatRange range : currentBlock.textFormats()) {
                            //
                            // ... если в самом начале разрыва находится часть выделения
                            //     предыдущего блока, объединяем их
                            //
                            if (range.start == 0
                                && isReviewFormatEquals(ranges.last().format, range.format)) {
                                ranges.last().length += range.length + 1;
                            }
                            //
                            // ... в противном случае просто сохраняем выделение,
                            //     корректируя стартовую позицию
                            //
                            else {
                                range.start += previousBlock.length();
                                ranges.append(range);
                            }
                        }
                        return ranges;
                    } ();
                    QTextLayout::FormatRange lastFormatRange{0, 0, QTextCharFormat()};
                    for (const QTextLayout::FormatRange& range : ranges) {
                        if (!range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                            continue;
                        }

                        //
                        // Корректируем позиции выделения
                        //
                        int start = range.start;
                        int length = range.length;
                        bool isSelected = false;
                        if (cursor.selectionStart() < (cursor.block().position() + range.start + range.length)
                            || cursor.selectionEnd() > (cursor.block().position() + range.start)) {
                            //
                            // ... редакторская заметка попадает в текущее выделение
                            //
                            isSelected = true;
                            //
                            if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                start = 0;
                            } else {
                                start = range.start - (cursor.selectionStart() - currentBlock.position());
                            }
                            //
                            if (cursor.selectionEnd() < (cursor.block().position() + range.start + range.length)) {
                                if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                    length = cursor.selectionEnd() - cursor.selectionStart();
                                } else {
                                    length -= cursor.block().position() + range.start + range.length - cursor.selectionEnd();
                                }
                            } else {
                                if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                    length = currentBlock.position() + range.start + range.length - cursor.selectionStart();
                                }
                            }
                        }
                        if (!isSelected) {
                            continue;
                        }

                        //
                        // Если следующий формат равен предыдущему и он является продолжает предыдущего
                        //
                        if (isReviewFormatEquals(lastFormatRange.format, range.format)
                            && ((lastFormatRange.start + lastFormatRange.length) == start)) {
                            //
                            // Объединяем их в одну сущность
                            //
                            lastFormatRange.length += length;
                        }
                        //
                        // В противном случае
                        //
                        else {
                            //
                            // Если предыдущий формат был задан, запишем его
                            //
                            if (lastFormatRange.length > 0) {
                                writeReviewMark(lastFormatRange);
                            }
                            //
                            // и перейдём к обработке следующего формата
                            //
                            lastFormatRange.start = start;
                            lastFormatRange.length = length;
                            lastFormatRange.format = range.format;
                        }
                    }
                    //
                    // Запишем последний формат
                    //
                    writeReviewMark(lastFormatRange);
                    //
                    // Пишем конец
                    //
                    writer.writeEndElement();
                }


                //
                // Пишем форматирование текста блока
                //
                if (hasFormatting(previousBlock) || hasFormatting(currentBlock)) {
                    auto writeFormatting = [&writer] (const QTextLayout::FormatRange& _range) {
                        writer.writeStartElement(kNodeFormat);
                        //
                        // Данные пользовательского форматирования
                        //
                        writer.writeAttribute(ATTRIBUTE_FORMAT_FROM, QString::number(_range.start));
                        writer.writeAttribute(ATTRIBUTE_FORMAT_LENGTH, QString::number(_range.length));
                        if (_range.format.hasProperty(QTextFormat::FontWeight)) {
                            writer.writeAttribute(ATTRIBUTE_FORMAT_BOLD, _range.format.font().bold() ? "true" : "false");
                        }
                        if (_range.format.hasProperty(QTextFormat::FontItalic)) {
                            writer.writeAttribute(ATTRIBUTE_FORMAT_ITALIC, _range.format.font().italic() ? "true" : "false");
                        }
                        if (_range.format.hasProperty(QTextFormat::TextUnderlineStyle)) {
                            writer.writeAttribute(ATTRIBUTE_FORMAT_UNDERLINE, _range.format.font().underline() ? "true" : "false");
                        }
                        //
                        writer.writeEndElement();
                    };

                    //
                    // Пишем начало
                    //
                    writer.writeStartElement(kNodeFormatGroup);
                    //
                    // Пишем тело
                    //
                    // ... но сперва сформируем список форматов, скорректировав его, если был разрыв
                    //
                    auto ranges = [previousBlock, currentBlock] {
                        if (!previousBlock.isValid()) {
                            return currentBlock.textFormats();
                        }

                        auto ranges = previousBlock.textFormats();
                        for (QTextLayout::FormatRange range : currentBlock.textFormats()) {
                            //
                            // ... если в самом начале разрыва находится часть выделения
                            //     предыдущего блока, объединяем их
                            //
                            if (range.start == 0
                                && isFormattingEquals(ranges.last().format, range.format)) {
                                ranges.last().length += range.length + 1;
                            }
                            //
                            // ... в противном случае просто сохраняем выделение,
                            //     корректируя стартовую позицию
                            //
                            else {
                                range.start += previousBlock.length();
                                ranges.append(range);
                            }
                        }
                        return ranges;
                    } ();
                    QTextLayout::FormatRange lastFormatRange{0, 0, QTextCharFormat()};
                    for (const QTextLayout::FormatRange& range : ranges) {
                        if (!range.format.boolProperty(ScenarioBlockStyle::PropertyIsFormatting)) {
                            continue;
                        }

                        //
                        // Корректируем позиции выделения
                        //
                        int start = range.start;
                        int length = range.length;
                        bool isSelected = false;
                        if (cursor.selectionStart() < (cursor.block().position() + range.start + range.length)
                            || cursor.selectionEnd() > (cursor.block().position() + range.start)) {
                            //
                            // ... форматирование попадает в текущее выделение
                            //
                            isSelected = true;
                            //
                            if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                start = 0;
                            } else {
                                start = range.start - (cursor.selectionStart() - currentBlock.position());
                            }
                            //
                            if (cursor.selectionEnd() < (cursor.block().position() + range.start + range.length)) {
                                if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                    length = cursor.selectionEnd() - cursor.selectionStart();
                                } else {
                                    length -= cursor.block().position() + range.start + range.length - cursor.selectionEnd();
                                }
                            } else {
                                if (cursor.selectionStart() > (cursor.block().position() + range.start)) {
                                    length = currentBlock.position() + range.start + range.length - cursor.selectionStart();
                                }
                            }
                        }
                        if (!isSelected) {
                            continue;
                        }

                        //
                        // Если следующий формат равен предыдущему и он является продолжает предыдущего
                        //
                        if (isFormattingEquals(lastFormatRange.format, range.format)
                            && ((lastFormatRange.start + lastFormatRange.length) == start)) {
                            //
                            // Объединяем их в одну сущность
                            //
                            lastFormatRange.length += length;
                        }
                        //
                        // В противном случае
                        //
                        else {
                            //
                            // Если предыдущий формат был задан, запишем его
                            //
                            if (lastFormatRange.length > 0) {
                                writeFormatting(lastFormatRange);
                            }
                            //
                            // и перейдём к обработке следующего формата
                            //
                            lastFormatRange.start = start;
                            lastFormatRange.length = length;
                            lastFormatRange.format = range.format;
                        }
                    }
                    //
                    // Запишем последний формат, если необходимо
                    //
                    if (lastFormatRange.format.isValid()) {
                        writeFormatting(lastFormatRange);
                    }
                    //
                    // Пишем конец
                    //
                    writer.writeEndElement();
                }

                //
                // Закрываем текущий элемент
                //
                writer.writeEndElement();
            }
            //
            // Если граница таблицы
            //
            else if (currentType == ScenarioBlockStyle::PageSplitter) {
                writer.writeEmptyElement(isInTable ? kNodeSplitterStart : kNodeSplitterEnd);
            }

            //
            // Снимем выделение
            //
            cursor.clearSelection();
        }

    } while (cursor.position() < _endPosition
             && !cursor.atEnd());

    //
    // Закроем открытые папки
    //
    while (openedFolders > 0) {
        writer.writeStartElement("folder_footer");
        writer.writeCDATA(QObject::tr("END OF FOLDER", "ScenarioXml"));
        writer.writeEndElement();
        --openedFolders;
    }

    //
    // Добавим корневой элемент
    //
    writer.writeEndElement(); // scenario
    writer.writeEndDocument();

    return resultXml;
}

QString ScenarioXml::scenarioToXml(ScenarioModelItem* _fromItem, ScenarioModelItem* _toItem)
{
    //
    // Определить интервал текста из которого нужно создать xml-представление
    //
    // ... начало
    int startPosition = _fromItem->position();
    // ... конец
    int endPosition = _fromItem->endPosition();

    int toItemEndPosition = _toItem->endPosition();
    if (endPosition < toItemEndPosition) {
        endPosition = toItemEndPosition;
    }

    //
    // Сформировать xml-строку
    //
    return scenarioToXml(startPosition, endPosition);
}

void ScenarioXml::xmlToScenario(int _position, const QString& _xml, bool _remainLinkedData)
{
    QXmlStreamReader reader(_xml);
    if (reader.readNextStartElement()
        && reader.name().toString() == kNodeScript) {
        const QString version = reader.attributes().value(ATTRIBUTE_VERSION).toString();
        if (version.isEmpty()) {
            xmlToScenarioV0(_position, _xml);
        } else if (version == "1.0") {
            xmlToScenarioV1(_position, _xml, _remainLinkedData);
        }
    }
}

int ScenarioXml::xmlToScenario(ScenarioModelItem* _insertParent, ScenarioModelItem* _insertBefore, const QString& _xml, bool _removeLastMime)
{
    //
    // Определим позицию для вставки данных
    //
    int insertPosition = m_scenario->positionToInsertMime(_insertParent, _insertBefore);

    //
    // Если документ пуст
    // Если происходит копирование, или вставка из другого источника
    // Если пользователь не пытается вставить данные на своё же место
    //
    if (m_scenario->document()->isEmpty()
        || !_removeLastMime
        || (insertPosition != m_lastMimeFrom
            && insertPosition != m_lastMimeTo)) {
        //
        // Начинаем операцию вставки
        //
        QTextCursor cursor(m_scenario->document());
        cursor.beginEditBlock();

        //
        // Если необходимо удалить прошлое выделение
        //
        bool needCorrectPosition = false;
        if (_removeLastMime) {
            if (m_lastMimeFrom < insertPosition) {
                needCorrectPosition = true;
            }

            int removedSymbols = removeLastMime();
            if (needCorrectPosition) {
                insertPosition -= removedSymbols;
            }
        }

        //
        // Вставим пустой блок для нового элемента
        //
        m_scenario->document()->setCursorPosition(cursor, insertPosition);
        cursor.insertBlock();
        //
        // ... скорректируем позицию курсора
        //
        if (needCorrectPosition || insertPosition != 0) {
            insertPosition = cursor.position();
        }
        //
        // ... перенесём данные оставшиеся в предыдущем блоке, в новое место
        //
        else {
            cursor.movePosition(QTextCursor::PreviousBlock);
            if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*> (cursor.block().userData())) {
                SceneHeadingBlockInfo* movedInfo = info->clone();
                cursor.block().setUserData(nullptr);
                cursor.movePosition(QTextCursor::NextBlock);
                cursor.block().setUserData(movedInfo);
            }
        }

        //
        // Вставка данных
        //
        const bool remainLinkedData = _removeLastMime;
        xmlToScenario(insertPosition, _xml, remainLinkedData);

        //
        // Завершаем операцию
        //
        cursor.endEditBlock();
    }

    return insertPosition;
}

int ScenarioXml::removeLastMime()
{
    int removedSymbols = 0;

    if (m_lastMimeFrom != m_lastMimeTo
        && m_lastMimeFrom < m_lastMimeTo) {
        const int documentCharactersCount = m_scenario->document()->characterCount();

        //
        // Расширим область чтобы не оставалось пустых строк
        //
        if (m_lastMimeTo != (documentCharactersCount - 1)){
            ++m_lastMimeTo;
        } else if (m_lastMimeFrom > 0) {
            --m_lastMimeFrom;
        }

        //
        // Проверяем, чтобы область не выходила за границы документа
        //
        if (m_lastMimeTo >= documentCharactersCount) {
            m_lastMimeTo = documentCharactersCount - 1;
        }

        QTextCursor cursor(m_scenario->document());
        cursor.setPosition(m_lastMimeFrom);
        cursor.setPosition(m_lastMimeTo, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        removedSymbols = m_lastMimeTo - m_lastMimeFrom;
    }

    m_lastMimeFrom = -1;
    m_lastMimeTo = -1;

    return removedSymbols;
}

void ScenarioXml::xmlToScenarioV0(int _position, const QString& _xml)
{
    //
    // Происходит ли обработка первого блока
    //
    bool firstBlockHandling = true;
    //
    // Необходимо ли изменить тип блока, в который вставляется текст
    //
    bool needChangeFirstBlockType = false;

    //
    // Начинаем операцию вставки
    //
    QTextCursor cursor(m_scenario->document());
    cursor.setPosition(_position);
    cursor.beginEditBlock();

    //
    // Если вставка в пустой блок, то изменим его тип
    //
    if (cursor.block().text().simplified().isEmpty()) {
        needChangeFirstBlockType = true;
    }

    //
    // Последний использемый тип блока при обработке загружаемого текста
    //
    ScenarioBlockStyle::Type lastTokenType = ScenarioBlockStyle::Undefined;

    QXmlStreamReader reader(_xml);
    while (!reader.atEnd()) {
        //
        // Даём возможность выполниться графическим операциям
        //
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement: {
                //
                // Определить тип текущего блока
                //
                ScenarioBlockStyle::Type tokenType = ScenarioBlockStyle::Undefined;
                QString tokenName = reader.name().toString();
                tokenType = ScenarioBlockStyle::typeForName(tokenName);

                //
                // Если определён тип блока, то обработать его
                //
                if (tokenType != ScenarioBlockStyle::Undefined) {
                    ScenarioBlockStyle currentStyle = ScenarioTemplateFacade::getTemplate().blockStyle(tokenType);

                    if (!firstBlockHandling) {
                        cursor.insertBlock();
                    }

                    //
                    // Если нужно добавим заголовок стиля
                    //
                    if (currentStyle.hasHeader()) {
                        ScenarioBlockStyle headerStyle = ScenarioTemplateFacade::getTemplate().blockStyle(currentStyle.headerType());
                        cursor.setBlockFormat(headerStyle.blockFormat());
                        cursor.setBlockCharFormat(headerStyle.charFormat());
                        cursor.setCharFormat(headerStyle.charFormat());
                        cursor.insertText(currentStyle.header());
                        cursor.insertBlock();
                    }

                    //
                    // Если необходимо сменить тип блока
                    //
                    if ((firstBlockHandling && needChangeFirstBlockType)
                        || !firstBlockHandling) {

                        //
                        // Установим стиль блока
                        //
                        cursor.setBlockFormat(currentStyle.blockFormat());
                        cursor.setBlockCharFormat(currentStyle.charFormat());
                        cursor.setCharFormat(currentStyle.charFormat());
                    }

                    //
                    // Корректируем информацию о шаге
                    //
                    if (firstBlockHandling) {
                        firstBlockHandling = false;
                    }
                }

                //
                // Если необходимо, загрузить информацию о сцене
                //
                if (tokenType == ScenarioBlockStyle::SceneHeading
                    || tokenType == ScenarioBlockStyle::FolderHeader) {
                    QString synopsis = reader.attributes().value("synopsis").toString();
                    SceneHeadingBlockInfo* info = new SceneHeadingBlockInfo;
                    QTextDocument doc;
                    doc.setHtml(synopsis);
                    info->setDescription(doc.toPlainText());
                    cursor.block().setUserData(info);
                }

                //
                // Обновим последний использовавшийся тип блока
                //
                lastTokenType = tokenType;

                break;
            }

            case QXmlStreamReader::Characters: {
                if (!reader.isWhitespace()) {
                    QString textToInsert = reader.text().toString();

                    //
                    // Если необходимо так же вставляем префикс и постфикс стиля
                    //
                    ScenarioBlockStyle currentStyle = ScenarioTemplateFacade::getTemplate().blockStyle(lastTokenType);
                    if (!currentStyle.prefix().isEmpty()
                        && !textToInsert.startsWith(currentStyle.prefix())) {
                        textToInsert.prepend(currentStyle.prefix());
                    }
                    if (!currentStyle.postfix().isEmpty()
                        && !textToInsert.endsWith(currentStyle.postfix())) {
                        textToInsert.append(currentStyle.postfix());
                    }

                    //
                    // Пишем сам текст
                    //
                    cursor.insertText(textToInsert);
                }
                break;
            }

            default: {
                break;
            }
        }
    }

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();
}

void ScenarioXml::xmlToScenarioV1(int _position, const QString& _xml, bool _remainLinkedData)
{
    //
    // Происходит ли обработка первого блока
    //
    bool firstBlockHandling = true;
    //
    // Необходимо ли изменить тип блока, в который вставляется текст
    //
    bool needChangeFirstBlockType = false;

    //
    // Начинаем операцию вставки
    //
    ScriptTextCursor cursor(m_scenario->document());
    cursor.setPosition(_position);
    cursor.beginEditBlock();

    //
    // Если вставка в пустой блок, то изменим его тип
    //
    if (cursor.block().text().simplified().isEmpty()) {
        needChangeFirstBlockType = true;
    }

    //
    // Собственно считываем данные
    //
    auto xmlToRead = _xml;
#ifdef Q_OS_MAC
        //
        // Это две разные буквы!
        // Первую даёт нам мак, когда открываешь файл через двойной щелчок на нём
        //
        xmlToRead.replace("й", "й");
#endif
    QXmlStreamReader reader(xmlToRead);
    while (!reader.atEnd()) {
        //
        // Даём возможность выполниться графическим операциям
        //
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement: {
                //
                // Определить тип текущего блока
                //
                ScenarioBlockStyle::Type tokenType = ScenarioBlockStyle::Undefined;
                QString tokenName = reader.name().toString();
                tokenType = ScenarioBlockStyle::typeForName(tokenName);

                //
                // Если определён тип блока, то обработать его
                //
                if (tokenType != ScenarioBlockStyle::Undefined) {
                    ScenarioBlockStyle currentStyle = ScenarioTemplateFacade::getTemplate().blockStyle(tokenType);

                    if (firstBlockHandling) {
                        cursor.block().setVisible(true);
                    } else {
                        cursor.insertBlock();
                    }

                    //
                    // Если необходимо сменить тип блока
                    //
                    if ((firstBlockHandling && needChangeFirstBlockType)
                        || !firstBlockHandling) {

                        //
                        // Установим стиль блока
                        //
                        cursor.setBlockFormat(currentStyle.blockFormat());
                        cursor.setBlockCharFormat(currentStyle.charFormat());
                        cursor.setCharFormat(currentStyle.charFormat());
                    }

                    //
                    // Корректируем информацию о шаге
                    //
                    if (firstBlockHandling) {
                        firstBlockHandling = false;
                    }

                    //
                    // Если необходимо, загрузить информацию о сцене
                    //
                    if ((tokenType == ScenarioBlockStyle::SceneHeading
                        || tokenType == ScenarioBlockStyle::FolderHeader)
                        && dynamic_cast<SceneHeadingBlockInfo*>(cursor.block().userData()) == nullptr) {
                        SceneHeadingBlockInfo* info = new SceneHeadingBlockInfo;
                        if (reader.attributes().hasAttribute(ATTRIBUTE_UUID)) {
                            const QString uuid = reader.attributes().value(ATTRIBUTE_UUID).toString();
                            if (_remainLinkedData || !isScenarioHaveUuid(uuid)) {
                                info->setUuid(uuid);
                            }
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_COLOR)) {
                            info->setColors(reader.attributes().value(ATTRIBUTE_COLOR).toString());
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_STAMP)) {
                            info->setStamp(reader.attributes().value(ATTRIBUTE_STAMP).toString());
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_TITLE)) {
                            info->setName(TextEditHelper::fromHtmlEscaped(reader.attributes().value(ATTRIBUTE_TITLE).toString()));
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_SCENE_NUMBER) && _remainLinkedData) {
                            info->setSceneNumber(reader.attributes().value(ATTRIBUTE_SCENE_NUMBER).toString());
                            info->setSceneNumberFixed(true);
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_SCENE_NUMBER_FIX_NESTING)) {
                            info->setSceneNumberFixNesting(reader.attributes().value(ATTRIBUTE_SCENE_NUMBER_FIX_NESTING).toInt());
                        }
                        if (reader.attributes().hasAttribute(ATTRIBUTE_SCENE_NUMBER_SUFFIX)) {
                            info->setSceneNumberSuffix(reader.attributes().value(ATTRIBUTE_SCENE_NUMBER_SUFFIX).toInt());
                        }
                        cursor.block().setUserData(info);
                    }
                    //
                    // Для всех остальных блоков создаём структурку с данными о блоке
                    //
                    else {
                        TextBlockInfo* info = tokenType == ScenarioBlockStyle::Character
                                              ? new CharacterBlockInfo
                                              : new TextBlockInfo;
                        cursor.block().setUserData(info);
                    }

                    //
                    // Загружаем закладки, если установлены
                    //
                    if (reader.attributes().hasAttribute(ATTRIBUTE_BOOKMARK)) {
                        TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(cursor.block().userData());
                        blockInfo->setHasBookmark(true);
                        blockInfo->setBookmark(reader.attributes().value(ATTRIBUTE_BOOKMARK).toString());
                        blockInfo->setBookmarkColor(reader.attributes().value(ATTRIBUTE_BOOKMARK_COLOR).toString());
                        cursor.block().setUserData(blockInfo);
                    }

                    //
                    // Загружаем дифы, если заданы
                    //
                    if (reader.attributes().hasAttribute(ATTRIBUTE_DIFF_ADDED)
                        || reader.attributes().hasAttribute(ATTRIBUTE_DIFF_REMOVED)) {
                        TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(cursor.block().userData());

                        if (reader.attributes().hasAttribute(ATTRIBUTE_DIFF_ADDED)) {
                            blockInfo->setDiffType(TextBlockInfo::kDiffAdded);
                        } else if (reader.attributes().hasAttribute(ATTRIBUTE_DIFF_REMOVED)) {
                            blockInfo->setDiffType(TextBlockInfo::kDiffRemoved);
                        }
                        cursor.block().setUserData(blockInfo);
                    }

                    //
                    // Скрываем блоки, которых не должно быть видно в текщем режиме сценария
                    //
                    if (!m_scenario->document()->visibleBlocksTypes().contains(tokenType)) {
                        cursor.block().setVisible(false);
                    }

                    ScenarioTextDocument::updateBlockRevision(cursor);
                }
                //
                // Обработка остальных тэгов
                //
                else {
                    //
                    // Редакторские заметки
                    //
                    if (tokenName == kNodeReview) {
                        const int start = reader.attributes().value(ATTRIBUTE_REVIEW_FROM).toInt();
                        const int length = reader.attributes().value(ATTRIBUTE_REVIEW_LENGTH).toInt();
                        const bool highlight = reader.attributes().value(ATTRIBUTE_REVIEW_IS_HIGHLIGHT).toString() == "true";
                        const bool done = reader.attributes().value(ATTRIBUTE_REVIEW_DONE).toString() == "true";
                        const QColor foreground(reader.attributes().value(ATTRIBUTE_REVIEW_COLOR).toString());
                        const QColor background(reader.attributes().value(ATTRIBUTE_REVIEW_BGCOLOR).toString());
                        //
                        // ... считываем комментарии
                        //
                        QStringList comments, authors, dates;
                        while (reader.readNextStartElement()) {
                            if (reader.name() == kNodeReviewComment) {
                                comments << TextEditHelper::fromHtmlEscaped(reader.attributes().value(ATTRIBUTE_REVIEW_COMMENT).toString());
                                authors << reader.attributes().value(ATTRIBUTE_REVIEW_AUTHOR).toString();
                                dates << reader.attributes().value(ATTRIBUTE_REVIEW_DATE).toString();

                                reader.skipCurrentElement();
                            }
                        }


                        //
                        // Собираем формат редакторской заметки
                        //
                        QTextCharFormat reviewFormat;
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyIsReviewMark, true);
                        if (foreground.isValid()) {
                            reviewFormat.setForeground(foreground);
                        }
                        if (background.isValid()) {
                            reviewFormat.setBackground(background);
                        }
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyIsHighlight, highlight);
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyIsDone, done);
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyComments, comments);
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, authors);
                        reviewFormat.setProperty(ScenarioBlockStyle::PropertyCommentsDates, dates);


                        //
                        // Вставляем в документ
                        //
                        QTextCursor reviewCursor = cursor;
                        int startDelta = 0;
                        if (reviewCursor.block().position() < _position
                            && (reviewCursor.block().position() + reviewCursor.block().length()) > _position) {
                            startDelta = _position - reviewCursor.block().position();
                        }
                        reviewCursor.setPosition(reviewCursor.block().position() + start + startDelta);
                        reviewCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length);
                        reviewCursor.mergeCharFormat(reviewFormat);
                    }

                    //
                    // Форматирование
                    //
                    if (tokenName == kNodeFormat) {
                        const int start = reader.attributes().value(ATTRIBUTE_FORMAT_FROM).toInt();
                        const int length = reader.attributes().value(ATTRIBUTE_FORMAT_LENGTH).toInt();
                        const bool bold = reader.attributes().value(ATTRIBUTE_FORMAT_BOLD).toString() == "true";
                        const bool italic = reader.attributes().value(ATTRIBUTE_FORMAT_ITALIC).toString() == "true";
                        const bool underline = reader.attributes().value(ATTRIBUTE_FORMAT_UNDERLINE).toString() == "true";


                        //
                        // Собираем формат
                        //
                        QTextCharFormat format;
                        format.setProperty(ScenarioBlockStyle::PropertyIsFormatting, true);
                        format.setFontWeight(bold ? QFont::Bold : QFont::Normal);
                        format.setFontItalic(italic);
                        format.setFontUnderline(underline);


                        //
                        // Вставляем в документ
                        //
                        QTextCursor formattingCursor = cursor;
                        int startDelta = 0;
                        if (formattingCursor.block().position() < _position
                            && (formattingCursor.block().position() + formattingCursor.block().length()) > _position) {
                            startDelta = _position - formattingCursor.block().position();
                        }
                        formattingCursor.setPosition(formattingCursor.block().position() + start + startDelta);
                        formattingCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length);
                        formattingCursor.mergeCharFormat(format);
                    }

                    ScenarioTextDocument::updateBlockRevision(cursor);
                }

                break;
            }

            case QXmlStreamReader::Characters: {
                if (!reader.isWhitespace()) {
                    QString textToInsert = TextEditHelper::fromHtmlEscaped(reader.text().toString());

                    //
                    // Пишем сам текст
                    //
                    cursor.insertText(textToInsert);

                    ScenarioTextDocument::updateBlockRevision(cursor);
                }
                break;
            }

            default: {
                break;
            }
        }
    }

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();
}

bool ScenarioXml::isScenarioHaveUuid(const QString& _uuid) const
{
    auto currentBlock = m_scenario->document()->begin();
    while (currentBlock.isValid()) {
        const ScenarioBlockStyle::Type currentBlockType = ScenarioBlockStyle::forBlock(currentBlock);
        if (currentBlockType == ScenarioBlockStyle::SceneHeading
            || currentBlockType == ScenarioBlockStyle::FolderHeader) {
            if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(currentBlock.userData())) {
                if (info != nullptr
                    && info->uuid() == _uuid) {
                    return true;
                }
            }
        }
        currentBlock = currentBlock.next();
    }
    return false;
}
