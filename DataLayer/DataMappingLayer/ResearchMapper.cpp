#include "ResearchMapper.h"

#include <DataLayer/Database/Database.h>

#include <Domain/Research.h>

#include <3rd_party/Helpers/ImageHelper.h>

#include <QSqlQuery>

using namespace DataMappingLayer;


namespace {
    const QString kColumns = " id, parent_id, type, name, description, url, image, color, sort_order ";
    const QString IMAGE_COLUMN = "image";
    const QString kTableName = " research ";
    const QString CHARACTERS_FILTER = QString(" WHERE type = %1 ORDER BY name").arg(Research::Character);
    const QString LOCATIONS_FILTER = QString(" WHERE type = %1 ORDER BY name").arg(Research::Location);
}

ResearchImageMapper::ResearchImageMapper() :
    AbstractImageWrapper()
{
    //
    // Кэшируем всего 20 изображений
    //
    m_imagesCache.setMaxCost(20);
}

QPixmap ResearchImageMapper::image(const DomainObject* _forObject) const
{
    const int objectId = _forObject->id().value();
    //
    // Если изображение объекта есть среди новых, возвращаем его
    //
    if (m_newImages.contains(objectId)) {
        return m_newImages[objectId];
    }
    //
    // Если изображения нет нигде
    //
    if (!m_imagesCache.contains(objectId)) {
        //
        // ... загрузим его из базы данных в кэш
        //
        QSqlQuery query = DatabaseLayer::Database::query();
        query.prepare(QString("SELECT " + IMAGE_COLUMN +
                              " FROM " + kTableName +
                              " WHERE id = %1 "
                              )
                      .arg(objectId));
        query.exec();
        query.next();

        QPixmap* image = new QPixmap(ImageHelper::imageFromBytes(query.value(IMAGE_COLUMN).toByteArray()));
        m_imagesCache.insert(objectId, image);
    }
    //
    // Возвращаем изображение из кэша
    //
    return *m_imagesCache[_forObject->id().value()];
}

void ResearchImageMapper::setImage(const QPixmap& _image, const DomainObject* _forObject)
{
    const int objectId = _forObject->id().value();
    //
    // Убираем кэшированную версию изображения
    //
    m_imagesCache.remove(objectId);
    //
    // Сохраняем новую версию
    //
    m_newImages[objectId] = _image;
}

void ResearchImageMapper::save(const DomainObject* _forObject, QSqlQuery* _query) const
{
    const int objectId = _forObject->id().value();

    //
    // Если изображение для данного элемента не менялось, ничего не делаем с ним
    //
    if (!m_newImages.contains(objectId)) {
        return;
    }

    //
    // Извлекаем изображение из списка несохранённых и сохраняем его в базе данных
    //
    if (_query != nullptr) {
        _query->prepare(QString("UPDATE " + kTableName +
                               " SET " + IMAGE_COLUMN + " = ? "
                                                        " WHERE id = %1 "
                               )
                        .arg(objectId));
        const QPixmap image = m_newImages.take(objectId);
        _query->addBindValue(ImageHelper::bytesFromImage(image));
    }
}

void ResearchImageMapper::remove(const DomainObject* _forObject) const
{
    const int objectId = _forObject->id().value();
    m_imagesCache.remove(objectId);
    m_newImages.remove(objectId);
}

// ****

Research* ResearchMapper::find(const Identifier& _id)
{
    return dynamic_cast<Research*>(abstractFind(_id));
}

ResearchTable* ResearchMapper::findAll()
{
    //
    // Настраиваем порядок сортировки, т.к. он важен при построении дерева разработки
    //
    const QString sortQuery = " ORDER BY parent_id, sort_order";

    return qobject_cast<ResearchTable*>(abstractFindAll(sortQuery));
}

ResearchTable* ResearchMapper::findCharacters()
{
    return qobject_cast<ResearchTable*>(abstractFindAll(CHARACTERS_FILTER));
}

ResearchTable* ResearchMapper::findLocations()
{
    return qobject_cast<ResearchTable*>(abstractFindAll(LOCATIONS_FILTER));
}

void ResearchMapper::insert(Research* _research)
{
    _research->setImageWrapper(&m_imageWrapper);

    abstractInsert(_research);
}

bool ResearchMapper::update(Research* _research)
{
    DatabaseLayer::Database::transaction();
    //
    // Сохраним изображение, если нужно
    //
    QSqlQuery q_imageSaver = DatabaseLayer::Database::query();
    m_imageWrapper.save(_research, &q_imageSaver);
    if (!q_imageSaver.lastQuery().isEmpty()) {
        executeSql(q_imageSaver);
    }
    //
    // Обновим сам элемент разработки
    //
    const bool saveState = abstractUpdate(_research);
    DatabaseLayer::Database::commit();

    return saveState;
}

void ResearchMapper::remove(Research* _research)
{
    DatabaseLayer::Database::transaction();
    m_imageWrapper.remove(_research);
    abstractDelete(_research);
    DatabaseLayer::Database::commit();
}

void ResearchMapper::refreshCharacters(DomainObjectsItemModel* _model)
{
    refresh(_model, CHARACTERS_FILTER);
}

void ResearchMapper::refreshLocations(DomainObjectsItemModel* _model)
{
    refresh(_model, LOCATIONS_FILTER);
}

QString ResearchMapper::findStatement(const Identifier& _id) const
{
    QString findStatement =
            QString("SELECT " + kColumns +
                    " FROM " + kTableName +
                    " WHERE id = %1 "
                    )
            .arg(_id.value());
    return findStatement;
}

QString ResearchMapper::findAllStatement() const
{
    return "SELECT " + kColumns + " FROM  " + kTableName;
}

QString ResearchMapper::insertStatement(DomainObject* _subject, QVariantList& _insertValues) const
{
    QString insertStatement =
            QString("INSERT INTO " + kTableName +
                    " (" + kColumns + ") "
                    " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?) "
                    );

    Research* research = dynamic_cast<Research*>(_subject );
    _insertValues.clear();
    _insertValues.append(research->id().value());
    _insertValues.append((research->parent() == nullptr || !research->parent()->id().isValid()) ? QVariant() : research->parent()->id().value());
    _insertValues.append(research->type());
    _insertValues.append(research->name());
    _insertValues.append(research->description());
    _insertValues.append(research->url());
    _insertValues.append(QByteArray()); // Вместо изображения пустой массив байт
    _insertValues.append(research->color().isValid() ? research->color().name() : QVariant());
    _insertValues.append(research->sortOrder());

    return insertStatement;
}

QString ResearchMapper::updateStatement(DomainObject* _subject, QVariantList& _updateValues) const
{
    QString updateStatement =
            QString("UPDATE " + kTableName +
                    " SET parent_id = ?, "
                    " type = ?, "
                    " name = ?, "
                    " description = ?, "
                    " url = ?, "
                    " color = ?, "
                    " sort_order = ? "
                    " WHERE id = ? "
                    );

    Research* research = dynamic_cast<Research*>(_subject);
    _updateValues.clear();
    _updateValues.append((research->parent() == nullptr || !research->parent()->id().isValid()) ? QVariant() : research->parent()->id().value());
    _updateValues.append(research->type());
    _updateValues.append(research->name());
    _updateValues.append(research->description());
    _updateValues.append(research->url());
    _updateValues.append(research->color().isValid() ? research->color().name() : QVariant());
    _updateValues.append(research->sortOrder());
    _updateValues.append(research->id().value());

    return updateStatement;
}

QString ResearchMapper::deleteStatement(DomainObject* _subject, QVariantList& _deleteValues) const
{
    QString deleteStatement = "DELETE FROM " + kTableName + " WHERE id = ?";

    _deleteValues.clear();
    _deleteValues.append(_subject->id().value());

    return deleteStatement;
}

DomainObject* ResearchMapper::doLoad(const Identifier& _id, const QSqlRecord& _record)
{
    Research* parent = nullptr;
    if (!_record.value("parent_id").isNull()) {
        parent = find(Identifier(_record.value("parent_id").toInt()));
    }
    const Research::Type type = static_cast<Research::Type>(_record.value("type").toInt());
    const QString name = _record.value("name").toString();
    const QString description = _record.value("description").toString();
    const QString url = _record.value("url").toString();
    const QColor color = QColor(_record.value("color").toString());
    const int sortOrder = _record.value("sort_order").toInt();

    return ResearchBuilder::create(_id, parent, type, sortOrder, name, description, url, color, &m_imageWrapper);
}

void ResearchMapper::doLoad(DomainObject* _domainObject, const QSqlRecord& _record)
{
    if (Research* research = dynamic_cast<Research*>(_domainObject)) {
        Research* parent = nullptr;
        if (!_record.value("parent_id").isNull()) {
            parent = find(Identifier(_record.value("parent_id").toInt()));
        }
        research->setParent(parent);

        const Research::Type type = static_cast<Research::Type>(_record.value("type").toInt());
        research->setType(type);

        const QString name = _record.value("name").toString();
        research->setName(name);

        const QString description = _record.value("description").toString();
        research->setDescription(description);

        const QString url = _record.value("url").toString();
        research->setUrl(url);

        const QColor color = QColor(_record.value("color").toString());
        research->setColor(color);

        const int sortOrder = _record.value("sort_order").toInt();
        research->setSortOrder(sortOrder);
    }
}

DomainObjectsItemModel* ResearchMapper::modelInstance()
{
    return new ResearchTable;
}

ResearchMapper::ResearchMapper()
{
}
