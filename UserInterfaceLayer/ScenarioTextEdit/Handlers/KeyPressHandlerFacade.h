#ifndef KEYPRESSHANDLERFACADE_H
#define KEYPRESSHANDLERFACADE_H

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

#include <QMap>

class QEvent;
class QKeyEvent;

namespace UserInterface {
    class ScenarioTextEdit;
}

namespace KeyProcessingLayer
{
    class AbstractKeyHandler;
    class PrepareHandler;
    class PreHandler;
    class SceneHeadingHandler;
    class SceneCharactersHandler;
    class ActionHandler;
    class CharacterHandler;
    class ParentheticalHandler;
    class DialogHandler;
    class TransitionHandler;
    class NoteHandler;
    class TitleHeaderHandler;
    class TitleHandler;
    class NoprintableTextHandler;
    class FolderHeaderHandler;
    class FolderFooterHandler;
    class SceneDescriptionHandler;
    class LyricsHandler;

    /**
     * @brief Класс обработчика нажатия клавиш в текстовом редакторе
     */
    class KeyPressHandlerFacade
    {
    public:
        /**
         * @brief Подготовиться к обработке
         */
        void prepare(QKeyEvent* _event);

        /**
         * @brief Предварительная обработка
         */
        void prepareForHandle(QKeyEvent* _event);

        /**
         * @brief Подготовить к обработке
         */
        void prehandle();

        /**
         * @brief Обработка
         */
        void handle(QEvent* _event, bool _pre = false);

        /**
         * @brief Нужно ли отправлять событие в базовый класс
         */
        bool needSendEventToBaseClass() const;

        /**
         * @brief Нужно ли чтобы курсор был обязательно видим пользователю
         */
        bool needEnsureCursorVisible() const;

        /**
         * @brief Нужно ли делать подготовку к обработке блока
         */
        bool needPrehandle() const;

    private:
        KeyPressHandlerFacade(UserInterface::ScenarioTextEdit* _editor);

        /**
         * @brief Получить обработчик для заданного типа
         */
        AbstractKeyHandler* handlerFor(BusinessLogic::ScenarioBlockStyle::Type _type);

    private:
        UserInterface::ScenarioTextEdit* m_editor;

        PrepareHandler* m_prepareHandler = nullptr;
        PreHandler* m_preHandler = nullptr;
        SceneHeadingHandler* m_sceneHeaderHandler = nullptr;
        SceneCharactersHandler* m_sceneCharactersHandler = nullptr;
        ActionHandler* m_actionHandler = nullptr;
        CharacterHandler* m_characterHandler = nullptr;
        ParentheticalHandler* m_parentheticalHandler = nullptr;
        DialogHandler* m_dialogHandler = nullptr;
        TransitionHandler* m_transitionHandler = nullptr;
        NoteHandler* m_noteHandler = nullptr;
        TitleHeaderHandler* m_titleheaderHandler = nullptr;
        TitleHandler* m_titleHandler = nullptr;
        NoprintableTextHandler* m_simpleTextHandler = nullptr;
        FolderHeaderHandler* m_folderHeaderHandler = nullptr;
        FolderFooterHandler* m_folderFooterHandler = nullptr;
        SceneDescriptionHandler* m_sceneDescriptionHandler = nullptr;
        LyricsHandler* m_lyricsHandler = nullptr;

    /**
     * @brief Одиночка
     */
    /** @{ */
    public:
        static KeyPressHandlerFacade* instance(UserInterface::ScenarioTextEdit* _editor);

    private:
        static QMap<UserInterface::ScenarioTextEdit*, KeyPressHandlerFacade*> s_instance;
    /** @} */
    };
}

#endif // KEYPRESSHANDLERFACADE_H
