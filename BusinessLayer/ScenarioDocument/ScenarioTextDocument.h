#ifndef SCENARIOTEXTDOCUMENT_H
#define SCENARIOTEXTDOCUMENT_H

#include "ScenarioTemplate.h"
#include <3rd_party/Helpers/DiffMatchPatchHelper.h>

#include <QTextDocument>
#include <QTextCursor>

namespace Domain {
    class ScenarioChange;
}

class QDomNodeList;

namespace BusinessLogic
{
    class ScenarioReviewModel;
    class ScenarioXml;
    class ScriptBookmarksModel;
    class ScriptTextCorrector;


    /**
     * @brief Расширение класса текстового документа для предоставления интерфейса для обработки mime
     */
    class ScenarioTextDocument : public QTextDocument
    {
        Q_OBJECT

    public:
        /**
         * @brief Обновить ревизию блока
         * @note Это приходится делать вручную, т.к. изменения пользовательских свойств блока
         *		 не отслеживаются автоматически
         */
        /** @{ */
        static void updateBlockRevision(QTextBlock& _block);
        static void updateBlockRevision(QTextCursor& _cursor);
        /** @} */

    public:
        explicit ScenarioTextDocument(QObject *parent, ScenarioXml* _xmlHandler);

        /**
         * @brief Сформировать xml из сценария и рассчитать его хэш
         */
        bool updateScenarioXml();

        /**
         * @brief Получить xml сценария
         */
        QString scenarioXml() const;

        /**
         * @brief Получить текущий хэш сценария
         */
        QByteArray scenarioXmlHash() const;

        /**
         * @brief Загрузить сценарий
         */
        void load(const QString& _scenarioXml);

        /**
         * @brief Получить майм представление данных в указанном диапазоне
         */
        QString mimeFromSelection(int _startPosition, int _endPosition) const;

        /**
         * @brief Вставить данные в указанную позицию документа
         */
        void insertFromMime(int _insertPosition, const QString& _mimeData);

        /**
         * @brief Можно ли применить патч так, чтобы он не скломал документ
         */
        bool canApplyPatch(const QString& _patch) const;

        /**
         * @brief Применить патч
         */
        int applyPatch(const QString& _patch, bool _checkXml = false);

        /**
         * @brief Применить множество патчей
         * @note Метод для оптимизации, перестраивается весь документ
         */
        void applyPatches(const QList<QString>& _patches);

        /**
         * @brief Сохранить изменения текста
         */
        Domain::ScenarioChange* saveChanges();

        /**
         * @brief Собственные реализации отмены/повтора последнего действия
         * @param _forced - при задании истинным, отменяемое действие не переносится в список
         *                  действий доступных к повторению и не помещается в историю изменений
         */
        /** @{ */
        int undoReimpl(bool _forced = false);
        void addUndoChange(Domain::ScenarioChange* change);
        void updateUndoStack();
        int redoReimpl();
        /** @} */

        /**
         * @brief Собственные реализации проверки доступности отмены/повтора последнего действия
         */
        /** @{ */
        bool isUndoAvailableReimpl() const;
        bool isRedoAvailableReimpl() const;
        /** @} */

        /**
         * @brief Установить курсор в заданную позицию
         */
        void setCursorPosition(QTextCursor& _cursor, int _position,
            QTextCursor::MoveMode _moveMode = QTextCursor::MoveAnchor);

        /**
         * @brief Получить модель редакторсках правок
         */
        ScenarioReviewModel* reviewModel() const;

        /**
         * @brief Получить модель закладок
         */
        ScriptBookmarksModel* bookmarksModel() const;

        /**
         * @brief Документ в режиме отображения поэпизодника или сценария
         */
        bool outlineMode() const;

        /**
         * @brief Установить режим отображения поэпизодника или сценария
         */
        void setOutlineMode(bool _outlineMode);

        /**
         * @brief Получить список видимых блоков в зависимости от режима отображения поэпизодника или сценария
         */
        QList<BusinessLogic::ScenarioBlockStyle::Type> visibleBlocksTypes() const;

        /**
         * @brief Настроить необходимость корректировок
         */
        void setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks);

        /**
         * @brief Произвести корректировки текста
         */
        void correct(int _position = -1, int _charsRemoved = 0, int _charsAdded = 0);

    signals:
        /**
         * @brief Сигналы уведомляющие об этапах применения патчей
         */
        /** @{ */
        void beforePatchesApply();
        void afterPatchesApply();
        /** @} */

        /**
         * @brief В документ были внесены редакторские примечания
         */
        void reviewChanged();

        /**
         * @brief В документе были изменены закладки
         */
        void bookmarksChanged();

        /**
         * @brief Изменилась доступность повтора отменённого действия
         */
        void redoAvailableChanged(bool _isRedoAvailable);

    private:
        /**
         * @brief Обновить идентификаторы изменившихся блоков
         */
        void updateBlocksIds(int _position, int _charsRemoved, int _charsAdded);

        /**
         * @brief Процедура удаления одинаковый первых и последних частей в xml-строках у _xmls
         * _reversed = false - удаляем первые, = true - удаляем последние
         */
        void removeIdenticalParts(QPair<DiffMatchPatchHelper::ChangeXml, DiffMatchPatchHelper::ChangeXml>& _xmls);

    private:
        /**
         * @brief Обработчик xml
         */
        ScenarioXml* m_xmlHandler;

        /**
         * @brief Применяется ли патч в данный момент
         */
        bool m_isPatchApplyProcessed;

        /**
         * @brief  Xml текст сценария и его MD5-хэш
         * @note Xml сценария не должен быть null т.к. он участвует в формировании патчей,
         *       а diffMatchPatch этого не допускает, поэтому инициилизируем его пустой строкой
         */
        /** @{ */
        QString m_scenarioXml = "";
        QByteArray m_scenarioXmlHash;
        /** @} */

        /**
         * @brief Xml текст сценария и его MD5-хэш на момент последнего сохранения изменений
         * @note Xml сценария не должен быть null т.к. он участвует в формировании патчей,
         *       а diffMatchPatch этого не допускает, поэтому инициилизируем его пустой строкой
         */
        /** @{ */
        QString m_lastSavedScenarioXml = "";
        QByteArray m_lastSavedScenarioXmlHash;
        /** @} */

        /**
         * @brief Стеки для отмены/повтора последнего действия
         */
        /** @{ */
        QList<Domain::ScenarioChange*> m_undoStack;
        QList<Domain::ScenarioChange*> m_redoStack;
        /** @{ */

        /**
         * @brief Модель редакторских правок документа
         */
        ScenarioReviewModel* m_reviewModel = nullptr;

        /**
         * @brief Модель закладок документа
         */
        ScriptBookmarksModel* m_bookmarksModel = nullptr;

        /**
         * @brief Включён ли режим отображения поэпизодного плана (true) или сценария (false)
         */
        bool m_outlineMode;

        /**
         * @brief Корректировщик текста документа
         */
        ScriptTextCorrector* m_corrector;
    };
}


#endif // SCENARIOTEXTDOCUMENT_H
