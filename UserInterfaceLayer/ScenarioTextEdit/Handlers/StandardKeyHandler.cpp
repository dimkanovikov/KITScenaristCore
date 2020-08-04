#include "StandardKeyHandler.h"

#include "../ScenarioTextEdit.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QKeyEvent>
#include <QTextBlock>

using namespace KeyProcessingLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;

namespace {
    /**
     * @brief Получить тип перехода/смены в зависимости от заданных параметров
     */
    static ScenarioBlockStyle::Type actionFor(bool _tab, bool _jump, ScenarioBlockStyle::Type _blockType) {
        const QString settingsKey =
                QString("scenario-editor/styles-%1/from-%2-by-%3")
                .arg(_jump ? "jumping" : "changing")
                .arg(ScenarioBlockStyle::typeName(_blockType))
                .arg(_tab ? "tab" : "enter");

        int result =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    settingsKey, DataStorageLayer::SettingsStorage::ApplicationSettings
                    ).toInt();

        return (ScenarioBlockStyle::Type)result;
    }

    /**
     * @brief Вспомогательные константы для использования с функцией actionFor
     */
    /** @{ */
    const bool TAB = true;
    const bool ENTER = false;
    const bool JUMP = true;
    const bool CHANGE = false;
    /** @} */
}


StandardKeyHandler::StandardKeyHandler(ScenarioTextEdit* _editor) :
    AbstractKeyHandler(_editor)
{
}

bool StandardKeyHandler::autoJumpToNextBlock()
{
    return
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "scenario-editor/auto-styles-jumping",
                DataStorageLayer::SettingsStorage::ApplicationSettings)
            .toInt();
}

ScenarioBlockStyle::Type StandardKeyHandler::jumpForTab(ScenarioBlockStyle::Type _blockType)
{
    return ::actionFor(TAB, JUMP, _blockType);
}

ScenarioBlockStyle::Type StandardKeyHandler::jumpForEnter(ScenarioBlockStyle::Type _blockType)
{
    return ::actionFor(ENTER, JUMP, _blockType);
}

ScenarioBlockStyle::Type StandardKeyHandler::changeForTab(ScenarioBlockStyle::Type _blockType)
{
    return ::actionFor(TAB, CHANGE, _blockType);
}

ScenarioBlockStyle::Type StandardKeyHandler::changeForEnter(ScenarioBlockStyle::Type _blockType)
{
    return ::actionFor(ENTER, CHANGE, _blockType);
}

void StandardKeyHandler::handleDelete(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // TODO: как быть с невидимыми блоками?
        //

        //
        // Удаление
        //
        removeCharacters(false);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleBackspace(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // Удаление
        //
        removeCharacters(true);

        //
        // Переходим к видимому блоку
        //
        QTextCursor cursor = editor()->textCursor();
        while (!cursor.atStart()
               && !cursor.block().isVisible()) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::StartOfBlock);
        }
        editor()->setTextCursor(cursor);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleEscape(QKeyEvent*)
{
    editor()->closeCompleter();
}

void StandardKeyHandler::handleUp(QKeyEvent* _event)
{
    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    if (!editor()->isCompleterVisible()) {
        const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
        const QTextCursor::MoveMode cursorMoveMode =
                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = editor()->textCursor();
        cursor.beginEditBlock();

        //
        // Исходная позиция курсора
        //
        int initCursorPosition = cursor.position();

        //
        // Рассчитаем количество символов от края
        //
        int marginFromLineStart = 0;
        {
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atStart()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
            }
            marginFromLineStart =
                    initCursorPosition
                    - cursor.position()
                    - (cursor.atStart() ? 0 : 1);
        }

        //
        // В данный момент курсор либо в начале документа, либо поднялся к концу предыдущей строки
        //

        if (!cursor.atStart()) {
            //
            // Если мы поднялись на строку вверх, но попали в невидимый блок, перейдём к предыдущему видимому
            //
            const QTextBlock firstDocumentBlock = cursor.document()->firstBlock();
            while (cursor.block() != firstDocumentBlock
                   && (!cursor.block().isVisible()
                       || ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::PageSplitter
                       || cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection))) {
                cursor.movePosition(QTextCursor::PreviousBlock, cursorMoveMode);
                cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
            }

            //
            // Сместим курсор в предыдущей строке на то кол-во символов, на которое он был смещён прежде
            //
            {
                int currentLineEndPosition = cursor.position();
                int currentLineYCoordinate = editor()->cursorRect(cursor).y();
                while (!cursor.atStart()
                       && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                    cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
                }

                //
                // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую строку
                //
                if (!cursor.atStart()) {
                    cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
                }

                int currentLineStartPosition = cursor.position();
                if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                    cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode, marginFromLineStart);
                } else {
                    cursor.setPosition(currentLineEndPosition, cursorMoveMode);
                }
            }
        }

        cursor.endEditBlock();
        editor()->setTextCursor(cursor);
    }
}

void StandardKeyHandler::handleDown(QKeyEvent* _event)
{
    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    if (!editor()->isCompleterVisible()) {
        const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
        const QTextCursor::MoveMode cursorMoveMode =
                isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

        QTextCursor cursor = editor()->textCursor();
        cursor.beginEditBlock();

        //
        // Исходная позиция курсора
        //
        int initCursorPosition = cursor.position();

        //
        // Рассчитаем количество символов от края
        //
        int marginFromLineStart = 0;
        {
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atStart()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
            }
            marginFromLineStart =
                    initCursorPosition
                    - cursor.position()
                    - (cursor.atStart() ? 0 : 1);
        }

        //
        // Вернём курсор в исходное положение
        //
        cursor.setPosition(initCursorPosition, cursorMoveMode);

        //
        // Сместим курсор к следующей строке или к концу документа
        //
        {
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atEnd()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }
        }

        //
        // В данный момент курсор либо в конце документа, либо перешёл к началу следующей строки
        //

        if (!cursor.atEnd()) {
            //
            // Если мы опустились на строку вниз, но попали в невидимый блок, перейдём к следующему видимому
            //
            while (!cursor.atEnd()
                   && (!cursor.block().isVisible()
                       || ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::PageSplitter
                       || cursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection))) {
                cursor.movePosition(QTextCursor::NextBlock, cursorMoveMode);
                cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
            }

            //
            // Сместим курсор в следующей строке на то кол-во символов, на которое он был смещён прежде
            //
            {
                int currentLineStartPosition = cursor.position();
                int currentLineYCoordinate = editor()->cursorRect(cursor).y();
                while (!cursor.atEnd()
                       && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                    cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
                }

                //
                // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую строку
                //
                if (!cursor.atEnd()) {
                    cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
                }

                int currentLineEndPosition = cursor.position();
                if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                    const int moveRepeats = currentLineEndPosition - currentLineStartPosition - marginFromLineStart;
                    cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode, moveRepeats);
                } else {
                    cursor.setPosition(currentLineEndPosition, cursorMoveMode);
                }
            }
        }

        cursor.endEditBlock();
        editor()->setTextCursor(cursor);
    }
}

void StandardKeyHandler::handlePageUp(QKeyEvent* _event)
{
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleUp(_event);
    }

    cursor.endEditBlock();
}

void StandardKeyHandler::handlePageDown(QKeyEvent* _event)
{
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleDown(_event);
    }

    cursor.endEditBlock();
}

void StandardKeyHandler::handleOther(QKeyEvent*)
{
    if (editor()->isCompleterVisible()) {
        editor()->closeCompleter();
    }
}


// **** private ****


void StandardKeyHandler::removeCharacters(bool _backward)
{
    QTextCursor cursor = editor()->textCursor();

    if (!cursor.hasSelection()) {
        //
        // Если в начале документа нажат backspace
        //
        if (cursor.atStart()
            && _backward == true) {
            return;
        }

        //
        // Если в конце документа нажат delete
        //
        if (cursor.atEnd()
            && _backward == false) {
            return;
        }
    }

    //
    // Определим границы выделения
    //
    int topCursorPosition = 0;
    int bottomCursorPosition = 0;
    {
        if (cursor.hasSelection()) {
            topCursorPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
            bottomCursorPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
        } else {
            topCursorPosition = cursor.position() - (_backward ? 1 : 0);
            bottomCursorPosition = topCursorPosition + 1;

            QTextCursor checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            //
            // ... переходим через корректирующие блоки блоки вперёд
            //
            if (!_backward
                && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.block().next().blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    bottomCursorPosition = checkCursor.position();
                }
            }
            //
            // ... назад
            //
            else if (_backward
                     && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionStart)
                    || checkCursor.block().next().blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);
                    checkCursor.movePosition(QTextCursor::PreviousCharacter);

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);

                    topCursorPosition = checkCursor.position();
                }
            }
        }
    }

    //
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    ScenarioBlockStyle topStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Undefined);
    QTextBlock topBlock;
    {
        QTextCursor topCursor(editor()->document());
        topCursor.setPosition(topCursorPosition);
        topStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::forBlock(topCursor.block()));
        topBlock = topCursor.block();
    }
    //
    // ... и конца
    //
    ScenarioBlockStyle bottomStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::Undefined);
    QTextBlock bottomBlock;
    {
        QTextCursor bottomCursor(editor()->document());
        bottomCursor.setPosition(bottomCursorPosition);
        bottomStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::forBlock(bottomCursor.block()));
        bottomBlock = bottomCursor.block();
    }

    //
    // Если удаление затрагивает несколько блоков или удаление происходит в заголовочном блоке,
    // возможно понадобится расширить область выделения
    //
    if (topBlock != bottomBlock
        || topStyle.isHeader()) {
        //
        // Если верхний блок является заголовком - расширим выделение до следующего блока, сдвигая курсор влево
        //
        if (topStyle.isHeader())
        {
            QTextCursor topCursor(editor()->document());
            topCursor.setPosition(topCursorPosition);

            while (topBlock == topCursor.block()
                   && !topCursor.atStart()) {
                topCursor.movePosition(QTextCursor::PreviousCharacter);
                topStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::forBlock(topCursor.block()));
            }

            topCursorPosition = topCursor.position();
        }

        //
        // Если нижний блок является заголовком - расширим выделение, сдвигая курсор вправо
        //
        if (bottomStyle.isHeader())
        {
            QTextCursor bottomCursor(editor()->document());
            bottomCursor.setPosition(bottomCursorPosition);

            while (bottomBlock == bottomCursor.block()
                   && !bottomCursor.atEnd()) {
                bottomCursor.movePosition(QTextCursor::NextCharacter);
                bottomStyle = ScenarioTemplateFacade::getTemplate().blockStyle(ScenarioBlockStyle::forBlock(bottomCursor.block()));
            }

            bottomCursorPosition = bottomCursor.position();
        }
    }

    //
    // Определим стиль результирующего блока
    //
    ScenarioBlockStyle::Type targetType = _backward ? topStyle.type() : bottomStyle.type();
    {
        if (topBlock == bottomBlock) {
            targetType = topStyle.type();
        } else {
            if (topStyle.isEmbeddable() && !bottomStyle.isEmbeddable() && !bottomBlock.text().isEmpty()) {
                targetType = bottomStyle.type();
            } else if (!topBlock.text().isEmpty()) {
                targetType = topStyle.type();
            } else if (!bottomBlock.text().isEmpty()) {
                targetType = bottomStyle.type();
            }
        }
    }

    //
    // Собственно удаление
    //
    cursor.beginEditBlock();
    {
        //
        // Подсчитать количество группирующих элементов входящих в выделение
        //
        QList<int> groupsToDeleteCounts;
        const bool needToDeleteGroups =
                topBlock != bottomBlock
                && ((topStyle.isEmbeddable() && (!bottomBlock.text().isEmpty() || (bottomCursorPosition - topCursorPosition > 1)))
                    || (bottomStyle.isEmbeddable() && (!topBlock.text().isEmpty() || (bottomCursorPosition - topCursorPosition > 1))));
        if (needToDeleteGroups) {
            groupsToDeleteCounts = findGroupCountsToDelete(topCursorPosition, bottomCursorPosition);
        }

        //
        // Удалить текст
        //
        cursor.setPosition(topCursorPosition);
        cursor.setPosition(bottomCursorPosition, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        //
        // Удалить вторые половинки группирующих элементов
        //
        if (needToDeleteGroups) {
            removeGroupsPairs(cursor.position(), groupsToDeleteCounts);
        }
    }

    //
    // Применим финальный стиль
    //
    if (ScenarioBlockStyle::forBlock(editor()->textCursor().block()) != targetType) {
        editor()->applyScenarioTypeToBlockText(targetType);
    }

    //
    // Если и верхний и нижний блоки являются группирующими,
    // то нужно стереть то, что после них остаётся (а как правило это одна половинка)
    //
    if (topBlock != bottomBlock && topStyle.isEmbeddable() && bottomStyle.isEmbeddable()) {
        if (cursor.block().text().isEmpty()) {
            if (cursor.atStart()) {
                cursor.deleteChar();
            } else {
                cursor.deletePreviousChar();
            }
        } else {
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.removeSelectedText();
        }
    }

    //
    // Завершим операцию удаления
    //
    cursor.endEditBlock();
}

namespace {
    const int FOLDER_HEADER = 0;
    const int FOLDER_FOOTER = 1;
}

QList<int> StandardKeyHandler::findGroupCountsToDelete(int _topCursorPosition, int _bottomCursorPosition)
{
    QList<int> groupCountsToDelete;
    groupCountsToDelete << 0 << 0;

    //
    // Начнём поиск с заданной позиции
    //
    QTextCursor searchGroupsCursor(editor()->document());
    searchGroupsCursor.setPosition(_topCursorPosition);

    while (searchGroupsCursor.position() <= _bottomCursorPosition) {
        //
        // Определим тип блока
        //
        ScenarioBlockStyle::Type currentType =
                ScenarioBlockStyle::forBlock(searchGroupsCursor.block());

        //
        // Если найден блок открывающий группу, то нужно удалить закрывающий блок
        //
        if (currentType == ScenarioBlockStyle::FolderHeader) {
            ++groupCountsToDelete[FOLDER_FOOTER];
        }

        //
        // Если найден блок закрывающий группу
        // ... если все группы закрыты, нужно удалить предыдущую открытую
        // ... в противном случае закрываем открытую группу
        //
        else if (currentType == ScenarioBlockStyle::FolderFooter) {
            if (groupCountsToDelete.value(FOLDER_FOOTER) == 0) {
                ++groupCountsToDelete[FOLDER_HEADER];
            }
            else {
                --groupCountsToDelete[FOLDER_FOOTER];
            }
        }

        //
        // Если дошли до конца, прерываем выполнение
        //
        if (searchGroupsCursor.atEnd()) {
            break;
        }

        //
        // Перейдём к следующему блоку или концу блока
        //
        searchGroupsCursor.movePosition(QTextCursor::EndOfBlock);
        searchGroupsCursor.movePosition(QTextCursor::NextBlock);
    }

    return groupCountsToDelete;
}

void StandardKeyHandler::removeGroupsPairs(int _cursorPosition, const QList<int>& _groupCountsToDelete)
{
    //
    // Удалим пары из последующего текста
    //
    // ... папки
    //
    if (_groupCountsToDelete.value(FOLDER_FOOTER) > 0) {
        QTextCursor cursor(editor()->document());
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.value(FOLDER_FOOTER);
        do {
            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);

            ScenarioBlockStyle::Type currentType =
                    ScenarioBlockStyle::forBlock(cursor.block());

            if (currentType == ScenarioBlockStyle::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если жто был самый последний блок
                    //
                    if (cursor.atEnd()) {
                        cursor.deletePreviousChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScenarioBlockStyle::FolderHeader) {
                // ... встретилась новая группа, которую не нужно удалять
                ++openedGroups;
            }
        } while (groupsToDeleteCount > 0
                 && !cursor.atEnd());
    }

    //
    // Удалим пары из предшествующего текста
    //
    // ... папки
    //
    if (_groupCountsToDelete.value(FOLDER_HEADER) > 0) {
        QTextCursor cursor = editor()->textCursor();
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.value(FOLDER_HEADER);
        do {
            cursor.movePosition(QTextCursor::PreviousBlock);
            ScenarioBlockStyle::Type currentType =
                    ScenarioBlockStyle::forBlock(cursor.block());

            if (currentType == ScenarioBlockStyle::FolderHeader) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если это был самый первый блок
                    //
                    if (cursor.atStart()) {
                        cursor.deleteChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScenarioBlockStyle::FolderFooter) {
                // ... встретилась новая группа, которую не нужно удалять
                ++openedGroups;
            }
            cursor.movePosition(QTextCursor::StartOfBlock);
        } while (groupsToDeleteCount > 0
                 && !cursor.atStart());
    }
}
