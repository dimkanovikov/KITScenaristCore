#ifndef SCENARIOTEXTEDIT_H
#define SCENARIOTEXTEDIT_H

#include <3rd_party/Widgets/CompletableTextEdit/CompletableTextEdit.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

namespace BusinessLogic {
    class ScenarioTextDocument;
}

class QCompleter;

namespace UserInterface
{
    class ShortcutsManager;


    /**
     * @brief Текстовый редактор сценария
     */
    class ScenarioTextEdit : public CompletableTextEdit
    {
        Q_OBJECT

    public:
        explicit ScenarioTextEdit(QWidget* _parent);

        /**
         * @brief Установить документ для редактирования
         */
        void setScenarioDocument(BusinessLogic::ScenarioTextDocument* _document);

        /**
         * @brief Вставить новый блок
         * @param Тип блока
         */
        void addScenarioBlock(BusinessLogic::ScenarioBlockStyle::Type _blockType);

        /**
         * @brief Установить вид текущего блока
         * @param Тип блока
         */
        void changeScenarioBlockType(BusinessLogic::ScenarioBlockStyle::Type _blockType, bool _forced = false);

        /**
         * @brief Установить заданный тип блока для всех выделенных блоков
         */
        void changeScenarioBlockTypeForSelection(BusinessLogic::ScenarioBlockStyle::Type _blockType);

        /**
         * @brief Применить тип блока ко всему тексту в блоке
         * @param Тип для применения
         */
        void applyScenarioTypeToBlockText(BusinessLogic::ScenarioBlockStyle::Type _blockType);

        /**
         * @brief Получить вид блока в котором находится курсор
         */
        BusinessLogic::ScenarioBlockStyle::Type scenarioBlockType() const;

        /**
         * @brief Своя реализация установки курсора
         */
        void setTextCursorReimpl(const QTextCursor& _cursor);

        /**
         * @brief Получить значение флага сигнализирующего сохранять ли данные во время редактирования
         */
        bool storeDataWhenEditing() const;

        /**
         * @brief Установить значение флага сигнализирующего сохранять ли данные во время редактирования
         */
        void setStoreDataWhenEditing(bool _store);

        /**
         * @brief Показываются ли в редакторе номера сцен
         */
        bool showSceneNumbers() const;

        /**
         * @brief Установить значение необходимости отображать номера сцен
         */
        void setShowSceneNumbers(bool _show);

        /**
         * @brief Префикс номеров сцен
         */
        QString sceneNumbersPrefix() const;

        /**
         * @brief Установить префикс номеров сцен
         */
        void setSceneNumbersPrefix(const QString& _prefix);

        /**
         * @brief Показываеются ли в редакторе номера реплик
         */
        bool showDialoguesNumbers() const;

        /**
         * @brief Установить необходимость показывать номера реплик
         */
        void setShowDialoguesNumbers(bool _show);

        /**
         * @brief Установить необходиость подсветки блоков
         */
        void setHighlightBlocks(bool _highlight);

        /**
         * @brief Установить значение необходимости подсвечивать текущую строку
         */
        void setHighlightCurrentLine(bool _highlight);

        /**
         * @brief Установить необходимость автоматических замен
         */
        void setAutoReplacing(bool _capitalizeFirstWord, bool _correctDoubleCapitals,
            bool _replaceThreeDots, bool _smartQuotes);

        /**
         * @brief Показывать ли автодополнения в пустых блоках
         */
        void setShowCharactersSuggestions(bool _show);
        bool showCharactersSuggestions() const;

        /**
         * @brief Установить необходимость проигрывания звуков клавиатуры
         */
        void setKeyboardSoundEnabled(bool _enabled);

        /**
         * @brief Редактор в режиме отображения поэпизодника или сценария
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
         * @brief Обновить сочетания клавиш для переходов между блоками
         */
        void updateShortcuts();

        /**
         * @brief Получить сочетание смены для блока
         */
        QString shortcut(BusinessLogic::ScenarioBlockStyle::Type _forType) const;

        /**
         * @brief Установить список дополнительных курсоров для отрисовки
         */
        void setAdditionalCursors(const QMap<QString, int>& _cursors);

        /**
         * @brief Пролистать сценарий, чтобы был виден заданный курсор соавтора
         */
        void scrollToAdditionalCursor(int _index);

        /**
         * @brief Установить список действий рецензирования для контекстного меню
         */
        void setReviewContextMenuActions(const QList<QAction*>& _actions);

        /**
         * @brief Создать контекстное меню, чтобы добавить в него дополнительные действия
         * @note Управление памятью передаётся вызывающему метод
         */
        QMenu* createContextMenu(const QPoint& _pos, QWidget* _parent = 0) override;

        /**
         * @brief Доступно ли действие повтора отменённого действия
         */
        bool isRedoAvailable() const;

        /**
         * @brief Настроить форматирование выделенного текста
         */
        /** @{ */
        void setTextBold(bool _bold);
        void setTextItalic(bool _italic);
        void setTextUnderline(bool _underline);
        /** @} */

        /**
         * @brief Задать контекст для менеджера горячих клавиш
         */
        void setShortcutsContextWidget(QWidget* _context);

    signals:
        /**
         * @brief Запрос на отмену последнего действия
         */
        void undoRequest();

        /**
         * @brief Запрос на повтор последнего действия
         */
        void redoRequest();

        /**
         * @brief Изменилось состояние доступности повтора отменённого действия
         */
        void redoAvailableChanged(bool _isRedoAvailable);

        /**
         * @brief Сменился стиль под курсором
         */
        void currentStyleChanged();

        /**
         * @brief Изменён стиль блока
         */
        void styleChanged();

        /**
         * @brief В документ были внесены редакторские примечания
         */
        void reviewChanged();

        /**
         * @brief Пользователь хочет добавить закладку в заданном месте документа
         */
        void addBookmarkRequested(int _position);

        /**
         * @brief Пользователь хочет изменить закладку в заданном месте документа
         */
        void editBookmarkRequested(int _position);

        /**
         * @brief Пользователь хочет убрать закладку в заданном месте документа
         */
        void removeBookmarkRequested(int _position);

        /**
         * @brief Пользователь хочет переименовать номер сцены
         */
        void renameSceneNumberRequested(const QString& _newName, int _position);

    protected:
        /**
         * @brief Нажатия многих клавиш обрабатываются вручную
         */
        void keyPressEvent(QKeyEvent* _event) override;

        /**
         * @brief Переопределяем, чтобы самостоятельно обрабатывать вводимый пользователем текст
         */
        void inputMethodEvent(QInputMethodEvent* _event) override;

        /**
         * @brief Дополнительная функция для обработки нажатий самим редактором
         * @return Обработано ли событие
         */
        bool keyPressEventReimpl(QKeyEvent* _event);

        /**
         * @brief Переопределяется для корректной загрузки больших документов
         */
        void paintEvent(QPaintEvent* _event) override;

        /**
         * @brief Переопределяется для обработки тройного клика
         */
        /** @{ */
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        /** @} */

        /**
         * @brief Переопределяем работу с буфером обмена для использования собственного майм типа данных
         */
        /** @{ */
        bool canInsertFromMimeData(const QMimeData* _source) const override;
        QMimeData* createMimeDataFromSelection() const override;
        void insertFromMimeData(const QMimeData* _source) override;
        /** @} */

    private:
        /**
         * @brief Скорректировать позиции курсоров соавторов
         */
        void aboutCorrectAdditionalCursors(int _position, int _charsRemoved, int _charsAdded);

        /**
         * @brief Обработки изменения выделения
         */
        void aboutSelectionChanged();

        /**
         * @brief Сохранить состояние редактора
         */
        void aboutSaveEditorState();

        /**
         * @brief Загрузить состояние редактора
         */
        void aboutLoadEditorState();

        /**
         * @brief Очистить текущий блок от установленного в нём типа
         */
        void cleanScenarioTypeFromBlock();

        /**
         * @brief Применить заданный тип к текущему блоку редактора
         * @param Тип блока
         */
        void applyScenarioTypeToBlock(BusinessLogic::ScenarioBlockStyle::Type _blockType);

        /**
         * @brief Скорректировать введённый текст
         *
         * - изменить регистр текста в начале предложения, если это необходимо
         * - иправить проблему ДВойных ЗАглавных
         * - убрать лишние пробелы
         */
        void updateEnteredText(const QString& _eventText);

        /**
         * @brief Оканчивается ли строка сокращением
         */
        bool stringEndsWithAbbrev(const QString& _text);
        /**
         * @brief Выделить блок при тройном клике
         */
        bool selectBlockOnTripleClick(QMouseEvent* _event);

    private:
        void initEditor();
        void initEditorConnections();
        void removeEditorConnections();
        void initView();
        void initConnections();

    private:
        /**
         * @brief Документ
         */
        BusinessLogic::ScenarioTextDocument* m_document;

        /**
         * @brief Количеств
         */
        int m_mouseClicks;

        /**
         * @brief Время последнего клика мышки, мс
         */
        qint64 m_lastMouseClickTime;

        /**
         * @brief Необходимо ли сохранять данные при вводе
         */
        bool m_storeDataWhenEditing;

        /**
         * @brief Отображать ли номер сцен
         */
        bool m_showSceneNumbers;

        /**
         * @brief Префикс номеров сцен
         */
        QString m_sceneNumbersPrefix;

        /**
         * @brief Отображать ли номера реплик
         */
        bool m_showDialoguesNumbers;

        /**
         * @brief Подсвечивать блоки
         */
        bool m_highlightBlocks;

        /**
         * @brief Подсвечивать текущую линию
         */
        bool m_highlightCurrentLine;

        /**
         * @brief Использовать автозамены для особых случаев
         */
        /** @{ */
        bool m_capitalizeFirstWord;
        bool m_correctDoubleCapitals;
        bool m_replaceThreeDots;
        bool m_smartQuotes;
        /** @} */

        /**
         * @brief Показывать автодополения в пустых блоках
         */
        bool m_showCharacterSuggestions;

        /**
         * @brief Включена ли опция проигрывания звуков клавиатруы
         */
        bool m_keyboardSoundEnabled = false;

        /**
         * @brief Курсоры соавторов
         */
        QMap<QString, int> m_additionalCursors;

        /**
         * @brief Скорректированные позиции курсоров, после локальных изменений текста
         */
        QMap<QString, int> m_additionalCursorsCorrected;

        /**
         * @brief Управляющий шорткатами
         */
        ShortcutsManager* m_shortcutsManager;

        /**
         * @brief Список действий рецензирования для контекстного меню
         */
        QList<QAction*> m_reviewContextMenuActions;
    };
}

#endif // SCENARIOTEXTEDIT_H
