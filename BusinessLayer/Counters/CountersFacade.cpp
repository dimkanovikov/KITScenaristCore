#include "CountersFacade.h"

#include "Counter.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QApplication>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using BusinessLogic::CountersFacade;
using BusinessLogic::Counter;
using BusinessLogic::ScenarioBlockStyle;


Counter CountersFacade::calculate(QTextDocument* _document, int _fromCursorPosition, int _toCursorPosition)
{
    //
    // Проверить какие счётчики необходимо рассчитать
    //
    bool calculateWords =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "counters/words/used",
                DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();
    bool calculateCharacters =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "counters/simbols/used",
                DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();

    Counter counter;
    //
    // Рассчитываем, если необходимо
    //
    if (calculateWords || calculateCharacters) {
        //
        // Определим текст, который необходимо обсчитать
        //
        QTextCursor cursor(_document);
        cursor.setPosition(_fromCursorPosition);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

            const QTextBlock& block = cursor.block();
            const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
            if (block.isVisible()
                && blockType != ScenarioBlockStyle::NoprintableText
                && blockType != ScenarioBlockStyle::FolderHeader
                && blockType != ScenarioBlockStyle::FolderFooter) {
                QString text = cursor.selectedText();
                if (calculateWords) {
                    counter.addWords(wordsCount(text));
                }
                if (calculateCharacters) {
                    counter.addCharactersWithSpaces(charactersWithSpacesCount(text));
                    counter.addCharactersWithoutSpaces(charactersWithoutSpacesCount(text));
                }
            }

            cursor.movePosition(QTextCursor::NextCharacter);
        } while (!cursor.atEnd()
                 && cursor.position() < _toCursorPosition);
    }

    return counter;
}

BusinessLogic::Counter CountersFacade::calculateFull(QTextDocument* _document)
{
    Counter counter;
    QTextBlock block = _document->begin();
    while (block.isValid()) {
        const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(block);
        if (blockType != ScenarioBlockStyle::NoprintableText
            && blockType != ScenarioBlockStyle::FolderHeader
            && blockType != ScenarioBlockStyle::FolderFooter) {
            const Counter blockCounter = calculateFull(block);
            counter.addWords(blockCounter.words());
            counter.addCharactersWithoutSpaces(blockCounter.charactersWithoutSpaces());
            counter.addCharactersWithSpaces(blockCounter.charactersWithSpaces());
        }

        block = block.next();
    }

    return counter;
}

BusinessLogic::Counter CountersFacade::calculateFull(const QTextBlock& _block)
{
    Counter counter;
    //
    // Считаем только видимые блоки
    //
    const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(_block);
    if (_block.isVisible()
        && blockType != ScenarioBlockStyle::NoprintableText
        && blockType != ScenarioBlockStyle::FolderHeader
        && blockType != ScenarioBlockStyle::FolderFooter) {
        //
        // Определим текст, который необходимо обсчитать
        //
        QString text = _block.text();
        //
        // ... слова
        //
        counter.setWords(wordsCount(text));
        //
        // ... символы
        //
        counter.setCharactersWithSpaces(charactersWithSpacesCount(text));
        counter.setCharactersWithoutSpaces(charactersWithoutSpacesCount(text));
    }

    return counter;
}

QStringList CountersFacade::countersInfo(int pageCount, const BusinessLogic::Counter& _counter)
{
    //
    // Проверить какие счётчики необходимо рассчитать
    //
    bool calculatePages =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "counters/pages/used",
                DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();
    bool calculateWords =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "counters/words/used",
                DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();
    bool calculateCharacters =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "counters/simbols/used",
                DataStorageLayer::SettingsStorage::ApplicationSettings).toInt();
    //
    // Рассчитать счётчики
    //
    QStringList result;
    if (calculatePages) {
        result.append(pageInfo(pageCount));
    }

    if (calculateWords) {
        result.append(wordsInfo(_counter.words()));
    }

    if (calculateCharacters) {
        result.append(charactersInfo(_counter.charactersWithSpaces(), _counter.charactersWithoutSpaces()));
    }

    return result;
}

int CountersFacade::wordsCount(const QString& _text)
{
    return _text.split(" ", QString::SkipEmptyParts).count();
}

int CountersFacade::charactersWithSpacesCount(const QString& _text)
{
    return _text.size();
}

int CountersFacade::charactersWithoutSpacesCount(const QString& _text)
{
    return QString(_text).replace(" ", "").size();
}

QString CountersFacade::pageInfo(int _count)
{
    return QString(" %1: <b>%2</b>")
            .arg(QApplication::translate("BusinessLogic::CountersFacade", "Pages"))
            .arg(_count);
}

QString CountersFacade::wordsInfo(int _count)
{
    return QString(" %1: <b>%2</b>")
            .arg(QApplication::translate("BusinessLogic::CountersFacade", "Words"))
            .arg(_count);
}

QString CountersFacade::charactersInfo(int _countWithSpaces, int _countWithoutSpaces)
{
    return QString(" %1: <b>%2 | %3</b>")
            .arg(QApplication::translate("BusinessLogic::CountersFacade", "Symbols"))
            .arg(_countWithoutSpaces).arg(_countWithSpaces);
}
