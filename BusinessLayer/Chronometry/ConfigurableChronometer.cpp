#include "ConfigurableChronometer.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>

using namespace DataStorageLayer;
using namespace BusinessLogic;


ConfigurableChronometer::ConfigurableChronometer()
{
}

QString ConfigurableChronometer::name() const
{
    return "configurable-chronometer";
}

qreal ConfigurableChronometer::calculateFrom(const QTextBlock& _block, int _from, int _length) const
{
    Q_UNUSED(_from);

    const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(_block);
    if (blockType != ScenarioBlockStyle::SceneHeading
        && blockType != ScenarioBlockStyle::Action
        && blockType != ScenarioBlockStyle::Dialogue
        && blockType != ScenarioBlockStyle::Lyrics) {
        return 0;
    }

    //
    // Длительность зависит от блока
    //
    qreal secondsForParagraph = 0;
    qreal secondsForEvery50 = 0;
    QString secondsForParagraphKey;
    QString secondsForEvery50Key;

    if (blockType == ScenarioBlockStyle::Action) {
        secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/action";
        secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/action";
    } else if (blockType == ScenarioBlockStyle::Dialogue
               || blockType == ScenarioBlockStyle::Lyrics) {
        secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/dialog";
        secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/dialog";
    } else {
        secondsForParagraphKey = "chronometry/configurable/seconds-for-paragraph/scene_heading";
        secondsForEvery50Key = "chronometry/configurable/seconds-for-every-50/scene_heading";
    }

    //
    // Получим значения длительности
    //
    secondsForParagraph =
            StorageFacade::settingsStorage()->value(
                secondsForParagraphKey,
                SettingsStorage::ApplicationSettings)
            .toDouble();

    secondsForEvery50 =
            StorageFacade::settingsStorage()->value(
                secondsForEvery50Key,
                SettingsStorage::ApplicationSettings)
            .toDouble();

    const int every50 = 50;
    const qreal secondsPerCharacter = secondsForEvery50 / every50;
    const qreal textChron = secondsForParagraph + _length * secondsPerCharacter;
    return textChron;
}
