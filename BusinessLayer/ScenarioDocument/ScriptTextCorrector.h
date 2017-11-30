#ifndef SCRIPTTEXTCORRECTOR_H
#define SCRIPTTEXTCORRECTOR_H

#include <QObject>
#include <QHash>
#include <QVector>

class QTextDocument;


namespace BusinessLogic
{
    /**
     * @brief Класс корректирующий текст сценария
     */
    class ScriptTextCorrector : public QObject
    {
        Q_OBJECT

    public:
        explicit ScriptTextCorrector(QTextDocument* _document);

        /**
         * @brief Скорректировать текст сценария
         */
        void correct();

    private:
        /**
         * @brief Документ который будем корректировать
         */
        QTextDocument* m_document = nullptr;

        /**
         * @brief Структура элемента модели блоков
         */
        struct BlockInfo {
            /**
             * @brief Высота блока
             */
            qreal height;
            /**
             * @brief Позиция блока от начала страницы
             */
            qreal top;
        };

        /**
         * @brief Модель блоков <порядковый номер блока, параметры блока>
         */
        QHash<int, BlockInfo> m_blockItems;
    };
}

#endif // SCRIPTTEXTCORRECTOR_H
