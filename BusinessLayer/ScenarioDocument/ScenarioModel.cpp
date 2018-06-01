#include "ScenarioModel.h"

#include "ScenarioModelItem.h"
#include "ScenarioDocument.h"
#include "ScenarioXml.h"

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QApplication>
#include <QMimeData>
#include <QQueue>

using namespace BusinessLogic;


QString ScenarioModel::MIME_TYPE = "application/x-scenarist/scenario-tree";

ScenarioModel::ScenarioModel(QObject *parent, ScenarioXml* _xmlHandler) :
    QAbstractItemModel(parent),
    m_rootItem(new ScenarioModelItem(0)),
    m_xmlHandler(_xmlHandler),
    m_lastMime(0),
    m_scenesCount(0)
{
    Q_ASSERT(m_xmlHandler);

    m_rootItem->setHeader(QObject::tr("Scenario"));
    m_rootItem->setType(ScenarioModelItem::Scenario);
}

ScenarioModel::~ScenarioModel()
{
    delete m_rootItem;
    m_rootItem = 0;
}

void ScenarioModel::prependItem(ScenarioModelItem* _item, ScenarioModelItem* _parentItem)
{
    //
    // Если родитель не задан им становится сам сценарий
    //
    if (_parentItem == 0) {
        _parentItem = m_rootItem;
    }

    //
    // Если такого элемента ещё нет у родителя
    //
    if (_parentItem->rowOfChild(_item) == -1) {
        QModelIndex parentIndex = indexForItem(_parentItem);
        int itemRowIndex = 0; // т.к. в самое начало

        beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
        _parentItem->prependItem(_item);
        ++m_scenesCount;
        endInsertRows();
    }
}

void ScenarioModel::addItem(ScenarioModelItem* _item, ScenarioModelItem* _parentItem)
{
    //
    // Если родитель не задан им становится сам сценарий
    //
    if (_parentItem == 0) {
        _parentItem = m_rootItem;
    }

    //
    // Если такого элемента ещё нет у родителя
    //
    if (_parentItem->rowOfChild(_item) == -1) {
        QModelIndex parentIndex = indexForItem(_parentItem);

        //
        // Определим позицию вставки
        //
        int itemRowIndex = 0;
        for (int childIndex = _parentItem->childCount() - 1; childIndex >= 0; --childIndex) {
            ScenarioModelItem* child = _parentItem->childAt(childIndex);
            if (child->position() < _item->position()) {
                itemRowIndex = childIndex + 1;
                break;
            }
        }

        beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
        _parentItem->insertItem(itemRowIndex, _item);
        ++m_scenesCount;
        endInsertRows();
    }
}

void ScenarioModel::insertItem(ScenarioModelItem* _item, ScenarioModelItem* _afterSiblingItem)
{
    ScenarioModelItem* parent = _afterSiblingItem->parent();

    //
    // Если такого элемента ещё нет у родителя
    //
    if (parent->rowOfChild(_item) == -1) {
        QModelIndex parentIndex = indexForItem(parent);
        int itemRowIndex = parent->rowOfChild(_afterSiblingItem) + 1;

        beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
        parent->insertItem(itemRowIndex, _item);
        ++m_scenesCount;
        endInsertRows();
    }
}

void ScenarioModel::removeItem(ScenarioModelItem* _item)
{
    //
    // Удаляем всех детей
    //
    if (_item->hasChildren()) {
        for (int childIndex = _item->childCount()-1; childIndex >= 0; --childIndex) {
            removeItem(_item->childAt(childIndex));
        }
    }

    //
    // Удаляем сам элемент
    //
    ScenarioModelItem* itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    //
    // ... если его удалось найти
    //     иногда это может случатся, когда дети родителя были удалены, но удаляющий не учёл этого
    //
    if (itemRowIndex >= 0) {
        beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
        itemParent->removeItem(_item);
        --m_scenesCount;
        endRemoveRows();
    }
}

void ScenarioModel::removeItems(const QVector<ScenarioModelItem*>& _items)
{
    auto removeItemsImpl = [this] (ScenarioModelItem* _parent, const QVector<ScenarioModelItem*>& _itemsToDelete) {
        const QModelIndex itemParentIndex = indexForItem(_parent);
        const int firstRow = indexForItem(_itemsToDelete.first()).row();
        const int lastRow = indexForItem(_itemsToDelete.last()).row();
        beginRemoveRows(itemParentIndex, firstRow, lastRow);
        for (ScenarioModelItem* itemToDelete : _itemsToDelete) {
            _parent->removeItem(itemToDelete);
            --m_scenesCount;
        }
        endRemoveRows();
    };

    //
    // Проходим список элементов на удаление и удаляем всех с одним родителем
    //
    ScenarioModelItem* lastParent = nullptr;
    QVector<ScenarioModelItem*> itemsToDelete;
    for (ScenarioModelItem* item : _items) {
        ScenarioModelItem* itemParent = item->parent();

        //
        // Если родитель не сменился
        //
        if (itemParent == lastParent) {
            //
            // ... и если это последовательный элемент, добавляем элемент в список на удаление
            //
            if (itemsToDelete.isEmpty()
                || lastParent->rowOfChild(itemsToDelete.last()) == (lastParent->rowOfChild(item) - 1)) {
                itemsToDelete.append(item);
            }
            //
            // ... в противном случае удаляем элементы из текущего списка и начинаем формировать новый
            //
            else {
                removeItemsImpl(lastParent, itemsToDelete);
                itemsToDelete.clear();
                itemsToDelete.append(item);
            }
        }
        //
        // Если у следующего элемента другой родитель
        //
        else {
            //
            // Если есть список элементов на удаление
            //
            if (!itemsToDelete.isEmpty()) {
                removeItemsImpl(lastParent, itemsToDelete);
            }

            //
            // Перейдём к новому родителю
            //
            lastParent = itemParent;
            itemsToDelete.clear();
            itemsToDelete.append(item);
        }
    }

    //
    // Если после выполнения цикла остался список на удаление, удалим элементы из модели
    //
    if (!itemsToDelete.isEmpty()) {
        removeItemsImpl(lastParent, itemsToDelete);
    }
}

void ScenarioModel::updateItem(ScenarioModelItem* _item)
{
    //
    // Если элемент уже в списке, то обновим, в противном случае просто игнорируем
    //
    if (_item->parent() != 0) {
        const QModelIndex indexForUpdate = indexForItem(_item);
        emit dataChanged(indexForUpdate, indexForUpdate);
    }
}

QModelIndex ScenarioModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    QModelIndex resultIndex;
    if (_row < 0
        || _row > rowCount(_parent)
        || _column < 0
        || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))
        ) {
        resultIndex = QModelIndex();
    } else {
        ScenarioModelItem* parentItem = itemForIndex(_parent);
        Q_ASSERT(parentItem);

        ScenarioModelItem* indexItem = parentItem->childAt(_row);
        if (indexItem != 0) {
            resultIndex = createIndex(_row, _column, indexItem);
        }
    }
    return resultIndex;
}

QModelIndex ScenarioModel::parent(const QModelIndex& _child) const
{
    QModelIndex parentIndex;
    if (_child.isValid()) {
        ScenarioModelItem* childItem = itemForIndex(_child);
        ScenarioModelItem* parentItem = childItem->parent();
        if (parentItem != 0
            && parentItem != m_rootItem) {
            ScenarioModelItem* grandParentItem = parentItem->parent();
            if (grandParentItem != 0) {
                int row = grandParentItem->rowOfChild(parentItem);
                parentIndex = createIndex(row, 0, parentItem);
            }
        }
    }
    return parentIndex;
}

int ScenarioModel::columnCount(const QModelIndex&) const
{
    return 1;
}

int ScenarioModel::rowCount(const QModelIndex& _parent) const
{
    int rowCount = 0;
    if (_parent.isValid() && (_parent.column() != 0)) {
        //
        // Ноль строк
        //
    } else {
        ScenarioModelItem* item = itemForIndex(_parent);
        if (item != 0) {
            rowCount = item->childCount();
        }
    }
    return rowCount;
}

Qt::ItemFlags ScenarioModel::flags(const QModelIndex& _index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    ScenarioModelItem* item = itemForIndex(_index);
    if (item->type() == ScenarioModelItem::Folder
        || item->type() == ScenarioModelItem::Scenario) {
        flags |= Qt::ItemIsDropEnabled;
    }

    return flags;
}

QVariant ScenarioModel::data(const QModelIndex& _index, int _role) const
{
    QVariant result;

    ScenarioModelItem* item = itemForIndex(_index);
    switch (_role) {
        case Qt::DisplayRole: {
            result = item->header();
            break;
        }

        case Qt::DecorationRole: {
            result = QVariant::fromValue(item->icon());
            break;
        }

        case Qt::ToolTipRole: {
            if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
                result = item->fullText();
            }
            break;
        }

        //
        // Тип элемента
        //
        case TypeIndex: {
            result = item->type();
            break;
        }

        //
        // Цвет сцены
        //
        case ColorIndex: {
            result = item->colors();
            break;
        }

        //
        // Штамп сцены
        //
        case StampIndex: {
            result = item->stamp();
            break;
        }

        //
        // Название сцены
        //
        case TitleIndex: {
            result = item->title();
            break;
        }

        //
        // Текст сцены
        //
        case SceneTextIndex: {
            if (item->type() == ScenarioModelItem::Scenario) {
                result = item->description();
            } else {
                result = item->text();
            }
            break;
        }

        //
        // Описание
        //
        case DescriptionIndex: {
            result = item->description();
            break;
        }

        //
        // Длительность
        //
        case DurationIndex: {
            result = item->duration();
            break;
        }

        //
        // Номер сцены
        //
        case SceneNumberIndex: {
            if (item->type() == ScenarioModelItem::Scene) {
                result = item->sceneNumber();
            }
            break;
        }

        //
        // Если ли заметки для данного элемента
        //
        case HasNoteIndex: {
            result = item->hasNote();
            break;
        }

        //
        // Видимость элемента
        //
        case VisibilityIndex: {
            result = true;
            //
            // Скрываем только первый блок, если он содержит текст ИЗ ЗТМ
            //
            if (item->type() == ScenarioModelItem::Undefined && TextEditHelper::smartToUpper(item->header()) == tr("FADE IN:")) {
                result = false;
            }
            break;
        }

        default: {
            break;
        }
    }

    return result;
}

bool ScenarioModel::dropMimeData(
        const QMimeData* _data, Qt::DropAction _action,
        int _row, int _column, const QModelIndex& _parent)
{
    /*
     * Вставка данных в этом случае происходит напрямую в текст документа, а не в дерево,
     * само дерево просто перестраивается после всех манипуляций с текстовым редактором
     */

    Q_UNUSED(_column);

    //
    // _row - индекс, куда вставлять, если в папку, то он равен -1 и если в самый низ списка, то он тоже равен -1
    //

    bool isDropSucceed = false;

    if (_data != 0
        && _data->hasFormat(MIME_TYPE)) {

        switch (_action) {
            case Qt::IgnoreAction: {
                isDropSucceed = true;
                break;
            }

            case Qt::MoveAction:
            case Qt::CopyAction: {

                //
                // Получим структурные элементы дерева, чтобы понять, куда вкладывать данные
                //
                // ... элемент, в который будут вкладываться данные
                ScenarioModelItem* parentItem = itemForIndex(_parent);
                // ... элемент, перед которым будут вкладываться данные
                ScenarioModelItem* insertBeforeItem = parentItem->childAt(_row);

                //
                // Если производится перемещение данных
                //
                bool removeLastMime = false;
                if (m_lastMime == _data) {
                    removeLastMime = true;
                }

                //
                // Вставим данные
                //
                int insertPosition = m_xmlHandler->xmlToScenario(parentItem, insertBeforeItem, _data->data(MIME_TYPE), removeLastMime);
                isDropSucceed = true;
                emit mimeDropped(insertPosition);

                break;
            }

            default: {
                break;
            }
        }
    }

    return isDropSucceed;
}

QMimeData* ScenarioModel::mimeData(const QModelIndexList& _indexes) const
{
    QMimeData* mimeData = new QMimeData;

    if (!_indexes.isEmpty()) {
        //
        // Выделение может быть только последовательным, но нужно учесть ситуацию, когда в выделение
        // попадает родительский элемент и не все его дочерние элементы, поэтому просто использовать
        // последний элемент некорректно, нужно проверить, не входит ли его родитель в выделение
        //

        QModelIndexList correctedIndexes;
        foreach (const QModelIndex& index, _indexes) {
            if (!_indexes.contains(index.parent())) {
                correctedIndexes.append(index);
            }
        }

        //
        // Для того, чтобы запретить разрывать папки проверяем выделены ли элементы одного уровня
        //
        bool itemsHaveSameParent = true;
        if (!correctedIndexes.isEmpty()) {
            const QModelIndex& genericParent = correctedIndexes.first().parent();
            foreach (const QModelIndex& index, correctedIndexes) {
                if (index.parent() != genericParent) {
                    itemsHaveSameParent = false;
                    break;
                }
            }
        } else {
            itemsHaveSameParent = false;
        }

        //
        // Если выделены элементы одного уровня, то создаём майм-данные
        //
        if (itemsHaveSameParent) {
            qSort(correctedIndexes);

            QModelIndex fromIndex = correctedIndexes.first();
            QModelIndex toIndex = correctedIndexes.last();

            //
            // Определяем элементы из которых будет состоять выделение
            //
            ScenarioModelItem* fromItem = itemForIndex(fromIndex);
            ScenarioModelItem* toItem = itemForIndex(toIndex);

            //
            // Сформируем данные
            //
            mimeData->setData(
                        MIME_TYPE,
                        m_xmlHandler->scenarioToXml(fromItem, toItem).toUtf8());
        }
    }

    m_lastMime = mimeData;

    return mimeData;
}

QStringList ScenarioModel::mimeTypes() const
{
    return QStringList() << MIME_TYPE;
}

Qt::DropActions ScenarioModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions ScenarioModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void ScenarioModel::updateSceneNumbers()
{
    //
    // Для начала, построим список из сцен
    //
    QVector<ScenarioModelItem*> scenes;
    scenes.reserve(m_scenesCount);

    //
    // Заполним список из сцен
    //
    QQueue<ScenarioModelItem*> queue;
    queue.push_back(m_rootItem);
    while (!queue.empty()) {
        ScenarioModelItem* item = queue.front();
        queue.pop_front();
        if (item->type() == ScenarioModelItem::Scene) {
            scenes.push_back(item);
        } else {
            for (int i = 0; i != item->childCount(); ++i) {
                queue.push_back(item->childAt(i));
            }
        }
    }

    //
    // Обновим номера
    //
    QString prevPrefix = "";
    unsigned sceneNumberCounter = 0;
    bool anyFixed = false;

    for (int i = 0; i != scenes.size(); ++i) {

        if (scenes[i]->isFixed()) {
            //
            // Если сцена зафиксирована, то номера последующих сцен будут отталкиваться от нее
            //
            prevPrefix = scenes[i]->sceneNumber();
            sceneNumberCounter = 0;
            anyFixed = true;
        } else {
            //
            // А если не зафиксирована, то обновим для нее номер
            //
            if (i != 0
                    && i + 1 != scenes.size()
                    && scenes[i - 1]->isFixed()) {
                //
                // Если предыдущая сцена зафиксирована, то, возможно, текущая сцена находится
                // между зафиксированными сценами разной глубины фиксации. Тогда ее порядковый номер
                // должен начинаться с последней зафиксированной сцены, глубина фиксации которой
                // на 1 меньше предыдущей из ближайшей группы
                //
                int nearestNextFixed = -1; // Номер ближайшей зафиксированной сцены, глубина фиксации которой на 1 меньше предыдущей
                for (int j = i + 1; j != scenes.size(); ++j) {
                    if (scenes[j]->fixNesting() != 0
                            && scenes[j]->fixNesting() < scenes[i - 1]->fixNesting()
                            && scenes[j]->isFixed()) {
                        nearestNextFixed = j;
                        //
                        // Нашли такую сцену
                        //
                        break;
                    }
                    if (scenes[j]->fixNesting() >= scenes[i - 1]->fixNesting()) {
                        //
                        // Вышли из зоны поиска. Не нашли
                        //
                        break;
                    }
                }
                if (nearestNextFixed != -1) {
                    //
                    // Окей, у нас есть номер ближайшей сцены, глубина фиксации которой на 1 меньше,
                    // чем у предыдущей по отношению к текущей. Теперь надо найти последнюю сцену из этой группы фиксации
                    // и взять ее номер
                    //
                    unsigned maxSuffix = scenes[nearestNextFixed]->numberSuffix();
                    for (int j = nearestNextFixed + 1; j != scenes.size(); ++j) {
                        if (scenes[j]->fixNesting() >= scenes[i - 1]->fixNesting()) {
                            //
                            // Вышли из зоны поиска
                            //
                            break;
                        }
                        if (scenes[j]->fixNesting() + 1 != scenes[i - 1]->fixNesting()
                                || !scenes[j]->isFixed()) {
                            //
                            // А эти сцены мы игнорируем, потому что их номер нас не интересует
                            // (незафиксированные или с еще меньшей фиксацией)
                            //
                            continue;
                        }
                        maxSuffix = std::max(maxSuffix, scenes[j]->numberSuffix());
                    }

                    //
                    // Вот порядковый номер текущей сцены
                    //
                    sceneNumberCounter = maxSuffix + 1;

                    //
                    // При этом, нужно скопировать глубину фиксации для текущей сцены из группы, к которой она относится
                    //
                    scenes[i]->setFixNesting(scenes[nearestNextFixed]->fixNesting());
                }
            } else if (i != 0
                       && !scenes[i - 1]->isFixed()) {
                //
                // Если предыдущая и текущая незафиксированы, то у них должна быть одинаковая группа фиксации
                //
                scenes[i]->setFixNesting(scenes[i - 1]->fixNesting());
            }

            //
            // Наконец, зададим номер сцены
            //
            QString sceneCounter;
            if (prevPrefix.isEmpty()) {
                //
                // Если это сцена "верхнего уровня", то у нее номер, а не буква
                //
                sceneCounter = QString::number(1 + sceneNumberCounter);
            } else {
                //
                // Иначе, добавим букву к суффиксу
                // А если буквы закончились, то нумеруем их еще и цифрой
                //
                const unsigned alphabetLen = 'B' - 'A' + 1;
                unsigned sceneLetter = sceneNumberCounter % alphabetLen;
                unsigned sceneLetterNumber = sceneNumberCounter / alphabetLen;
                sceneCounter = prevPrefix + ('A' + sceneLetter);
                if (sceneLetterNumber != 0) {
                    sceneCounter += QString::number(sceneLetterNumber);
                }
            }

            //
            // Непосредственно задаем номер сцены
            //
            if (scenes[i]->setSceneNumber(sceneCounter)) {
                scenes[i]->setNumberSuffix(sceneNumberCounter);
                const QModelIndex itemIndex = indexForItem(scenes[i]);
                emit dataChanged(itemIndex, itemIndex);
            }
            ++sceneNumberCounter;
        }
    }

    //
    // Уведомим, если в модели есть фиксированные сцена, а мы не знали или наоборот
    //
    if (anyFixed != m_anyFixed) {
        m_anyFixed = anyFixed;
        emit fixedScenesChanged(m_anyFixed);
    }

    m_scenesCount = scenes.size();
}

void ScenarioModel::lockUnlockSceneNumbers(bool _lock)
{
    ScenarioModelItem* item;
    QQueue<ScenarioModelItem*> queue;
    queue.push_back(m_rootItem);

    //
    // Просто обходим дерево и ставим атрибуты фиксации
    //
    while(!queue.empty()) {
        item = queue.front();
        queue.pop_front();
        if (item->type() == ScenarioModelItem::Scene) {
            item->setFixed(_lock);
            if (_lock) {
                item->setFixNesting(item->fixNesting() + 1);
            } else {
                item->setFixNesting(0);
                item->setNumberSuffix(0);
                item->setSceneNumber(0);
            }
        }
        else {
            for (int itemIndex = 0; itemIndex < item->childCount(); ++itemIndex) {
                queue.push_back(item->childAt(itemIndex));
            }
        }
    }

    //
    // Обновляем номера сцен (на всякий случай?)
    //
    updateSceneNumbers();
}

int ScenarioModel::scenesCount() const
{
    return m_scenesCount;
}

int ScenarioModel::duration() const
{
    return m_rootItem->duration();
}

Counter ScenarioModel::counter() const
{
    return m_rootItem->counter();
}

ScenarioModelItem* ScenarioModel::itemForIndex(const QModelIndex& _index) const
{
    ScenarioModelItem* resultItem = m_rootItem;
    if (_index.isValid()) {
        ScenarioModelItem* item = static_cast<ScenarioModelItem*>(_index.internalPointer());
        if (item != nullptr) {
            resultItem = item;
        }
    }
    return resultItem;
}

QModelIndex ScenarioModel::indexForItem(ScenarioModelItem* _item) const
{
    if (_item == nullptr) {
        return QModelIndex();
    }

    QModelIndex parent;
    if (_item->hasParent()
        && _item->parent()->hasParent()) {
        parent = indexForItem(_item->parent());
    } else {
        parent = QModelIndex();
    }

    int row;
    if (_item->hasParent()
        && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
    } else {
        row = m_rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

namespace {
    static ScenarioModelItem* scenarioModelItemForUuid(ScenarioModelItem* _parent, const QString& _uuid) {
        ScenarioModelItem* searchedItem = nullptr;
        for (int childIndex = 0; childIndex < _parent->childCount(); ++childIndex) {
            ScenarioModelItem* child = _parent->childAt(childIndex);
            if (child->uuid() == _uuid) {
                searchedItem = child;
                break;
            } else {
                if (child->hasChildren()) {
                    searchedItem = scenarioModelItemForUuid(child, _uuid);
                    if (searchedItem != nullptr) {
                        break;
                    }
                }
            }
        }

        return searchedItem;
    }
}

QModelIndex ScenarioModel::indexForUuid(const QString& _uuid) const
{
    if (_uuid.isEmpty()) {
        return QModelIndex();
    }

    return indexForItem(::scenarioModelItemForUuid(m_rootItem, _uuid));
}

namespace {
    const int CARD_WIDTH = 210;
    const int CARD_HEIGHT = 100;
    const int CARDS_SPACE = 40;

    /**
     * @brief Сформировать строку xml-акта для элемента
     */
    static QString actXmlFor(const ScenarioModelItem* _item, int _x, int _y) {
        QString actXml = "<act ";
        actXml.append(QString("id=\"%1\" ").arg(_item->uuid()));
        actXml.append(QString("title=\"%1\" ")
                       .arg(_item->title().isEmpty()
                            ? TextEditHelper::toHtmlEscaped(TextEditHelper::smartToUpper(_item->header()))
                            : TextEditHelper::toHtmlEscaped(TextEditHelper::smartToUpper(_item->title()))));
        actXml.append(QString("description=\"%1\" ")
                       .arg(_item->description().isEmpty()
                            ? TextEditHelper::toHtmlEscaped(_item->fullText())
                            : TextEditHelper::toHtmlEscaped(_item->description())));
        actXml.append(QString("colors=\"%1\" ").arg(_item->colors()));
        actXml.append(QString("x=\"%1\" ").arg(_x));
        actXml.append(QString("y=\"%1\" ").arg(_y));
        actXml.append("/>\n");

        return actXml;
    }

    /**
     * @brief Сформировать строку xml-карточки для элемента
     */
    static QString cardXmlFor(const ScenarioModelItem* _item, int _x, int _y, bool _isEmbedded) {
        QString cardXml = "<card ";
        cardXml.append(QString("id=\"%1\" ").arg(_item->uuid()));
        cardXml.append(QString("is_folder=\"%1\"").arg(_item->type() == ScenarioModelItem::Folder ? "true" : "false"));
        cardXml.append(QString("number=\"%1\" ").arg(_item->sceneNumber()));
        cardXml.append(QString("title=\"%1\" ")
                       .arg(_item->title().isEmpty()
                            ? TextEditHelper::toHtmlEscaped(TextEditHelper::smartToUpper(_item->header()))
                            : TextEditHelper::toHtmlEscaped(TextEditHelper::smartToUpper(_item->title()))));
        cardXml.append(QString("description=\"%1\" ")
                       .arg(_item->description().isEmpty()
                            ? TextEditHelper::toHtmlEscaped(_item->fullText())
                            : TextEditHelper::toHtmlEscaped(_item->description())));
        cardXml.append(QString("stamp=\"%1\" ").arg(_item->stamp()));
        cardXml.append(QString("colors=\"%1\" ").arg(_item->colors()));
        cardXml.append(QString("is_embedded=\"%1\" ").arg(_isEmbedded ? "true" : "false"));
        cardXml.append(QString("x=\"%1\" ").arg(_x));
        cardXml.append(QString("y=\"%1\" ").arg(_y));
        cardXml.append("/>\n");

        return cardXml;
    }

    /**
     * @brief Сформировать xml схемы для детей элемента
     */
    static QString actChildsXml(const ScenarioModelItem* _parent, int& _x, int& _y) {
        QString xml;
        for (int childIndex = 0; childIndex < _parent->childCount(); ++childIndex) {
            const ScenarioModelItem* child = _parent->childAt(childIndex);
            xml.append(cardXmlFor(child, _x, _y, true));

            _x += CARD_WIDTH + CARDS_SPACE;
        }

        return xml;
    }
}

QString ScenarioModel::simpleScheme() const
{
    QString xml("<?xml version=\"1.0\"?>\n"
                "<cards x=\"0\" y=\"0\" width=\"0\" height=\"0\" scale=\"1\" >\n");

    //
    // Пробегаем по всем элементам
    //
    int x= 0;
    int y = 0;
    for (int childIndex = 0; childIndex < m_rootItem->childCount(); ++childIndex) {
        const ScenarioModelItem* child = m_rootItem->childAt(childIndex);
        //
        // ... акт
        //
        if (child->type() == ScenarioModelItem::Folder) {
            x = 0;
            y += CARD_HEIGHT + CARDS_SPACE;

            xml.append(actXmlFor(child, x, y));

            x = 0;
            y += CARD_HEIGHT + CARDS_SPACE;

            //
            // ... пробегаем только один вложенный уровень
            //
            xml.append(::actChildsXml(child, x, y));
        }
        //
        // ... карточка
        //
        else {
            xml.append(cardXmlFor(child, x, y, false));

            x += CARD_WIDTH + CARDS_SPACE;
        }
    }

    xml.append("</cards>\n\n");

    return xml;
}

// ********

void ScenarioModelFiltered::setDragDropEnabled(bool _enabled)
{
    if (m_dragDropEnabled != _enabled) {
        m_dragDropEnabled = _enabled;
    }
}

Qt::ItemFlags ScenarioModelFiltered::flags(const QModelIndex &_index) const
{
    Qt::ItemFlags result = sourceModel()->flags(mapToSource(_index));
    if (!m_dragDropEnabled) {
        result ^= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    return result;
}

bool ScenarioModelFiltered::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    QModelIndex index = sourceModel()->index(_sourceRow, 0, _sourceParent);
    bool visible = sourceModel()->data(index, ScenarioModel::VisibilityIndex).toBool();
    return visible;
}
