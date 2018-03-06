#include "CharacterHandler.h"

#include "../ScenarioTextEdit.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockParsers.h>

#include <Domain/Research.h>
#include <Domain/CharacterState.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/ResearchStorage.h>
#include <DataLayer/DataStorageLayer/CharacterStateStorage.h>

#include <QKeyEvent>
#include <QStringListModel>
#include <QTextBlock>

using namespace KeyProcessingLayer;
using namespace DataStorageLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;


CharacterHandler::CharacterHandler(ScenarioTextEdit* _editor) :
    StandardKeyHandler(_editor),
    m_sceneCharactersModel(new QStringListModel(_editor))
{
}

void CharacterHandler::prehandle()
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text().trimmed();

    //
    // Пробуем определить кто сейчас должен говорить
    //
    if (currentBlockText.isEmpty()) {
        QString previousCharacter, character;

        //
        // ... для этого ищем предпоследнего персонажа сцены
        //
        cursor.movePosition(QTextCursor::PreviousBlock);
        while (!cursor.atStart()
               && ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::SceneHeading) {
            if (ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::Character) {
                //
                // Нашли предыдущего персонажа
                //
                if (previousCharacter.isEmpty()) {
                    previousCharacter = CharacterParser::name(cursor.block().text());
                }
                //
                // Нашли потенциального говорящего
                //
                else {
                    //
                    // Выберем его в списке вариантов
                    //
                    character = CharacterParser::name(cursor.block().text());
                    if (character != previousCharacter) {
                        break;
                    }
                }
            }

            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::StartOfBlock);
        }

        //
        // Показываем всплывающую подсказку
        //
        QAbstractItemModel* model = 0;
        if (!character.isEmpty()) {
            m_sceneCharactersModel->setStringList(QStringList() << character);
            model = m_sceneCharactersModel;
        } else if (!previousCharacter.isEmpty()) {
            m_sceneCharactersModel->setStringList(QStringList() << previousCharacter);
            model = m_sceneCharactersModel;
        } else {
            model = StorageFacade::researchStorage()->characters();
        }
        editor()->complete(model, QString());
    }
}
void CharacterHandler::handleEnter(QKeyEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text().trimmed();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... текущая секция
    CharacterParser::Section currentSection = CharacterParser::section(cursorBackwardText);


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Вставить выбранный вариант
        //
        editor()->applyCompletion();

        //
        // Обновим курсор, т.к. после автозавершения он смещается
        //
        cursor = editor()->textCursor();

        //
        // Дописать необходимые символы
        //
        switch (currentSection) {
            case CharacterParser::SectionState: {
                cursor.insertText(")");
                break;
            }

            default: {
                break;
            }
        }

        //
        // Если нужно автоматически перепрыгиваем к следующему блоку
        //
        if (_event != 0 // ... чтобы таб не переводил на новую строку
            && autoJumpToNextBlock()
            && currentSection == CharacterParser::SectionName) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::Character));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addScenarioBlock(ScenarioBlockStyle::Character);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль
                //
                editor()->changeScenarioBlockType(changeForEnter(ScenarioBlockStyle::Character));
            } else {
                //! Текст не пуст

                //
                // Сохраним имя персонажа
                //
                storeCharacter();

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставим блок имени героя перед собой
                    //
                    editor()->addScenarioBlock(ScenarioBlockStyle::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::Character));
                } else {
                    //! Внутри блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addScenarioBlock(ScenarioBlockStyle::Dialogue);
                }
            }
        }
    }
}

void CharacterHandler::handleTab(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Работаем, как ENTER
        //
        handleEnter();
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Ни чего не делаем
            //
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль на описание действия
                //
                editor()->changeScenarioBlockType(changeForTab(ScenarioBlockStyle::Character));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Сохраним имя персонажа
                    //
                    storeCharacter();

                    //
                    // Вставить блок ремарки
                    //
                    editor()->addScenarioBlock(jumpForTab(ScenarioBlockStyle::Character));
                } else {
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

void CharacterHandler::handleOther(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    const QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void CharacterHandler::handleInput(QInputMethodEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    int cursorPosition = cursor.positionInBlock();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
#ifdef Q_OS_ANDROID
    QString stringForInsert;
    if (!_event->preeditString().isEmpty()) {
        stringForInsert = _event->preeditString();
    } else {
        stringForInsert = _event->commitString();
    }
    currentBlockText.insert(cursorPosition, stringForInsert);
    cursorPosition += stringForInsert.length();
#endif
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursorPosition);

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void CharacterHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    //
    // Текущая секция
    //
    CharacterParser::Section currentSection = CharacterParser::section(_cursorBackwardText);

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* sectionModel = 0;
    //
    // ... в соответствии со введённым в секции текстом
    //
    QString sectionText;

    QTextCursor cursor = editor()->textCursor();
    switch (currentSection) {
        case CharacterParser::SectionName: {
            QStringList sceneCharacters;
            //
            // Когда введён один символ имени пробуем оптимизировать поиск персонажей из текущей сцены
            //
            if (_cursorBackwardText.length() < 2) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                while (!cursor.atStart()
                       && ScenarioBlockStyle::forBlock(cursor.block()) != ScenarioBlockStyle::SceneHeading) {
                    if (ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::Character) {
                        const QString characterName = CharacterParser::name(cursor.block().text());
                        if (characterName.startsWith(_cursorBackwardText, Qt::CaseInsensitive)
                            && !sceneCharacters.contains(characterName)) {
                            sceneCharacters.append(characterName);
                        }
                    } else if (ScenarioBlockStyle::forBlock(cursor.block()) == ScenarioBlockStyle::SceneCharacters) {
                        const QStringList characters = SceneCharactersParser::characters(cursor.block().text());
                        foreach (const QString& characterName, characters) {
                            if (characterName.startsWith(_cursorBackwardText, Qt::CaseInsensitive)
                                && !sceneCharacters.contains(characterName)) {
                                sceneCharacters.append(characterName);
                            }
                        }
                    }
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    cursor.movePosition(QTextCursor::StartOfBlock);
                }
            }

            //
            // По возможности используем список персонажей сцены
            //
            if (!sceneCharacters.isEmpty()) {
                m_sceneCharactersModel->setStringList(sceneCharacters);
                sectionModel = m_sceneCharactersModel;
            } else {
                sectionModel = StorageFacade::researchStorage()->characters();
            }
            sectionText = CharacterParser::name(_currentBlockText);
            break;
        }

        case CharacterParser::SectionState: {
            sectionModel = StorageFacade::characterStateStorage()->all();
            sectionText = CharacterParser::state(_currentBlockText);
            break;
        }

        default: {
            break;
        }
    }

    //
    // Дополним текст
    //
    editor()->complete(sectionModel, sectionText);
}

void CharacterHandler::storeCharacter() const
{
    if (editor()->storeDataWhenEditing()) {
        //
        // Получим необходимые значения
        //
        // ... курсор в текущем положении
        const QTextCursor cursor = editor()->textCursor();
        // ... блок текста в котором находится курсор
        const QTextBlock currentBlock = cursor.block();
        // ... текст блока
        const QString currentBlockText = currentBlock.text();
        // ... текст до курсора
        const QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
        // ... имя персонажа
        const QString characterName = CharacterParser::name(cursorBackwardText);
        // ... состояние персонажа
        const QString characterState = CharacterParser::state(cursorBackwardText);

        //
        // Сохраняем персонажа
        //
        StorageFacade::researchStorage()->storeCharacter(characterName);
        StorageFacade::characterStateStorage()->storeCharacterState(characterState);
    }
}
