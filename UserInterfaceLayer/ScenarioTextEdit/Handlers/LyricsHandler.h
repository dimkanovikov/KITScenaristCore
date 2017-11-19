#ifndef LYRICSHANDLER_H
#define LYRICSHANDLER_H

#include "StandardKeyHandler.h"


namespace KeyProcessingLayer
{
    /**
     * @brief Класс выполняющий обработку нажатия клавиш в блоке лирики
     */
    class LyricsHandler : public StandardKeyHandler
    {
    public:
        LyricsHandler(UserInterface::ScenarioTextEdit* _editor);

    protected:
        /**
         * @brief Реализация интерфейса AbstractKeyHandler
         */
        /** @{ */
        void handleEnter(QKeyEvent* _event = 0);
        void handleTab(QKeyEvent* _event = 0);
        void handleOther(QKeyEvent *_event);
        /** @} */
    };
}

#endif // LYRICSHANDLER_H
