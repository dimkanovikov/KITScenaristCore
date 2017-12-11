#include "ScriptTextCorrector.h"

#include "ScenarioTemplate.h"
#include "ScenarioTextBlockParsers.h"

#include <3rd_party/Helpers/RunOnce.h>
#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QTextBlock>
#include <QTextDocument>

using BusinessLogic::CharacterParser;
using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScenarioTemplateFacade;
using BusinessLogic::ScriptTextCorrector;

namespace {
    /**
     * @brief Список символов пунктуации, разделяющие предложения
     */
    const QList<QChar> PUNCTUATION_CHARACTERS = { '.', '!', '?', QString("…").at(0)};

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    //: Continued
    static const char* CONTINUED = QT_TRANSLATE_NOOP("BusinessLogic::ScriptTextCorrector", "CONT'D");
    static const QString continuedTerm() {
        return QString(" (%1)").arg(QApplication::translate("BusinessLogic::ScriptTextCorrector", CONTINUED));
    }

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
    void updateBlockLayout(int _pageWidth, QTextBlock& _block) {
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
}


ScriptTextCorrector::ScriptTextCorrector(QTextDocument* _document) :
    QObject(_document),
    m_document(_document)
{
    Q_ASSERT_X(m_document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

void ScriptTextCorrector::clear()
{
    m_lastDocumentSize = QSizeF();
    m_currentBlockNumber = 0;
    m_blockItems.clear();
}

#include <QDebug>
#include <QDateTime>
void ScriptTextCorrector::correct(int _position)
{
    qDebug() << "correct start\t" << QTime::currentTime().toString("hh.mm.ss.zzz");

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
    if (m_lastDocumentSize.width() != pageWidth
        || m_lastDocumentSize.height() != pageHeight) {
        m_blockItems.clear();
        m_lastDocumentSize = QSizeF(pageWidth, pageHeight);
    }

    //
    // Карту декораций перестраиваем целиком
    //
    m_decorations.clear();

    //
    // Определим список блоков для принудительной ручной проверки
    // NOTE: Это необходимо для того, чтобы корректно обрабатывать изменение текста
    //       в следующих за переносом блоках
    //
    QSet<int> blocksToRecheck;
    if (_position != -1) {
        QTextBlock blockToRecheck = m_document->findBlock(_position);
        //
        // Максимальное количество блоков которое может быть перенесено на новую страницу
        //
        int recheckBlocksCount = 5;
        do {
            blocksToRecheck.insert(blockToRecheck.blockNumber());
            blockToRecheck = blockToRecheck.previous();
        } while (blockToRecheck.isValid()
                 && (recheckBlocksCount-- > 0
                     || blockToRecheck.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)));
    }

    //
    // Начинаем работу с документом
    //
    QTextCursor cursor(m_document);
    cursor.beginEditBlock();

    //
    // Идём по каждому блоку документа с самого начала
    //
    QTextBlock block = m_document->begin();
    //
    // ... значение нижней позиции последнего блока относительно начала страницы
    //
    qreal lastBlockHeight = 0;
    m_currentBlockNumber = 0;
    while (block.isValid()) {
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
                lastBlockHeight == 0
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
        if (m_blockItems.contains(m_currentBlockNumber)
            && m_blockItems[m_currentBlockNumber].height == blockHeight
            && m_blockItems[m_currentBlockNumber].top == lastBlockHeight
            && !blocksToRecheck.contains(m_currentBlockNumber)) {
            //
            // Если не изменилась
            //
            // ... в данном случае блок не может быть на разрыве
            //
            Q_ASSERT(atPageBreak == false);
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
        } else {
            qDebug() << m_currentBlockNumber << block.blockNumber();
            qDebug() << "not eq bi:" << m_blockItems[m_currentBlockNumber].height << m_blockItems[m_currentBlockNumber].top;
            qDebug() << "not eq cb:" << blockHeight << lastBlockHeight;
            qDebug() << block.text();
            qDebug() << m_blockItems[m_currentBlockNumber - 1].height << m_blockItems[m_currentBlockNumber - 1].top;
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
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
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
            cursor.movePosition(QTextCursor::EndOfBlock);
            do {
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
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
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                if (cursor.selectionStart() != cursor.selectionEnd()) {
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
            ::updateBlockLayout(pageWidth, block);
            continue;
        }


        //
        // Работаем с переносами
        //
        if (atPageEnd || atPageBreak) {
            switch (ScenarioBlockStyle::forBlock(block)) {
                //
                // Если это время и место
                //
                case ScenarioBlockStyle::SceneHeading: {
                    //
                    // Переносим на следующую страницу
                    //
                    moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);

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
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);
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
                        moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
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
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);
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
                            moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
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
                                moveCurrentBlockWithThreePreviousToNextPage(prePrePreviousBlock, prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
                            }
                            //
                            // В противном случае просто переносим вместе с участниками
                            //
                            else {
                                moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
                            }
                        }
                        //
                        // В противном случае просто переносим вместе с участниками
                        //
                        else {
                            moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
                        }
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);
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
                                    cursor.insertBlock();
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
                                    while (cursor.block().text().startsWith(" ")) {
                                        cursor.deleteChar();
                                    }
                                    //
                                    // ... помечаем блоки, как разорванные
                                    //
                                    QTextBlockFormat breakStartFormat = blockFormat;
                                    breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                    cursor.movePosition(QTextCursor::PreviousBlock);
                                    cursor.setBlockFormat(breakStartFormat);
                                    //
                                    QTextBlockFormat breakEndFormat = blockFormat;
                                    breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                    cursor.movePosition(QTextCursor::NextBlock);
                                    cursor.setBlockFormat(breakEndFormat);
                                    //
                                    // ... обновим лэйаут оторванного блока
                                    //
                                    block = cursor.block();
                                    ::updateBlockLayout(pageWidth, block);
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
                                moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
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
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
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
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);
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
                                    cursor.insertBlock();
                                    //
                                    // ... запоминаем параметры блока оставшегося в конце страницы
                                    //
                                    const qreal breakStartBlockHeight =
                                            lineToBreak * blockLineHeight + blockFormat.topMargin() + blockFormat.bottomMargin();
                                    m_blockItems[m_currentBlockNumber++] = BlockInfo{breakStartBlockHeight, lastBlockHeight};
                                    //
                                    // ... если после разрыва остался пробел, уберём его
                                    //
                                    while (cursor.block().text().startsWith(" ")) {
                                        cursor.deleteChar();
                                    }
                                    //
                                    // ... помечаем блоки, как разорванные
                                    //
                                    QTextBlockFormat breakStartFormat = blockFormat;
                                    breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                    cursor.movePosition(QTextCursor::PreviousBlock);
                                    cursor.setBlockFormat(breakStartFormat);
                                    //
                                    QTextBlockFormat breakEndFormat = blockFormat;
                                    breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                    cursor.movePosition(QTextCursor::NextBlock);
                                    cursor.setBlockFormat(breakEndFormat);
                                    //
                                    // ... переносим оторванный конец на следующую страницу,
                                    //     если на текущую влезает ещё хотя бы одна строка текста
                                    //
                                    block = cursor.block();
                                    const qreal sizeToPageEnd = pageHeight - lastBlockHeight - breakStartBlockHeight;
                                    if (sizeToPageEnd >= blockFormat.topMargin() + blockLineHeight) {
                                        moveBlockToNextPage(block, sizeToPageEnd, pageHeight, cursor);
                                        block = cursor.block();
                                    }
                                    ::updateBlockLayout(pageWidth, block);
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
                                moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
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
                                    moveCurrentBlockWithTwoPreviousToNextPage(prePreviousBlock, previousBlock, pageHeight, cursor, block, lastBlockHeight);
                                }
                                //
                                // В противном случае просто переносим вместе с участниками
                                //
                                else {
                                    moveCurrentBlockWithPreviousToNextPage(previousBlock, pageHeight, cursor, block, lastBlockHeight);
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                moveCurrentBlockToNextPage(blockFormat, blockHeight, pageHeight, cursor, block, lastBlockHeight);
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

    qDebug() << "correct bend\t" << QTime::currentTime().toString("hh.mm.ss.zzz");
    cursor.endEditBlock();
    qDebug() << "correct end\t" << QTime::currentTime().toString("hh.mm.ss.zzz");
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

void ScriptTextCorrector::moveCurrentBlockWithThreePreviousToNextPage(const QTextBlock& _prePrePreviousBlock, const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight, QTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
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
    moveBlockToNextPage(_prePrePreviousBlock, sizeToPageEnd, _pageHeight, _cursor);
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
    const QTextBlock& _previousBlock, qreal _pageHeight, QTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
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
    moveBlockToNextPage(_prePreviousBlock, sizeToPageEnd, _pageHeight, _cursor);
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
    qreal _pageHeight, QTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight)
{
    --m_currentBlockNumber;

    const QTextBlockFormat previousBlockFormat = _previousBlock.blockFormat();
    const qreal previousBlockHeight =
            previousBlockFormat.lineHeight() * _previousBlock.layout()->lineCount()
            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight + previousBlockHeight;
    moveBlockToNextPage(_previousBlock, sizeToPageEnd, _pageHeight, _cursor);
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
    qreal _blockHeight, qreal _pageHeight, QTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight;
    moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _cursor);
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
    qreal _pageHeight, qreal _pageWidth, QTextCursor& _cursor, QTextBlock& _block,
    qreal& _lastBlockHeight)
{
    //
    // Вставить блок
    //
    _cursor.setPosition(_block.position());
    _cursor.insertBlock();
    _cursor.movePosition(QTextCursor::PreviousBlock);
    //
    // Оформить его, как ремарку
    //
    ScenarioBlockStyle parentheticalStyle =
        ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Parenthetical);
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
    ::updateBlockLayout(_pageWidth, _block);
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
    _cursor.movePosition(QTextCursor::NextBlock);
    _block = _cursor.block();
    const qreal sizeToPageEnd = _pageHeight - _lastBlockHeight - moreBlockHeight;
    if (sizeToPageEnd >= _blockFormat.topMargin() + _blockFormat.lineHeight()) {
        moveBlockToNextPage(_block, sizeToPageEnd, _pageHeight, _cursor);
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
        _cursor.insertBlock();
        _cursor.movePosition(QTextCursor::PreviousBlock);
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
        _cursor.insertText(characterName + ::continuedTerm());
        _block = _cursor.block();
        //
        ::updateBlockLayout(_pageWidth, _block);
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
    qreal _pageHeight, QTextCursor& _cursor)
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
        _cursor.insertBlock();
        _cursor.movePosition(QTextCursor::PreviousBlock);
        _cursor.setBlockFormat(decorationFormat);
        //
        // Запоминаем параметры текущего блока
        //
        m_blockItems[m_currentBlockNumber++] = BlockInfo{blockHeight, _pageHeight - _spaceToPageEnd + blockIndex * blockHeight};
        m_decorations[_cursor.block().position()] = _cursor.block().length();
        //
        // Переведём курсор на блок после декорации
        //
        _cursor.movePosition(QTextCursor::NextBlock);
    }
}
