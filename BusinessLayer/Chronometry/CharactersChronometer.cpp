#include "CharactersChronometer.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>

using namespace DataStorageLayer;
using namespace BusinessLogic;


CharactersChronometer::CharactersChronometer()
{
}

QString CharactersChronometer::name() const
{
    return "characters-chronometer";
}

float CharactersChronometer::calculateFrom(const QTextBlock& _block, int _from, int _length) const
{
    //
    // Не включаем в хронометраж непечатный текст, заголовок и окончание папки, а также описание сцены
    //
    const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(_block);
    if (blockType == ScenarioBlockStyle::NoprintableText
        || blockType == ScenarioBlockStyle::FolderHeader
        || blockType == ScenarioBlockStyle::FolderFooter
        || blockType == ScenarioBlockStyle::SceneDescription) {
        return 0;
    }

    //
    // Рассчитаем длительность одного символа
    //
    const int characters =
            StorageFacade::settingsStorage()->value(
                "chronometry/characters/characters",
                SettingsStorage::ApplicationSettings)
            .toInt();
    const int seconds =
            StorageFacade::settingsStorage()->value(
                "chronometry/characters/seconds",
                SettingsStorage::ApplicationSettings)
            .toInt();
    const bool considerSpaces =
            StorageFacade::settingsStorage()->value(
                "chronometry/characters/consider-spaces",
                SettingsStorage::ApplicationSettings)
            .toInt();
    const float characterChron = (float)seconds / (float)characters;

    //
    // Рассчитаем длительность текста
    //
    QString textForChron = _block.text().mid(_from, _length);
    textForChron = textForChron.remove("\n").simplified();
    if (!considerSpaces) {
        textForChron = textForChron.remove(" ");
    }

    const float textChron = textForChron.length() * characterChron;
    return textChron;
}
