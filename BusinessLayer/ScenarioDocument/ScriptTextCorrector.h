#ifndef SCRIPTTEXTCORRECTOR_H
#define SCRIPTTEXTCORRECTOR_H

#include <QHash>
#include <QMap>
#include <QObject>
#include <QSizeF>
#include <QVector>

#include <cmath>

class QTextBlock;
class QTextBlockFormat;
class QTextDocument;


namespace BusinessLogic
{
    class ScriptTextCursor;

    /**
     * @brief Класс корректирующий текст сценария
     */
    class ScriptTextCorrector : public QObject
    {
        Q_OBJECT

    public:
        /**
         * @brief Автоматически добавляемые продолжения в диалогах
         */
        static QString continuedTerm();

    public:
        explicit ScriptTextCorrector(QTextDocument* _document, const QString& _templateName = QString());

        /**
         * @brief Установить необходимость корректировать текст блоков имён персонажей
         */
        void setNeedToCorrectCharactersNames(bool _need);

        /**
         * @brief Установить необходимость корректировать текст на разрывах страниц
         */
        void setNeedToCorrectPageBreaks(bool _need);

        /**
         * @brief Очистить все сохранённые параметры
         */
        void clear();

        /**
         * @brief Выполнить корректировки
         */
        void correct(int _position = -1, int _charsRemoved = 0, int _charsAdded = 0);

        /**
         * @brief Определить позицию курсора с учётом декораций по позиции без учёта декораций
         */
        int correctedPosition(int _position) const;

    private:
        /**
         * @brief Скорректировать имена персонажей
         */
        void correctCharactersNames(int _position = -1, int _charsRemoved = 0, int _charsAdded = 0);

        /**
         * @brief Скорректировать текст сценария
         */
        void correctPageBreaks(int _position = -1);

        //
        // Функции работающие в рамках текущей коррекции
        //

        /**
         * @brief Сместить текущий блок вместе с тремя предыдущими на следующую страницу
         */
        void moveCurrentBlockWithThreePreviousToNextPage(const QTextBlock& _prePrePreviousBlock,
            const QTextBlock& _prePreviousBlock, const QTextBlock& _previousBlock, qreal _pageHeight,
            qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight);

        /**
         * @brief Сместить текущий блок вместе с двумя предыдущими на следующую страницу
         */
        void moveCurrentBlockWithTwoPreviousToNextPage(const QTextBlock& _prePreviousBlock,
            const QTextBlock& _previousBlock, qreal _pageHeight, qreal _pageWidth,
            ScriptTextCursor& _cursor, QTextBlock& _block, qreal& _lastBlockHeight);

        /**
         * @brief Сместить текущий блок вместе с предыдущим на следующую страницу
         */
        void moveCurrentBlockWithPreviousToNextPage(const QTextBlock& _previousBlock,
            qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block,
            qreal& _lastBlockHeight);

        /**
         * @brief Сместить текущий блок на следующую страницу
         */
        void moveCurrentBlockToNextPage(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
            qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block,
            qreal& _lastBlockHeight);

        /**
         * @brief Разорвать блок диалога
         */
        void breakDialogue(const QTextBlockFormat& _blockFormat, qreal _blockHeight,
            qreal _pageHeight, qreal _pageWidth, ScriptTextCursor& _cursor, QTextBlock& _block,
            qreal& _lastBlockHeight);

        //
        // Вспомогательные функции
        //

        /**
         * @brief Найти предыдущий блок
         */
        QTextBlock findPreviousBlock(const QTextBlock& _block);

        /**
         * @brief Найти следующий блок, который не является декорацией
         */
        QTextBlock findNextBlock(const QTextBlock& _block);

        /**
         * @brief Сместить блок в начало следующей страницы
         * @param _block - блок для смещения
         * @param _spaceToPageEnd - количество места до конца страницы
         * @param _pageHeight - высота страницы
         * @param _cursor - курсор редактироуемого документа
         */
        void moveBlockToNextPage(const QTextBlock& _block, qreal _spaceToPageEnd, qreal _pageHeight,
            qreal _pageWidth, ScriptTextCursor& _cursor);

    private:
        /**
         * @brief Документ который будем корректировать
         */
        QTextDocument* m_document = nullptr;

        /**
         * @brief Шаблон оформления сценария
         */
        QString m_templateName;

        /**
         * @brief Необходимо ли корректировать текст блоков имён персонажей
         */
        bool m_needToCorrectCharactersNames = false;

        /**
         * @brief Необходимо ли корректировать текст на разрывах страниц
         */
        bool m_needToCorrectPageBreaks = true;

        /**
         * @brief Размер документа при последней проверке
         */
        QSizeF m_lastDocumentSize;

        /**
         * @brief Номер текущего блока при корректировке
         * @note Используем собственный счётчик номеров, т.к. во время
         *       коррекций номера блоков могут скакать в QTextBlock
         */
        int m_currentBlockNumber = 0;

        /**
         * @brief Структура элемента модели блоков
         */
        struct BlockInfo {
            BlockInfo() = default;
            explicit BlockInfo(qreal _height, qreal _top) :
                height(_height),
                top(_top)
            {}

            /**
             * @brief Валиден ли блок
             */
            bool isValid() const {
                return !std::isnan(height) && !std::isnan(top);
            }

            /**
             * @brief Высота блока
             */
            qreal height = std::numeric_limits<qreal>::quiet_NaN();

            /**
             * @brief Позиция блока от начала страницы
             */
            qreal top = std::numeric_limits<qreal>::quiet_NaN();
        };

        /**
         * @brief Модель параметров блоков
         */
        QVector<BlockInfo> m_blockItems;

        /**
         * @brief Список декораций документа <позиция, длина>
         * @note Используется для поиска положения курсора при наложении патча
         */
        QMap<int, int> m_decorations;
    };
}

#endif // SCRIPTTEXTCORRECTOR_H
