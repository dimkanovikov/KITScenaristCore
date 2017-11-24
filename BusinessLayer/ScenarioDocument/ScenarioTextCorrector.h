#ifndef SCENARIOTEXTCORRECTOR_H
#define SCENARIOTEXTCORRECTOR_H

class QTextBlock;
class QTextCursor;
class QTextDocument;


namespace BusinessLogic
{
    class ScenarioTextDocument;

    /**
     * @brief Класс корректирующий текст сценария
     */
    class ScenarioTextCorrector
    {
    public:
        /**
         * @brief Настроить работу корректора
         */
        static void configure(bool _continueDialogues, bool _correctTextOnPageBreaks);

        /**
         * @brief Удалить декорации в заданном интервале текста. Если _endPosition равен нулю, то
         *		  удалять до конца.
         */
        static void removeDecorations(QTextDocument* _document, int _startPosition = 0, int _endPosition = 0);

        /**
         * @brief Скорректировать сценарий на разрывах страниц
         */
        static void correctScenarioText(ScenarioTextDocument* _document, int _startPosition, bool _force = false);

        /**
         * @brief Скорректировать документ на разрывах страниц
         * @note Используется при экспорте
         */
        static void correctDocumentText(QTextDocument* _document, int _startPosition = 0);

    private:
        /**
         * @brief Продолжать диалоги
         */
        static bool s_continueDialogues;

        /**
         * @brief Корректировать текст на разрывах страниц
         */
        static bool s_correctTextOnPageBreaks;
    };
}

#endif // SCENARIOTEXTCORRECTOR_H
