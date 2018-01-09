#ifndef CHARACTERHANDLER_H
#define CHARACTERHANDLER_H

#include "StandardKeyHandler.h"

class QStringListModel;


namespace KeyProcessingLayer
{
	/**
	 * @brief Класс выполняющий обработку нажатия клавиш в блоке персонажа
	 */
	class CharacterHandler : public StandardKeyHandler
	{
	public:
		CharacterHandler(UserInterface::ScenarioTextEdit* _editor);

		/**
		 * @brief При входе в блок, пробуем определить персонажа, который будет говорить
		 */
		void prehandle();

	protected:
		/**
		 * @brief Реализация интерфейса AbstractKeyHandler
		 */
		/** @{ */
        void handleEnter(QKeyEvent* _event = nullptr) override;
        void handleTab(QKeyEvent* _event = nullptr) override;
        void handleOther(QKeyEvent* _event = nullptr) override;
        void handleInput(QInputMethodEvent* _event) override;
		/** @} */

    private:
        /**
         * @brief Показать автодополнение, если это возможно
         */
        void complete(const QString& _currentBlockText, const QString& _cursorBackwardText);

        /**
         * @brief Сохранить персонажа
         */
		void storeCharacter() const;

	private:
		/**
		 * @brief Модель персонажей текущей сцены
		 */
		QStringListModel* m_sceneCharactersModel;
	};
}

#endif // CHARACTERHANDLER_H
