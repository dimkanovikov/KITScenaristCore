#ifndef SCRIPTTEXTCORRECTOR_H
#define SCRIPTTEXTCORRECTOR_H

#include <QVector>


namespace BusinessLogic
{
    class ScenarioTextDocument;

    /**
     * @brief Класс корректирующий текст сценария
     */
    class ScriptTextCorrector
    {
    public:
        explicit ScriptTextCorrector(ScenarioTextDocument* _document);

        /**
         * @brief Скорректировать текст сценария начиная с заданной позиции
         */
        void correct(int _startPosition = 0);

    private:
        /**
         * @brief Документ который будем корректировать
         */
        ScenarioTextDocument* m_document = nullptr;

        /**
         * @brief Структура элемента модели блоков
         */
        struct BlockItem {
            int height = 0;

        };

        /**
         * @brief Модель блоков
         */
        QVector<BlockItem> m_items;
    };
}

#endif // SCRIPTTEXTCORRECTOR_H
