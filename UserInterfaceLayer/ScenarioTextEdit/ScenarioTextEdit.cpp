#include "ScenarioTextEdit.h"
#include "ScenarioTextEditPrivate.h"

#include "Handlers/KeyPressHandlerFacade.h"

#include <Domain/Research.h>

#include <BusinessLayer/Import/FountainImporter.h>
#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockParsers.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioReviewModel.h>
#include <BusinessLayer/ScenarioDocument/ScriptTextCursor.h>
#include <BusinessLayer/ScenarioDocument/ScriptTextCorrector.h>

#include <DataLayer/DataStorageLayer/ResearchStorage.h>
#include <DataLayer/DataStorageLayer/StorageFacade.h>

#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Helpers/ColorHelper.h>

#include <3rd_party/Widgets/FlatButton/FlatButton.h>

#include <QAbstractItemView>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QCompleter>
#include <QDateTime>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QStringListModel>
#include <QStyleHints>
#ifndef MOBILE_OS
#include <QSound>
#endif
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextTable>
#include <QTimer>
#include <QWheelEvent>
#include <QWidgetAction>

using UserInterface::ScenarioTextEdit;
using UserInterface::ShortcutsManager;
using namespace BusinessLogic;

namespace {
    /**
     * @brief Флаг положения курсора
     * @note Используется для корректировки скрола при совместном редактировании
     */
    const char* CURSOR_RECT = "cursorRect";

    /**
      * @brief Позиция мыши в момент вызова контекстного меню
      * @note Используется для определения над каким блоком было вызвано действие контекстного меню
      */
    const char* kLastMousePosKey = "lastMousePos";
}


ScenarioTextEdit::ScenarioTextEdit(QWidget* _parent) :
    CompletableTextEdit(_parent),
    m_mouseClicks(0),
    m_lastMouseClickTime(0),
    m_storeDataWhenEditing(true),
    m_showSceneNumbers(false),
    m_showDialoguesNumbers(false),
    m_highlightBlocks(false),
    m_highlightCurrentLine(false),
    m_capitalizeFirstWord(false),
    m_correctDoubleCapitals(false),
    m_replaceThreeDots(false),
    m_smartQuotes(false),
    m_showSuggestionsInEmptyBlocks(false),
    m_shortcutsManager(new ShortcutsManager(this))
{
    setAttribute(Qt::WA_KeyCompression);

    m_document = new ScenarioTextDocument(this, nullptr);
    setDocument(m_document);
    initEditor();

    initView();
    initConnections();
}

void ScenarioTextEdit::setScenarioDocument(ScenarioTextDocument* _document)
{
    removeEditorConnections();

    //
    // Удалим курсоры
    //
    m_additionalCursors.clear();
    m_additionalCursorsCorrected.clear();

    m_document = _document;
    setDocument(m_document);
    setHighlighterDocument(m_document);

    if (m_document != 0) {
        initEditor();

        _document->correct();
    }
}

void ScenarioTextEdit::addScenarioBlock(ScenarioBlockStyle::Type _blockType)
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Если работаем в режиме поэпизодника и добавляется заголовок новой сцены, группы сцен,
    // или папки, то нужно сдвинуть курсор до конца текущей сцены
    //
    if (outlineMode()
        && scenarioBlockType() == ScenarioBlockStyle::SceneDescription
        && (_blockType == ScenarioBlockStyle::SceneHeading
            || _blockType == ScenarioBlockStyle::FolderHeader)) {
        ScenarioBlockStyle::Type currentBlockType = ScenarioBlockStyle::forBlock(cursor.block());
        while (!cursor.atEnd()
               && currentBlockType != ScenarioBlockStyle::SceneHeading
               && currentBlockType != ScenarioBlockStyle::FolderFooter
               && currentBlockType != ScenarioBlockStyle::FolderHeader) {
            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
            currentBlockType = ScenarioBlockStyle::forBlock(cursor.block());
        }
        //
        // Если дошли не до конца документа, а до начала новой сцены,
        // возвращаем курсор в конец предыдущего блока
        //
        if (!cursor.atEnd()
            || currentBlockType == ScenarioBlockStyle::FolderFooter) {
            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
    }

    //
    // Если работает в режиме не поэпизодника, то блок нужно сместить после всех блоков
    // описания текущей сцены
    //
    if (!outlineMode()
        && _blockType != ScenarioBlockStyle::SceneDescription) {
        ScenarioBlockStyle::Type nextBlockType = ScenarioBlockStyle::forBlock(cursor.block().next());
        while (!cursor.atEnd()
               && nextBlockType == ScenarioBlockStyle::SceneDescription) {
            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
            nextBlockType = ScenarioBlockStyle::forBlock(cursor.block().next());
        }
    }

    //
    // Вставим блок
    //
    cursor.insertBlock();
    setTextCursorReimpl(cursor);

    //
    // Применим стиль к новому блоку
    //
    applyScenarioTypeToBlock(_blockType);

    //
    // Уведомим о том, что стиль сменился
    //
    emit currentStyleChanged();

    cursor.endEditBlock();
}

void ScenarioTextEdit::changeScenarioBlockType(ScenarioBlockStyle::Type _blockType, bool _forced)
{
    //
    // Если работаем в режиме поэпизодника, то запрещаем менять стиль блока на персонажей, реплики и пр.
    //
    if (outlineMode()
        && (_blockType == ScenarioBlockStyle::Action
            || _blockType == ScenarioBlockStyle::Character
            || _blockType == ScenarioBlockStyle::Parenthetical
            || _blockType == ScenarioBlockStyle::Dialogue
            || _blockType == ScenarioBlockStyle::Transition
            || _blockType == ScenarioBlockStyle::Note
            || _blockType == ScenarioBlockStyle::TitleHeader
            || _blockType == ScenarioBlockStyle::Title
            || _blockType == ScenarioBlockStyle::NoprintableText
            || _blockType == ScenarioBlockStyle::Lyrics)) {
        return;
    }

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Если работаем в режиме поэпизодника и описание сцены меняется на заголовок сцены,
    // или папки, и после текущего блока не идёт блок с описанием сцены (т.е. мы внутри сцены с текстом),
    // то текущий блок нужно перенести в конец текущей сцены
    //
    const ScenarioBlockStyle::Type nextBlockType = ScenarioBlockStyle::forBlock(cursor.block().next());
    if (outlineMode()
        && scenarioBlockType() == ScenarioBlockStyle::SceneDescription
        && (_blockType == ScenarioBlockStyle::SceneHeading
            || _blockType == ScenarioBlockStyle::FolderHeader)
        && nextBlockType != ScenarioBlockStyle::SceneDescription) {
        //
        // Сохраним текст блока, а сам блок удалим
        //
        const QString blockText = cursor.block().text();
        moveCursor(QTextCursor::StartOfBlock);
        moveCursor(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        textCursor().deleteChar();

        //
        // Если блок был не в конце документа, перейдём к предыдущему
        //
        if (!textCursor().atEnd()) {
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::EndOfBlock);
        }

        //
        // Вставляем блок
        //
        addScenarioBlock(_blockType);

        //
        // Восстанавливаем текст
        //
        textCursor().insertText(blockText);

        //
        // Уведомим о том, что стиль сменился
        //
        emit currentStyleChanged();
    }

    if (scenarioBlockType() != _blockType) {
        //
        // Нельзя сменить стиль заголовка титра и конечных элементов групп и папок
        //
        bool canChangeType =
                _forced
                || ((scenarioBlockType() != ScenarioBlockStyle::TitleHeader)
                    && (scenarioBlockType() != ScenarioBlockStyle::FolderFooter));

        //
        // Если текущий вид можно сменить
        //
        if (canChangeType) {
            //
            // Закроем подсказку
            //
            closeCompleter();

            //
            // Обработаем предшествующий установленный стиль
            //
            cleanScenarioTypeFromBlock();

            //
            // Применим новый стиль к блоку
            //
            applyScenarioTypeToBlock(_blockType);

            //
            // Уведомим о том, что стиль сменился
            //
            emit currentStyleChanged();
        }
    }

    cursor.endEditBlock();

    emit styleChanged();
}

void ScenarioTextEdit::changeScenarioBlockTypeForSelection(ScenarioBlockStyle::Type _blockType)
{
    //
    // Если нет выделения, меняем обычным способом
    //
    if (!textCursor().hasSelection()) {
        changeScenarioBlockType(_blockType);
        return;
    }

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Задаём тип для каждого блока из выделения
    //
    const int startPosition = std::min(cursor.selectionStart(), cursor.selectionEnd());
    const int endPosition = std::max(cursor.selectionStart(), cursor.selectionEnd());
    cursor.setPosition(startPosition);
    while (!cursor.atEnd()
           && cursor.position() < endPosition) {
        setTextCursor(cursor);
        changeScenarioBlockType(_blockType);
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    }
    cursor.endEditBlock();

    //
    // Возвращаем курсор в исходное положение
    //
    cursor.setPosition(startPosition);
    cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void ScenarioTextEdit::applyScenarioTypeToBlockText(ScenarioBlockStyle::Type _blockType)
{
    applyScenarioTypeToBlock(_blockType);

    emit styleChanged();
}

ScenarioBlockStyle::Type ScenarioTextEdit::scenarioBlockType() const
{
    return ScenarioBlockStyle::forBlock(textCursor().block());
}

void ScenarioTextEdit::setTextCursorReimpl(const QTextCursor& _cursor)
{
    int verticalScrollValue = verticalScrollBar()->value();
    setTextCursor(_cursor);
    verticalScrollBar()->setValue(verticalScrollValue);
}

bool ScenarioTextEdit::storeDataWhenEditing() const
{
    return m_storeDataWhenEditing;
}

void ScenarioTextEdit::setStoreDataWhenEditing(bool _store)
{
    if (m_storeDataWhenEditing != _store) {
        m_storeDataWhenEditing = _store;
    }
}

bool ScenarioTextEdit::showSceneNumbers() const
{
    return m_showSceneNumbers;
}

void ScenarioTextEdit::setShowSceneNumbers(bool _show)
{
    if (m_showSceneNumbers != _show) {
        m_showSceneNumbers = _show;
    }
}

QString ScenarioTextEdit::sceneNumbersPrefix() const
{
    return m_sceneNumbersPrefix;
}

void ScenarioTextEdit::setSceneNumbersPrefix(const QString& _prefix)
{
    m_sceneNumbersPrefix = _prefix;
}

bool ScenarioTextEdit::showDialoguesNumbers() const
{
    return m_showDialoguesNumbers;
}

void ScenarioTextEdit::setShowDialoguesNumbers(bool _show)
{
    if (m_showDialoguesNumbers != _show) {
        m_showDialoguesNumbers = _show;
    }
}

void ScenarioTextEdit::setHighlightBlocks(bool _highlight)
{
    if (m_highlightBlocks != _highlight) {
        m_highlightBlocks = _highlight;
    }
}

void ScenarioTextEdit::setHighlightCurrentLine(bool _highlight)
{
    if (m_highlightCurrentLine != _highlight) {
        m_highlightCurrentLine = _highlight;
    }
}

void ScenarioTextEdit::setAutoReplacing(bool _capitalizeFirstWord, bool _correctDoubleCapitals,
    bool _replaceThreeDots, bool _smartQuotes)
{
    if (m_capitalizeFirstWord != _capitalizeFirstWord) {
        m_capitalizeFirstWord = _capitalizeFirstWord;
    }
    if (m_correctDoubleCapitals != _correctDoubleCapitals) {
        m_correctDoubleCapitals = _correctDoubleCapitals;
    }
    if (m_replaceThreeDots != _replaceThreeDots) {
        m_replaceThreeDots = _replaceThreeDots;
    }
    if (m_smartQuotes != _smartQuotes) {
        m_smartQuotes = _smartQuotes;
    }
}

void ScenarioTextEdit::setShowSuggestionsInEmptyBlocks(bool _show)
{
    if (m_showSuggestionsInEmptyBlocks != _show) {
        m_showSuggestionsInEmptyBlocks = _show;
    }
}

void ScenarioTextEdit::setKeyboardSoundEnabled(bool _enabled)
{
    if (m_keyboardSoundEnabled != _enabled) {
        m_keyboardSoundEnabled = _enabled;
    }
}

bool ScenarioTextEdit::outlineMode() const
{
    return m_document == 0 ? true : m_document->outlineMode();
}

void ScenarioTextEdit::setOutlineMode(bool _outlineMode)
{
    if (m_document != 0) {
        m_document->setOutlineMode(_outlineMode);
        relayoutDocument();

        //
        // Убедимся, что курсор виден, а так же делаем так, чтобы он не скакал по экрану
        //
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        ensureCursorVisible();
    }
}

QList<ScenarioBlockStyle::Type> ScenarioTextEdit::visibleBlocksTypes() const
{
    return m_document == 0 ? QList<ScenarioBlockStyle::Type>() : m_document->visibleBlocksTypes();
}

void ScenarioTextEdit::updateShortcuts()
{
    m_shortcutsManager->update();
}

QString ScenarioTextEdit::shortcut(ScenarioBlockStyle::Type _forType) const
{
    return m_shortcutsManager->shortcut(_forType);
}

void ScenarioTextEdit::setAdditionalCursors(const QMap<QString, int>& _cursors)
{
    if (m_additionalCursors != _cursors) {
        //
        // Обновим позиции
        //
        QMutableMapIterator<QString, int> iter(m_additionalCursors);
        while (iter.hasNext()) {
            iter.next();

            const QString username = iter.key();
            const int cursorPosition = iter.value();

            //
            // Если такой пользователь есть, то возможно необходимо обновить значение
            //
            if (_cursors.contains(username)) {
                const int newCursorPosition = _cursors.value(username);
                if (cursorPosition != newCursorPosition) {
                    iter.setValue(newCursorPosition);
                    m_additionalCursorsCorrected.insert(username, newCursorPosition);
                }
            }
            //
            // Если нет, удаляем
            //
            else {
                iter.remove();
                m_additionalCursorsCorrected.remove(username);
            }
        }

        //
        // Добавим новых
        //
        foreach (const QString& username, _cursors.keys()) {
            if (!m_additionalCursors.contains(username)) {
                const int cursorPosition = _cursors.value(username);
                m_additionalCursors.insert(username, cursorPosition);
                m_additionalCursorsCorrected.insert(username, cursorPosition);
            }
        }
    }
}

void ScenarioTextEdit::scrollToAdditionalCursor(int _additionalCursorIndex)
{
    QTextCursor cursor(m_document);
    m_document->setCursorPosition(cursor, (m_additionalCursorsCorrected.begin() + _additionalCursorIndex).value());
    ensureCursorVisible(cursor);
}

void ScenarioTextEdit::setReviewContextMenuActions(const QList<QAction*>& _actions)
{
    if (m_reviewContextMenuActions != _actions) {
        m_reviewContextMenuActions = _actions;
    }
}

QMenu* ScenarioTextEdit::createContextMenu(const QPoint& _pos, QWidget* _parent)
{
    QMenu* menu = CompletableTextEdit::createContextMenu(_pos, _parent);

    //
    // Добавляем действия рецензирования в меню
    //
    if (!m_reviewContextMenuActions.isEmpty() && textCursor().hasSelection()) {
        QAction* firstAction = menu->actions().first();
        menu->insertActions(firstAction, m_reviewContextMenuActions);
        menu->insertSeparator(firstAction);
    }

    //
    // Добавляем возможность добавления закладки
    //
    if (!isReadOnly()) {
        const QTextCursor cursor = cursorForPosition(_pos);
        const QTextBlock block = cursor.block();
        const TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(block.userData());

        QAction* firstAction = menu->actions().first();

        const int blockPosition = block.position();
        if (blockInfo != nullptr
            && blockInfo->hasBookmark()) {
            QAction* removeBookmark = new QAction(tr("Remove bookmark"), menu);
            connect(removeBookmark, &QAction::triggered,
                    this, [this, blockPosition] { emit removeBookmarkRequested(blockPosition); });
            menu->insertAction(firstAction, removeBookmark);
        } else {
            QAction* addBookmark = new QAction(tr("Add bookmark"), menu);
            connect(addBookmark, &QAction::triggered,
                    this, [this, blockPosition] { emit addBookmarkRequested(blockPosition); });
            menu->insertAction(firstAction, addBookmark);
        }
        menu->insertSeparator(firstAction);
    }

    //
    // Добавляем возможность разделить страницу и обратно
    //
    if (!isReadOnly()) {
        const ScriptTextCursor cursor = cursorForPosition(_pos);
        QAction* splitPage = new QAction(tr("Split page"), menu);
        splitPage->setCheckable(true);
        splitPage->setProperty(kLastMousePosKey, _pos);
        if (cursor.isBlockInTable()) {
            splitPage->setChecked(true);
            connect(splitPage, &QAction::toggled, this, &ScenarioTextEdit::unsplitPage);
        } else {
            connect(splitPage, &QAction::toggled, this, &ScenarioTextEdit::splitPage);
        }
        menu->insertAction(menu->actions().first(), splitPage);
    }

    //
    // Добавляем в меню фозможности форматирования
    //
    if (!isReadOnly()
        && textCursor().hasSelection()) {
        QWidget* widget = new QWidget(menu);
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setSpacing(0);
        layout->setContentsMargins(QMargins());
        //
        FlatButton* bold = new FlatButton;
        bold->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        bold->setIcons(QIcon(":/Graphics/Iconset/format-bold.svg"));
        bold->setCheckable(true);
        bold->setChecked(textCursor().charFormat().font().bold());
        bold->setProperty("inContextMenu", true);
        bold->setProperty("firstInContextMenu", true);
        connect(bold, &FlatButton::toggled, this, &ScenarioTextEdit::setTextBold);
        layout->addWidget(bold);
        //
        FlatButton* italic = new FlatButton;
        italic->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        italic->setIcons(QIcon(":/Graphics/Iconset/format-italic.svg"));
        italic->setCheckable(true);
        italic->setChecked(textCursor().charFormat().font().italic());
        italic->setProperty("inContextMenu", true);
        connect(italic, &FlatButton::toggled, this, &ScenarioTextEdit::setTextItalic);
        layout->addWidget(italic);
        //
        FlatButton* underline = new FlatButton;
        underline->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        underline->setIcons(QIcon(":/Graphics/Iconset/format-underline.svg"));
        underline->setCheckable(true);
        underline->setChecked(textCursor().charFormat().font().underline());
        underline->setProperty("inContextMenu", true);
        underline->setProperty("lastInContextMenu", true);
        connect(underline, &FlatButton::toggled, this, &ScenarioTextEdit::setTextUnderline);
        layout->addWidget(underline);

        QWidgetAction* formattingActions = new QWidgetAction(menu);
        formattingActions->setDefaultWidget(widget);

        QAction* firstAction = menu->actions().first();
        menu->insertAction(firstAction, formattingActions);
        menu->insertSeparator(firstAction);
    }

    if (!isReadOnly()) {
        const QTextCursor cursor = cursorForPosition(_pos);

        //
        // Находим ближайший сверху блок, который является заголовком сцены
        //
        const SceneHeadingBlockInfo* blockInfo = nullptr;
        QTextBlock block = cursor.block();
        for (; block.isValid(); block = block.previous()) {
            if (ScenarioBlockStyle::forBlock(block) == ScenarioBlockStyle::SceneHeading) {
                blockInfo = dynamic_cast<SceneHeadingBlockInfo*>(block.userData());
                break;
            }
        }

        //
        // Если нашли и он зафиксирован, то даем пользователю возможность переименовать
        //
        if (blockInfo && blockInfo->isSceneNumberFixed()) {
            QAction* firstAction = menu->actions().first();
            QAction* renameSceneNumber = new QAction(tr("Change scene number"), menu);
            menu->insertAction(firstAction, renameSceneNumber);
            menu->insertSeparator(firstAction);
            const QString oldSceneNumber = blockInfo->sceneNumber();
            int position = block.position();
            connect(renameSceneNumber, &QAction::triggered, this, [this, oldSceneNumber, position] {
                emit renameSceneNumberRequested(oldSceneNumber, position);
            });
        }
    }

    return menu;
}

bool ScenarioTextEdit::isRedoAvailable() const
{
    return m_document != nullptr
                         && m_document->isRedoAvailableReimpl();
}

void ScenarioTextEdit::splitPage()
{
    //
    // Получим курсор для блока, который хочет разделить пользователь
    //
    const QPoint cursorPos = sender()->property(kLastMousePosKey).toPoint();
    QTextCursor cursor = cursorForPosition(cursorPos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();

    //
    // Сохраним текущий формат блока и вырежем его текст, или текст выделения
    //
    const ScenarioBlockStyle::Type lastBlockType = ScenarioBlockStyle::forBlock(cursor.block());
    if (!textCursor().hasSelection()) {
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }
    const QString mime = m_document->mimeFromSelection(textCursor().selectionStart(),
                                                       textCursor().selectionEnd());
    textCursor().removeSelectedText();

    //
    // Назначим блоку перед таблицей формат PageSplitter
    //
    changeScenarioBlockType(ScenarioBlockStyle::PageSplitter);
    //
    // ... и запретим позиционировать в нём курсор
    //
    auto pageSplitterBlockFormat = textCursor().blockFormat();
    pageSplitterBlockFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
    textCursor().setBlockFormat(pageSplitterBlockFormat);

    //
    // Вставляем таблицу
    //
    const auto scriptTemplate = ScenarioTemplateFacade::getTemplate();
    const qreal tableWidth = 100;
    const qreal leftColumnWidth = scriptTemplate.splitterLeftSidePercents();
    const qreal rightColumnWidth = tableWidth - leftColumnWidth;
    QTextTableFormat tableFormat;
    tableFormat.setWidth(QTextLength{QTextLength::PercentageLength, tableWidth});
    tableFormat.setColumnWidthConstraints({ QTextLength{QTextLength::PercentageLength, leftColumnWidth},
                                            QTextLength{QTextLength::PercentageLength, rightColumnWidth} });
//    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    cursor.insertTable(1, 2, tableFormat);

    //
    // Назначим блоку после таблицы формат PageSplitter
    //
    changeScenarioBlockType(ScenarioBlockStyle::PageSplitter);
    //
    // ... и запретим позиционировать в нём курсор
    //
    textCursor().setBlockFormat(pageSplitterBlockFormat);

    //
    // Вставляем параграф после таблицы
    //
    addScenarioBlock(lastBlockType);
    moveCursor(QTextCursor::PreviousBlock);

    //
    // Применяем сохранённый формат блока каждой из колонок
    //
    moveCursor(QTextCursor::PreviousCharacter);
    changeScenarioBlockType(lastBlockType);
    moveCursor(QTextCursor::PreviousCharacter);
    changeScenarioBlockType(lastBlockType);

    //
    // Вставляем текст в первую колонку
    //
    m_document->insertFromMime(textCursor().position(), mime);

    cursor.endEditBlock();
}

void ScenarioTextEdit::unsplitPage()
{
    //
    // Получим курсор для блока, из которого пользователь хочет убрать разделение
    //
    const QPoint cursorPos = sender()->property(kLastMousePosKey).toPoint();
    ScriptTextCursor cursor = cursorForPosition(cursorPos);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();

    //
    // Идём до начала таблицы
    //
    while (ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::PageSplitter) {
        cursor.movePosition(QTextCursor::PreviousBlock);
    }

    //
    // Выделяем и сохраняем текст из первой ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable* table = cursor.currentTable();
    while (table->cellAt(cursor).column() == 0) {
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    const QString firstColumnData =
            cursor.selectedText().isEmpty()
            ? QString()
            : m_document->mimeFromSelection(cursor.selectionStart(), cursor.selectionEnd());
    cursor.removeSelectedText();

    //
    // Выделяем и сохраняем текст из второй ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor));
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    const QString secondColumnData =
            cursor.selectedText().isEmpty()
            ? QString()
            : m_document->mimeFromSelection(cursor.selectionStart(), cursor.selectionEnd());
    cursor.removeSelectedText();

    //
    // Удаляем таблицу
    // Делается это только таким костылём, как удалить таблицу по-человечески я не нашёл...
    //
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.deletePreviousChar();

    //
    // Вставляем текст из удалённых ячеек
    //
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    const int insertPosition = cursor.position();
    if (!secondColumnData.isEmpty()) {
        cursor.insertBlock();
        m_document->insertFromMime(cursor.position(), secondColumnData);
        cursor.setPosition(insertPosition);
    }
    if (!firstColumnData.isEmpty()) {
        cursor.insertBlock();
        m_document->insertFromMime(cursor.position(), firstColumnData);
    }

    cursor.endEditBlock();
}

void ScenarioTextEdit::setTextBold(bool _bold)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setProperty(ScenarioBlockStyle::PropertyIsFormatting, true);
    format.setFontWeight(_bold ? QFont::Bold : QFont::Normal);
    cursor.mergeCharFormat(format);
}

void ScenarioTextEdit::setTextItalic(bool _italic)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setProperty(ScenarioBlockStyle::PropertyIsFormatting, true);
    format.setFontItalic(_italic);
    cursor.mergeCharFormat(format);
}

void ScenarioTextEdit::setTextUnderline(bool _underline)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QTextCharFormat format = cursor.charFormat();
    format.setProperty(ScenarioBlockStyle::PropertyIsFormatting, true);
    format.setFontUnderline(_underline);
    cursor.mergeCharFormat(format);
}

void ScenarioTextEdit::keyPressEvent(QKeyEvent* _event)
{
#ifndef MOBILE_OS
    //
    // Музицируем
    //
    if (m_keyboardSoundEnabled) {
        static QSound s_returnSound(":/Audio/Sound/return.wav");
        static QSound s_spaceSound(":/Audio/Sound/space.wav");
        static QSound s_deleteSound(":/Audio/Sound/backspace.wav");
        static QSound s_key1Sound(":/Audio/Sound/key-01.wav");
        static QSound s_key2Sound(":/Audio/Sound/key-02.wav");
        static QSound s_key3Sound(":/Audio/Sound/key-03.wav");
        static QSound s_key4Sound(":/Audio/Sound/key-04.wav");
        static QVector<QSound*> s_keySounds = { &s_key1Sound, &s_key2Sound, &s_key3Sound, &s_key4Sound };
        switch (_event->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter: {
                s_returnSound.play();
                break;
            }

            case Qt::Key_Space: {
                s_spaceSound.play();
                break;
            }

            case Qt::Key_Backspace:
            case Qt::Key_Delete: {
                s_deleteSound.play();
                break;
            }

            default: {
                if (!_event->text().isEmpty()) {
                    const int firstSoundId = 0;
                    const int maxSoundId = 3;
                    static int lastSoundId = firstSoundId;
                    if (lastSoundId > maxSoundId) {
                        lastSoundId = firstSoundId;
                    }
                    s_keySounds[lastSoundId++]->play();
                }
                break;
            }
        }
    }
#endif

#ifdef MOBILE_OS
    //
    // Не перехватываем событие кнопки назад в мобильных приложениях
    //
    if (_event->key() == Qt::Key_Back) {
        _event->ignore();
        return;
    }
#endif

    //
    // Отмену и повтор последнего действия, делаем без последующей обработки
    //
    // Если так не делать, то это приведёт к вставке странных символов, которые непонятно откуда берутся :(
    // Например:
    // 1. после реплики идёт время и место
    // 2. вставляем после реплики описание действия
    // 3. отменяем последнее действие
    // 4. в последующем времени и месте появляется символ "кружочек со стрелочкой"
    //
    // FIXME: разобраться
    //
    if (_event == QKeySequence::Undo
        || _event == QKeySequence::Redo) {
        if (_event == QKeySequence::Undo) {
            emit undoRequest();
        }
        else if (_event == QKeySequence::Redo) {
            emit redoRequest();
        }
        _event->accept();
        return;
    }

    //
    // Получим обработчик
    //
    KeyProcessingLayer::KeyPressHandlerFacade* handler =
            KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

    //
    // Получим курсор в текущем положении
    // Начнём блок операций
    //
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Подготовка к обработке
    //
    handler->prepare(_event);

    //
    // Предварительная обработка
    //
    handler->prepareForHandle(_event);

    //
    // Отправить событие в базовый класс
    //
    if (handler->needSendEventToBaseClass()) {
        if (!keyPressEventReimpl(_event)) {
            CompletableTextEdit::keyPressEvent(_event);
        }

        updateEnteredText(_event->text());

        TextEditHelper::beautifyDocument(textCursor(), _event->text(), m_replaceThreeDots, m_smartQuotes);
    }

    //
    // Обработка
    //
    handler->handle(_event);

    //
    // Событие дошло по назначению
    //
    _event->accept();

    //
    // Завершим блок операций
    //
    cursor.endEditBlock();

    //
    // Убедимся, что курсор виден
    //
    if (handler->needEnsureCursorVisible()) {
        ensureCursorVisible();
    }

    //
    // Подготовим следующий блок к обработке
    //
    if (handler->needPrehandle()) {
        handler->prehandle();
    }
}

void ScenarioTextEdit::inputMethodEvent(QInputMethodEvent* _event)
{
    //
    // Закроем подстановщика, перед событием
    //
    closeCompleter();

    //
    // Имитируем события нажатия клавиш Enter & Backspace
    //
    Qt::Key pressedKey = Qt::Key_unknown;
    QString eventText = _event->commitString();
    if (eventText == "\n") {
        pressedKey = Qt::Key_Return;
        eventText.clear();
    } else if (eventText == "\t") {
        pressedKey = Qt::Key_Tab;
        eventText.clear();
    } else if (_event->replacementLength() == 1
               && _event->replacementStart() < 0) {
        pressedKey = Qt::Key_Backspace;
    }
    //
    // ... если определилсь, что будем имитировать, имитируем
    //
    if (pressedKey != Qt::Key_unknown) {
        QKeyEvent keyEvent(QKeyEvent::KeyPress, pressedKey, Qt::NoModifier, eventText);
        keyPressEvent(&keyEvent);
        _event->accept();
        return;
    }

    //
    // Вызываем стандартный обработчик
    //
    CompletableTextEdit::inputMethodEvent(_event);

    //
    // Получим обработчик
    //
    KeyProcessingLayer::KeyPressHandlerFacade* handler =
            KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

    //
    // Получим курсор в текущем положении
    // Начнём блок операций
    //
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Если был введён какой-либо текст, скорректируем его
    //
    if (!_event->commitString().isEmpty()) {
        updateEnteredText(_event->commitString());
        TextEditHelper::beautifyDocument(textCursor(), _event->commitString(), m_replaceThreeDots, m_smartQuotes);
    }

    //
    // Обработка
    //
    handler->handle(_event);

    //
    // Завершим блок операций
    //
    cursor.endEditBlock();
}

bool ScenarioTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;
    //
    // Переопределяем
    //
    // ... перевод курсора к следующему символу
    //
    if (_event == QKeySequence::MoveToNextChar) {
        moveCursor(QTextCursor::NextCharacter);
        while (!textCursor().atEnd()
               && (!textCursor().block().isVisible()
                   || ScenarioBlockStyle::forBlock(textCursor().block()) == ScenarioBlockStyle::PageSplitter
                   || textCursor().blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::NextBlock);
        }
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        moveCursor(QTextCursor::PreviousCharacter);
        while (!textCursor().atStart()
               && (!textCursor().block().isVisible()
                   || ScenarioBlockStyle::forBlock(textCursor().block()) == ScenarioBlockStyle::PageSplitter
                   || textCursor().blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::StartOfBlock);
            moveCursor(QTextCursor::PreviousCharacter);
        }
    }
    //
    // ... перевод курсора к концу строки
    //
    else if (_event == QKeySequence::MoveToEndOfLine
             || _event == QKeySequence::SelectEndOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor =
            _event->modifiers().testFlag(Qt::ShiftModifier)
                ? QTextCursor::KeepAnchor
                : QTextCursor::MoveAnchor;
        while (!cursor.atBlockEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
            if (cursorRect(cursor).y() > startY) {
                cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
    //
    // ... перевод курсора к началу строки
    //
    else if (_event == QKeySequence::MoveToStartOfLine
             || _event == QKeySequence::SelectStartOfLine) {
        QTextCursor cursor = textCursor();
        const int startY = cursorRect(cursor).y();
        const QTextCursor::MoveMode keepAncor =
            _event->modifiers().testFlag(Qt::ShiftModifier)
                ? QTextCursor::KeepAnchor
                : QTextCursor::MoveAnchor;
        while (!cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, keepAncor);
            if (cursorRect(cursor).y() < startY) {
                cursor.movePosition(QTextCursor::NextCharacter, keepAncor);
                setTextCursor(cursor);
                break;
            }
        }
        setTextCursor(cursor);
    }
    //
    // Поднятие/опускание регистра букв
    // Работает в три шага:
    // 1. ВСЕ ЗАГЛАВНЫЕ
    // 2. Первая заглавная
    // 3. все строчные
    //
    else if (_event->modifiers().testFlag(Qt::ControlModifier)
             && (_event->key() == Qt::Key_Up
                 || _event->key() == Qt::Key_Down)) {
        //
        // Нужно ли убирать выделение после операции
        //
        bool clearSelection = false;
        //
        // Если выделения нет, работаем со словом под курсором
        //
        QTextCursor cursor = textCursor();
        const int sourcePosition = cursor.position();
        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
            clearSelection = true;
        }

        const bool toUpper = _event->key() == Qt::Key_Up;
        const QString selectedText = cursor.selectedText();
        const QChar firstChar = selectedText.at(0);
        const bool firstToUpper = TextEditHelper::smartToUpper(firstChar) != firstChar;
        const bool textInUpper = (selectedText.length() > 1) && (TextEditHelper::smartToUpper(selectedText) == selectedText);
        const int fromPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
        const int toPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
        for (int position = fromPosition; position < toPosition; ++position) {
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            if (toUpper) {
                if (firstToUpper) {
                    cursor.insertText(position == fromPosition ? TextEditHelper::smartToUpper(cursor.selectedText()) : cursor.selectedText().toLower());
                } else {
                    cursor.insertText(TextEditHelper::smartToUpper(cursor.selectedText()));
                }
            } else {
                if (textInUpper) {
                    cursor.insertText(position == fromPosition ? TextEditHelper::smartToUpper(cursor.selectedText()) : cursor.selectedText().toLower());
                } else {
                    cursor.insertText(cursor.selectedText().toLower());
                }
            }
        }

        if (clearSelection) {
            cursor.setPosition(sourcePosition);
        } else {
            cursor.setPosition(fromPosition);
            cursor.setPosition(toPosition, QTextCursor::KeepAnchor);
        }
        setTextCursor(cursor);
    }
    //
    // Делаем собственную обработку операции вставить, т.к. стандартная всегда
    // уводит полосу прокрутки в начало
    //
    else if (_event == QKeySequence::Paste) {
        const int lastVBarValue = verticalScrollBar()->value();
        paste();
        verticalScrollBar()->setValue(lastVBarValue);
    }
    //
    // Сделать текст полужирным
    //
    else if (_event == QKeySequence::Bold) {
        setTextBold(!textCursor().charFormat().font().bold());
    }
    //
    // Сделать текст курсивом
    //
    else if (_event == QKeySequence::Italic) {
        setTextItalic(!textCursor().charFormat().font().italic());
    }
    //
    // Сделать текст подчёркнутым
    //
    else if (_event == QKeySequence::Underline) {
        setTextUnderline(!textCursor().charFormat().font().underline());
    }
#ifdef Q_OS_MAC
    //
    // Особая комбинация для вставки точки независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && _event->key() == Qt::Key_Period) {
        insertPlainText(".");
    }
    //
    // Особая комбинация для вставки запятой независимо от раскладки
    //
    else if (_event->modifiers().testFlag(Qt::MetaModifier)
             && _event->modifiers().testFlag(Qt::AltModifier)
             && _event->key() == Qt::Key_Comma) {
        insertPlainText(",");
    }
#endif
    else {
        isEventHandled = false;
    }

    return isEventHandled;
}

void ScenarioTextEdit::paintEvent(QPaintEvent* _event)
{
    //
    // Определить область прорисовки слева от текста
    //
    const int pageLeft = 0;
    const int pageRight = viewport()->width();
    const int spaceBetweenSceneNumberAndText = 10;
    const int textLeft = pageLeft
                         - (QLocale().textDirection() == Qt::LeftToRight ? 0 : horizontalScrollBar()->maximum())
                         + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const int textRight = pageRight
                          + (QLocale().textDirection() == Qt::LeftToRight ? horizontalScrollBar()->maximum() : 0)
                          - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const int leftDelta = (QLocale().textDirection() == Qt::LeftToRight ? -1 : 1) * horizontalScrollBar()->value();
    int colorRectWidth = 0;
    int verticalMargin = 0;



    //
    // Подсветка блоков и строки, если нужно
    //
    {
        QPainter painter(viewport());
        if (m_highlightBlocks) {
            const int opacity = 33;

            //
            // Если курсор в блоке реплики, закрасить примыкающие блоки диалогов
            //
            {
                const QVector<ScenarioBlockStyle::Type> dialogueTypes = { ScenarioBlockStyle::Character,
                                                                          ScenarioBlockStyle::Parenthetical,
                                                                          ScenarioBlockStyle::Dialogue,
                                                                          ScenarioBlockStyle::Lyrics};
                if (dialogueTypes.contains(scenarioBlockType())) {
                    //
                    // Идём до самого начала блоков диалогов
                    //
                    QTextCursor cursor = textCursor();
                    while (cursor.movePosition(QTextCursor::PreviousBlock)) {
                        if (!dialogueTypes.contains(ScenarioBlockStyle::forBlock(cursor.block()))) {
                            cursor.movePosition(QTextCursor::NextBlock);
                            break;
                        }
                    }

                    //
                    // Проходим каждый из блоков по-очереди
                    //
                    do {
                        const QRect topCursorRect = cursorRect(cursor);
                        const QString characterName = BusinessLogic::CharacterParser::name(cursor.block().text());
                        const Domain::Research* character = DataStorageLayer::StorageFacade::researchStorage()->character(characterName);
                        const QColor characterColor = character == nullptr ? QColor() : character->color();

                        //
                        // Идём до конца блока диалога
                        //
                        while (cursor.movePosition(QTextCursor::NextBlock)) {
                            const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
                            //
                            // Если дошли до персонажа, или до конца диалога, возвращаемся на блок назад и раскрашиваем
                            //
                            if (blockType == ScenarioBlockStyle::Character
                                || !dialogueTypes.contains(blockType)) {
                                cursor.movePosition(QTextCursor::StartOfBlock);
                                cursor.movePosition(QTextCursor::PreviousCharacter);
                                break;
                            }
                        }
                        const QRect bottomCursorRect = cursorRect(cursor);
                        cursor.movePosition(QTextCursor::NextBlock);

                        if (characterName.isEmpty()
                            && !characterColor.isValid()) {
                            continue;
                        }

                        //
                        // Раскрашиваем
                        //
                        verticalMargin = topCursorRect.height() / 2;
                        QColor noizeColor = textColor();
                        if (characterColor.isValid()) {
                            noizeColor = characterColor;
                        }
                        noizeColor.setAlpha(opacity);
                        const QRect noizeRect(QPoint(textLeft + leftDelta, topCursorRect.top() - verticalMargin),
                                              QPoint(textRight + leftDelta, bottomCursorRect.bottom() + verticalMargin));
                        painter.fillRect(noizeRect, noizeColor);
                    } while(!cursor.atEnd()
                            && dialogueTypes.contains(ScenarioBlockStyle::forBlock(cursor.block())));
                }
            }

            //
            // Закрасить блок сцены/папки
            //
            {
                //
                // Идём до начала блока сцены, считая по пути вложенные папки
                //
                QTextCursor cursor = textCursor();
                int closedFolders = 0;
                bool isScene = scenarioBlockType() == ScenarioBlockStyle::SceneHeading;
                if (scenarioBlockType() != ScenarioBlockStyle::SceneHeading
                    && scenarioBlockType() != ScenarioBlockStyle::FolderHeader) {
                    while (cursor.movePosition(QTextCursor::PreviousBlock)) {
                        const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
                        if (blockType == ScenarioBlockStyle::SceneHeading) {
                            isScene = true;
                            break;
                        } else if (blockType == ScenarioBlockStyle::FolderFooter) {
                            ++closedFolders;
                        } else if (blockType == ScenarioBlockStyle::FolderHeader) {
                            if (closedFolders > 0) {
                                --closedFolders;
                            } else {
                                break;
                            }
                        }
                    }
                }
                const QRect topCursorRect = cursorRect(cursor);
                QColor noizeColor = textColor();
                if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(cursor.block().userData())) {
                    if (!info->colors().isEmpty()) {
                        noizeColor = QColor(info->colors().split(";").first());
                    }
                }
                noizeColor.setAlpha(opacity);

                //
                // Идём до конца блока сцены, считая по пути вложенные папки
                //
                cursor = textCursor();
                int openedFolders = 0;
                while (cursor.movePosition(QTextCursor::NextBlock)) {
                    const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
                    if (isScene
                        && blockType == ScenarioBlockStyle::SceneHeading) {
                        cursor.movePosition(QTextCursor::PreviousBlock);
                        break;
                    } else if (blockType == ScenarioBlockStyle::FolderHeader) {
                        ++openedFolders;
                    } else if (blockType == ScenarioBlockStyle::FolderFooter) {
                        if (openedFolders > 0) {
                            --openedFolders;
                        } else {
                            if (isScene) {
                                cursor.movePosition(QTextCursor::PreviousBlock);
                            }
                            break;
                        }
                    }
                }
                cursor.movePosition(QTextCursor::EndOfBlock);
                const QRect bottomCursorRect = cursorRect(cursor);

                verticalMargin = topCursorRect.height() / 2;
                colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");

                //
                // Закрасим область сцены
                //
                const QRect noizeRect(QPoint(pageLeft, topCursorRect.top() - verticalMargin),
                                      QPoint(pageRight, bottomCursorRect.bottom() + verticalMargin));
                painter.fillRect(noizeRect, noizeColor);
            }
        }

        //
        // Подсветка строки
        //
        if (m_highlightCurrentLine) {
            const QRect cursorR = cursorRect();
            const QRect highlightRect(0, cursorR.top(), viewport()->width(), cursorR.height());
            QColor lineColor = palette().highlight().color().lighter();
            lineColor.setAlpha(40);
            painter.fillRect(highlightRect, lineColor);
        }
    }

    CompletableTextEdit::paintEvent(_event);

    //
    // Прорисовка дополнительных элементов редактора
    //
    {
        //
        // Декорации слева от текста
        //
        {
            QPainter painter(viewport());
            clipPageDecorationRegions(&painter);

            //
            // Определим начальный и конечный блоки на экране
            //
            QTextBlock topBlock = document()->lastBlock();
            {
                QTextCursor topCursor;
                for (int delta = 0 ; delta < viewport()->height()/4; delta += 10) {
                    topCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
                    if (topCursor.block().isVisible()
                        && topBlock.blockNumber() > topCursor.block().blockNumber()) {
                        topBlock = topCursor.block();
                    }
                }
            }
            //
            // ... идём до начала сцены
            //
            while (ScenarioBlockStyle::forBlock(topBlock) != ScenarioBlockStyle::SceneHeading
                   && ScenarioBlockStyle::forBlock(topBlock) != ScenarioBlockStyle::FolderHeader
                   && topBlock != document()->firstBlock()) {
                topBlock = topBlock.previous();
            }
            //
            QTextBlock bottomBlock = document()->firstBlock();
            {
                QTextCursor bottomCursor;
                for (int delta = viewport()->height() ; delta > viewport()->height()*3/4; delta -= 10) {
                    bottomCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
                    if (bottomCursor.block().isVisible()
                        && bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
                        bottomBlock = bottomCursor.block();
                    }
                }
            }
            if (bottomBlock == document()->firstBlock()) {
                bottomBlock = document()->lastBlock();
            }
            bottomBlock = bottomBlock.next();

            //
            // Проходим блоки на экране и декорируем их
            //
            QTextBlock block = topBlock;
            const QRectF viewportGeometry = viewport()->geometry();
            int lastSceneBlockBottom = 0;
            QColor lastSceneColor;
            int lastCharacterBlockBottom = 0;
            QColor lastCharacterColor;

            QTextCursor cursor(document());
            while (block.isValid() && block != bottomBlock) {
                //
                // Стиль текущего блока
                //
                const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);

                if (block.isVisible()) {
                    cursor.setPosition(block.position());
                    const QRect cursorR = cursorRect(cursor);
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    const QRect cursorREnd = cursorRect(cursor);

                    //
                    // Определим цвет сцены
                    //
                    if (blockType == ScenarioBlockStyle::SceneHeading
                        || blockType == ScenarioBlockStyle::FolderHeader) {
                        lastSceneBlockBottom = cursorR.top();
                        verticalMargin = cursorR.height() / 2;
                        colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");
                        lastSceneColor = QColor();
                        if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(block.userData())) {
                            if (!info->colors().isEmpty()) {
                                lastSceneColor = QColor(info->colors().split(";").first());
                            }
                        }
                    }

                    //
                    // Нарисуем цвет сцены
                    //
                    if (lastSceneColor.isValid()) {
                        const QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                        ? textRight + leftDelta
                                        : textLeft - colorRectWidth + leftDelta,
                                        lastSceneBlockBottom - verticalMargin);
                        const QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                            ? textRight + colorRectWidth + leftDelta
                                            : textLeft + leftDelta,
                                            cursorREnd.bottom() + verticalMargin);
                        const QRectF rect(topLeft, bottomRight);
                        painter.fillRect(rect, lastSceneColor);
                    }

                    //
                    // Определим цвет персонажа
                    //
                    if (blockType == ScenarioBlockStyle::Character) {
                        lastCharacterBlockBottom = cursorR.top();
                        verticalMargin = cursorR.height() / 2;
                        colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");
                        lastCharacterColor = QColor();
                        const QString characterName = BusinessLogic::CharacterParser::name(block.text());
                        if (auto character = DataStorageLayer::StorageFacade::researchStorage()->character(characterName)) {
                            if (character->color().isValid()) {
                                lastCharacterColor = character->color();
                            }
                        }
                    } else if (blockType != ScenarioBlockStyle::Parenthetical
                               && blockType != ScenarioBlockStyle::Dialogue
                               && blockType != ScenarioBlockStyle::Lyrics) {
                        lastCharacterColor = QColor();
                    }

                    //
                    // Нарисуем цвет персонажа
                    //
                    if (lastCharacterColor.isValid()) {
                        const QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                        ? textLeft - colorRectWidth + leftDelta
                                        : textRight + leftDelta,
                                        lastCharacterBlockBottom - verticalMargin);
                        const QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                            ? textLeft + leftDelta
                                            : textRight + colorRectWidth + leftDelta,
                                            cursorREnd.bottom() + verticalMargin);
                        const QRectF rect(topLeft, bottomRight);
                        painter.fillRect(rect, lastCharacterColor);
                    }

                    //
                    // Курсор на экране
                    //
                    // ... ниже верхней границы
                    if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                        // ... и выше нижней
                        && cursorR.top() < viewportGeometry.bottom()) {

                        //
                        // Прорисовка закладок
                        //
                        TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(block.userData());
                        if (blockInfo != nullptr
                            && blockInfo->hasBookmark()) {
                            //
                            // Определим область для отрисовки и выведем номер сцены в редактор
                            //
                            QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                            ? pageLeft + leftDelta
                                            : textRight + leftDelta,
                                            cursorR.top());
                            QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                                ? textLeft + leftDelta
                                                : pageRight + leftDelta,
                                                cursorR.bottom());
                            QRectF rect(topLeft, bottomRight);
                            painter.setBrush(blockInfo->bookmarkColor());
                            painter.setPen(Qt::transparent);
                            painter.drawRect(rect);
                            painter.setPen(Qt::white);
                        } else {
                            painter.setPen(palette().text().color());
                        }

                        //
                        // Прорисовка символа пустой строки
                        //
                        if (!block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                            && blockType != ScenarioBlockStyle::PageSplitter
                            && block.text().simplified().isEmpty()) {
                            //
                            // Определим область для отрисовки и выведем символ в редактор
                            //
                            QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                            ? pageLeft + leftDelta
                                            : textRight + leftDelta,
                                            cursorR.top());
                            QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                                ? textLeft + leftDelta
                                                : pageRight + leftDelta,
                                                cursorR.bottom() + 2);
                            QRectF rect(topLeft, bottomRight);
                            painter.setFont(cursor.charFormat().font());
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, "» ");
                        }
                        //
                        // Прорисовка номера для строки
                        //
                        else {
                            //
                            // Прорисовка номеров сцен, если необходимо
                            //
                            if (m_showSceneNumbers
                                && blockType == ScenarioBlockStyle::SceneHeading) {
                                //
                                // Определим номер сцены
                                //
                                if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(blockInfo)) {
                                    const QString sceneNumber = m_sceneNumbersPrefix + info->sceneNumber() + ".";

                                    //
                                    // Определим область для отрисовки и выведем номер сцены в редактор
                                    //
                                    QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                                    ? pageLeft + leftDelta
                                                    : textRight + leftDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                                        ? textLeft + leftDelta
                                                        : pageRight + leftDelta,
                                                        cursorR.bottom());
                                    QRectF rect(topLeft, bottomRight);
                                    painter.setFont(cursor.charFormat().font());
                                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, sceneNumber);
                                }
                            }
                            //
                            // Прорисовка номеров реплик, если необходимо
                            //
                            if (m_showDialoguesNumbers
                                && blockType == ScenarioBlockStyle::Character
                                && (!block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
                                    || block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter))) {
                                //
                                // Определим номер реплики
                                //
                                if (CharacterBlockInfo* info = dynamic_cast<CharacterBlockInfo*>(blockInfo)) {
                                    const QString dialogueNumber = QString::number(info->dialogueNumbder()) + ":";

                                    //
                                    // Определим область для отрисовки и выведем номер реплики в редактор
                                    //
                                    painter.setFont(cursor.charFormat().font());
                                    const int numberDelta = painter.fontMetrics().width(dialogueNumber);
                                    QRectF rect;
                                    //
                                    // Если имя персонажа находится не с самого края листа
                                    //
                                    if (block.blockFormat().leftMargin() > numberDelta) {
                                        //
                                        // ... то поместим номер реплики внутри текстовой области,
                                        //     чтобы их было удобно отличать от номеров сцен
                                        //
                                        QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText - numberDelta,
                                                        cursorR.top());
                                        QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                                            ? textLeft + leftDelta + spaceBetweenSceneNumberAndText + numberDelta
                                                            : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                                            cursorR.bottom());
                                        rect = QRectF(topLeft, bottomRight);
                                    }
                                    //
                                    // В противном же случае
                                    //
                                    else {
                                        //
                                        // ... позиционируем номера реплик на полях, так же как и номера сцен
                                        //
                                        QPointF topLeft(QLocale().textDirection() == Qt::LeftToRight
                                                        ? pageLeft + leftDelta
                                                        : textRight + leftDelta,
                                                        cursorR.top());
                                        QPointF bottomRight(QLocale().textDirection() == Qt::LeftToRight
                                                            ? textLeft + leftDelta
                                                            : pageRight + leftDelta,
                                                            cursorR.bottom());
                                        rect = QRectF(topLeft, bottomRight);
                                    }
                                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
                                }
                            }

                            //
                            // Прорисовка автоматических (ПРОД) для реплик
                            //
                            if (blockType == ScenarioBlockStyle::Character
                                && block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCharacterContinued)
                                && !block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)) {
                                painter.setFont(cursor.charFormat().font());

                                //
                                // Определим место положение конца имени персонажа
                                //
                                const int continuedTermWidth = painter.fontMetrics().width(ScriptTextCorrector::continuedTerm());
                                const QPoint topLeft = QLocale().textDirection() == Qt::LeftToRight
                                                       ? cursorREnd.topLeft()
                                                       : cursorREnd.topRight() - QPoint(continuedTermWidth, 0);
                                const QPoint bottomRight = QLocale().textDirection() == Qt::LeftToRight
                                                           ? cursorREnd.bottomRight() + QPoint(continuedTermWidth, 0)
                                                           : cursorREnd.bottomLeft();
                                const QRect rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, ScriptTextCorrector::continuedTerm());
                            }
                        }
                    }

                    lastSceneBlockBottom = cursorREnd.bottom();
                }

                block = block.next();
            }
        }

        //
        // Курсоры соавторов
        //
        {
            //
            // Ширина области курсора, для отображения имени автора курсора
            //
            const unsigned cursorAreaWidth = 20;

            if (!m_additionalCursors.isEmpty()
                && m_document != nullptr) {
                QPainter painter(viewport());
                painter.setFont(QFont("Sans", 8));
                painter.setPen(Qt::white);

                const QRectF viewportGeometry = viewport()->geometry();
                QPoint mouseCursorPos = mapFromGlobal(QCursor::pos());
                mouseCursorPos.setY(mouseCursorPos.y() + viewport()->mapFromParent(QPoint(0,0)).y());
                int cursorIndex = 0;
                foreach (const QString& username, m_additionalCursorsCorrected.keys()) {
                    QTextCursor cursor(m_document);
                    m_document->setCursorPosition(cursor, m_additionalCursorsCorrected.value(username));
                    const QRect cursorR = cursorRect(cursor);

                    //
                    // Если курсор на экране
                    //
                    // ... ниже верхней границы
                    if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                        // ... и выше нижней
                        && cursorR.top() < viewportGeometry.bottom()) {
                        //
                        // ... рисуем его
                        //
                        painter.fillRect(cursorR, ColorHelper::cursorColor(cursorIndex));

                        //
                        // ... декорируем
                        //
                        {
                            //
                            // Если мышь около него, то выводим имя соавтора
                            //
                            QRect extandedCursorR = cursorR;
                            extandedCursorR.setLeft(extandedCursorR.left() - cursorAreaWidth/2);
                            extandedCursorR.setWidth(cursorAreaWidth);
                            if (extandedCursorR.contains(mouseCursorPos)) {
                                const QRect usernameRect(
                                    cursorR.left() - 1,
                                    cursorR.top() - painter.fontMetrics().height() - 2,
                                    painter.fontMetrics().width(username) + 2,
                                    painter.fontMetrics().height() + 2);
                                painter.fillRect(usernameRect, ColorHelper::cursorColor(cursorIndex));
                                painter.drawText(usernameRect, Qt::AlignCenter, username);
                            }
                            //
                            // Если нет, то рисуем небольшой квадратик
                            //
                            else {
                                painter.fillRect(cursorR.left() - 2, cursorR.top() - 5, 5, 5,
                                    ColorHelper::cursorColor(cursorIndex));
                            }
                        }
                    }

                    ++cursorIndex;
                }
            }
        }
    }
}

void ScenarioTextEdit::mousePressEvent(QMouseEvent* _event)
{
    if (!selectBlockOnTripleClick(_event)) {
        CompletableTextEdit::mousePressEvent(_event);
    }
}

void ScenarioTextEdit::mouseDoubleClickEvent(QMouseEvent* _event)
{
    if (!selectBlockOnTripleClick(_event)) {
        CompletableTextEdit::mouseDoubleClickEvent(_event);
    }
}

bool ScenarioTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    bool canInsert = false;
    if (_source->formats().contains(ScenarioDocument::MIME_TYPE)
        || _source->hasText()) {
        canInsert = true;
    }
    return canInsert;
}

QMimeData* ScenarioTextEdit::createMimeDataFromSelection() const
{
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("text/plain", textCursor().selection().toPlainText().toUtf8());

    //
    // Поместим в буфер данные о тексте в специальном формате
    //
    {
        QTextCursor cursor = textCursor();
        cursor.setPosition(textCursor().selectionStart());
        cursor.setPosition(textCursor().selectionEnd());

        mimeData->setData(
                    ScenarioDocument::MIME_TYPE,
                    m_document->mimeFromSelection(textCursor().selectionStart(),
                                                  textCursor().selectionEnd()).toUtf8());
    }

    return mimeData;
}

void ScenarioTextEdit::insertFromMimeData(const QMimeData* _source)
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Если есть выделение, удаляем выделенный текст
    //
    if (cursor.hasSelection()) {
        //
        // Запомним стиль блока начала выделения
        //
        QTextCursor helper = cursor;
        helper.setPosition(cursor.selectionStart());
        ScenarioBlockStyle::Type type = ScenarioBlockStyle::forBlock(helper.block());
        cursor.deleteChar();
        changeScenarioBlockType(type);
    }

#ifndef MOBILE_OS
    QString textToInsert;

    //
    // Если вставляются данные в сценарном формате, то вставляем как положено
    //
    if (_source->formats().contains(ScenarioDocument::MIME_TYPE)) {
        textToInsert = _source->data(ScenarioDocument::MIME_TYPE);
    }
    //
    // Если простой текст, то вставляем его, импортировав с фонтана
    // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
    //       не воспринимался как титульная страница
    //
    else if (_source->hasText()) {
        FountainImporter fountainImporter;
        textToInsert = fountainImporter.importScript("\n" + _source->text());
    }

    m_document->insertFromMime(cursor.position(), textToInsert);
#else
    //
    // На мобилках мы можем оперировать только текстом
    //
    cursor.insertText(_source->text());
#endif

    cursor.endEditBlock();
}

bool ScenarioTextEdit::canComplete() const
{
    bool result = true;
    //
    // Если нельзя показывать в пустих блоках, проверяем не пуст ли блок
    //
    if (!m_showSuggestionsInEmptyBlocks) {
        result = textCursor().block().text().isEmpty() == false;
    }

    return result;
}

void ScenarioTextEdit::aboutCorrectAdditionalCursors(int _position, int _charsRemoved, int _charsAdded)
{
    if (_charsAdded != _charsRemoved) {
        foreach (const QString& username, m_additionalCursorsCorrected.keys()) {
            int additionalCursorPosition = m_additionalCursorsCorrected.value(username);
            if (additionalCursorPosition > _position) {
                additionalCursorPosition += _charsAdded - _charsRemoved;
                m_additionalCursorsCorrected[username] = additionalCursorPosition;
            }
        }
    }
}

void ScenarioTextEdit::aboutSelectionChanged()
{
    //
    // Если включена опция для редактора
    //

    //
    // Отобразить вспомогатаельную панель
    //
}

void ScenarioTextEdit::aboutSaveEditorState()
{
    setProperty(CURSOR_RECT, cursorRect());
}

void ScenarioTextEdit::aboutLoadEditorState()
{
    //
    // FIXME: задумано это для того, чтобы твой курсор не смещался вниз, если вверху текст редактирует соавтор,
    //		  но при отмене собственных изменений работает ужасно, поэтому пока отключим данный код.
    //		  Возможно это из-за того, что при отмене собственных изменений курсор не отправляется к
    //		  месту, где заканчивается изменение, а остаётся в своей позиции. Но тогда нужно как-то
    //		  разводить собственные патчи и патчи соавторов
    //
//	QApplication::processEvents();
//	const QRect prevCursorRect = property(CURSOR_RECT).toRect();
//	QRect currentCursorRect = cursorRect();

//	//
//	// Корректируем позицию курсора, пока
//	// - не восстановим предыдущее значение
//	// - не сдвинем прокрутку в самый верх
//	// - не сдвинем прокрутку в самый низ
//	//
//	while (prevCursorRect.y() != currentCursorRect.y()
//		   && verticalScrollBar()->value() != verticalScrollBar()->minimum()
//		   && verticalScrollBar()->value() != verticalScrollBar()->maximum()) {
//		int verticalDelta = 0;
//		if (prevCursorRect.y() < currentCursorRect.y()) {
//			verticalDelta = 1;
//		} else {
//			verticalDelta = -1;
//		}
//		verticalScrollBar()->setValue(verticalScrollBar()->value() + verticalDelta);
//		currentCursorRect = cursorRect();
//		qDebug() << cursorRect() << prevCursorRect;
//	}

}

void ScenarioTextEdit::cleanScenarioTypeFromBlock()
{
    QTextCursor cursor = textCursor();
    ScenarioBlockStyle oldBlockStyle = ScenarioTemplateFacade::getTemplate().blockStyle(scenarioBlockType());

    //
    // Удалить завершающий блок группы сцен
    //
    if (oldBlockStyle.isEmbeddableHeader()) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextBlock);

        // ... открытые группы на пути поиска необходимого для обновления блока
        int openedGroups = 0;
        bool isFooterUpdated = false;
        do {
            ScenarioBlockStyle::Type currentType =
                    ScenarioBlockStyle::forBlock(cursor.block());

            if (currentType == oldBlockStyle.embeddableFooter()) {
                if (openedGroups == 0) {
                    //
                    // Запомним стиль предыдущего блока
                    //
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    ScenarioBlockStyle::Type previousBlockType = ScenarioBlockStyle::forBlock(cursor.block());
                    cursor.movePosition(QTextCursor::NextBlock);
                    //
                    // Удаляем закрывающий блок
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.deleteChar();
                    cursor.deletePreviousChar();
                    //
                    // Восстановим стиль предыдущего блока
                    //
                    if (ScenarioBlockStyle::forBlock(cursor.block()) != previousBlockType) {
                        QTextCursor lastTextCursor = textCursor();
                        setTextCursor(cursor);
                        applyScenarioTypeToBlockText(previousBlockType);
                        setTextCursor(lastTextCursor);
                    }
                    isFooterUpdated = true;
                } else {
                    --openedGroups;
                }
            } else if (currentType == oldBlockStyle.type()) {
                // ... встретилась новая группа
                ++openedGroups;
            }
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        } while (!isFooterUpdated
                 && !cursor.atEnd());
    }

    //
    // Удалить заголовок если происходит смена стиля в текущем блоке
    // в противном случае его не нужно удалять
    //
    if (oldBlockStyle.hasHeader()) {
        QTextCursor headerCursor = cursor;
        headerCursor.movePosition(QTextCursor::StartOfBlock);
        headerCursor.movePosition(QTextCursor::PreviousCharacter);
        if (ScenarioBlockStyle::forBlock(headerCursor.block()) == oldBlockStyle.headerType()) {
            headerCursor.select(QTextCursor::BlockUnderCursor);
            headerCursor.deleteChar();

            //
            // Если находимся в самом начале документа, то необходимо удалить ещё один символ
            //
            if (headerCursor.position() == 0) {
                headerCursor.deleteChar();
            }
        }
    }

    //
    // Убрать декорации
    //
    if (oldBlockStyle.hasDecoration()) {
        QString blockText = cursor.block().text();
        //
        // ... префикс
        //
        if (blockText.startsWith(oldBlockStyle.prefix())) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            for (int repeats = 0; repeats < oldBlockStyle.prefix().length(); ++repeats) {
                cursor.deleteChar();
            }
        }

        //
        // ... постфикс
        //
        if (blockText.endsWith(oldBlockStyle.postfix())) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            for (int repeats = 0; repeats < oldBlockStyle.postfix().length(); ++repeats) {
                cursor.deletePreviousChar();
            }
        }
    }
}

void ScenarioTextEdit::applyScenarioTypeToBlock(ScenarioBlockStyle::Type _blockType)
{
    ScriptTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const ScenarioBlockStyle newBlockStyle = ScenarioTemplateFacade::getTemplate().blockStyle(_blockType);

    //
    // Обновим стили
    //
    cursor.setBlockCharFormat(newBlockStyle.charFormat());
    cursor.setBlockFormat(newBlockStyle.blockFormat(cursor.isBlockInTable()));

    //
    // Применим стиль текста ко всему блоку, выделив его,
    // т.к. в блоке могут находиться фрагменты в другом стиле
    // + сохраняем форматирование выделений
    //
    {
        cursor.movePosition(QTextCursor::StartOfBlock);

        //
        // Если в блоке есть выделения, обновляем цвет только тех частей, которые не входят в выделения
        //
        QTextBlock currentBlock = cursor.block();
        if (!currentBlock.textFormats().isEmpty()) {
            foreach (const QTextLayout::FormatRange& range, currentBlock.textFormats()) {
                if (!range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                    cursor.setPosition(currentBlock.position() + range.start);
                    cursor.setPosition(cursor.position() + range.length, QTextCursor::KeepAnchor);
                    cursor.mergeCharFormat(newBlockStyle.charFormat());
                }
            }
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
        //
        // Если выделений нет, обновляем блок целиком
        //
        else {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(newBlockStyle.charFormat());
        }

        cursor.clearSelection();
    }

    //
    // Вставим префикс и постфикс стиля, если необходимо
    //
    if (newBlockStyle.hasDecoration()) {
        //
        // Запомним позицию курсора в блоке
        //
        int cursorPosition = cursor.position();

        QString blockText = cursor.block().text();
        if (!newBlockStyle.prefix().isEmpty()
            && !blockText.startsWith(newBlockStyle.prefix())) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.insertText(newBlockStyle.prefix());

            cursorPosition += newBlockStyle.prefix().length();
        }
        if (!newBlockStyle.postfix().isEmpty()
            && !blockText.endsWith(newBlockStyle.postfix())) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.insertText(newBlockStyle.postfix());
        }

        cursor.setPosition(cursorPosition);
        setTextCursorReimpl(cursor);
    }

    //
    // Принудительно обновим ревизию блока
    //
    ScenarioTextDocument::updateBlockRevision(cursor);

    //
    // Вставим заголовок, если необходимо
    //
    if (newBlockStyle.hasHeader()) {
        ScenarioBlockStyle headerStyle = ScenarioTemplateFacade::getTemplate().blockStyle(newBlockStyle.headerType());

        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.insertBlock();
        cursor.movePosition(QTextCursor::PreviousCharacter);

        cursor.setBlockCharFormat(headerStyle.charFormat());
        cursor.setBlockFormat(headerStyle.blockFormat(cursor.isBlockInTable()));

        cursor.insertText(newBlockStyle.header());
    }

    //
    // Для заголовка папки нужно создать завершение, захватив всё содержимое сцены
    //
    if (newBlockStyle.isEmbeddableHeader()) {
        ScenarioBlockStyle footerStyle = ScenarioTemplateFacade::getTemplate().blockStyle(newBlockStyle.embeddableFooter());

        //
        // Запомним позицию курсора
        //
        int lastCursorPosition = textCursor().position();

        //
        // Ищем конец сцены
        //
        do {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        } while (!cursor.atEnd()
                 && ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::SceneHeading
                 && ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::FolderHeader
                 && ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::FolderFooter);

        //
        // Если забежали на блок следующей сцены, вернёмся на один символ назад
        //
        if (!cursor.atEnd() && cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousCharacter);
        }

        //
        // Когда дошли до конца сцены, вставляем закрывающий блок
        //
        cursor.insertBlock();
        cursor.setBlockCharFormat(footerStyle.charFormat());
        cursor.setBlockFormat(footerStyle.blockFormat(cursor.isBlockInTable()));

        //
        // т.к. вставлен блок, нужно вернуть курсор на место
        //
        cursor.setPosition(lastCursorPosition);
        setTextCursor(cursor);

        //
        // Эмулируем нажатие кнопки клавиатуры, чтобы обновился футер стиля
        //
        QKeyEvent empyEvent(QEvent::KeyPress, -1, Qt::NoModifier);
        keyPressEvent(&empyEvent);
    }

    cursor.endEditBlock();
}

void ScenarioTextEdit::updateEnteredText(const QString& _eventText)
{
    //
    // Получим значения
    //
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... стиль шрифта блока
    QTextCharFormat currentCharFormat = currentBlock.charFormat();

    //
    // Если был введён текст
    //
    if (!_eventText.isEmpty()) {
        //
        // Определяем необходимость установки верхнего регистра для первого символа блока
        //
        if (cursorBackwardText != " "
            && (cursorBackwardText == _eventText
                || cursorBackwardText == (currentCharFormat.stringProperty(ScenarioBlockStyle::PropertyPrefix)
                                          + _eventText))) {
            //
            // Сформируем правильное представление строки
            //
            bool isFirstUpperCase = currentCharFormat.boolProperty(ScenarioBlockStyle::PropertyIsFirstUppercase);
            QString correctedText = _eventText;
            if (isFirstUpperCase) {
                correctedText[0] = TextEditHelper::smartToUpper(correctedText[0]);
            }

            //
            // Стираем предыдущий введённый текст
            //
            for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
                cursor.deletePreviousChar();
            }

            //
            // Выводим необходимый
            //
            cursor.insertText(correctedText);
            setTextCursor(cursor);
        }
        //
        // Иначе обрабатываем по обычным правилам
        //
        else {
            //
            // Если перед нами конец предложения
            // и не сокращение
            // и после курсора нет текста (для ремарки допустима скобка)
            //
            QString endOfSentancePattern = QString("([.]|[?]|[!]|[…]) %1$").arg(_eventText);
            if (m_capitalizeFirstWord
                && cursorBackwardText.contains(QRegularExpression(endOfSentancePattern))
                && !stringEndsWithAbbrev(cursorBackwardText)
                && (cursorForwardText.isEmpty() || cursorForwardText == ")")) {
                //
                // Сделаем первую букву заглавной
                //
                QString correctedText = _eventText;
                correctedText[0] = TextEditHelper::smartToUpper(correctedText[0]);

                //
                // Стираем предыдущий введённый текст
                //
                for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
                    cursor.deletePreviousChar();
                }

                //
                // Выводим необходимый
                //
                cursor.insertText(correctedText);
                setTextCursor(cursor);
            }
            //
            // Исправляем проблему ДВойных ЗАглавных
            //
            else if (m_correctDoubleCapitals) {
                QString right3Characters = cursorBackwardText.right(3).simplified();

                //
                // Если две из трёх последних букв находятся в верхнем регистре, то это наш случай
                //
                if (!right3Characters.contains(" ")
                    && right3Characters.length() == 3
                    && right3Characters != TextEditHelper::smartToUpper(right3Characters)
                    && right3Characters.left(2) == TextEditHelper::smartToUpper(right3Characters.left(2))
                    && right3Characters.left(2).at(0).isLetter()
                    && right3Characters.left(2).at(1).isLetter()
                    && _eventText != TextEditHelper::smartToUpper(_eventText)) {
                    //
                    // Сделаем предпоследнюю букву строчной
                    //
                    QString correctedText = right3Characters;
                    correctedText[correctedText.length() - 2] = correctedText[correctedText.length() - 2].toLower();

                    //
                    // Стираем предыдущий введённый текст
                    //
                    for (int repeats = 0; repeats < correctedText.length(); ++repeats) {
                        cursor.deletePreviousChar();
                    }

                    //
                    // Выводим необходимый
                    //
                    cursor.insertText(correctedText);
                    setTextCursor(cursor);
                }
            }

            //
            // Если была попытка ввести несколько пробелов подряд, или пробел в начале строки,
            // удаляем этот лишний пробел
            //
            if (cursorBackwardText == " "
                || cursorBackwardText.endsWith("  ")) {
                cursor.deletePreviousChar();
            }
        }
    }

}

bool ScenarioTextEdit::stringEndsWithAbbrev(const QString& _text)
{
    Q_UNUSED(_text);

    //
    // FIXME проработать словарь сокращений
    //

    return false;
}

bool ScenarioTextEdit::selectBlockOnTripleClick(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton) {
        const qint64 curMouseClickTime = QDateTime::currentMSecsSinceEpoch();
        const qint64 timeDelta = curMouseClickTime - m_lastMouseClickTime;
        if (timeDelta <= (QApplication::styleHints()->mouseDoubleClickInterval())) {
            m_mouseClicks += 1;
        } else {
            m_mouseClicks = 1;
        }
        m_lastMouseClickTime = curMouseClickTime;
    }

    //
    // Тройной клик обрабатываем самостоятельно
    //
    if (m_mouseClicks > 2) {
        m_mouseClicks = 1;
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
        _event->accept();
        if (_event->button() != Qt::RightButton) {
            return true;
        }
    }

    return false;
}

void ScenarioTextEdit::initEditor()
{
    //
    // Настроим стиль первого блока, если необходимо
    //
    QTextCursor cursor(document());
    setTextCursor(cursor);
    if (BusinessLogic::ScenarioBlockStyle::forBlock(cursor.block())
        == BusinessLogic::ScenarioBlockStyle::Undefined) {
        applyScenarioTypeToBlockText(ScenarioBlockStyle::SceneHeading);
    }

    initEditorConnections();
}

void ScenarioTextEdit::initEditorConnections()
{
    if (m_document != 0) {
        connect(m_document, &ScenarioTextDocument::contentsChange, this, &ScenarioTextEdit::aboutCorrectAdditionalCursors);
        connect(m_document, &ScenarioTextDocument::beforePatchApply, this, &ScenarioTextEdit::aboutSaveEditorState);
        connect(m_document, &ScenarioTextDocument::afterPatchApply, this, &ScenarioTextEdit::aboutLoadEditorState);
        connect(m_document, &ScenarioTextDocument::reviewChanged, this, &ScenarioTextEdit::reviewChanged);
        connect(m_document, &ScenarioTextDocument::redoAvailableChanged, this, &ScenarioTextEdit::redoAvailableChanged);
    }
}

void ScenarioTextEdit::removeEditorConnections()
{
    if (m_document != 0) {
        disconnect(m_document, &ScenarioTextDocument::contentsChange, this, &ScenarioTextEdit::aboutCorrectAdditionalCursors);
        disconnect(m_document, &ScenarioTextDocument::beforePatchApply, this, &ScenarioTextEdit::aboutSaveEditorState);
        disconnect(m_document, &ScenarioTextDocument::afterPatchApply, this, &ScenarioTextEdit::aboutLoadEditorState);
        disconnect(m_document, &ScenarioTextDocument::reviewChanged, this, &ScenarioTextEdit::reviewChanged);
        disconnect(m_document, &ScenarioTextDocument::redoAvailableChanged, this, &ScenarioTextEdit::redoAvailableChanged);
    }
}

void ScenarioTextEdit::initView()
{
    //
    // Параметры внешнего вида
    //
    const int MINIMUM_WIDTH = 100;
    const int MINIMUM_HEIGHT = 100;

    setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);

    updateShortcuts();
}

void ScenarioTextEdit::initConnections()
{
    //
    // При перемещении курсора может меняться стиль блока
    //
    connect(this, &ScenarioTextEdit::cursorPositionChanged, this, &ScenarioTextEdit::currentStyleChanged, Qt::UniqueConnection);
}
