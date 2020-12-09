#include "StorageFacade.h"

#include "PlaceStorage.h"
#include "ScenarioDayStorage.h"
#include "TimeStorage.h"
#include "CharacterStateStorage.h"
#include "TransitionStorage.h"
#include "ScenarioStorage.h"
#include "ScenarioChangeStorage.h"
#include "ScenarioDataStorage.h"
#include "ResearchStorage.h"
#include "SettingsStorage.h"
#include "DatabaseHistoryStorage.h"
#include "ScriptVersionStorage.h"

#include <3rd_party/Helpers/TextUtils.h>

using namespace DataStorageLayer;


QString StorageFacade::userName()
{
    //
    // Если пользователь авторизован, то используем его логин и имейл
    //
    const QString email = settingsStorage()->value("application/email", SettingsStorage::ApplicationSettings);
    const QString username = settingsStorage()->value("application/username", SettingsStorage::ApplicationSettings);
    if (!email.isEmpty()) {
        return QString("%1 %2").arg(username).arg(TextUtils::directedText(email, '[', ']'));
    }

    //
    // А если не авторизован, то используем имя пользователя из системы
    //
    return username;
}

QString StorageFacade::userEmail()
{
    return settingsStorage()->value("application/email", SettingsStorage::ApplicationSettings);
}

void StorageFacade::clearStorages()
{
    placeStorage()->clear();
    scenarioDayStorage()->clear();
    timeStorage()->clear();
    characterStateStorage()->clear();
    transitionStorage()->clear();
    scenarioStorage()->clear();
    scenarioChangeStorage()->clear();
    scenarioDataStorage()->clear();
    researchStorage()->clear();
    scriptVersionStorage()->clear();
}

void StorageFacade::refreshStorages()
{
    researchStorage()->refresh();
    placeStorage()->refresh();
    scenarioDayStorage()->refresh();
    transitionStorage()->refresh();
    timeStorage()->refresh();
    characterStateStorage()->refresh();
    scenarioDataStorage()->refresh();
    scriptVersionStorage()->refresh();

    //
    // Хранилища со списком изменений сценария и сам сценарий обновляются по другому принципу
    //
    //	scenarioStorage()->refresh();
    //	scenarioChangeStorage()->refresh();
}

PlaceStorage* StorageFacade::placeStorage()
{
    if (s_placeStorage == nullptr) {
        s_placeStorage = new PlaceStorage;
    }
    return s_placeStorage;
}

ScenarioDayStorage* StorageFacade::scenarioDayStorage()
{
    if (s_scenarioDayStorage == nullptr) {
        s_scenarioDayStorage = new ScenarioDayStorage;
    }
    return s_scenarioDayStorage;
}

TimeStorage* StorageFacade::timeStorage()
{
    if (s_timeStorage == nullptr) {
        s_timeStorage = new TimeStorage;
    }
    return s_timeStorage;
}

CharacterStateStorage*StorageFacade::characterStateStorage()
{
    if (s_characterStateStorage == nullptr) {
        s_characterStateStorage = new CharacterStateStorage;
    }
    return s_characterStateStorage;
}

TransitionStorage* StorageFacade::transitionStorage()
{
    if (s_transitionsStorage == nullptr) {
        s_transitionsStorage = new TransitionStorage;
    }
    return s_transitionsStorage;
}

ScenarioStorage* StorageFacade::scenarioStorage()
{
    if (s_scenarioStorage == nullptr) {
        s_scenarioStorage = new ScenarioStorage;
    }
    return s_scenarioStorage;
}

ScenarioChangeStorage* StorageFacade::scenarioChangeStorage()
{
    if (s_scenarioChangeStorage == nullptr) {
        s_scenarioChangeStorage = new ScenarioChangeStorage;
    }
    return s_scenarioChangeStorage;
}

ScenarioDataStorage* StorageFacade::scenarioDataStorage()
{
    if (s_scenarioDataStorage == nullptr) {
        s_scenarioDataStorage = new ScenarioDataStorage;
    }
    return s_scenarioDataStorage;
}

ResearchStorage* StorageFacade::researchStorage()
{
    if (s_researchStorage == nullptr) {
        s_researchStorage = new ResearchStorage;
    }
    return s_researchStorage;
}

SettingsStorage* StorageFacade::settingsStorage()
{
    if (s_settingsStorage == nullptr) {
        s_settingsStorage = new SettingsStorage;
    }
    return s_settingsStorage;
}

DatabaseHistoryStorage* StorageFacade::databaseHistoryStorage()
{
    if (s_databaseHistoryStorage == nullptr) {
        s_databaseHistoryStorage = new DatabaseHistoryStorage;
    }
    return s_databaseHistoryStorage;
}

ScriptVersionStorage* StorageFacade::scriptVersionStorage()
{
    if (s_scriptVersionStorage == nullptr) {
        s_scriptVersionStorage = new ScriptVersionStorage;
    }
    return s_scriptVersionStorage;
}

PlaceStorage* StorageFacade::s_placeStorage = nullptr;
ScenarioDayStorage* StorageFacade::s_scenarioDayStorage = nullptr;
TimeStorage* StorageFacade::s_timeStorage = nullptr;
CharacterStateStorage* StorageFacade::s_characterStateStorage = nullptr;
TransitionStorage* StorageFacade::s_transitionsStorage = nullptr;
ScenarioStorage* StorageFacade::s_scenarioStorage = nullptr;
ScenarioChangeStorage* StorageFacade::s_scenarioChangeStorage = nullptr;
ScenarioDataStorage* StorageFacade::s_scenarioDataStorage = nullptr;
ResearchStorage* StorageFacade::s_researchStorage = nullptr;
SettingsStorage* StorageFacade::s_settingsStorage = nullptr;
DatabaseHistoryStorage* StorageFacade::s_databaseHistoryStorage = nullptr;
ScriptVersionStorage* StorageFacade::s_scriptVersionStorage = nullptr;
