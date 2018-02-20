#include "CeltxImporter.h"

using BusinessLogic::AbstractImporter;
using BusinessLogic::CeltxImporter;
using BusinessLogic::ImportParameters;


CeltxImporter::CeltxImporter() :
    AbstractImporter()
{
}

QString CeltxImporter::importScript(const BusinessLogic::ImportParameters& _importParameters) const
{
    //
    // Открыть архив и извлечь файл, название которого начинается со "script"
    //
    // Прочитать содержимое файла
    //
    // Извлечь из него:
    // <title>(.*)</title> - название сценария
    // <meta(.*)> - поля титульной страницы
    //    "<meta content=\"title page author\" name=\"Author\">"
    //    "<meta content=\"title page based on\" name=\"DC.source\">"
    //    "<meta content=\"title page copyright\" name=\"DC.rights\">"
    //    "<meta content=\"title page contact\" name=\"CX.contact\">"
    //    "<meta content=\"title page by\" name=\"CX.byline\">"
    // <body>(.*)</body> - содержимое сценария
    //
    // Разобрать название и поля титульной страницы
    //
    // Разобрать содержимое сценария
    // - удалить все <br>
    // - удалить все переносы строк и пустоты
    // - остальное можно разобрать, как xml
    //
}
