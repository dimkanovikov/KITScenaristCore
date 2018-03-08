#include "PagesChronometer.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>
#include <QTextDocument>

using namespace DataStorageLayer;
using namespace BusinessLogic;


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

    return blockHeight * seconds / pageHeight;
}
