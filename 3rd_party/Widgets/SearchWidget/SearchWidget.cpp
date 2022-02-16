#include "SearchWidget.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <3rd_party/Widgets/PagesTextEdit/PageTextEdit.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBlock>


SearchWidget::SearchWidget(QWidget* _parent, bool _showTypesCombo) :
    QFrame(_parent),
    m_editor(nullptr),
    m_searchText(new QLineEdit(this)),
    m_caseSensitive(new QPushButton(this)),
    m_prevMatch(new QPushButton(this)),
    m_nextMatch(new QPushButton(this)),
    m_replaceText(new QLineEdit(this)),
    m_replaceOne(new QPushButton(this)),
    m_replaceAll(new QPushButton(this)),
    m_searchIn(new QComboBox(this))
{
    initView(_showTypesCombo);
    initStyleSheet();
    initConnections();
}

void SearchWidget::setEditor(PageTextEdit* _editor)
{
    if (m_editor != _editor) {
        m_editor = _editor;
    }
}

void SearchWidget::selectText()
{
    m_searchText->selectAll();
}

void SearchWidget::setSearchOnly(bool _isSearchOnly)
{
    m_replaceText->setVisible(!_isSearchOnly);
    m_replaceOne->setVisible(!_isSearchOnly);
    m_replaceAll->setVisible(!_isSearchOnly);
}

void SearchWidget::aboutFindNext()
{
    findText(false);
}

void SearchWidget::aboutFindPrev()
{
    findText(true);
}

void SearchWidget::aboutReplaceOne()
{
    if (!m_editor) {
        return;
    }

    const QString replaceText = m_replaceText->text();
    const QString searchText = m_searchText->text();
    QTextCursor cursor = m_editor->textCursor();
    bool selectedTextEqual =
        m_caseSensitive->isChecked()
        ? cursor.selectedText() == searchText
        : cursor.selectedText().toLower() == searchText.toLower();
    if (selectedTextEqual) {
        cursor.insertText(replaceText);
        aboutFindNext();
    }
}

void SearchWidget::aboutReplaceAll()
{
    if (!m_editor) {
        return;
    }

    const QString replaceText = m_replaceText->text();
    const QString searchText = m_searchText->text();
    const int diffSize = replaceText.size() - searchText.size();
    aboutFindNext();
    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    int firstCursorPosition = cursor.selectionStart();
    while (cursor.hasSelection()) {
        cursor.insertText(replaceText);
        aboutFindNext();
        cursor = m_editor->textCursor();

        //
        // Корректируем начальную позицию поиска, для корректного завершения при втором проходе по документу
        //
        if (cursor.selectionStart() < firstCursorPosition) {
        firstCursorPosition += diffSize;
        }

        //
        // Прерываем случай, когда пользователь пытается заменить слово без учёта регистра
        // на такое же, например "иван" на "Иван" или когда заменяемое слово является частью нового,
        // но т.к. поиск производится без учёта регистра, он зацикливается
        //
        if (cursor.selectionStart() == firstCursorPosition) {
        break;
        }
    }
    cursor.endEditBlock();
}

void SearchWidget::initView(bool _showTypesCombo)
{
    setFocusProxy(m_searchText);
    m_searchText->setPlaceholderText(tr("Find..."));
    m_searchText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_caseSensitive->setFixedWidth(20);
    m_caseSensitive->setCheckable(true);
    m_caseSensitive->setText("Aa");
    QFont caseSensitiveFont = m_caseSensitive->font();
    caseSensitiveFont.setItalic(true);
    m_caseSensitive->setFont(caseSensitiveFont);
    m_caseSensitive->setToolTip(tr("Case Sensitive"));

    m_prevMatch->setFixedWidth(20);
    m_prevMatch->setText(
#ifdef Q_OS_WIN
                QSysInfo::windowsVersion() == QSysInfo::WV_XP ? "◄" : "▲"
#else
                "◀"
#endif
                );
    m_prevMatch->setShortcut(QKeySequence("Shift+F3"));
    m_prevMatch->setToolTip(tr("Find Prev"));
    m_nextMatch->setFixedWidth(20);
    m_nextMatch->setText(
#ifdef Q_OS_WIN
                QSysInfo::windowsVersion() == QSysInfo::WV_XP ? "►" : "▼"
#else
                "▶"
#endif
                );
    m_nextMatch->setShortcut(QKeySequence("F3"));
    m_nextMatch->setToolTip(tr("Find Next"));
    m_searchIn->addItem(tr("In whoole document"), BusinessLogic::ScenarioBlockStyle::Undefined);
    m_searchIn->addItem(tr("In scene heading"), BusinessLogic::ScenarioBlockStyle::SceneHeading);
    m_searchIn->addItem(tr("In action"), BusinessLogic::ScenarioBlockStyle::Action);
    m_searchIn->addItem(tr("In character"), BusinessLogic::ScenarioBlockStyle::Character);
    m_searchIn->addItem(tr("In dialogue"), BusinessLogic::ScenarioBlockStyle::Dialogue);
    m_searchIn->addItem(tr("In parenthetical"), BusinessLogic::ScenarioBlockStyle::Parenthetical);
    m_searchIn->addItem(tr("In lyrics"), BusinessLogic::ScenarioBlockStyle::Lyrics);
    m_searchIn->setVisible(_showTypesCombo);
    m_searchIn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_replaceText->setPlaceholderText(tr("Replace with..."));
    m_replaceText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_replaceOne->setText(tr("Replace"));
    m_replaceAll->setText(tr("All"));

    QHBoxLayout* layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->addWidget(m_searchText);
    layout->addWidget(m_caseSensitive);
    layout->addWidget(m_prevMatch);
    layout->addWidget(m_nextMatch);
    layout->addSpacing(16);
    if (_showTypesCombo) {
        layout->addWidget(m_searchIn);
        layout->addSpacing(16);
    }
    layout->addWidget(m_replaceText);
    layout->addWidget(m_replaceOne);
    layout->addWidget(m_replaceAll);

    setLayout(layout);
}

void SearchWidget::initStyleSheet()
{
    setFrameShape(QFrame::Box);
    QString styleSheet = "*[searchWidget=\"true\"] {"
                         "  border: 0px solid black; "
                         "  border-top-width: 1px; "
                         "  border-top-style: solid; "
                         "  border-top-color: palette(dark);"
                         "}";
    if (QLocale().textDirection() == Qt::LeftToRight) {
        styleSheet += "*[middle=\"true\"] { border-left: 0; min-width: 20px; }"
                      "*[last=\"true\"] { border-left: 0; min-width: 20px; }";
    } else {
        styleSheet += "*[middle=\"true\"] { border-right: 0; min-width: 20px; }"
                      "*[last=\"true\"] { border-right: 0; min-width: 20px; }";
    }
    setStyleSheet(styleSheet);
    setProperty("searchWidget", true);
    m_caseSensitive->setProperty("middle", true);
    m_prevMatch->setProperty("middle", true);
    m_nextMatch->setProperty("last", true);
    m_replaceOne->setProperty("middle", true);
    m_replaceAll->setProperty("last", true);
}

void SearchWidget::initConnections()
{
    connect(m_searchText, &QLineEdit::textChanged,
            this, static_cast<void(SearchWidget::*)()>(&SearchWidget::aboutFindNext));
    connect(m_searchText, &QLineEdit::returnPressed, this, &SearchWidget::aboutFindNext);
    connect(m_prevMatch, &QPushButton::clicked, this, &SearchWidget::aboutFindPrev);
    connect(m_nextMatch, &QPushButton::clicked, this, &SearchWidget::aboutFindNext);
    connect(m_replaceOne, &QPushButton::clicked, this, &SearchWidget::aboutReplaceOne);
    connect(m_replaceAll, &QPushButton::clicked, this, &SearchWidget::aboutReplaceAll);
}

void SearchWidget::findText(bool _backward)
{
    const QString searchText = m_searchText->text();
    if (searchText.isEmpty()
            || !m_editor) {
        //
        // Сохраняем искомый текст
        //
        m_lastSearchText = searchText;
        return;
    }

    //
    // Поиск осуществляется от позиции курсора
    //
    QTextCursor cursor = m_editor->textCursor();
    if (searchText != m_lastSearchText) {
        cursor.setPosition(qMin(cursor.selectionStart(),cursor.selectionEnd()));
    }

    //
    // Настроить направление поиска
    //
    QTextDocument::FindFlags findFlags;
    if (_backward) {
        findFlags |= QTextDocument::FindBackward;
    }
    //
    // Учёт регистра
    //
    if (m_caseSensitive->isChecked()) {
        findFlags |= QTextDocument::FindCaseSensitively;
    }

    //
    // Поиск
    //
    bool searchRestarted = false;
    bool restartSearch = false;
    do {
        restartSearch = false;
        cursor = m_editor->document()->find(searchText, cursor, findFlags);
        BusinessLogic::ScenarioBlockStyle::Type searchType =
                (BusinessLogic::ScenarioBlockStyle::Type)m_searchIn->currentData().toInt();
        BusinessLogic::ScenarioBlockStyle::Type blockType =
                BusinessLogic::ScenarioBlockStyle::forBlock(cursor.block());
        if (!cursor.isNull()) {
            if (searchType == BusinessLogic::ScenarioBlockStyle::Undefined
                || searchType == blockType) {
                m_editor->ensureCursorVisible(cursor);
            } else {
                restartSearch = true;
            }
        } else {
            //
            // Если достигнут конец, или начало документа зацикливаем поиск, если это первый проход
            //
            if (searchRestarted == false) {
                searchRestarted = true;
                cursor = m_editor->textCursor();
                cursor.movePosition(_backward ? QTextCursor::End : QTextCursor::Start);
                cursor = m_editor->document()->find(searchText, cursor, findFlags);
                blockType = BusinessLogic::ScenarioBlockStyle::forBlock(cursor.block());
                if (!cursor.isNull()) {
                    if (searchType == BusinessLogic::ScenarioBlockStyle::Undefined
                        || searchType == blockType) {
                        m_editor->ensureCursorVisible(cursor);
                    } else {
                        restartSearch = true;
                    }
                }
            } else {
                break;
            }
        }
    } while (!cursor.block().isVisible() || restartSearch);

    //
    // Сохраняем искомый текст
    //
    m_lastSearchText = searchText;
}
