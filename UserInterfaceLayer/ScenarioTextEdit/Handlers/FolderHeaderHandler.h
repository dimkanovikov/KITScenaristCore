#ifndef FOLDERHEADERHANDLER_H
#define FOLDERHEADERHANDLER_H

#include "StandardKeyHandler.h"


namespace KeyProcessingLayer
{
	/**
	 * @brief Класс выполняющий обработку нажатия клавиш в блоке заголовка папки
	 */
	class FolderHeaderHandler : public StandardKeyHandler
	{
	public:
		FolderHeaderHandler(UserInterface::ScenarioTextEdit* _editor);

	protected:
		/**
		 * @brief Реализация интерфейса AbstractKeyHandler
		 */
		/** @{ */
        void handleEnter(QKeyEvent* _event = 0) override;
        void handleTab(QKeyEvent* _event = 0) override;
        void handleOther(QKeyEvent* _event = 0) override;
        void handleInput(QInputMethodEvent* _event) override;
		/** @} */

    private:
        /**
         * @brief Найти закрывающий блок и обновить его текст
         */
        void updateFooter();
	};
}

#endif // FOLDERHEADERHANDLER_H
