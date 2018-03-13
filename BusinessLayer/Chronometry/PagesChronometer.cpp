#include "PagesChronometer.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>
#include <QTextDocument>

using namespace DataStorageLayer;
using namespace BusinessLogic;

namespace {
    /**
     * @brief Подсчитать количество строк в тексте исходя из знания
     *        о том сколько символов вмещается в одну строку
     */
    static int linesInText(const QString& _text, int _lineLength)
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
}


PagesChronometer::PagesChronometer()
{
}

QString PagesChronometer::name() const
{
    return "pages-chronometer";
}

qreal PagesChronometer::calculateFrom(const QTextBlock& _block, int _from, int _lenght) const
{
    Q_UNUSED(_from);
    Q_UNUSED(_lenght);

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
    const qreal seconds =
            StorageFacade::settingsStorage()->value(
                "chronometry/pages/seconds",
                SettingsStorage::ApplicationSettings)
            .toInt();

    //
    // Если работаем в постраничном режиме, то определяем хронометраж по факту
    //
    qreal chron = 0.0;
    if (_block.document()->pageSize().height() > 0) {
        //
        // Определить высоту текущего блока
        //
        const QTextBlockFormat blockFormat = _block.blockFormat();
        const qreal blockLineHeight = blockFormat.lineHeight();
        const int blockLineCount = _block.layout()->lineCount();
        //
        // ... если блок первый на странице, то для него не нужно учитывать верхний отступ
        //
        const qreal blockHeight = blockLineHeight * blockLineCount + blockFormat.topMargin() + blockFormat.bottomMargin();

        //
        // Определим высоту страницы
        //
        const QTextFrameFormat rootFrameFormat = _block.document()->rootFrame()->frameFormat();
        const qreal pageHeight = _block.document()->pageSize().height()
                                 - rootFrameFormat.topMargin()
                                 - rootFrameFormat.bottomMargin();

        chron = blockHeight * seconds / pageHeight;
    }
    //
    // В противном случае, считаем по символам как раньше и было
    //
    else {
        int lineLength = 0;
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
        const float linesPerPage = 54;
        const float lineChron = seconds / linesPerPage;
        chron = (qreal)(linesInText(text, lineLength) + additionalLines) * lineChron;
    }

    return chron;
}
