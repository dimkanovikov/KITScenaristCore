#ifndef SCRIPTTEXTCORRECTOR_H
#define SCRIPTTEXTCORRECTOR_H

#include <QObject>
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
        struct BlockItem {
            qreal topMargin;
            qreal bottomMargin;
            int linesCount;
            qreal lineHeight;

        };

        /**
         * @brief Модель блоков
         */
        QVector<BlockItem> m_blockItems;
    };
}

#endif // SCRIPTTEXTCORRECTOR_H
