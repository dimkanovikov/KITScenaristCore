#ifndef SCENEHEADINGHANDLER_H
#define SCENEHEADINGHANDLER_H

#include "StandardKeyHandler.h"


namespace KeyProcessingLayer
{
	/**
	 * @brief Класс выполняющий обработку нажатия клавиш в блоке время и место
	 */
	class SceneHeadingHandler : public StandardKeyHandler
	{
	public:
		SceneHeadingHandler(UserInterface::ScenarioTextEdit* _editor);

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
         * @brief Сохранить данные сцены
         */
		void storeSceneParameters() const;
	};
}

#endif // SCENEHEADINGHANDLER_H
