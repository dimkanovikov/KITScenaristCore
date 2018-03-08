#include "PagesChronometer.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>

using namespace DataStorageLayer;
using namespace BusinessLogic;


PagesChronometer::PagesChronometer()
{
}

QString PagesChronometer::name() const
{
    return "pages-chronometer";
}

float PagesChronometer::calculateFrom(const QTextBlock& _block, int _from, int _lenght) const
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
    // Получим значение длительности одной страницы текста
    //
    const int seconds =
            StorageFacade::settingsStorage()->value(
                "chronometry/pages/seconds",
                SettingsStorage::ApplicationSettings)
            .toInt();

    //
    // Высчитываем длительность строки на странице, из знания о том, сколько строк на странице
    //
    const float linesPerPage = 54;
    const float lineChron = seconds / linesPerPage;

    //
    // Длина строки в зависимости от типа
    //
    int lineLength = 0;

    //
    // Количество дополнительных строк
    // По-умолчанию равно единице, чтобы учесть отступ перед блоком
    //
    int additionalLines = 1;

    switch (blockType) {
        case ScenarioBlockStyle::SceneCharacters: {
            lineLength = 58;
            additionalLines = 0;
            break;
        }

        case ScenarioBlockStyle::Character: {
            lineLength = 31;
            break;
        }

        case ScenarioBlockStyle::Dialogue: {
            lineLength = 28;
            additionalLines = 0;
            break;
        }

        case ScenarioBlockStyle::Parenthetical: {
            lineLength = 18;
            additionalLines = 0;
            break;
        }

        case ScenarioBlockStyle::Title: {
            lineLength = 18;
            break;
        }

        case ScenarioBlockStyle::Lyrics: {
            lineLength = 35;
            break;
        }

        default: {
            lineLength = 58;
            break;
        }
    }

    //
    // Подсчитаем хронометраж
    //
    const QString text = _block.text().mid(_from, _lenght);
    const float textChron = (float)(linesInText(text, lineLength) + additionalLines) * lineChron;
    return textChron;
}

int PagesChronometer::linesInText(const QString& _text, int _lineLength) const
{
    //
    // Переносы не должны разрывать текст
    //
    // Берём текст на границе блока, если попадаем в слово, то идём назад до пробела
    // дальше делаем отступ на ширину блока от текущего места и опять идём назад до пробела
    // до тех пор, пока не зайдём за конец блока
    //
    const int textLength = _text.length();
    int currentPosition = _lineLength;
    int lastCurrentPosition = 0;
    int linesCount = 1;
    while (currentPosition < textLength) {
        if (_text.at(currentPosition) == ' ') {
            ++linesCount;
            lastCurrentPosition = currentPosition;
            currentPosition += _lineLength;
        } else {
            --currentPosition;
        }

        //
        // Ставим предохранитель на случай длинных слов, которые превышают допустимую ширину блока
        //
        if (currentPosition == lastCurrentPosition) {
            linesCount = _text.length() / _lineLength + ((_text.length() % _lineLength > 0) ? 1 : 0);
            break;
        }
    }

    return linesCount;
}
