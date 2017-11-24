#include "ScriptTextCorrector.h"

#include "ScenarioTextDocument.h"

#include <QTextBlock>

using BusinessLogic::ScriptTextCorrector;
using BusinessLogic::ScenarioTextDocument;



ScriptTextCorrector::ScriptTextCorrector(ScenarioTextDocument* _document) :
    m_document(_document)
{

}

void ScriptTextCorrector::correct(int _startPosition)
{
    //
    // Формируем модель от заданной позиции до конца документа
    //
    auto block = m_document->begin();
    while (block.isValid()) {
        //
        // Определяем высоту блока
        //

        block = block.next();
    }

    //
    // Проходим документ и расставляем переносы
    //
}
