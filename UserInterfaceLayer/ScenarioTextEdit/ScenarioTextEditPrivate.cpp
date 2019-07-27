#include "ScenarioTextEditPrivate.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/ShortcutHelper.h>

#include <QShortcut>
#include <QSignalMapper>

using UserInterface::ShortcutsManager;
using BusinessLogic::ScenarioBlockStyle;


ShortcutsManager::ShortcutsManager(UserInterface::ScenarioTextEdit* _editor) :
    QObject(_editor),
    m_editor(_editor)
{
    Q_ASSERT(_editor);
}

void ShortcutsManager::setContextWidget(QWidget* _context)
{
    if (m_context == _context) {
        return;
    }

    m_context = _context;
    qDeleteAll(m_shortcuts);

    //
    // Создаём шорткаты
    //
    createOrUpdateShortcut(ScenarioBlockStyle::SceneHeading);
    createOrUpdateShortcut(ScenarioBlockStyle::SceneCharacters);
    createOrUpdateShortcut(ScenarioBlockStyle::Action);
    createOrUpdateShortcut(ScenarioBlockStyle::Character);
    createOrUpdateShortcut(ScenarioBlockStyle::Parenthetical);
    createOrUpdateShortcut(ScenarioBlockStyle::Dialogue);
    createOrUpdateShortcut(ScenarioBlockStyle::Transition);
    createOrUpdateShortcut(ScenarioBlockStyle::Note);
    createOrUpdateShortcut(ScenarioBlockStyle::Title);
    createOrUpdateShortcut(ScenarioBlockStyle::NoprintableText);
    createOrUpdateShortcut(ScenarioBlockStyle::FolderHeader);
    createOrUpdateShortcut(ScenarioBlockStyle::Lyrics);

    //
    // Настраиваем их
    //
    QSignalMapper* mapper = new QSignalMapper(this);
    foreach (int type, m_shortcuts.keys()) {
        QShortcut* shortcut = m_shortcuts.value(type);
        connect(shortcut, SIGNAL(activated()), mapper, SLOT(map()));
        mapper->setMapping(shortcut, type);
    }
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(changeTextBlock(int)));
}

void ShortcutsManager::update()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    foreach (int type, m_shortcuts.keys()) {
        createOrUpdateShortcut(type);
    }
}

QString ShortcutsManager::shortcut(int _forBlockType) const
{
    QString result;
    if (m_shortcuts.contains(_forBlockType)) {
        result = ShortcutHelper::makeShortcut(m_shortcuts.value(_forBlockType)->key().toString());
    }
    return result;
}

void ShortcutsManager::changeTextBlock(int _blockType) const
{
    if (!m_editor->isReadOnly()) {
        ScenarioBlockStyle::Type blockType = (ScenarioBlockStyle::Type)_blockType;
        m_editor->changeScenarioBlockType(blockType);
    }
}

void ShortcutsManager::createOrUpdateShortcut(int _forBlockType)
{
    if (m_context == nullptr) {
        return;
    }

    ScenarioBlockStyle::Type blockType = (ScenarioBlockStyle::Type)_forBlockType;
    const QString typeShortName = ScenarioBlockStyle::typeName(blockType);
    const QString keySequenceText =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                QString("scenario-editor/shortcuts/%1").arg(typeShortName),
                DataStorageLayer::SettingsStorage::ApplicationSettings
                );
    const QKeySequence keySequence(keySequenceText);

    QShortcut* shortcut = 0;
    if (m_shortcuts.contains(_forBlockType)) {
        shortcut = m_shortcuts.value(_forBlockType);
        shortcut->setKey(keySequence);
    } else {
        shortcut = new QShortcut(keySequence, m_context, 0, 0, Qt::WidgetWithChildrenShortcut);
    }

    m_shortcuts[_forBlockType] = shortcut;
}
