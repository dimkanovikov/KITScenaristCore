#include "SpellCheckTextEdit.h"
#include "SpellChecker.h"
#include "SpellCheckHighlighter.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QMenu>
#include <QStandardPaths>
#include <QTimer>

#include <QtGui/private/qtextdocument_p.h>


namespace {
    /**
     * @brief Максимальное кол-во подсказок для проверки орфографии
     */
    const int kSuggestionsActionsMaxCount = 5;
}

SpellCheckTextEdit::SpellCheckTextEdit(QWidget *_parent) :
    PageTextEdit(_parent),
    m_spellChecker(nullptr),
    m_spellCheckHighlighter(nullptr)
{
    //
    // Настроим проверяющего
    //
    m_spellChecker = SpellChecker::createSpellChecker(userDictionaryfile());

    //
    // Настраиваем подсветку слов не прошедших проверку орфографии
    //
    m_spellCheckHighlighter = new SpellCheckHighlighter(nullptr, m_spellChecker);
    connect(this, &SpellCheckTextEdit::cursorPositionChanged,
            this, &SpellCheckTextEdit::rehighlighWithNewCursor);

    //
    // Настраиваем действия контекстного меню для слов не прошедших проверку орфографии
    //
    // ... игнорировать слово
    m_ignoreWordAction = new QAction(tr("Ignore"), this);
    connect(m_ignoreWordAction, &QAction::triggered, this, &SpellCheckTextEdit::aboutIgnoreWord);
    // ... добавить слово в словарь
    m_addWordToUserDictionaryAction = new QAction(tr("Add to dictionary"), this);
    connect(m_addWordToUserDictionaryAction, &QAction::triggered, this, &SpellCheckTextEdit::aboutAddWordToUserDictionary);
    // ... добавляем несколько пустых пунктов, для последующего помещения в них вариантов
    for (int actionIndex = 0; actionIndex < kSuggestionsActionsMaxCount; ++actionIndex) {
        m_suggestionsActions.append(new QAction(QString(), this));
        connect(m_suggestionsActions.at(actionIndex), &QAction::triggered, this, &SpellCheckTextEdit::aboutReplaceWordOnSuggestion);
    }
}

void SpellCheckTextEdit::setUseSpellChecker(bool _use)
{
    m_spellCheckHighlighter->setUseSpellChecker(_use);
}

bool SpellCheckTextEdit::useSpellChecker() const
{
    return m_spellCheckHighlighter->useSpellChecker();
}

void SpellCheckTextEdit::setSpellCheckLanguage(SpellChecker::Language _language)
{
    if (!useSpellChecker()) {
        return;
    }

    if (m_spellChecker->spellingLanguage() != _language) {
        //
        // Установим язык проверяющего
        //
        m_spellChecker->setSpellingLanguage(_language);

        //
        // Заново выделим слова не проходящие проверку орфографии вновь заданного языка
        //
        m_spellCheckHighlighter->rehighlight();
    }
}

SpellChecker::Language SpellCheckTextEdit::spellCheckLanguage() const
{
    return m_spellChecker->spellingLanguage();
}

QMenu* SpellCheckTextEdit::createContextMenu(const QPoint& _pos, QWidget* _parent)
{
    //
    // Запомним позицию курсора
    //
    m_lastCursorPosition = _pos;

    //
    // Сформируем стандартное контекстное меню
    //
    QMenu* menu = createStandardContextMenu(_parent);

    //
    // Скрываем пункты меню отмены и повтора последнего действия
    //
    for (QAction* menuAction : menu->findChildren<QAction*>()) {
        if (menuAction->text().endsWith(QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText))
            || menuAction->text().endsWith(QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText))) {
            menuAction->disconnect();
            menuAction->setVisible(false);
        }
    }

    //
    // Добавим пункты меню, связанные с проверкой орфографии
    //
    if (m_spellCheckHighlighter->useSpellChecker()) {
        //
        // Определим слово под курсором
        //
        const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(m_lastCursorPosition));
        const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
        const QString text = cursorWordStart.block().text();
        const QString wordUnderCursor = text.mid(cursorWordStart.positionInBlock(), cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());

        QString wordInCorrectRegister = wordUnderCursor;
        if (cursorForPosition(_pos).charFormat().fontCapitalization() == QFont::AllUppercase) {
            //
            // Приведем к верхнему регистру
            //
            wordInCorrectRegister = wordUnderCursor.toUpper();
        }

        //
        // Если слово не проходит проверку орфографии добавим дополнительные действия в контекстное меню
        //
        if (!m_spellChecker->spellCheckWord(wordInCorrectRegister)) {
            // ... действие, перед которым вставляем дополнительные пункты
            QStringList suggestions = m_spellChecker->suggestionsForWord(wordUnderCursor);
            // ... вставляем варианты
            QAction* actionInsertBefore = menu->actions().first();
            int addedSuggestionsCount = 0;
            for (const QString& suggestion : suggestions) {
                if (addedSuggestionsCount < kSuggestionsActionsMaxCount) {
                    m_suggestionsActions.at(addedSuggestionsCount)->setText(suggestion);
                    m_suggestionsActions.at(addedSuggestionsCount)->setEnabled(true);
                    menu->insertAction(actionInsertBefore, m_suggestionsActions.at(addedSuggestionsCount));
                    ++addedSuggestionsCount;
                } else {
                    break;
                }
            }
            if (addedSuggestionsCount == 0) {
                m_suggestionsActions.first()->setText(tr("Suggestions not found"));
                m_suggestionsActions.first()->setEnabled(false);
                menu->insertAction(actionInsertBefore, m_suggestionsActions.first());
            }
            menu->insertSeparator(actionInsertBefore);
            // ... вставляем дополнительные действия
            menu->insertAction(actionInsertBefore, m_ignoreWordAction);
            menu->insertAction(actionInsertBefore, m_addWordToUserDictionaryAction);
            menu->insertSeparator(actionInsertBefore);
        }
        //
        // TODO: тезаурус готов, нужны доработки по визуализации и по качеству самой базы слов
        //
        //
        // Если проходит проверку, то добавляем вхождения из тезауруса
        //
//		else {
//			QMap<QString, QSet<QString> > thesaurusEntries =
//				m_spellChecker->synonimsForWord(wordWithoutPunct.toLower());
//			QMenu* thesaurusMenu = new QMenu(menu);
//			//
//			// Если есть вхождения, то показываем меню с ними
//			//
//			if (!thesaurusEntries.isEmpty()) {
//				QList<QString> entries = thesaurusEntries.keys();
//				qSort(entries);
//				foreach (const QString& entry, entries) {
//					QMenu* entryMenu = new QMenu(menu);
//					QList<QString> entryItems = thesaurusEntries.value(entry).toList();
//					qSort(entryItems);
//					foreach (const QString& entryItem, entryItems) {
//						entryMenu->addAction(entryItem);
//					}
//					QAction* entryAction = thesaurusMenu->addAction(entry);
//					entryAction->setMenu(entryMenu);
//				}
//			}
//			//
//			// Если нет вхождений, то так и говорим
//			//
//			else {
//				QAction* entryAction = thesaurusMenu->addAction(tr("No available information."));
//				entryAction->setEnabled(false);
//			}

//			QAction* thesaurusAction =
//				new QAction(tr("%1 in dictionary").arg(wordWithoutPunct), menu);
//			thesaurusAction->setMenu(thesaurusMenu);
//			QAction* actionInsertBefore = menu->actions().first();
//			menu->insertAction(actionInsertBefore, thesaurusAction);
//			menu->insertSeparator(actionInsertBefore);
//		}
    }

    return menu;
}

void SpellCheckTextEdit::clear()
{
    m_prevBlock = QTextBlock();
    PageTextEdit::clear();
}

QString SpellCheckTextEdit::userDictionaryfile() const
{
    QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString hunspellDictionariesFolderPath = appDataFolderPath + QDir::separator() + "Hunspell";
    QString dictionaryFilePath = hunspellDictionariesFolderPath + QDir::separator() + "UserDictionary.dict";
    return dictionaryFilePath;
}

void SpellCheckTextEdit::contextMenuEvent(QContextMenuEvent* _event)
{
    QMenu* contextMenu = createContextMenu(_event->pos());

    //
    // Покажем меню, а после очистим от него память
    //
    contextMenu->exec(_event->globalPos());
    delete contextMenu;
}

void SpellCheckTextEdit::setHighlighterDocument(QTextDocument* _document)
{
    m_prevBlock = QTextBlock();
    m_spellCheckHighlighter->setDocument(_document);
}

void SpellCheckTextEdit::aboutIgnoreWord() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(m_lastCursorPosition);

    //
    // Уберем пунктуацию
    //
    const QString wordUnderCursorWithoutPunct = removePunctutaion(wordUnderCursor);

    //
    // Скорректируем регистр слова
    //
    const QString wordUnderCursorWithoutPunctInCorrectRegister = wordUnderCursorWithoutPunct.toLower();

    //
    // Объявляем проверяющему о том, что это слово нужно игнорировать
    //
    m_spellChecker->ignoreWord(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение с игнорируемых слов
    //
    m_spellCheckHighlighter->rehighlight();
}

void SpellCheckTextEdit::aboutAddWordToUserDictionary() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(m_lastCursorPosition);

    //
    // Уберем пунктуацию в слове
    //
    const QString wordUnderCursorWithoutPunct = removePunctutaion(wordUnderCursor);

    //
    // Приведем к нижнему регистру
    //
    const QString wordUnderCursorWithoutPunctInCorrectRegister = wordUnderCursorWithoutPunct.toLower();

    //
    // Объявляем проверяющему о том, что это слово нужно добавить в пользовательский словарь
    //
    m_spellChecker->addWordToDictionary(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение со слов добавленных в словарь
    //
    m_spellCheckHighlighter->rehighlight();
}

void SpellCheckTextEdit::aboutReplaceWordOnSuggestion()
{
    if (QAction* suggestAction = qobject_cast<QAction*>(sender())) {
        QTextCursor cursor = cursorForPosition(m_lastCursorPosition);
        cursor = moveCursorToStartWord(cursor);
        QTextCursor endCursor = moveCursorToEndWord(cursor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, endCursor.positionInBlock() - cursor.positionInBlock());
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.insertText(suggestAction->text());
        setTextCursor(cursor);
        cursor.endEditBlock();

        //
        // Немного магии. Мы хотим перепроверить блок, в котором изменили слово.
        // Беда в том, что SpellCheckHighlighter не будет проверять слово в блоке,
        // если оно сейчас редактировалось. Причем, эта проверка идет по позиции.
        // А значит при проверке другого блока, слово на этой позиции не проверится.
        // Поэтому, мы ему говорим, что слово не редактировалось, проверяй весь абзац
        // а затем восстанавливаем в прежнее состояние
        //
        bool isEdited = m_spellCheckHighlighter->isEdited();
        m_spellCheckHighlighter->rehighlightBlock(cursor.block());
        m_spellCheckHighlighter->setEdited(isEdited);
    }
}

QTextCursor SpellCheckTextEdit::moveCursorToStartWord(QTextCursor cursor) const
{
    cursor.movePosition(QTextCursor::StartOfWord);
    QString text = cursor.block().text();

    //
    // Цикл ниже необходим, потому что movePosition(StartOfWord)
    // считает - и ' другими словами
    // Примеры "кто-" - еще не закончив печатать слово, получим
    // его подсветку
    //
    while (cursor.positionInBlock() > 0 &&
           (text[cursor.positionInBlock()] == '\''
            || text[cursor.positionInBlock()] == "’"
            || text[cursor.positionInBlock()] == '-'
            || text[cursor.positionInBlock() - 1] == '\''
            || text[cursor.positionInBlock() - 1] == '-')) {
            cursor.movePosition(QTextCursor::PreviousCharacter);
            cursor.movePosition(QTextCursor::StartOfWord);
    }
    return cursor;
}

QTextCursor SpellCheckTextEdit::moveCursorToEndWord(QTextCursor cursor) const
{
    QRegExp splitWord("[^\\w'’-]");
    splitWord.indexIn(cursor.block().text(), cursor.positionInBlock());
    int pos = splitWord.pos();
    if (pos == -1) {
        pos= cursor.block().text().length();
    }
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, pos - cursor.positionInBlock());
    return cursor;
}

void SpellCheckTextEdit::rehighlighWithNewCursor()
{
    if (!m_spellCheckHighlighter->useSpellChecker()) {
        return;
    }

    //
    // Если редактирование документа не закончено, но позиция курсора сменилась, откладываем проверку орфографии
    //
    if (document()->docHandle()->isInEditBlock()) {
        QTimer::singleShot(100, this, &SpellCheckTextEdit::rehighlighWithNewCursor);
        return;
    }

    QTextCursor cursor = textCursor();
    cursor = moveCursorToStartWord(cursor);
    m_spellCheckHighlighter->setCursorPosition(cursor.positionInBlock());
    if (m_prevBlock.isValid()) {
        m_spellCheckHighlighter->rehighlightBlock(m_prevBlock);
    }
    m_prevBlock = textCursor().block();
}

QString SpellCheckTextEdit::wordOnPosition(const QPoint& _position) const
{
    const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(_position));
    const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
    const QString text = cursorWordStart.block().text();
    return text.mid(cursorWordStart.positionInBlock(), cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());
}

QString SpellCheckTextEdit::removePunctutaion(const QString &_word) const
{
    //
    // Убираем знаки препинания окружающие слово
    //
    QString wordWithoutPunct = _word.trimmed();
    while (!wordWithoutPunct.isEmpty()
           && (wordWithoutPunct.at(0).isPunct()
               || wordWithoutPunct.at(wordWithoutPunct.length()-1).isPunct())) {
        if (wordWithoutPunct.at(0).isPunct()) {
            wordWithoutPunct = wordWithoutPunct.mid(1);
        } else {
            wordWithoutPunct = wordWithoutPunct.left(wordWithoutPunct.length()-1);
        }
    }
    return wordWithoutPunct;
}
