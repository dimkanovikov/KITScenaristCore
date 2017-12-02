#include "ScenarioTextCorrector.h"

#include "ScenarioTemplate.h"
#include "ScenarioTextBlockParsers.h"
#include "ScenarioTextDocument.h"

#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDebug>
#include <QFontMetricsF>
#include <QPair>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    //: Continued
    static const char* CONTINUED = QT_TRANSLATE_NOOP("BusinessLogic::ScenarioTextCorrector", "CONT'D");
    static const QString continuedTerm() {
        return QString(" (%1)").arg(QApplication::translate("BusinessLogic::ScenarioTextCorrector", CONTINUED));
    }

    /**
     * @brief Автоматически добавляемые продолжения в диалогах
     */
    static const char* MORE = QT_TRANSLATE_NOOP("BusinessLogic::ScenarioTextCorrector", "MORE");
    static const QString moreTerm() {
        return QString("(%1)").arg(QApplication::translate("BusinessLogic::ScenarioTextCorrector", MORE));
    }

    static const QList<QChar> PUNCTUATION_CHARACTERS = { '.', '!', '?', QString("…").at(0)};

    /**
     * @brief Разбить текст по предложениям
     */
    static QStringList splitTextToSentences(const QString& _text) {
        QStringList result;
        int lastSentenceEnd = 0;
        for (int charIndex = 0; charIndex < _text.length(); ++charIndex) {
            //
            // Если символ - пунктуация, разрываем
            //
            if (PUNCTUATION_CHARACTERS.contains(_text.at(charIndex))) {
                ++charIndex;
                //
                // Обрабатываем ситуацию со сдвоенными знаками, наподобии "!?"
                //
                if (charIndex < _text.length()
                    && PUNCTUATION_CHARACTERS.contains(_text.at(charIndex))) {
                    ++charIndex;
                }
                result.append(_text.mid(lastSentenceEnd, charIndex - lastSentenceEnd));
                lastSentenceEnd = charIndex;
            }
        }

        const QString lastSentence = _text.mid(lastSentenceEnd);
        const QString lastSentenceSimplified = lastSentence.simplified();
        if (!lastSentenceSimplified.isEmpty()
            && lastSentenceSimplified.length() > 1) {
            result.append(lastSentence);
        }

        return result;
    }

    /**
     * @brief Блок или курсор находится на границе сцены
     */
    /** @{ */
    static bool blockIsSceneBorder(const QTextBlock& _block) {
        return ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::SceneHeading
                || ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::FolderHeader
                || ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::FolderFooter;
    }
    static bool cursorAtSceneBorder(const QTextCursor& _cursor) {
        return blockIsSceneBorder(_cursor.block());
    }
    /** @} */

    /**
     * @brief Удалить заданные блок
     */
    static void removeTextBlock(QTextCursor& _cursor, const QTextBlock& _block) {
        _cursor.setPosition(_block.position());
        _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        _cursor.deleteChar();
    }

    /**
     * @brief BlockPages
     */
    class BlockInfo
    {
    public:
        BlockInfo() : onPageTop(false), movedToNextPage(false), topPage(0), bottomPage(0), topLinesCount(0), bottomLinesCount(0), width(0) {}
        BlockInfo(bool _onPageTop, bool _movedToNextPage, int _top, int _bottom, int _topLines, int _bottomLines, int _width)
            : onPageTop(_onPageTop), movedToNextPage(_movedToNextPage), topPage(_top), bottomPage(_bottom), topLinesCount(_topLines),
              bottomLinesCount(_bottomLines), width(_width)
        {}

        /**
         * @brief Блок находится в начале страницы
         */
        bool onPageTop;

        bool movedToNextPage;

        /**
         * @brief Страница, на которой начинается блок
         */
        int topPage;

        /**
         * @brief Стрница, на которой заканчивается блок
         */
        int bottomPage;

        /**
         * @brief Количество строк, занимаемых блоком на начальной странице
         */
        int topLinesCount;

        /**
         * @brief Количество строк, занимаемых блоком на конечной странице
         */
        int bottomLinesCount;

        /**
         * @brief Ширина блока
         */
        int width;
    };

    /**
     * @brief Получить номер страницы, на которой находится начало и конец блока
     */
    static BlockInfo getBlockInfo(const QTextBlock& _block) {
        QAbstractTextDocumentLayout* layout = _block.document()->documentLayout();
        const QRectF blockRect = layout->blockBoundingRect(_block);
        const qreal pageHeight = _block.document()->pageSize().height();
        int topPage = blockRect.top() / pageHeight;
        int bottomPage = blockRect.bottom() / pageHeight;

        //
        // Если хотя бы одна строка не влезает в конце текущей страницы,
        // то значит блок будет располагаться на следующей странице
        //
        const qreal positionOnTopPage = blockRect.top() - (topPage * pageHeight);
        const qreal pageBottomMargin = _block.document()->rootFrame()->frameFormat().bottomMargin();
        const qreal lineHeight = _block.blockFormat().lineHeight();
        bool onPageTop = false;
        if (pageHeight - positionOnTopPage - pageBottomMargin < lineHeight) {
            onPageTop = true;
            if (topPage == bottomPage) {
                ++topPage;
                ++bottomPage;
            } else {
                ++topPage;
            }
        }

        bool movedToNextPage = false;
        if (pageHeight - positionOnTopPage - pageBottomMargin > 0) {
            movedToNextPage = true;
        }

        //
        // Определим кол-во строк на страницах
        //
        int topLinesCount = blockRect.height() / lineHeight;
        int bottomLinesCount = 0;
        if (topPage != bottomPage) {
            topLinesCount = (pageHeight - positionOnTopPage - pageBottomMargin) / lineHeight;
            const qreal pagesInterval =
                pageBottomMargin + _block.document()->rootFrame()->frameFormat().topMargin();
            bottomLinesCount = (blockRect.height() - pagesInterval) / lineHeight - topLinesCount;
        }

        //
        // NOTE: Номера страниц ночинаются с нуля
        //
        return BlockInfo(onPageTop, movedToNextPage, topPage, bottomPage, topLinesCount, bottomLinesCount, blockRect.width());
    }

    /**
     * @brief Определить, сколько нужно вставить пустых блоков, для смещения заданного количества
     *		  строк с текстом для блока с этим форматом
     */
    static int neededEmptyBlocksForMove(const QTextBlockFormat& _format, int _linesWithText) {
        int result = 0;
        //
        // Если есть хоть один отступ, то получается, что нужно пустых блоков в два раза меньше,
        // чем строк, при том округляя всегда в большую сторону
        //
        if (_format.topMargin() > 0 || _format.bottomMargin() > 0) {
            result = _linesWithText / 2 + (_linesWithText % 2 > 0 ? 1 : 0);
        }
        //
        // Если отступов нет, нужно столько же блоков, сколько и строк
        //
        else {
            result = _linesWithText;
        }
        return result;
    }

    /**
     * @brief Определить, сколько строк нужно, чтобы поместить заданное количество строк в блоке
     *		  с этим форматом
     * @note для того, чтобы разместить блок, порой нужно немного больше места
     *		 например, чтобы разместить 2 строки, одного блока недостаточно, в то время,
     *		 как для смещения этого достаточно
     */
    static int neededLinesForPlace(const QTextBlockFormat& _format, int _linesWithText) {
        int result = 0;
        if (_linesWithText == 1) {
            result = 1;
        } else if (_format.topMargin() > 0 || _format.bottomMargin() > 0) {
            result = _linesWithText / 2 + (_linesWithText % 2 == 0 ? 1 : 0);
        } else {
            result = _linesWithText;
        }
        return result;
    }

    /**
     * @brief Узнать сколько строк занимает текст
     */
    static int linesCount(const QString _text, const QFont& _font, int _width) {
        static QTextLayout textLayout;

        textLayout.clearLayout();
        textLayout.setText(_text);
        textLayout.setFont(_font);

        textLayout.beginLayout();
        forever {
            QTextLine line = textLayout.createLine();
            if (!line.isValid()) {
                break;
            }

            line.setLineWidth(_width);
        }
        textLayout.endLayout();

        return textLayout.lineCount();
    }

    /**
     * @brief Проверить по месту ли распологается блок с декорацией
     */
    static bool checkCorrectionBlock(QTextCursor& _cursor, const QTextBlock& _block)
    {
        bool result = false;

        QTextBlock currentBlock = _block;
        QTextBlock nextBlock = _block.next();

        while (currentBlock != nextBlock
               && nextBlock.isValid()) {
            BlockInfo currentBlockInfo = getBlockInfo(currentBlock);
            BlockInfo nextBlockInfo = getBlockInfo(nextBlock);

            //
            // Если декорация в конце страницы
            //
            if (currentBlockInfo.topPage == currentBlockInfo.bottomPage
                && currentBlockInfo.topPage != nextBlockInfo.topPage) {
                //
                // Декорация с именем персонажа не может быть в конце страницы
                // Декорация с ДАЛЬШЕ не может быть в начале страницы
                //
                if (currentBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)
                    || nextBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionContinued)) {
                    //
                    // Декорация находится не по месту
                    //
                    result = false;
                    break;
                }
                //
                // Если следующий блок декорация
                //
                else if (nextBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                    && !nextBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)) {
                    //
                    // ... то удалим его и проверим новый следующий
                    //
                    removeTextBlock(_cursor, nextBlock);
                    nextBlock = currentBlock.next();
                }
                //
                // Если следующий блок не декорация или декорация с именем персонажа
                //
                else {
                    //
                    // Дальше можно не проверять, декорация находится по месту
                    //
                    result = true;
                    break;
                }
            }
            //
            // Если декорация не в конце страницы
            //
            else {
                //
                // Если следующий блок декорация
                //
                if (nextBlock.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                    //
                    // И если следующий блок на месте, то значит и текущий блок на месте
                    //
                    if (checkCorrectionBlock(_cursor, nextBlock)) {
                        result = true;
                        break;
                    }
                    //
                    // А если следующий блок не на месте
                    //
                    else {
                        //
                        // ... то удалим его и проверим новый следующий
                        //
                        removeTextBlock(_cursor, nextBlock);
                        nextBlock = currentBlock.next();
                    }
                }
                //
                // Если следующий блок не декорация
                //
                else {
                    //
                    // Дальше можно не проверять, декорация не по месту
                    //
                    result = false;
                    break;
                }
            }
        }

        return result;
    }

    /**
     * @brief Проверка блока на предмет, можно ли вместить текст следующий после блока на предыдущую страницу
     * @note Применяется только для блоков, прошедших проверку checkCorrectionBlock
     */
    static bool deepCheckCorrectionBlock(const QTextBlock& _block) {
        //
        // FIXME: объединить оба прохода, много повторяющегося кода
        //

        bool result = false;
        QTextBlock block = _block;

        //
        // Для разрывов
        //
        if (block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
            //
            // Если текст разрыва не заканчивается пунктуацией, то его нужно перепроверить
            //
            if (!block.text().isEmpty()
                && PUNCTUATION_CHARACTERS.contains(block.text().right(1).at(0))) {
                //
                // ... считаем сколько строк, замещаются декорациями на предыдущей странице
                //
                int decorationLines = 1; // 1 строка за разрыв
                if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::Dialogue) {
                    decorationLines -= 1; // минус одна строка за "ДАЛЬШЕ"
                }
                block = block.next();
                while (block.isValid()
                       && block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                       && !block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)) {
                    const int textLines = getBlockInfo(block).topLinesCount;
                    decorationLines += neededEmptyBlocksForMove(block.blockFormat(), textLines);
                    block = block.next();
                }
                //
                // ... если следующий за декорацией блок был вытолкнут, добавляем ещё одну строку
                //
                if (getBlockInfo(block).movedToNextPage) {
                    ++decorationLines;
                }
                //
                // ... пропускаем блок с декорацией с именем персонажа
                //
                if (block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)) {
                    block = block.next();
                }
                //
                // ... считаем сколько строк занимает конец разрыва
                //
                const BlockInfo contentBlockInfo = getBlockInfo(block);
                const QString firstSentence = splitTextToSentences(block.text()).first();
                const int textLines = linesCount(firstSentence, block.charFormat().font(), contentBlockInfo.width);
                int contentLines = neededLinesForPlace(block.blockFormat(), textLines);
                //
                // ... если текст можно поместить в разрыв, делаем дополнительную проверку
                //
                if (decorationLines == contentLines) {
                    //
                    // ... если блок состоит из одного предложения, а за ним следует ремарка и диалог, то считаем и их
                    //
                    if (block.text().simplified() == firstSentence
                        && block.next().isValid()) {
                        block = block.next();
                        ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
                        bool needAddNextBlockLines =
                                blockType == ScenarioBlockStyle::Dialogue
                                || blockType == ScenarioBlockStyle::Parenthetical;
                        if (needAddNextBlockLines) {
                            const int textLines = getBlockInfo(block).topLinesCount;
                            contentLines += neededLinesForPlace(block.blockFormat(), textLines);
                        }
                    }
                }
                //
                // ... проверяем можно ли вместить
                //
                result = decorationLines < contentLines;
            }
        }
        //
        // Для простых сдвигов
        //
        else {
            //
            // ... идём до первого коррекционного блока в связке
            //
            while (block.isValid()
                   && block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                block = block.previous();
            }
            block = block.next();
            //
            // ... cчитаем сколько строк сдвигается вниз
            //
            int decorationLines = 0;
            while (block.isValid()
                   && block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                   && !block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)) {
                const int textLines = getBlockInfo(block).topLinesCount;
                decorationLines += neededEmptyBlocksForMove(block.blockFormat(), textLines);
                block = block.next();
            }
            //
            // ... если следующий за декорацией блок был вытолкнут, добавляем ещё одну строку
            //
            if (getBlockInfo(block).movedToNextPage) {
                ++decorationLines;
            }
            //
            // ... пропускаем блок с декорацией с именем персонажа
            //
            if (block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter)) {
                block = block.next();
            }
            //
            // ... считаем сколько строк занимает текст, следующий за декорациями
            //
            int contentLines = 0;
            QString blockText = block.text();
            if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::Action) {
                //
                // ... для описания действия считаем по первому предложению
                //
                const BlockInfo contentBlockInfo = getBlockInfo(block);
                blockText = splitTextToSentences(block.text()).first();
                const int textLines = linesCount(blockText, block.charFormat().font(), contentBlockInfo.width);
                contentLines = neededLinesForPlace(block.blockFormat(), textLines);
            } else {
                //
                // ... для остальных блоков считаем по всему тексту блока
                //
                const int textLines = getBlockInfo(block).topLinesCount;
                contentLines = neededLinesForPlace(block.blockFormat(), textLines);
            }
            //
            // ... если текст можно поместить в разрыв, делаем дополнительную проверку,
            //	   возможно разрыв стоит по причине невозможности поместить подряд несколько блоков
            //
            if (decorationLines >= contentLines) {
                if (block.text().simplified() == blockText.simplified()) {
                    //
                    ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
                    QTextBlock nextBlock = block.next();
                    while (nextBlock.isValid() && !nextBlock.isVisible()) {
                        nextBlock = nextBlock.next();
                    }
                    BlockInfo nextBlockInfo = getBlockInfo(nextBlock);
                    //
                    // Место и время
                    //
                    if (blockType == ScenarioBlockStyle::SceneHeading) {
                        //
                        // ... после места и времени идут персонажи
                        //
                        if (ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::SceneCharacters) {
                            contentLines += neededLinesForPlace(nextBlock.blockFormat(), nextBlockInfo.topLinesCount);
                            nextBlock = nextBlock.next();
                            nextBlockInfo = getBlockInfo(nextBlock);
                        }
                        //
                        // ... после места и времени или после персонажей идёт описание действия
                        //
                        if (ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::Action) {
                            const QStringList sentences = splitTextToSentences(nextBlock.text());
                            QString nextBlockText;
                            int nextBlockTextLines = 0;
                            foreach (const QString& sentence, sentences) {
                                nextBlockText.append(sentence);
                                nextBlockTextLines = linesCount(nextBlockText, nextBlock.charFormat().font(), nextBlockInfo.width);
                                if (nextBlockTextLines >= 2) {
                                    break;
                                }
                            }
                            contentLines += neededLinesForPlace(nextBlock.blockFormat(), nextBlockTextLines);
                        }
                        //
                        // FIXME
                        // ... если после места и времени или после персонажей идёт имя персонажа
                        //
                    }
                    //
                    // Имя персонажа
                    //
                    else if (blockType == ScenarioBlockStyle::Character) {
                        //
                        // ... после имени идёт ремарка
                        //
                        if (ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::Parenthetical) {
                            contentLines += neededLinesForPlace(nextBlock.blockFormat(), nextBlockInfo.topLinesCount);
                            nextBlock = nextBlock.next();
                            nextBlockInfo = getBlockInfo(nextBlock);
                        }
                        //
                        // ... после имени или ремарки идёт реплика
                        //
                        if (ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::Dialogue) {
                            const QStringList sentences = splitTextToSentences(nextBlock.text());
                            QString nextBlockText;
                            int nextBlockTextLines = 0;
                            foreach (const QString& sentence, sentences) {
                                nextBlockText.append(sentence);
                                nextBlockTextLines = linesCount(nextBlockText, nextBlock.charFormat().font(), nextBlockInfo.width);
                                if (nextBlockTextLines >= 2) {
                                    break;
                                }
                            }
                            contentLines += neededLinesForPlace(nextBlock.blockFormat(), nextBlockTextLines);

                            //
                            // ... если присоединили весь текст блока, проверяем, не нужно ли присоединить следующий блок
                            //
                            if (nextBlockText == nextBlock.text()
                                && nextBlock.next().isValid()) {
                                QTextBlock nextNextBlock = nextBlock.next();
                                if (ScenarioBlockStyle::forBlock(nextNextBlock) == ScenarioBlockStyle::Parenthetical
                                    || ScenarioBlockStyle::forBlock(nextNextBlock) == ScenarioBlockStyle::Dialogue) {
                                    ++contentLines; // ДАЛЬШЕ
                                }
                            }
                            //
                            // ... если присоединили часть
                            //
                            else {
                                ++contentLines; // ДАЛЬШЕ
                            }
                        }
                    }
                }
            }
            //
            // ... проверяем можно ли вместить
            //
            result = decorationLines < contentLines;
        }

        return result;
    }

    /**
     * @brief Сместить блок вниз при помощи блоков декораций
     */
    static void moveBlockDown(QTextBlock& _block, QTextCursor& _cursor, int _position) {
        const BlockInfo blockInfo = getBlockInfo(_block);
        //
        // Смещаем блок, если он не в начале страницы
        //
        if (!blockInfo.onPageTop) {
            int emptyBlocksCount = neededEmptyBlocksForMove(_block.blockFormat(), blockInfo.topLinesCount);
            while (emptyBlocksCount-- > 0) {
                _cursor.beginEditBlock();
                _cursor.setPosition(_position);
                _cursor.insertBlock();
                _cursor.movePosition(QTextCursor::PreviousBlock);
                QTextBlockFormat format = _block.blockFormat();
                if (ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::SceneHeading) {
                    format.setProperty(ScenarioBlockStyle::PropertyType, ScenarioBlockStyle::SceneHeadingShadow);
                }
                format.setProperty(PageTextEdit::PropertyDontShowCursor, true);
                format.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
                _cursor.setBlockFormat(format);
                _cursor.endEditBlock();
            }
        }
    }

    /**
     * @brief Сместить блок вниз при помощи блоков декораций внутри диалога
     * @note Этот метод используется только для блоков ремарки и диалога
     */
    static void moveBlockDownInDialogue(QTextBlock& _block, QTextCursor& _cursor, int _position) {
        if (ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::Parenthetical
            || ScenarioBlockStyle::forBlock(_block) == ScenarioBlockStyle::Dialogue) {

            BlockInfo blockInfo = getBlockInfo(_block);
            int emptyBlocksCount = neededEmptyBlocksForMove(_block.blockFormat(), blockInfo.topLinesCount);
            bool isFirstDecoration = true;
            while (emptyBlocksCount-- > 0) {
                _cursor.setPosition(_position);
                _cursor.movePosition(QTextCursor::PreviousBlock);
                _cursor.movePosition(QTextCursor::EndOfBlock);
                QTextBlockFormat format = _block.blockFormat();
                format.setProperty(PageTextEdit::PropertyDontShowCursor, true);
                format.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);

                //
                // Первый вставляемый блок оформляем как ремарку и добавляем туда текст ДАЛЬШЕ
                //
                if (isFirstDecoration) {
                    isFirstDecoration = false;

                    ScenarioBlockStyle parentheticalStyle =
                        ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Parenthetical);
                    format = parentheticalStyle.blockFormat();
                    format.setProperty(PageTextEdit::PropertyDontShowCursor, true);
                    format.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
                    format.setProperty(ScenarioBlockStyle::PropertyIsCorrectionContinued, true);
                    _cursor.insertBlock(format);

                    _cursor.insertText(moreTerm());

                    //
                    // Обновляем позицию курсора, чтобы остальной текст добавлялся ниже блока с текстом ДАЛЬШЕ
                    //
                    _position += moreTerm().length() + 1;
                }
                //
                // Остальные блоки вставляются, как обычные корректировки
                //
                else {
                    _cursor.insertBlock(format);
                    _position += 1;
                }
            }

            //
            // Когда перешли на новую страницу - добавляем блок с именем персонажа и (ПРОД.)
            // и делаем его декорацией
            //
            QTextBlock characterBlock = _block.previous();
            while (characterBlock.isValid()
                   && ScenarioBlockStyle::forBlock(characterBlock) != ScenarioBlockStyle::Character) {
                characterBlock = characterBlock.previous();
            }
            if (characterBlock.isValid()) {
                QString characterName = CharacterParser::name(characterBlock.text());

                _cursor.setPosition(_position);
                _cursor.movePosition(QTextCursor::PreviousBlock);
                _cursor.movePosition(QTextCursor::EndOfBlock);

                ScenarioBlockStyle characterStyle =
                        ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Character);
                QTextBlockFormat format = characterStyle.blockFormat();
                format.setProperty(PageTextEdit::PropertyDontShowCursor, true);
                format.setProperty(ScenarioBlockStyle::PropertyIsCorrection, true);
                format.setProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter, true);
                _cursor.insertBlock(format);

                _cursor.insertText(characterName + continuedTerm());
                _cursor.movePosition(QTextCursor::EndOfBlock);
            }
        }
    }
}


void ScenarioTextCorrector::configure(bool _continueDialogues, bool _correctTextOnPageBreaks)
{
    if (s_continueDialogues != _continueDialogues) {
        s_continueDialogues = _continueDialogues;
    }

    //
    // FIXME: Отключил из-за плохой реализации
    //
    s_correctTextOnPageBreaks = false;
//    Q_UNUSED(_correctTextOnPageBreaks);
    if (s_correctTextOnPageBreaks != _correctTextOnPageBreaks) {
        s_correctTextOnPageBreaks = _correctTextOnPageBreaks;
    }
}

void ScenarioTextCorrector::removeDecorations(QTextDocument* _document, int _startPosition, int _endPosition)
{
    return;



    QTextCursor cursor(_document);
    cursor.setPosition(_startPosition);
    if (_endPosition == 0) {
        _endPosition = _document->characterCount();
    }

    cursor.beginEditBlock();
    while (!cursor.atEnd()
           && cursor.position() <= _endPosition) {
        //
        // Если текущий блок декорация, то убираем его
        //
        const QTextBlockFormat blockFormat = cursor.block().blockFormat();
        if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
            ::removeTextBlock(cursor, cursor.block());
        }
        //
        // Если в текущем блоке начинается разрыв, пробуем его вернуть
        //
        else if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
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
        }
        //
        // Если текущий блок не декорация, просто переходим к следующему
        //
        else {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextCharacter);
        }
    }
    cursor.endEditBlock();
}

void ScenarioTextCorrector::correctScenarioText(ScenarioTextDocument* _document, int _startPosition, bool _force)
{
    return;




    //
    // Вводим карту с текстами обрабатываемых документов, чтобы не выполнять лишнюю работу,
    // повторно обрабатывая один и тот же документ
    //
    static QHash<QTextDocument*, QString> s_documentHash;
    if (_force == false
        && s_documentHash.contains(_document)
        && s_documentHash.value(_document) == _document->scenarioXmlHash()) {
        return;
    }

    //
    // Защищаемся от рекурсии
    //
    static bool s_proccessedNow = false;
    if (s_proccessedNow == false) {
        s_proccessedNow = true;

        //
        // Собственно корректировки
        //
        correctDocumentText(_document, _startPosition);

        //
        // Обновляем текст текущего документа
        //
        s_documentHash[_document] = _document->scenarioXmlHash();

        s_proccessedNow = false;
    }
}

void ScenarioTextCorrector::correctDocumentText(QTextDocument* _document, int _startPosition)
{
    return;



    QTextCursor cursor(_document);

    //
    // Для имён персонажей, нужно добавлять ПРОД. (только, если имя полностью идентично предыдущему)
    //
    if (s_continueDialogues) {
        //
        // Сперва нужно подняться до начала сцены и начинать корректировки с этого положения
        //
        cursor.setPosition(_startPosition);
        while (!cursor.atStart()
               && !::cursorAtSceneBorder(cursor)) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::StartOfBlock);
        }

        //
        // Храним последнего персонажа сцены
        //
        QString lastSceneCharacter;
        do {
            if (ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::Character) {
                const QString character = CharacterParser::name(cursor.block().text());
                const bool isStartPositionInBlock =
                        cursor.block().position() <= _startPosition
                        && cursor.block().position() + cursor.block().length() > _startPosition;
                //
                // Если имя текущего персонажа не пусто и курсор не находится в этом блоке
                //
                if (!character.isEmpty() && !isStartPositionInBlock) {
                    //
                    // Не второе подряд появление, удаляем из него вспомогательный текст, если есть
                    //
                    if (lastSceneCharacter.isEmpty()
                        || character != lastSceneCharacter) {
                        QString blockText = cursor.block().text();
                        if (blockText.endsWith(continuedTerm(), Qt::CaseInsensitive)) {
                            cursor.setPosition(cursor.block().position() + blockText.indexOf(continuedTerm(), 0, Qt::CaseInsensitive));
                            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                            cursor.removeSelectedText();
                        }
                    }
                    //
                    // Если второе подряд, добавляем вспомогательный текст
                    //
                    else if (character == lastSceneCharacter){
                        QString characterState = CharacterParser::state(cursor.block().text());
                        if (characterState.isEmpty()) {
                            //
                            // ... вставляем текст
                            //
                            cursor.movePosition(QTextCursor::EndOfBlock);
                            cursor.insertText(continuedTerm());
                        }
                    }

                    lastSceneCharacter = character;
                }
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        } while (!cursor.atEnd()
                 && !::cursorAtSceneBorder(cursor));
    }

    //
    // Если нужно корректировать текст на разрывах и используется постраничный режим,
    // то обрабатываем блоки находящиеся в конце страницы
    //
    if (s_correctTextOnPageBreaks
        && _document->pageSize().isValid()) {

        //
        // Ставим курсор в блок, в котором происходит редактирование
        //
        cursor.setPosition(_startPosition);
        cursor.movePosition(QTextCursor::StartOfBlock);

        //
        // Если за 3 блока до текущего блока есть декорации, то проверим и их тоже
        //
        // Делать это нужно для того, чтобы корректно реагировать на ситуации, когда текст
        // был перенесён на следующую страницу из-за того что не влез на предыдущую, а потом
        // пользователь изменил его, оставив там меньше строк и теперь текст влезет
        //
        {
            QTextBlock block = cursor.block();
            int reply = 0;
            const int maxReplies = 3;
            while (reply++ < maxReplies
                   && block.isValid()) {
                while (block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                    || block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
                    cursor.setPosition(block.position());
                    block = block.previous();
                }
                block = block.previous();
            }
        }


        //
        // Корректируем обрывы строк
        //
        // Вносим корректировки пока не достигнем первого корректирующего блока,
        // если он стоит по месту, то прерываешь бег, в противном случае удаляем его
        // и продолжаем выполнять корректировки до следующего корректирующего блока, который
        // находится в правильном месте, или пока не достигнем конца документа
        //

        QTextBlock currentBlock = cursor.block();
        while (currentBlock.isValid()) {
            //
            // Определим следующий видимый блок
            //
            QTextBlock nextBlock = currentBlock.next();
            while (nextBlock.isValid() && !nextBlock.isVisible()) {
                nextBlock = nextBlock.next();
            }

            if (nextBlock.isValid()) {
                //
                // Если текущий блок декорация, проверяем, в правильном ли месте он расположен
                //
                {
                    const QTextBlockFormat blockFormat = currentBlock.blockFormat();

                    //
                    // Декорации могут находится только в конце страницы
                    //
                    if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                        //
                        // Если декорация по месту, прерываем корректировки
                        //
                        if (::checkCorrectionBlock(cursor, currentBlock)
                            && ::deepCheckCorrectionBlock(currentBlock)) {
                            break;
                        }
                        //
                        // А если не по месту, удаляем её и переходим к проверке следующего блока
                        //
                        else {
                            QTextBlock previuosBlock = currentBlock.previous();

                            ::removeTextBlock(cursor, currentBlock);

                            currentBlock = previuosBlock.next();
                            continue;
                        }
                    }
                    //
                    // Если в текущем блоке начинается разрыв
                    //
                    else if (blockFormat.boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
                        //
                        // Если разрыв по месту, прерываем корректировки
                        //
                        if (::checkCorrectionBlock(cursor, currentBlock)
                            && ::deepCheckCorrectionBlock(currentBlock)) {
                            break;
                        }
                        //
                        // А если не по месту, сшиваем блок и перепроверяем его
                        //
                        else {
                            QTextBlock previuosBlock = currentBlock.previous();

                            cursor.setPosition(currentBlock.position());
                            cursor.movePosition(QTextCursor::EndOfBlock);
                            //
                            // ... проходим сквозь все корректирующие блоки
                            //
                            QTextCursor breakCursor = cursor;
                            do {
                                breakCursor.movePosition(QTextCursor::NextBlock);
                            } while (breakCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection));
                            //
                            // ... если дошли до конца разрыва, то сшиваем его
                            //
                            if (breakCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)) {
                                cursor.setPosition(breakCursor.position(), QTextCursor::KeepAnchor);
                                cursor.insertText(" ");
                                QTextBlockFormat format = cursor.blockFormat();
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, QVariant());
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, QVariant());
                                cursor.setBlockFormat(format);
                            }
                            //
                            // ... если внутри разрыва оказался обычный блок, убираем все декорации вокруг
                            //
                            else {
                                //
                                // ... верхний блок разрыва
                                //
                                QTextBlockFormat format = cursor.blockFormat();
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, QVariant());
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, QVariant());
                                cursor.setBlockFormat(format);
                                //
                                // ... вставленные пользователем блоки
                                //
                                do {
                                    cursor.movePosition(QTextCursor::NextBlock);
                                } while (!cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                                         && !cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd));
                                //
                                // ... декорации
                                //
                                while (cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                                    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                                }
                                if (cursor.hasSelection()) {
                                    cursor.removeSelectedText();
                                }
                                //
                                // ... нижний блок разрыва
                                //
                                format = cursor.blockFormat();
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, QVariant());
                                format.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, QVariant());
                                cursor.setBlockFormat(format);
                            }

                            //
                            // ... перепроверяем текущий блок
                            //
                            currentBlock = previuosBlock.next();
                            continue;
                        }
                    }
                }


                //
                // Собственно корректировки
                //
                {
                    //
                    // Проверяем не находится ли текущий блок в конце страницы
                    //
                    BlockInfo currentBlockInfo = getBlockInfo(currentBlock);
                    BlockInfo nextBlockInfo = getBlockInfo(nextBlock);

                    //
                    // Нашли конец страницы, обрабатываем его соответствующим для типа блока образом
                    //
                    if (currentBlockInfo.topPage != nextBlockInfo.topPage) {

                        //
                        // Время и место
                        //
                        // переносим на следующую страницу
                        // - если в конце предыдущей страницы
                        //
                        if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::SceneHeading) {
                            ::moveBlockDown(currentBlock, cursor, currentBlock.position());
                        }


                        //
                        // Участники сцены
                        //
                        // переносим на следующую страницу
                        // - если в конце предыдущей страницы
                        // - если перед участниками стоит время и место, переносим и его тоже
                        //
                        else if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::SceneCharacters) {
                            int startPosition = currentBlock.position();
                            bool sceneCharactersMovedYet = false;

                            //
                            // Проверяем предыдущий блок
                            //
                            {
                                QTextBlock previousBlock = currentBlock.previous();
                                while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                    previousBlock = previousBlock.previous();
                                }
                                if (previousBlock.isValid()) {
                                    //
                                    // Если перед участниками сцены идёт время и место и его тоже переносим
                                    //
                                    if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                                        startPosition = previousBlock.position();
                                        ::moveBlockDown(currentBlock, cursor, startPosition);
                                        ::moveBlockDown(previousBlock, cursor, startPosition);
                                        sceneCharactersMovedYet = true;
                                    }
                                }
                            }

                            //
                            // Делаем пропуски необходимые для переноса участников сцены
                            //
                            if (!sceneCharactersMovedYet) {
                                ::moveBlockDown(currentBlock, cursor, startPosition);
                            }
                        }


                        //
                        // Описание действия
                        // - если находится на обеих страницах
                        // -- если на странице можно оставить текст, который займёт 2 и более строк,
                        //    оставляем максимум, а остальное переносим. Разрываем по предложениям
                        // -- в остальном случае переносим полностью
                        // --- если перед описанием действия идёт время и место, переносим и его тоже
                        // --- если перед описанием действия идёт список участников, то переносим их
                        //	   вместе с предыдущим блоком время и место
                        //
                        else if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::Action
                                 && currentBlockInfo.topPage != currentBlockInfo.bottomPage) {
                            //
                            // Пробуем разорвать так, чтобы часть текста осталась на предыдущей странице
                            //
                            const int availableLinesOnPageEnd = 2;
                            bool breakSuccess = false;
                            if (currentBlockInfo.topLinesCount >= availableLinesOnPageEnd) {
                                QStringList prevPageSentences = ::splitTextToSentences(currentBlock.text());
                                QStringList nextPageSentences;
                                while (!prevPageSentences.isEmpty()) {
                                    nextPageSentences << prevPageSentences.takeLast();
                                    const QString newText = prevPageSentences.join("");
                                    int linesCount = ::linesCount(newText, currentBlock.charFormat().font(), currentBlockInfo.width);
                                    //
                                    // ... если нашли место, где можно разорвать
                                    //
                                    if (linesCount <= currentBlockInfo.topLinesCount
                                        && linesCount >= availableLinesOnPageEnd) {
                                        cursor.setPosition(currentBlock.position() + newText.length());
                                        //
                                        // ... если есть пробел, уберём его
                                        //
                                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                                        if (cursor.selectedText() == " ") {
                                            cursor.removeSelectedText();
                                        } else {
                                            cursor.movePosition(QTextCursor::PreviousCharacter);
                                        }

                                        //
                                        // ... разрываем блок
                                        //
                                        cursor.insertBlock();

                                        //
                                        // ... добавляем свойства для разрывов
                                        //
                                        cursor.movePosition(QTextCursor::PreviousBlock);
                                        QTextBlockFormat breakStartFormat = cursor.blockFormat();
                                        breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                        cursor.setBlockFormat(breakStartFormat);
                                        //
                                        cursor.movePosition(QTextCursor::NextBlock);
                                        QTextBlockFormat breakEndFormat = cursor.blockFormat();
                                        breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                        cursor.setBlockFormat(breakEndFormat);

                                        QTextBlock blockForMove = cursor.block();
                                        ::moveBlockDown(blockForMove, cursor, cursor.block().position());

                                        breakSuccess = true;
                                        break;
                                    }
                                }
                            }

                            //
                            // Если блок не удалось разорвать переносим его на следующую страницу
                            //
                            if (breakSuccess == false) {
                                int startPosition = currentBlock.position();
                                bool actionMovedYet = false;

                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock previousBlock = currentBlock.previous();
                                while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                    previousBlock = previousBlock.previous();
                                }
                                if (previousBlock.isValid()) {
                                    //
                                    // Если перед описанием действия идёт время и место и его тоже переносим
                                    //
                                    if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                                        startPosition = previousBlock.position();
                                        ::moveBlockDown(currentBlock, cursor, startPosition);
                                        ::moveBlockDown(previousBlock, cursor, startPosition);
                                        actionMovedYet = true;
                                    }
                                    //
                                    // Если перед описанием действия идут участники сцены, то их тоже переносим
                                    //
                                    else if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneCharacters) {
                                        startPosition = previousBlock.position();
                                        bool sceneCharactersMovedYet = false;

                                        //
                                        // Проверяем предыдущий блок
                                        //
                                        QTextBlock prePreviousBlock = previousBlock.previous();
                                        while (prePreviousBlock.isValid() && !prePreviousBlock.isVisible()) {
                                            prePreviousBlock = prePreviousBlock.previous();
                                        }
                                        if (prePreviousBlock.isValid()) {
                                            //
                                            // Если перед участниками сцены идёт время и место его тоже переносим
                                            //
                                            if (ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                                startPosition = prePreviousBlock.position();
                                                ::moveBlockDown(currentBlock, cursor, startPosition);
                                                ::moveBlockDown(previousBlock, cursor, startPosition);
                                                ::moveBlockDown(prePreviousBlock, cursor, startPosition);
                                                actionMovedYet = true;
                                                sceneCharactersMovedYet = true;
                                            }
                                        }

                                        //
                                        // Делаем пропуски необходимые для переноса самих участников сцены
                                        //
                                        if (!sceneCharactersMovedYet) {
                                            ::moveBlockDown(currentBlock, cursor, startPosition);
                                            ::moveBlockDown(previousBlock, cursor, startPosition);
                                            actionMovedYet = true;
                                        }
                                    }
                                }

                                //
                                // Делаем пропуски необходимые для переноса самого описания действия
                                //
                                if (!actionMovedYet) {
                                    ::moveBlockDown(currentBlock, cursor, startPosition);
                                }
                            }
                        }


                        //
                        // Имя персонажа
                        //
                        // переносим на следующую страницу
                        // - если в конце предыдущей страницы
                        // - если перед именем персонажа идёт заголовок сцены, переносим их вместе
                        //
                        else if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::Character) {
                            int startPosition = currentBlock.position();
                            bool characterMovedYet = false;

                            //
                            // Проверяем предыдущий блок
                            //
                            QTextBlock previousBlock = currentBlock.previous();
                            while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                previousBlock = previousBlock.previous();
                            }
                            if (previousBlock.isValid()) {
                                //
                                // Если перед именем персонажа идёт время и место и его тоже переносим
                                //
                                if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                                    startPosition = previousBlock.position();
                                    ::moveBlockDown(currentBlock, cursor, startPosition);
                                    ::moveBlockDown(previousBlock, cursor, startPosition);
                                    characterMovedYet = true;
                                }
                                //
                                // Если перед именем персонажа идут участники сцены, то их тоже переносим
                                //
                                else if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneCharacters) {
                                    startPosition = previousBlock.position();
                                    bool sceneCharactersMovedYet = false;

                                    //
                                    // Проверяем предыдущий блок
                                    //
                                    QTextBlock prePreviousBlock = previousBlock.previous();
                                    while (prePreviousBlock.isValid() && !prePreviousBlock.isVisible()) {
                                        prePreviousBlock = prePreviousBlock.previous();
                                    }
                                    if (prePreviousBlock.isValid()) {
                                        //
                                        // Если перед участники сцены идёт время и место его тоже переносим
                                        //
                                        if (ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                            startPosition = prePreviousBlock.position();
                                            ::moveBlockDown(currentBlock, cursor, startPosition);
                                            ::moveBlockDown(previousBlock, cursor, startPosition);
                                            ::moveBlockDown(prePreviousBlock, cursor, startPosition);
                                            characterMovedYet = true;
                                            sceneCharactersMovedYet = true;
                                        }
                                    }

                                    //
                                    // Делаем пропуски необходимые для переноса самих участников сцены
                                    //
                                    if (!sceneCharactersMovedYet) {
                                        ::moveBlockDown(currentBlock, cursor, startPosition);
                                        ::moveBlockDown(previousBlock, cursor, startPosition);
                                        characterMovedYet = true;
                                    }
                                }
                            }

                            //
                            // Делаем пропуски необходимые для переноса самого имени персонажа
                            //
                            if (!characterMovedYet) {
                                ::moveBlockDown(currentBlock, cursor, startPosition);
                            }
                        }


                        //
                        // Ремарка
                        // - если перед ремаркой идёт имя персонажа, переносим их вместе на след. страницу
                        // - если перед ремаркой идёт реплика, вместо ремарки пишем ДАЛЬШЕ, а на следующую
                        //	 страницу добавляем сперва имя персонажа с (ПРОД), а затем саму ремарку
                        //
                        else if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::Parenthetical) {
                            int startPosition = currentBlock.position();
                            bool parentheticalMovedYet = false;

                            //
                            // Проверяем предыдущий блок
                            //
                            {
                                QTextBlock previousBlock = currentBlock.previous();
                                while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                    previousBlock = previousBlock.previous();
                                }
                                if (previousBlock.isValid()) {
                                    //
                                    // Если перед ремаркой идёт имя персонажа и его тоже переносим
                                    //
                                    if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Character) {
                                        startPosition = previousBlock.position();
                                        ::moveBlockDown(currentBlock, cursor, startPosition);
                                        ::moveBlockDown(previousBlock, cursor, startPosition);
                                        parentheticalMovedYet = true;
                                    }
                                    //
                                    // Если перед ремаркой идёт диалог, то переносим по правилам
                                    //
                                    else if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Dialogue) {
                                        ::moveBlockDownInDialogue(currentBlock, cursor, startPosition);
                                        parentheticalMovedYet = true;
                                    }
                                }
                            }

                            //
                            // Делаем пропуски необходимые для переноса самой ремарки
                            //
                            if (!parentheticalMovedYet) {
                                ::moveBlockDown(currentBlock, cursor, startPosition);
                            }
                        }


                        //
                        // Диалог
                        // - если можно, то оставляем текст так, чтобы он занимал не менее 2 строк,
                        //	 добавляем ДАЛЬШЕ и на следующей странице имя персонажа с (ПРОД) и сам диалог
                        // - в противном случае
                        // -- если перед диалогом идёт имя персонажа, то переносим их вместе на след.
                        // -- если перед диалогом идёт ремарка
                        // --- если перед ремаркой идёт имя персонажа, то переносим их всех вместе
                        // --- если перед ремаркой идёт диалог, то разрываем по ремарке, пишем вместо неё
                        //	   ДАЛЬШЕ, а на следующей странице имя персонажа с (ПРОД), ремарку и сам диалог
                        //
                        else if (ScenarioBlockStyle::forBlock(currentBlock) == ScenarioBlockStyle::Dialogue) {
                            //
                            // Блок на разрыве страниц
                            //
                            const bool blockOnPageBreak = currentBlockInfo.topPage != currentBlockInfo.bottomPage;
                            //
                            // Блок нельзя отрывать от последующего
                            //
                            const bool blockNeedGlueWithNext =
                                    currentBlockInfo.topPage == currentBlockInfo.bottomPage
                                    && (ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::Parenthetical
                                        || ScenarioBlockStyle::forBlock(nextBlock) == ScenarioBlockStyle::Dialogue);
                            if (blockOnPageBreak || blockNeedGlueWithNext) {
                                //
                                // Пробуем разорвать так, чтобы часть текста осталась на предыдущей странице.
                                //
                                const int availableLinesOnPageEnd = 2;
                                bool breakSuccess = false;
                                //
                                // Оставлять будем 2 строки, но проверять нужно на 3, т.к. нам ещё
                                // нужно поместить вспомогательную надпись ДАЛЬШЕ
                                //
                                if (currentBlockInfo.topLinesCount >= availableLinesOnPageEnd + 1) {
                                    QStringList prevPageSentences = ::splitTextToSentences(currentBlock.text());
                                    QStringList nextPageSentences;
                                    while (!prevPageSentences.isEmpty()) {
                                        nextPageSentences << prevPageSentences.takeLast();
                                        const QString newText = prevPageSentences.join("");
                                        int linesCount = ::linesCount(newText, currentBlock.charFormat().font(), currentBlockInfo.width);
                                        //
                                        // ... опять резервируем одну строку под надпись ДАЛЬШЕ
                                        //
                                        if (linesCount <= currentBlockInfo.topLinesCount - 1
                                            && linesCount >= availableLinesOnPageEnd) {
                                            cursor.setPosition(currentBlock.position() + newText.length());
                                            //
                                            // ... если есть пробел, уберём его
                                            //
                                            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                                            if (cursor.selectedText() == " ") {
                                                cursor.removeSelectedText();
                                            } else {
                                                cursor.movePosition(QTextCursor::PreviousCharacter);
                                            }

                                            //
                                            // ... разрываем блок
                                            //
                                            cursor.insertBlock();

                                            //
                                            // ... добавляем свойства для разрывов
                                            //
                                            cursor.movePosition(QTextCursor::PreviousBlock);
                                            QTextBlockFormat breakStartFormat = cursor.blockFormat();
                                            breakStartFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart, true);
                                            cursor.setBlockFormat(breakStartFormat);
                                            //
                                            cursor.movePosition(QTextCursor::NextBlock);
                                            QTextBlockFormat breakEndFormat = cursor.blockFormat();
                                            breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                            cursor.setBlockFormat(breakEndFormat);

                                            QTextBlock blockForMove = cursor.block();
                                            ::moveBlockDownInDialogue(blockForMove, cursor, cursor.block().position());

                                            breakSuccess = true;
                                            break;
                                        }
                                    }
                                }

                                //
                                // Если блок не удалось разорвать смотрим, что мы можем с ним сделать
                                //
                                if (breakSuccess == false) {
                                    int startPosition = currentBlock.position();
                                    bool dialogueMovedYet = false;

                                    //
                                    // Проверяем предыдущий блок
                                    //
                                    QTextBlock previousBlock = currentBlock.previous();
                                    while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                        previousBlock = previousBlock.previous();
                                    }
                                    if (previousBlock.isValid()) {
                                        //
                                        // Если перед диалогом идёт персонаж и его тоже переносим
                                        //
                                        if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Character) {
                                            startPosition = previousBlock.position();
                                            ::moveBlockDown(currentBlock, cursor, startPosition);
                                            ::moveBlockDown(previousBlock, cursor, startPosition);
                                            dialogueMovedYet = true;
                                        }
                                        //
                                        // Если перед диалогом идёт ремарка
                                        //
                                        else if (ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::Parenthetical) {
                                            startPosition = previousBlock.position();

                                            //
                                            // Проверяем предыдущий блок
                                            //
                                            QTextBlock prePreviousBlock = previousBlock.previous();
                                            while (prePreviousBlock.isValid() && !prePreviousBlock.isVisible()) {
                                                prePreviousBlock = prePreviousBlock.previous();
                                            }
                                            if (prePreviousBlock.isValid()) {
                                                //
                                                // Если перед ремаркой идёт персонаж, просто переносим их вместе
                                                //
                                                if (ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::Character) {
                                                    startPosition = prePreviousBlock.position();
                                                    ::moveBlockDown(currentBlock, cursor, startPosition);
                                                    ::moveBlockDown(previousBlock, cursor, startPosition);
                                                    ::moveBlockDown(prePreviousBlock, cursor, startPosition);
                                                    dialogueMovedYet = true;
                                                }
                                                //
                                                // В противном случае, заменяем ремарку на ДАЛЬШЕ и переносим её
                                                //
                                                else {
                                                    ::moveBlockDownInDialogue(previousBlock, cursor, startPosition);
                                                    dialogueMovedYet = true;
                                                }
                                            }
                                        }
                                    }

                                    //
                                    // Делаем пропуски необходимые для переноса самого диалога
                                    //
                                    if (!dialogueMovedYet) {
                                        ::moveBlockDown(currentBlock, cursor, startPosition);
                                    }
                                }

                            }
                        }
                    }
                }
            }

            currentBlock = nextBlock;
        }
    }
    //
    // В противном случае просто удаляем блоки с декорациями
    //
    else {
        removeDecorations(_document);
    }
}

bool ScenarioTextCorrector::s_continueDialogues = true;
bool ScenarioTextCorrector::s_correctTextOnPageBreaks = true;
