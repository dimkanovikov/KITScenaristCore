#include "FolderHeaderHandler.h"

#include "../ScenarioTextEdit.h"
#include "../ScenarioTextEditHelpers.h"

#include <BusinessLayer/ScenarioDocument/ScenarioTextBlockInfo.h>

#include <QKeyEvent>
#include <QTextBlock>

using namespace KeyProcessingLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;


FolderHeaderHandler::FolderHeaderHandler(ScenarioTextEdit* _editor) :
	StandardKeyHandler(_editor)
{
}

void FolderHeaderHandler::handleEnter(QKeyEvent*)
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
		// Ни чего не делаем
		//
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
				// Ни чего не делаем
				//
				editor()->changeScenarioBlockType(changeForEnter(ScenarioBlockStyle::FolderHeader));
			} else {
				//! Текст не пуст

				if (cursorBackwardText.isEmpty()) {
					//! В начале блока

					//
					// Вставить блок время и место перед папкой
                    //
					cursor.insertBlock();
					cursor.movePosition(QTextCursor::PreviousCharacter);
					cursor.setBlockFormat(QTextBlockFormat());
					editor()->setTextCursor(cursor);
					editor()->changeScenarioBlockType(ScenarioBlockStyle::SceneHeading);
                    editor()->moveCursor(QTextCursor::NextCharacter);

                    //
                    // Перенесём параметры из блока в котором они остались к текущему блоку
                    //
                    QTextCursor cursor = editor()->textCursor();
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*> (cursor.block().userData())) {
                        SceneHeadingBlockInfo* movedInfo = info->clone();
                        cursor.block().setUserData(nullptr);
                        cursor.movePosition(QTextCursor::NextBlock);
                        cursor.block().setUserData(movedInfo);
                    }
				} else if (cursorForwardText.isEmpty()) {
					//! В конце блока

					//
					// Вставить блок время и место
					//
					editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::FolderHeader));
                    editor()->textCursor().block().setUserData(nullptr);
				} else {
					//! Внутри блока

					//
					// Вставить блок время и место
					//
					editor()->addScenarioBlock(ScenarioBlockStyle::SceneHeading);
                    editor()->textCursor().block().setUserData(nullptr);
				}
			}
		}
	}
}

void FolderHeaderHandler::handleTab(QKeyEvent*)
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
		// Ни чего не делаем
		//
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
				// Ни чего не делаем
				//
				editor()->changeScenarioBlockType(changeForTab(ScenarioBlockStyle::FolderHeader));
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
					// Как ENTER
					//
					editor()->addScenarioBlock(jumpForTab(ScenarioBlockStyle::FolderHeader));
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

void FolderHeaderHandler::handleOther(QKeyEvent* _event)
{
	//
	// Если не было введено текста, прерываем операцию
	// _event->key() == -1 // событие посланное редактором текста, его необходимо обработать
	//
	if (_event == 0
		|| (_event->key() != -1 && _event->text().isEmpty())) {
		return;
	}

    updateFooter();
}

void FolderHeaderHandler::handleInput(QInputMethodEvent* _event)
{
    //
    // Если не было введено текста, прерываем операцию
    //
    if (_event->commitString().isEmpty()
        || _event->preeditString().isEmpty()) {
        return;
    }

    updateFooter();
}

void FolderHeaderHandler::updateFooter()
{
    QTextCursor cursor = editor()->textCursor();

    //
    // Если редактируется заголовок группы
    //
    if (editor()->scenarioBlockType() == ScenarioBlockStyle::FolderHeader) {
        //
        // ... открытые группы на пути поиска необходимого для обновления блока
        //
        int openedGroups = 0;
        bool isFooterUpdated = false;
        while (!isFooterUpdated && !cursor.atEnd()) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);

            ScenarioBlockStyle::Type currentType =
                    ScenarioBlockStyle::forBlock(cursor.block());

            if (currentType == ScenarioBlockStyle::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.insertText(Helpers::footerText(editor()->textCursor().block().text()));
                    isFooterUpdated = true;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScenarioBlockStyle::FolderHeader) {
                // ... встретилась новая группа
                ++openedGroups;
            }
        }
    }
}
