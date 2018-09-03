#include "ScriptTextCursor.h"

using BusinessLogic::ScriptTextCursor;


ScriptTextCursor::ScriptTextCursor()
    : QTextCursor()
{
}

ScriptTextCursor::ScriptTextCursor(const QTextCursor& _other)
    : QTextCursor(_other)
{
}

ScriptTextCursor::ScriptTextCursor(QTextDocument* _document)
    : QTextCursor(_document)
{
}

ScriptTextCursor::~ScriptTextCursor()
{
}

bool ScriptTextCursor::isBlockInTable() const
{
    return currentTable() != nullptr;
}
