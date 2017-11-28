#include "ScriptTextCorrector.h"

#include "ScenarioTemplate.h"

#include <3rd_party/Helpers/RunOnce.h>
#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextDocument>

using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScriptTextCorrector;

namespace {
    /**
     * @brief Список символов пунктуации, разделяющие предложения
     */
    const QList<QChar> PUNCTUATION_CHARACTERS = { '.', '!', '?', QString("…").at(0)};

    /**
     * @brief Обновить компановку текста для блока
     */
    void updateBlockLayout(QTextBlock& _block, int _pageWidth) {
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
     * @brief Сместить блок в начало следующей страницы
     * @param _cursor - курсор редактироуемого документа
     * @param _block - блок для смещения
     * @param _spaceToPageEnd - количество места до конца страницы
     */
    void moveBlockToNextPage(QTextCursor& _cursor, const QTextBlock& _block, qreal _spaceToPageEnd) {
        //
        // Смещаем курсор в начало блока
        //
        _cursor.setPosition(_block.position());
        //
        // Определим сколько пустых блоков нужно вставить
        //
        const QTextBlockFormat format = _block.blockFormat();
        int insertBlockCount = _spaceToPageEnd
                               / (format.lineHeight() + format.topMargin() + format.bottomMargin());
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
        while (insertBlockCount-- > 0) {
            _cursor.insertBlock();
            _cursor.movePosition(QTextCursor::PreviousBlock);
            _cursor.setBlockFormat(decorationFormat);
        }
    }
}


ScriptTextCorrector::ScriptTextCorrector(QTextDocument* _document) :
    QObject(_document),
    m_document(_document)
{
    Q_ASSERT_X(m_document, Q_FUNC_INFO, "Document couldn't be a nullptr");
}

#include <QDebug>
void ScriptTextCorrector::correct()
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
    const qreal pageHeight = m_document->pageSize().height()
                             - rootFrameFormat.topMargin()
                             - rootFrameFormat.bottomMargin();
    const qreal pageWidth = m_document->pageSize().width()
                             - rootFrameFormat.leftMargin()
                             - rootFrameFormat.rightMargin();
    if (pageHeight < 0)
        return;

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
    // ... является ли блок первым на странице, для первых не нужно учитывать верхний отступ
    //
    bool isBlockFirstOnPage = true;
    //
    // ... значение нижней позиции последнего блока относительно начала страницы
    //
    qreal lastBlockHeight = 0;
    while (block.isValid()) {
        //
        // Пропускаем невидимые блоки
        //
        if (!block.isVisible()) {
            block = block.next();
            continue;
        }

        const QTextBlockFormat blockFormat = block.blockFormat();

        //
        // Если блок декорация, то удаляем его
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
        // Если в текущем блоке начинается разрыв, пробуем его вернуть
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
            ::updateBlockLayout(block, pageWidth);
            continue;
        }


        //
        // Определить высоту текущего блока
        //
        const qreal blockLineHeight = blockFormat.lineHeight();
        const int blockLineCount = block.layout()->lineCount();
        const qreal blockHeight =
                isBlockFirstOnPage
                ? blockLineHeight * blockLineCount + blockFormat.bottomMargin()
                : blockLineHeight * blockLineCount + blockFormat.topMargin() + blockFormat.bottomMargin();
        //
        // ... и одной строки следующего
        //
        qreal nextBlockHeight = 0;
        if (block.next().isValid()) {
            const qreal nextBlockLineHeight = block.next().blockFormat().lineHeight();
            const QTextBlockFormat nextBlockFormat = block.next().blockFormat();
            nextBlockHeight = nextBlockLineHeight + nextBlockFormat.topMargin();
        }


        //
        // Если блок заканчивается на последней строке страницы
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
        // ... или попадает на разрыв
        //
        const bool atPageBreak =
                // сам блок не влезает
                (lastBlockHeight + blockHeight > pageHeight)
                // но влезает хотя бы одна строка
                && (lastBlockHeight + blockFormat.topMargin() + blockLineHeight < pageHeight);
        if (atPageEnd || atPageBreak) {
            switch (ScenarioBlockStyle::forBlock(block)) {
                //
                // Если это время и место
                //
                case ScenarioBlockStyle::SceneHeading: {
                    //
                    // Переносим на следующую страницу
                    //
                    const qreal sizeToPageEnd = pageHeight - lastBlockHeight;
                    ::moveBlockToNextPage(cursor, block, sizeToPageEnd);
                    //
                    // Обозначаем последнюю высоту, как высоту блока время и места
                    //
                    lastBlockHeight = blockHeight - blockFormat.topMargin();
                    break;
                }

                //
                // Если это участники сцены
                //
                case ScenarioBlockStyle::SceneCharacters: {
                    //
                    // Определим предыдущий блок
                    //
                    QTextBlock previousBlock = block.previous();
                    while (previousBlock.isValid() && !previousBlock.isVisible()) {
                        previousBlock = previousBlock.previous();
                    }
                    //
                    // Если перед ним идёт время и место, переносим его тоже
                    //
                    if (previousBlock.isValid()
                        && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                        const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                        const qreal previousBlockHeight =
                                previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                        const qreal sizeToPageEnd = pageHeight - lastBlockHeight + previousBlockHeight;
                        ::moveBlockToNextPage(cursor, previousBlock, sizeToPageEnd);
                        //
                        // Обозначаем последнюю высоту, как высоту блока время и места плюс высоту блока участников сцены
                        //
                        lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin() + blockHeight;
                    }
                    //
                    // В противном случае, просто переносим блок на следующую страницу
                    //
                    else {
                        const qreal sizeToPageEnd = pageHeight - lastBlockHeight;
                        ::moveBlockToNextPage(cursor, block, sizeToPageEnd);
                        //
                        // Обозначаем последнюю высоту, как высоту блока время и места
                        //
                        lastBlockHeight = blockHeight - blockFormat.topMargin();
                    }

                    break;
                }

                //
                // Если это описание действия

                // - если на странице можно оставить текст, который займёт 2 и более строк,
                //    оставляем максимум, а остальное переносим. Разрываем по предложениям
                // - в остальном случае переносим полностью
                // -- если перед описанием действия идёт время и место, переносим и его тоже
                // -- если перед описанием действия идёт список участников, то переносим их
                //	  вместе с предыдущим блоком время и место
                //
                case ScenarioBlockStyle::Action: {
                    //
                    // Если в конце страницы, оставляем как есть
                    //
                    if (atPageEnd) {
                        isBlockFirstOnPage = true;
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
                                    cursor.movePosition(QTextCursor::PreviousBlock);
                                    cursor.setBlockFormat(breakStartFormat);
                                    //
                                    QTextBlockFormat breakEndFormat = blockFormat;
                                    breakEndFormat.setProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd, true);
                                    cursor.movePosition(QTextCursor::NextBlock);
                                    cursor.setBlockFormat(breakEndFormat);
                                    //
                                    // ... переносим оторванный конец на следующую страницу, если нужно
                                    //
                                    block = cursor.block();
                                    ::updateBlockLayout(block, pageWidth);
                                    const qreal sizeToPageEnd = lastBlockHeight + blockFormat.topMargin() + blockLineHeight * lineToBreak;
                                    if (pageHeight - sizeToPageEnd >= blockFormat.topMargin() + blockLineHeight) {
                                        ::moveBlockToNextPage(cursor, block, sizeToPageEnd);
                                    }
                                    //
                                    // ... обозначаем последнюю высоту
                                    //
                                    lastBlockHeight = block.layout()->lineCount() * blockLineHeight + blockFormat.bottomMargin();
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
                            QTextBlock previousBlock = block.previous();
                            while (previousBlock.isValid() && !previousBlock.isVisible()) {
                                previousBlock = previousBlock.previous();
                            }
                            //
                            // Если перед ним идёт время и место, переносим его тоже
                            //
                            if (previousBlock.isValid()
                                && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneHeading) {
                                const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                                const qreal previousBlockHeight =
                                        previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                        + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                                const qreal sizeToPageEnd = pageHeight - lastBlockHeight + previousBlockHeight;
                                ::moveBlockToNextPage(cursor, previousBlock, sizeToPageEnd);
                                //
                                // Обозначаем последнюю высоту, как высоту блока время и места плюс высоту блока описания действия
                                //
                                lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin() + blockHeight;
                            }
                            //
                            // Если перед ним идут участники сцены, то проверим ещё на один блок назад
                            //
                            else if (previousBlock.isValid()
                                     && ScenarioBlockStyle::forBlock(previousBlock) == ScenarioBlockStyle::SceneCharacters) {
                                //
                                // Проверяем предыдущий блок
                                //
                                QTextBlock prePreviousBlock = previousBlock.previous();
                                while (prePreviousBlock.isValid() && !prePreviousBlock.isVisible()) {
                                    prePreviousBlock = prePreviousBlock.previous();
                                }
                                //
                                // Если перед участниками идёт время и место, переносим его тоже
                                //
                                if (prePreviousBlock.isValid()
                                    && ScenarioBlockStyle::forBlock(prePreviousBlock) == ScenarioBlockStyle::SceneHeading) {
                                    const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                                    const qreal previousBlockHeight =
                                            previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                                    const QTextBlockFormat prePreviousBlockFormat = prePreviousBlock.blockFormat();
                                    const qreal prePreviousBlockHeight =
                                            prePreviousBlockFormat.lineHeight() * prePreviousBlock.layout()->lineCount()
                                            + prePreviousBlockFormat.topMargin() + prePreviousBlockFormat.bottomMargin();
                                    const qreal sizeToPageEnd = pageHeight - lastBlockHeight + previousBlockHeight + prePreviousBlockHeight;
                                    ::moveBlockToNextPage(cursor, prePreviousBlock, sizeToPageEnd);
                                    //
                                    // Обозначаем последнюю высоту, как высоту блоков время и места, участников сцены плюс высоту блока описания действия
                                    //
                                    lastBlockHeight = prePreviousBlockHeight - prePreviousBlockFormat.topMargin() + previousBlockHeight + blockHeight;
                                }
                                //
                                // В противном случае просто переносим вместе с участниками
                                //
                                else {
                                    const QTextBlockFormat previousBlockFormat = previousBlock.blockFormat();
                                    const qreal previousBlockHeight =
                                            previousBlockFormat.lineHeight() * previousBlock.layout()->lineCount()
                                            + previousBlockFormat.topMargin() + previousBlockFormat.bottomMargin();
                                    const qreal sizeToPageEnd = pageHeight - lastBlockHeight + previousBlockHeight;
                                    ::moveBlockToNextPage(cursor, previousBlock, sizeToPageEnd);
                                    //
                                    // Обозначаем последнюю высоту, как высоту блока участников сцены плюс высоту блока описания действия
                                    //
                                    lastBlockHeight = previousBlockHeight - previousBlockFormat.topMargin() + blockHeight;
                                }
                            }
                            //
                            // В противном случае, просто переносим блок на следующую страницу
                            //
                            else {
                                const qreal sizeToPageEnd = pageHeight - lastBlockHeight;
                                ::moveBlockToNextPage(cursor, block, sizeToPageEnd);
                                //
                                // Обозначаем последнюю высоту, как высоту блока время и места
                                //
                                lastBlockHeight = blockHeight - blockFormat.topMargin();
                            }
                        }
                    }
                    break;
                }

                //
                // Если это имя персонажа, переносим на следующую страницу
                // - если перед именем идёт время и место, их переносим тоже
                // - если перед именем идут участники сцены, их переносим тоже, и если перед ними
                //   идёт время и место, его переносим тоже
                //

                //
                // Если это ремарка, переносим на следующую страницу
                // - если перед ремаркой идёт имя персонажа переносим и его тоже
                // - если перед ремаркой идёт реплика, то разрываем реплику, вставляя вместо ремарки
                //   ДАЛЬШЕ и добавляя на новой странице имя персонажа с (ПРОД.)
                //

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

                //
                // В остальных случаях просто переходим на следующую страницу
                //
                default: {
                    if (atPageEnd) {
                        isBlockFirstOnPage = true;
                        lastBlockHeight = 0;
                    } else {
                        lastBlockHeight += blockHeight;
                        lastBlockHeight -= pageHeight;
                    }
                    break;
                }
            }
        }
        //
        // Если блок находится посередине страницы, просто переходим к следующему
        //
        else {
            isBlockFirstOnPage = false;
            lastBlockHeight += blockHeight;
        }

        block = block.next();
    }

    cursor.endEditBlock();
}
