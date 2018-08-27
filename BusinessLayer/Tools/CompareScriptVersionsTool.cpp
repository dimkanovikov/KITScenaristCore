#include "CompareScriptVersionsTool.h"

#include <3rd_party/Helpers/DiffMatchPatchHelper.h>

#include <BusinessLayer/ScenarioDocument/ScenarioXml.h>

#include <QDomDocument>
#include <QString>

using BusinessLogic::CompareScriptVersionsTool;
using BusinessLogic::ScenarioXml;

namespace {
    const QString kAttributeDiffAdded = "diff_added";
    const QString kAttributeDiffRemoved = "diff_removed";
}


QString CompareScriptVersionsTool::compareScripts(const QString& _firstScript, const QString& _secondScript)
{
    //
    // Удалить весь мусор: комментарии и выделения цветом, номера и цвета сцен
    //
    QString resultScript = _secondScript;

    //
    // Получить дифы между сценариями
    //
    const auto patch = DiffMatchPatchHelper::makePatchXml(_firstScript, _secondScript);
    const auto diffs = DiffMatchPatchHelper::changedXml(_firstScript, patch);
    //
    // ... если блок удаляется имеем его в выделении, но не имеем в добавлении
    // ... если блок вставляется имеем его в добавлении, но не имеем во вставлении
    // ... если блок изменяется имеем предыдущую версию в выделении и новую в добавлении

    //
    auto nodeValue = [] (const QDomNode& _node) {
        return _node.childNodes().at(0).toElement().text();
    };
    auto storeNode = [] (const QDomNode& _node, QDomDocument& _document) {
        _document.appendChild(_document.importNode(_node, true));
    };

    //
    // Идём с последнего изменения наверх
    // Смотрим по обоим выделениям и определяем что это будет и добавляем, предварительно добавив внутрь нужный тэг
    //
    for (int diffIndex = diffs.size() - 1; diffIndex >= 0; --diffIndex) {
        const auto& diff = diffs.at(diffIndex);

        QDomDocument oldDocument;
        oldDocument.setContent(ScenarioXml::makeMimeFromXml(diff.first.xml));
        QDomNode oldData = oldDocument.childNodes().at(1);
        QDomDocument newDocument;
        newDocument.setContent(ScenarioXml::makeMimeFromXml(diff.second.xml));
        QDomNode newData = newDocument.childNodes().at(1);
        QDomDocument resultData;

        int oldDataIndex = 0;
        int newDataIndex = 0;

        //
        // Сперва проходим по элементам старой части изменённого текста
        //
        for (; oldDataIndex < oldData.childNodes().size(); ++oldDataIndex) {
            const auto oldDataNode = oldData.childNodes().at(oldDataIndex);

            //
            // ... сюда мы могли попасть, только если в новой части были блоки, такие же, как и в старой,
            //     но их количество было меньше, чем в старой => данный блок был удалён
            //
            if (newDataIndex >= newData.childNodes().size()) {
                auto oldDataElement = oldDataNode.toElement();
                oldDataElement.setAttribute(kAttributeDiffRemoved, 1);
                storeNode(oldDataElement, resultData);
                continue;
            }

            //
            // ... проверяем нет ли где впереди схожего блока
            //
            for (int newDataSubindex = newDataIndex; newDataSubindex < newData.childNodes().size(); ++newDataSubindex) {
                const auto newDataSubnode = newData.childNodes().at(newDataSubindex);

                //
                // ... если нашли одинаковый блок значит часть блоков была добавлена между,
                //     добавляем их как новые и переводим индекс новой части к текущему элементу
                //
                if (nodeValue(oldDataNode) == nodeValue(newDataSubnode)) {
                    for (int index = newDataIndex; index < newDataSubindex; ++index) {
                        auto newDataElement = newData.childNodes().at(index).toElement();
                        newDataElement.setAttribute(kAttributeDiffAdded, 1);
                        storeNode(newDataElement, resultData);
                    }

                    newDataIndex = newDataSubindex;
                    break;
                }
            }

            const auto newDataNode = newData.childNodes().at(newDataIndex);

            //
            // ... если блок не менялся имеем одинаковую версию в выделении и в добавлении
            //
            if (nodeValue(oldDataNode) == nodeValue(newDataNode)) {
                storeNode(oldDataNode, resultData);
                ++newDataIndex;
                continue;
            }
            //
            // ... если же блоки в одинаковых позициях разные, то значит старый был удалён, либо изменён,
            //     так что добавим его как удалённый, а новый будет добавлен позже
            //
            else {
                //
                // TODO: тут нужно добавить сравнение блоков посимвольно, чтобы показывать,
                //       какой именно текст в абзаце был изменён между версиями
                //
                auto oldDataElement = oldDataNode.toElement();
                oldDataElement.setAttribute(kAttributeDiffRemoved, 1);
                storeNode(oldDataElement, resultData);
                continue;
            }
        }
        //
        // Затем проходим по новой части, все части, которые в неё вошли будут помечены, как добавленные
        //
        for (; newDataIndex < newData.childNodes().size(); ++newDataIndex) {
            auto newDataElement = newData.childNodes().at(newDataIndex).toElement();
            newDataElement.setAttribute(kAttributeDiffAdded, 1);
            storeNode(newDataElement, resultData);
        }

        //
        // После того, как xml-изменения был сформирован поместим его в текст документа сценария
        //
        resultScript.replace(diff.second.xml, resultData.toString());

    }

    //
    // Сформировать новый сценарий вмещающий в себя все дифы и текст сценария
    // - добавленный блок + аттрибут added к блоку
    // - удалённый блок + аттрибут deleted к блоку
    // - изменённый блок должен быть представлен как два блока: один добавленный, второй удалённый
    //

    return resultScript;
}
