#ifndef SCRIPTTEXTCURSOR_H
#define SCRIPTTEXTCURSOR_H

#include <QTextCursor>


namespace BusinessLogic
{
    /**
     * @brief Класс курсора со вспомогательными функциями
     */
    class ScriptTextCursor : public QTextCursor
    {
    public:
        ScriptTextCursor();
        ScriptTextCursor(const QTextCursor &other);
        explicit ScriptTextCursor(QTextDocument* _document);
        ~ScriptTextCursor();

        /**
         * @brief Находится ли блок в таблице
         */
        bool isBlockInTable() const;
    };
}

#endif // SCRIPTTEXTCURSOR_H
