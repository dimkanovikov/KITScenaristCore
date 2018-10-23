#include "ScriptTextCorrector.h"

#include "ScenarioTemplate.h"
#include "ScenarioTextBlockInfo.h"
#include "ScenarioTextBlockParsers.h"
#include "ScriptTextCursor.h"

#include <3rd_party/Helpers/RunOnce.h>
#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QTextBlock>
#include <QTextDocument>

using BusinessLogic::CharacterBlockInfo;
using BusinessLogic::CharacterParser;
using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScenarioTemplateFacade;
using BusinessLogic::SceneHeadingBlockInfo;
using BusinessLogic::ScriptTextCorrector;
using BusinessLogic::ScriptTextCursor;

namespace {
    /**
     * @brief Список символов пунктуации, разделяющие предложения
     */
    const QList<QChar> PUNCTUATION_CHARACTERS = { '.', '!', '?', ':', ';', QString("…").at(0)};

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    //: Continued
    static const char* kContinuedTerm = QT_TRANSLATE_NOOP("BusinessLogic::ScriptTextCorrector", "CONT'D");

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static const char* MORE = QT_TRANSLATE_NOOP("BusinessLogic::ScriptTextCorrector", "MORE");
    static const QString moreTerm() {
        return QString("(%1)").arg(QApplication::translate("BusinessLogic::ScriptTextCorrector", MORE));
    }

    /**
     * @brief Обновить компановку текста для блока
     */
    void updateBlockLayout(int _pageWidth, const QTextBlock& _block) {
        _block.layout()->setText(_block.text());
        _block.layout()->beginLayout();
        forever {
            QTextLine line = _block.layout()->createLine();
            if (!line.isValid()) {
                break;
            }

            line.setLineWidth(_pageWidth
                              - _block.blockFormat().leftMargin()
                              - _block.blockFormat().rightMargin());
        }
        _block.layout()->endLayout();
    }

    /**
     * @brief Вставить блок, обновив при этом лэйаут вставленного блока
     */
    void insertBlock(int _pageWidth, QTextCursor& _cursor) {
        _cursor.insertBlock();
        updateBlockLayout(_pageWidth, _cursor.block());
    }
}


QString ScriptTextCorrector::continuedTerm()
{
    return QString(" (%1)").arg(QApplication::translate("BusinessLogic::ScriptTextCorrector", kContinuedTerm));
}

ScriptTextCorrector::ScriptTextCorrector(QTextDocument* _document, const QString& _templateName) :
    QObject(_document),
    m_document(_document),
    m_templateName(_templateName)
{
    Q_ASSERT_X(m_document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

void ScriptTextCorrector::setNeedToCorrectCharactersNames(bool _need)
{
    if (m_needToCorrectCharactersNames != _need) {
        m_needToCorrectCharactersNames = _need;
        correctCharactersNames();
    }
}

void ScriptTextCorrector::setNeedToCorrectPageBreaks(bool _need)
{
    if (m_needToCorrectPageBreaks != _need) {
        m_needToCorrectPageBreaks = _need;
        clear();
        correctPageBreaks();
    }
}

void ScriptTextCorrector::clear()
{
    m_lastDocumentSize = QSizeF();
    m_currentBlockNumber = 0;
    m_blockItems.clear();
}

void ScriptTextCorrector::correct(int _position, int _charRemoved, int _charAdded)
{
    //
    // Первой обязательно выполняется корректировка текста имён персонажей,
    // т.к. она может привести к изменению кол-ва строк имени персонажа
    //

    if (m_needToCorrectCharactersNames) {
        correctCharactersNames(_position, _charRemoved, _charAdded);
    }

    if (m_needToCorrectPageBreaks) {
        correctPageBreaks(_position);
    }
}

int ScriptTextCorrector::correctedPosition(int _position) const
{
    int positionDelta = 0;
    for (auto iter = m_decorations.begin(); iter != m_decorations.end(); ++iter) {
        //
        // Прерываем выполнение, если текущая декорация находится за искомой позицией
        //
        if (iter.key() > _position + positionDelta) {
            break;
        }

        positionDelta += iter.value();
    }
    return _position + positionDelta;
}

void ScriptTextCorrector::correctCharactersNames(int _position, int _charsRemoved, int _charsAdded)
{
    //
    // Избегаем рекурсии
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Определим границы работы алгоритма
    //
    int startPosition = _position;
    int endPosition = _position + (std::max(_charsRemoved, _charsAdded));
    if (startPosition == -1) {
        startPosition = 0;
        endPosition = m_document->characterCount();
    }

    //
    // Начинаем работу с документом
    //
    ScriptTextCursor cursor(m_document);
    cursor.beginEditBlock();

    //
    // Расширим выделение
    //
    // ... от начала сцены
    //
    QVector<ScenarioBlockStyle::Type> sceneBorders = { ScenarioBlockStyle::SceneHeading,
                                                       ScenarioBlockStyle::FolderHeader,
                                                       ScenarioBlockStyle::FolderFooter };
    QTextBlock block = m_document->findBlock(startPosition);
    while (block != m_document->begin()) {
        const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
        if (sceneBorders.contains(blockType)) {
            break;
        }

        block = block.previous();
    }
    //
    // ... и до конца
    //
    {
        QTextBlock endBlock = m_document->findBlock(endPosition);
        while (endBlock.isValid()
               && endBlock != m_document->end()) {
            const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(endBlock);
            if (sceneBorders.contains(blockType)) {
                break;
            }

            endBlock = endBlock.next();
        }
        endPosition = endBlock.previous().position();
    }

    //
    // Корректируем имена пресонажей в изменённой части документа
    //
    QString lastCharacterName;
    do {
        const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
        //
        // Если дошли до новой сцены, очищаем последнее найдённое имя персонажа
        //
        if (sceneBorders.contains(blockType)) {
            lastCharacterName.clear();
        }
        //
        // Корректируем имя персонажа при необходимости
        //
        else if (blockType == ScenarioBlockStyle::Character) {
            const QString characterName = CharacterParser::name(block.text());
            const bool isStartPositionInBlock =
                    block.position() < startPosition
                    && block.position() + block.length() > startPosition;
            //
            // Если имя текущего персонажа не пусто и курсор не находится в редактируемом блоке
            //
            if (!characterName.isEmpty() && !isStartPositionInBlock) {
                //
                // Не второе подряд появление, удаляем из него вспомогательный текст, если есть
                //
                if (lastCharacterName.isEmpty()
                    || characterName != lastCharacterName) {
                    QTextBlockFormat characterFormat = block.blockFormat();
                    if (characterFormat.boolProperty(ScenarioBlockStyle::PropertyIsCharacterContinued)) {
                        characterFormat.setProperty(ScenarioBlockStyle::PropertyIsCharacterContinued, false);
                        cursor.setPosition(block.position());
                        cursor.setBlockFormat(characterFormat);
                    }
                }
                //
                // Если второе подряд, добавляем вспомогательный текст
                //
                else if (characterName == lastCharacterName){
                    const QString characterState = CharacterParser::state(block.text());
                    //
                    // ... помечаем блок, что нужно отрисовывать вспомогательный текст
                    //
                    QTextBlockFormat characterFormat = block.blockFormat();
                    characterFormat.setProperty(ScenarioBlockStyle::PropertyIsCharacterContinued,
                                                m_needToCorrectCharactersNames && characterState.isEmpty());
                    cursor.setPosition(block.position());
                    cursor.setBlockFormat(characterFormat);
                }

                lastCharacterName = characterName;
            }
        }

        block = block.next();
    } while (block.isValid()
             && block.position() < endPosition);

    cursor.endEditBlock();
}

void ScriptTextCorrector::correctPageBreaks(int _position)
{
    //
    // Избегаем рекурсии
    //
    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

    //
    // Определим высоту страницы
    //
    const QTextFrameFormat rootFrameFormat = m_document->rootFrame()->frameFormat();
    //
    const qreal pageWidth = m_document->pageSize().width()
                             - rootFrameFormat.leftMargin()
                             - rootFrameFormat.rightMargin();
    if (pageWidth < 0)
        return;
    //
    const qreal pageHeight = m_document->pageSize().height()
                             - rootFrameFormat.topMargin()
                             - rootFrameFormat.bottomMargin();
    if (pageHeight < 0)
        return;
    //
    // Если размер изменился, необходимо пересчитать все блоки
    //
    if (!qFuzzyCompare(m_lastDocumentSize.width(), pageWidth)
        || !qFuzzyCompare(m_lastDocumentSize.height(), pageHeight)) {
        m_blockItems.clear();
        m_lastDocumentSize = QSizeF(pageWidth, pageHeight);
    }

    //
    // При необходимости скорректируем размер модели параметров блоков для дальнейшего использования
    //
    {
        const int blocksCount = m_document->blockCount();
        if (m_blockItems.size() < blocksCount) {
            m_blockItems.resize(blocksCount * 2);
        }
    }

    //
    // Карту декораций перестраиваем целиком
    //
    m_decorations.clear();

    //
    // Определим список блоков для принудительной ручной проверки
    // NOTE: Это необходимо для того, чтобы корректно обрабатывать изменение текста
    //       в предыдущих и следующих за переносом блоках
    //
    QSet<int> blocksToRecheck;
    if (_position != -1) {
        QTextBlock blockToRecheck = m_document->findBlock(_position);
        //
        // ... спускаемся на два блока вперёд
        //
        blockToRecheck = blockToRecheck.next();
        blockToRecheck = blockToRecheck.next();
        //
        // ... и возвращаемся на пять блоков назад
        // NOTE: максимальное количество блоков которое может быть перенесено на новую страницу
        //
        int recheckBlocksCount = 5;
        do {
            blocksToRecheck.insert(blockToRecheck.blockNumber());
            blockToRecheck = blockToRecheck.previous();
        } while (blockToRecheck.isValid()
                 && (recheckBlocksCount-- > 0
                     || blockToRecheck.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                     || blockToRecheck.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)
                     || blockToRecheck.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)));
    }

    //
    // Начинаем работу с документом
    //
    ScriptTextCursor cursor(m_document);
    cursor.beginEditBlock();

    //
    // Идём по каждому блоку документа с самого начала
    //
    QTextBlock block = m_document->begin();
    //
    // ... значение нижней позиции последнего блока относительно начала страницы
    //
    qreal lastBlockHeight = 0.0;
    m_currentBlockNumber = 0;
    while (block.isValid()) {
        if (block.text().contains("Камера спускается")) {
            int i = 0;
            ++i;
        }

        //
        // Пропускаем невидимые блоки
        //
        if (!block.isVisible()) {
            block = block.next();
            ++m_currentBlockNumber;
            continue;
        }


        //
        // Определить высоту текущего блока
        //
        const QTextBlockFormat blockFormat = block.blockFormat();
        const qreal blockLineHeight = blockFormat.lineHeight();
        const int blockLineCount = block.layout()->lineCount();
        //
        // ... если блок первый на странице, то для него не нужно учитывать верхний отступ
        //
        const qreal blockHeight =
                qFuzzyCompare(lastBlockHeight, 0.0)
                ? blockLineHeight * blockLineCount + blockFormat.bottomMargin()
                : blockLineHeight * blockLineCount + blockFormat.topMargin() + blockFormat.bottomMargin();
        //
        // ... и высоту одной строки следующего
        //
        qreal nextBlockHeight = 0;
        if (block.next().isValid()) {
            const qreal nextBlockLineHeight = block.next().blockFormat().lineHeight();
            const QTextBlockFormat nextBlockFormat = block.next().blockFormat();
            nextBlockHeight = nextBlockLineHeight + nextBlockFormat.topMargin();
        }


        //
        // Определим, заканчивается ли блок на последней строке страницы
        //
        const bool atPageEnd =
                // сам блок влезает
                (lastBlockHeight + blockHeight <= pageHeight)
                // и
                && (
                    // добавление одной строки перекинет его уже на новую страницу
                    lastBlockHeight + blockHeight + blockLineHeight > pageHeight
                    // или 1 строка следующего блока уже не влезет на эту страницу
                    || lastBlockHeight + blockHeight + nextBlockHeight > pageHeight);
        //
        // ... или, может попадает на разрыв
        //
        const bool atPageBreak =
                // сам блок не влезает
                (lastBlockHeight + blockHeight > pageHeight)
                // но влезает хотя бы одна строка
                && (lastBlockHeight + blockFormat.topMargin() + blockLineHeight < pageHeight);

        //
        // Проверяем, изменилась ли позиция блока,
        // и что текущий блок это не изменённый блок под курсором
        //
        if (m_blockItems[m_currentBlockNumber].isValid()
            && qFuzzyCompare(m_blockItems[m_currentBlockNumber].height, blockHeight)
            && qFuzzyCompare(m_blockItems[m_currentBlockNumber].top, lastBlockHeight)
            && !blocksToRecheck.contains(m_currentBlockNumber)) {
            //
            // Если не изменилась
            //
            // ... в данном случае блок не может быть на разрыве
            //
            Q_ASSERT_X(atPageBreak == false, Q_FUNC_INFO, "Normally cached blocks can't be placed on page breaks");
            //
            // ... то корректируем позицию
            //
            if (atPageEnd) {
                lastBlockHeight = 0;
            } else if (atPageBreak) {
                lastBlockHeight += blockHeight - pageHeight;
            } else {
                lastBlockHeight += blockHeight;
            }
            //
            // ... запомним декорацию при необходимости
            //
            if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                m_decorations[block.position()] = block.length();
            }
            //
            // ... и переходим к следующему блоку
            //
            block = block.next();
            ++m_currentBlockNumber;
            continue;
        }


        //
        // Если позиция блока изменилась, то работаем по алгоритму корректировки текста
        //


        //
        // Работаем с декорациями
        //
        // ... если блок декорация, то удаляем его
        //
        if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
            cursor.setPosition(block.position());
            cursor.movePosition(ScriptTextCursor::EndOfBlock, ScriptTextCursor::KeepAnchor);
            cursor.movePosition(ScriptTextCursor::NextCharacter, ScriptTextCursor::KeepAnchor);
            cursor.deleteChar();
            //
            // ... и продолжим со следующего блока
            //
            block = cursor.block();
            continue;
        }
        //
        // ... если в текущем блоке начинается разрыв, пробуем его вернуть
        //
        else if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
            cursor.setPosition(block.position());
            cursor.movePosition(ScriptTextCursor::EndOfBlock);
            do {
                cursor.movePosition(ScriptTextCursor::NextBlock, ScriptTextCursor::KeepAnchor);
            } while (cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection));
            //
            // ... если дошли до конца разрыва, то сшиваем его
            //
            if (cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)) {
                cursor.insertText(" ");
            }
            //
            // ... а если после начала разрыва идёт другой блок, то просто убираем декорации
            //
            else {
                cursor.movePosition(ScriptTextCursor::PreviousCharacter, ScriptTextCursor::KeepAnchor);
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
            }
            //
            // ... очищаем значения обрывов
            //
            QTextBlockFormat cleanFormat = blockFormat;
            cleanFormat.setProperty(PageTextEdit::PropertyDontShowCursor, QVariant());
            cleanFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, QVariant());
            cleanFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, QVariant());
            cursor.setBlockFormat(cleanFormat);
            //
            // ... и проработаем текущий блок с начала
            //
            block = cursor.block();
            updateBlockLayout(static_cast<int>(pageWidth), block);
            continue;
        }


        //
        // Работаем с переносами
        //
        if (m_needToCorrectPageBreaks
            && (atPageEnd || atPageBreak)) {
            switch (ScenarioBlockStyle::forBlock(block)) {
                //
                // Если это время и место
                //
                case ScenarioBlockStyle::SceneHeading: {
                    //
                    // Переносим на следующую страницу
                    //
                    moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);

                    break;
                }

                //
                // Если это участники сцены
                //
                case ScenarioBlockStyle::SceneCharacters: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт время и место, переносим его тоже
                    //
                    if (previousBlock.isValid()
                        && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }

                    break;
                }

                //
                // Если это имя персонажа
                //
                case ScenarioBlockStyle::Character: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт время и место, переносим его тоже
                    //
                    if (previousBlock.isValid()
                        && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }
                    //
                    // Если перед ним идут участники сцены, то проверим ещё на один блок назад
                    //
                    else if (previousBlock.isValid()
                             && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneCharacters) {
                        //
                        // Проверяем предыдущий блок
                        //
                        QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                        //
                        // Если перед участниками идёт время и место, переносим его тоже
                        //
                        if (prePreviousBlock.isValid()
                            && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }

                    break;
                }


                //
                // Если это ремарка
                //
                case ScenarioBlockStyle::Parenthetical: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = findPreviousBlock(block);
                    //
                    // Если перед ним идёт реплика или лирика, то вставляем блок ДАЛЬШЕ
                    // и переносим на следующую страницу вставляя ПЕРСОНАЖ (ПРОД.) перед ремаркой
                    //
                    if (previousBlock.isValid()
                        && (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Dialogue
                            || ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Lyrics)) {
                        breakDialogue(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }
                    //
                    // В противном случае, если перед ремаркой идёт персонаж
                    //
                    else if (previousBlock.isValid()
                             && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Character) {
                        //
                        // Проверяем предыдущий блок
                        //
                        QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                        //
                        // Если перед персонажем идёт время и место, переносим его тоже
                        //
                        if (prePreviousBlock.isValid()
                            && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае, если перед персонажем идут участники сцены
                        //
                        else if (prePreviousBlock.isValid()
                                 && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneCharacters) {
                            //
                            // Проверяем предыдущий блок
                            //
                            QTextBlock prePrePreviousBlock = findPreviousBlock(prePreviousBlock);
                            //
                            // Если перед участниками идёт время и место, переносим его тоже
                            //
                            if (prePreviousBlock.isValid()
                                && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                            //
                            // В противном случае просто переносим вместе с участниками
                            //
                            else {
                                moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                    }

                    break;
                }


                //
                // Если это реплика или лирика и попадает на разрыв
                // - если можно, то оставляем текст так, чтобы он занимал не менее 2 строк,
                //	 добавляем ДАЛЬШЕ и на следующей странице имя персонажа с (ПРОД) и остальной текст
                // - в противном случае
                // -- если перед диалогом идёт имя персонажа, то переносим их вместе на след.
                // -- если перед диалогом идёт ремарка
                // --- если перед ремаркой идёт имя персонажа, то переносим их всех вместе
                // --- если перед ремаркой идёт диалог, то разрываем по ремарке, пишем вместо неё
                //	   ДАЛЬШЕ, а на следующей странице имя персонажа с (ПРОД), ремарку и сам диалог
                //
                case ScenarioBlockStyle::Dialogue:
                case ScenarioBlockStyle::Lyrics: {
                    //
                    // Проверяем следующий блок
                    //
                    QTextBlock nextBlock = findNextBlock(block);
                    //
                    // Если реплика в конце страницы и после ней идёт не ремарка, то оставляем как есть
                    //
                    if (atPageEnd
                        && (!nextBlock.isValid()
                            || (nextBlock.isValid()
                                && ScenarioBlockStyle::forBlock(nextBlock) != ScenarioBlockStyle::Parenthetical))) {
                        //
                        // Запоминаем параметры текущего блока
                        //
                        m_blockItems[m_currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
                        //
                        // и идём дальше
                        //
                        lastBlockHeight = 0;
                    }
                    //
                    // В противном случае разрываем
                    //
                    else {
                        //
                        // Если влезает 2 или более строк
                        //
                        const int minPlacedLines = 2;
                        const int linesCanBePlaced = (pageHeight - lastBlockHeight - blockFormat.topMargin()) / blockLineHeight;
                        int lineToBreak = linesCanBePlaced - 1; // Резервируем одну строку, под блок (ДАЛЬШЕ)
                        //
                        // ... пробуем разорвать на максимально низкой строке
                        //
                        bool isBreaked = false;
                        while (lineToBreak >= minPlacedLines
                               && !isBreaked) {
                            const QTextLine line = block.layout()->lineAt(lineToBreak - 1); // -1 т.к. нужен индекс, а не порядковый номер
                            const QString lineText = block.text().mid(line.textStart(), line.textLength());
                            for (const auto& punctuation : PUNCTUATION_CHARACTERS) {
                                const int punctuationIndex = lineText.lastIndexOf(punctuation);
                                //
                                // ... нашлось место, где можно разорвать
                                //
                                if (punctuationIndex != -1) {
                                    //
                                    // ... разрываем
                                    //
                                    // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                    cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                    insertBlock(pageWidth, cursor);
                                    //
                                    // ... запоминаем параметры блока оставшегося в конце страницы
                                    //
                                    const qreal breakStartBlockHeight =
                                            lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                    m_blockItems[m_currentBlockNumber++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
                                    lastBlockHeight += breakStartBlockHeight;
                                    //
                                    // ... если после разрыва остался пробел, уберём его
                                    //
                                    if (cursor.block().text().startsWith(" ")) {
                                        cursor.deleteChar();
                                    }
                                    //
                                    // ... помечаем блоки, как разорванные
                                    //
                                    QTextBlockFormat breakStartFormat = blockFormat;
                                    breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                    cursor.movePosition(ScriptTextCursor::PreviousBlock);
                                    cursor.setBlockFormat(breakStartFormat);
                                    //
                                    QTextBlockFormat breakEndFormat = blockFormat;
                                    breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                    cursor.movePosition(ScriptTextCursor::NextBlock);
                                    cursor.setBlockFormat(breakEndFormat);
                                    //
                                    // ... обновим лэйаут оторванного блока
                                    //
                                    block = cursor.block();
                                    updateBlockLayout(pageWidth, block);
                                    const qreal breakEndBlockHeight =
                                            block.layout()->lineCount() * blockLineHeight + blockFormat.topMargin()
                                            + blockFormat.bottomMargin();
                                    //
                                    // ... и декорируем разрыв диалога по правилам
                                    //
                                    breakDialogue(blockFormat, breakEndBlockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    //
                                    // ... сохраняем оторванный конец блока и корректируем последнюю высоту
                                    //
                                    block = block.next();
                                    m_blockItems[m_currentBlockNumber++] = BlockInfo{breakEndBlockHeight, lastBlockHeight};
                                    lastBlockHeight += breakEndBlockHeight;
                                    //
                                    // ... помечаем, что разорвать удалось
                                    //
                                    isBreaked = true;
                                    break;
                                }
                            }

                            //
                            // На текущей строке разорвать не удалось, перейдём к предыдущей
                            //
                            --lineToBreak;
                        }

                        //
                        // Разорвать не удалось, переносим целиком
                        //
                        if (!isBreaked) {
                            //
                            // Определим предыдущий блок
                            //
                            QTextBlock previousBlock = findPreviousBlock(block);
                            //
                            // Если перед ним идёт персонаж, переносим его тоже
                            //
                            if (previousBlock.isValid()
                                && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Character) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед именем идёт время и место, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                                //
                                // Если перед именем идут участники сцены
                                //
                                else if (prePreviousBlock.isValid()
                                         && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneCharacters) {
                                    //
                                    // Проверяем предыдущий блок
                                    //
                                    QTextBlock prePrePreviousBlock = findPreviousBlock(prePreviousBlock);
                                    //
                                    // Если перед участниками идёт время и место, переносим его тоже
                                    //
                                    if (prePreviousBlock.isValid()
                                        && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                        moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    }
                                    //
                                    // В противном случае просто переносим вместе с участниками
                                    //
                                    else {
                                        moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                    }
                                }
                                //
                                // В противном случае просто переносим вместе с персонажем
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // Если перед ним идёт ремарка, то проверим ещё на один блок назад
                            //
                            else if (previousBlock.isValid()
                                     && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Parenthetical) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед ремаркой идёт персонаж, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::Character) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае разрываем на ремарке
                                //
                                else {
                                    const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                                    const qreal previousBlockHeight =
                                            previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                                    lastBlockHeight -= previousBlockHeight;
                                    --m_currentBlockNumber;
                                    breakDialogue(previousBlockFormat, previousBlockHeight, pageHeight, pageWidth, cursor, previousBlock, lastBlockHeight);

                                    block = previousBlock;
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                        }
                    }

                    break;
                }


                //
                // Если это описание действия или любой другой блок, для которого нет собственных правил
                //
                default: {
                    //
                    // Если в конце страницы, оставляем как есть
                    //
                    if (atPageEnd) {
                        //
                        // Запоминаем параметры текущего блока
                        //
                        m_blockItems[m_currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
                        //
                        // и идём дальше
                        //
                        lastBlockHeight = 0;
                    }
                    //
                    // Если на разрыве между страниц
                    //
                    else {
                        //
                        // Если влезает 2 или более строк
                        //
                        const int minPlacedLines = 2;
                        const int linesCanBePlaced = (pageHeight - lastBlockHeight - blockFormat.topMargin()) / blockLineHeight;
                        int lineToBreak = linesCanBePlaced;
                        //
                        // ... пробуем разорвать на максимально низкой строке
                        //
                        bool isBreaked = false;
                        while (lineToBreak >= minPlacedLines
                               && !isBreaked) {
                            const QTextLine line = block.layout()->lineAt(lineToBreak - 1); // -1 т.к. нужен индекс, а не порядковый номер
                            const QString lineText = block.text().mid(line.textStart(), line.textLength());
                            for (const auto& punctuation : PUNCTUATION_CHARACTERS) {
                                const int punctuationIndex = lineText.lastIndexOf(punctuation);
                                //
                                // ... нашлось место, где можно разорвать
                                //
                                if (punctuationIndex != -1) {
                                    //
                                    // ... разрываем
                                    //
                                    // +1, т.к. символ пунктуации нужно оставить в текущем блоке
                                    cursor.setPosition(block.position() + line.textStart() + punctuationIndex + 1);
                                    insertBlock(pageWidth, cursor);
                                    //
                                    // ... запоминаем параметры блока оставшегося в конце страницы
                                    //
                                    const qreal breakStartBlockHeight =
                                            lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                    m_blockItems[m_currentBlockNumber++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
                                    //
                                    // ... если после разрыва остался пробел, уберём его
                                    //
                                    if (cursor.block().text().startsWith(" ")) {
                                        cursor.deleteChar();
                                    }
                                    //
                                    // ... помечаем блоки, как разорванные
                                    //
                                    QTextBlockFormat breakStartFormat = blockFormat;
                                    breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                    cursor.movePosition(ScriptTextCursor::PreviousBlock);
                                    cursor.setBlockFormat(breakStartFormat);
                                    //
                                    QTextBlockFormat breakEndFormat = blockFormat;
                                    breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                    cursor.movePosition(ScriptTextCursor::NextBlock);
                                    cursor.setBlockFormat(breakEndFormat);
                                    //
                                    // ... переносим оторванный конец на следующую страницу,
                                    //     если на текущую влезает ещё хотя бы одна строка текста
                                    //
                                    block = cursor.block();
                                    const qreal sizeToPageEnd = pageHeight - lastBlockHeight - breakStartBlockHeight;
                                    if (sizeToPageEnd >= blockFormat.topMargin() + blockLineHeight) {
                                        moveBlockToNextPage(block, sizeToPageEnd, pageHeight, pageWidth, cursor);
                                        block = cursor.block();
                                    }
                                    updateBlockLayout(pageWidth, block);
                                    const qreal breakEndBlockHeight =
                                            block.layout()->lineCount() * blockLineHeight + blockFormat.bottomMargin();
                                    //
                                    // ... запоминаем параметры блока перенесённого на следующую страницу
                                    //
                                    m_blockItems[m_currentBlockNumber++] =
                                            BlockInfo{breakEndBlockHeight, 0};
                                    //
                                    // ... обозначаем последнюю высоту
                                    //
                                    lastBlockHeight = breakEndBlockHeight;
                                    //
                                    // ... помечаем, что разорвать удалось
                                    //
                                    isBreaked = true;
                                    break;
                                }
                            }

                            //
                            // На текущей строке разорвать не удалось, перейдём к предыдущей
                            //
                            --lineToBreak;
                        }

                        //
                        // Разорвать не удалось, переносим целиком
                        //
                        if (!isBreaked) {
                            //
                            // Определим предыдущий блок
                            //
                            QTextBlock previousBlock = findPreviousBlock(block);
                            //
                            // Если перед ним идёт время и место, переносим его тоже
                            //
                            if (previousBlock.isValid()
                                && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                                moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                            //
                            // Если перед ним идут участники сцены, то проверим ещё на один блок назад
                            //
                            else if (previousBlock.isValid()
                                     && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneCharacters) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = findPreviousBlock(previousBlock);
                                //
                                // Если перед участниками идёт время и место, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае просто переносим вместе с участниками
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, pageWidth, cursor, block, lastBlockHeight);
                            }
                        }
                    }

                    break;
                }
            }
        }
        //
        // Если блок находится посередине страницы, просто переходим к следующему
        //
        else {
            //
            // Запоминаем параметры текущего блока
            //
            m_blockItems[m_currentBlockNumber++] = BlockInfo{blockHeight, lastBlockHeight};
            //
            // и идём дальше
            //
            lastBlockHeight += blockHeight;
        }

        block = block.next();
    }

    cursor.endEditBlock();
}

void ScriptTextCorrector::moveCurrentBlockWithThreePreviousToNextPage(const QTextBlock& _prePrePreviousBlock,
    const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight,
    qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --m_currentBlockNumber;
    --m_currentBlockNumber;
    --m_currentBlockNumber;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight =
            prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
            + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const QTextBlockFormat prePrePreviousBlockFormat = _prePrePreviousBlock.blockFormat();
    const qreal prePrePreviousBlockHeight =
            prePrePreviousBlockFormat.lineHeight() * _prePrePreviousBlock.layout()->lineCount()
            + prePrePreviousBlockFormat.topMargin() + prePrePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight + prePreviousBlockHeight + prePrePreviousBlockHeight;
    moveBlockToNextPage(_prePrePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    m_blockItems[m_currentBlockNumber++] =
            BlockInfo{prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предпредпредыдущего блока
    //
    _lastBlockHeight = prePrePreviousBlockHeight - prePrePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScriptTextCorrector::moveCurrentBlockWithTwoPreviousToNextPage(const QTextBlock& _prePreviousBlock,
    const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor,
    QTextBlock& _block, qreal& _lastBlockHeight)
{
    --m_currentBlockNumber;
    --m_currentBlockNumber;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const QTextBlockFormat prePreviousBlockFormat = _prePreviousBlock.blockFormat();
    const qreal prePreviousBlockHeight =
            prePreviousBlockFormat.lineHeight() * _prePreviousBlock.layout()->lineCount()
            + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight + prePreviousBlockHeight;
    moveBlockToNextPage(_prePreviousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры блока время и места
    //
    m_blockItems[m_currentBlockNumber++] =
            BlockInfo{prePreviousBlockHeight - prePreviousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предпредыдущего блока
    //
    _lastBlockHeight = prePreviousBlockHeight - prePreviousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScriptTextCorrector::moveCurrentBlockWithPreviousToNextPage(const QTextBlock& _previousBlock,
    qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --m_currentBlockNumber;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight;
    moveBlockToNextPage(_previousBlock, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры предыдущего блока
    //
    m_blockItems[m_currentBlockNumber++] =
            BlockInfo{previousBlockHeight - previousBlockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту предыдущего блока
    //
    _lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin();
    //
    // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
    //
}

void ScriptTextCorrector::moveCurrentBlockToNextPage(const QTextBlockFormat& _blockFormat,
    qreal _blockHeight, qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight;
    moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
    _block = _cursor.block();
    //
    // Запоминаем параметры текущего блока
    //
    m_blockItems[m_currentBlockNumber++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
    //
    // Обозначаем последнюю высоту, как высоту текущего блока
    //
    _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
}

void ScriptTextCorrector::breakDialogue(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
    qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    //
    // Вставить блок
    //
    _cursor.setPosition(_block.position());
    insertBlock(_pageWidth, _cursor);
    updateBlockLayout(_pageWidth, _cursor.block());
    _cursor.movePosition(ScriptTextCursor::PreviousBlock);
    //
    // Оформить его, как ремарку
    //
    ScenarioBlockStyle parentheticalStyle =
        ScenarioTemplateFacade::getTemplate(m_templateName).blockStyle(ScenarioBlockStyle::Parenthetical);
    QTextBlockFormat parentheticalFormat = parentheticalStyle.blockFormat();
    parentheticalFormat.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
    parentheticalFormat.setProperty(ScenarioBlockStyle::PropertyIsCorrectionContinued, true);
    parentheticalFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    _cursor.setBlockFormat(parentheticalFormat);
    //
    // Вставить текст ДАЛЬШЕ
    //
    _cursor.insertText(::moreTerm());
    _block = _cursor.block();
    updateBlockLayout(_pageWidth, _block);
    const qreal moreBlockHeight =
            _block.layout()->lineCount() * parentheticalFormat.lineHeight()
            + parentheticalFormat.topMargin()
            + parentheticalFormat.bottomMargin();
    m_blockItems[m_currentBlockNumber++] = BlockInfo{moreBlockHeight, _lastBlockHeight};
    m_decorations[_block.position()] = _block.length();
    //
    // Перенести текущий блок на следующую страницу, если на текущей влезает
    // ещё хотя бы одна строка текста
    //
    _cursor.movePosition(ScriptTextCursor::NextBlock);
    _block = _cursor.block();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight - moreBlockHeight;
    if (sizeToPageEnd >= _blockFormat.topMargin() + _blockFormat.lineHeight()) {
        moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _pageWidth, _cursor);
        _block = _cursor.block();
    }
    //
    // Вставить перед текущим блоком декорацию ПЕРСОНАЖ (ПРОД.)
    // и для этого ищём предыдущий блок с именем персонажа
    //
    QTextBlock characterBlock = _block.previous();
    while (characterBlock.isValid()
           && ScenarioBlockStyle::forBlock(characterBlock) != ScenarioBlockStyle::Character) {
        characterBlock = characterBlock.previous();
    }
    //
    // Если нашли блок с именем персонажа
    //
    if (characterBlock.isValid()) {
        //
        // Вставляем блок
        //
        _cursor.setPosition(_block.position());
        insertBlock(_pageWidth, _cursor);
        _cursor.movePosition(ScriptTextCursor::PreviousBlock);
        //
        // Оформляем его, как имя персонажа
        //
        QTextBlockFormat characterFormat = characterBlock.blockFormat();
        characterFormat.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
        characterFormat.setProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter, true);
        characterFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        _cursor.setBlockFormat(characterFormat);
        //
        // И вставляем текст с именем персонажа
        //
        const QString characterName = CharacterParser::name(characterBlock.text());
        _cursor.insertText(characterName + continuedTerm());
        _block = _cursor.block();
        //
        updateBlockLayout(_pageWidth, _block);
        const qreal continuedBlockHeight =
                _block.layout()->lineCount() * characterFormat.lineHeight()
                + characterFormat.bottomMargin();
        m_blockItems[m_currentBlockNumber++] = BlockInfo{continuedBlockHeight, 0};
        m_decorations[_block.position()] = _block.length();
        //
        // Обозначаем последнюю высоту, как высоту предыдущего блока
        //
        _lastBlockHeight = continuedBlockHeight;
        //
        // Текущий блок будет обработан, как очередной блок посередине страницы при следующем проходе
        //
    }
    //
    // А если не нашли то оставляем блок прямо сверху страницы
    //
    else {
        m_blockItems[m_currentBlockNumber++] = BlockInfo{_blockHeight - _blockFormat.topMargin(), 0};
        _lastBlockHeight = _blockHeight - _blockFormat.topMargin();
    }
}

QTextBlock ScriptTextCorrector::findPreviousBlock(const QTextBlock& _block)
{
    QTextBlock previousBlock = _block.previous();
    while (previousBlock.isValid() && !previousBlock.isVisible()) {
        --m_currentBlockNumber;
        previousBlock = previousBlock.previous();
    }
    return previousBlock;
}

QTextBlock ScriptTextCorrector::findNextBlock(const QTextBlock& _block)
{
    QTextBlock nextBlock = _block.next();
    while (nextBlock.isValid()
           && !nextBlock.isVisible()
           && nextBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
        nextBlock = nextBlock.next();
    }
    return nextBlock;
}

void ScriptTextCorrector::moveBlockToNextPage(const QTextBlock& _block, qreal _spaceToPageEnd,
    qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor)
{
    //
    // Смещаем курсор в начало блока
    //
    _cursor.setPosition(_block.position());
    //
    // Определим сколько пустых блоков нужно вставить
    //
    const QTextBlockFormat format = _block.blockFormat();
    const qreal blockHeight = format.lineHeight() + format.topMargin() + format.bottomMargin();
    const int insertBlockCount = _spaceToPageEnd / blockHeight;
    //
    // Определим формат блоков декорации
    //
    QTextBlockFormat decorationFormat = format;
    if (ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::SceneHeading) {
        decorationFormat.setProperty(ScenarioBlockStyle::PropertyType, ScenarioBlockStyle::SceneHeadingShadow);
    }
    decorationFormat.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
    decorationFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    //
    // Вставляем блоки декорации
    //
    for (int blockIndex = 0; blockIndex < insertBlockCount; ++blockIndex) {
        //
        // Декорируем
        //
        insertBlock(_pageWidth, _cursor);
        _cursor.movePosition(ScriptTextCursor::PreviousBlock);
        _cursor.setBlockFormat(decorationFormat);
        //
        // Сохраним данные блока, чтобы перенести их к реальному владельцу
        //
        SceneHeadingBlockInfo* sceneBlockInfo = nullptr;
        if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(_cursor.block().userData())) {
            sceneBlockInfo = info->clone();
            _cursor.block().setUserData(nullptr);
        }
        CharacterBlockInfo* characterBlockInfo = nullptr;
        if (CharacterBlockInfo* info = dynamic_cast<CharacterBlockInfo*>(_cursor.block().userData())) {
            characterBlockInfo = info->clone();
            _cursor.block().setUserData(nullptr);
        }
        //
        // Запоминаем параметры текущего блока
        //
        m_blockItems[m_currentBlockNumber++] = BlockInfo{blockHeight, _pageHeight - _spaceToPageEnd + blockIndex * blockHeight};
        m_decorations[_cursor.block().position()] = _cursor.block().length();
        //
        // Переведём курсор на блок после декорации
        //
        _cursor.movePosition(ScriptTextCursor::NextBlock);
        if (sceneBlockInfo != nullptr) {
            _cursor.block().setUserData(sceneBlockInfo);
        }
        if (characterBlockInfo != nullptr) {
            _cursor.block().setUserData(characterBlockInfo);
        }
    }
}
