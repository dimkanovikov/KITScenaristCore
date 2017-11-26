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

    connect(m_document, &QTextDocument::contentsChanged, this, &ScriptTextCorrector::correct);
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
                // Если это описание действия и попадает на разрыв
                // - если на странице можно оставить текст, который займёт 2 и более строк,
                //    оставляем максимум, а остальное переносим. Разрываем по предложениям
                // - в остальном случае переносим полностью
                // -- если перед описанием действия идёт время и место, переносим и его тоже
                // -- если перед описанием действия идёт список участников, то переносим их
                //	  вместе с предыдущим блоком время и место
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
                // В противном случае просто переходим на следующую страницу
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
