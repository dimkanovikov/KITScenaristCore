#include "Research.h"

#include <3rd_party/Helpers/ImageHelper.h>
#include <3rd_party/Helpers/TextEditHelper.h>

#include <QDomDocument>
#include <QXmlStreamWriter>

using namespace Domain;


Research* Research::parent() const
{
    return m_parent;
}

void Research::setParent(Research* _parent)
{
    if (m_parent != _parent) {
        m_parent = _parent;

        changesNotStored();
    }
}

Research::Type Research::type() const
{
    return m_type;
}

void Research::setType(Research::Type _type)
{
    if (m_type != _type) {
        m_type = _type;

        changesNotStored();
    }
}

QString Research::name() const
{
    return m_name;
}

void Research::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;

        changesNotStored();
    }
}

QString Research::description() const
{
    return TextEditHelper::fromHtmlEscaped(m_description);
}

void Research::setDescription(const QString& _description)
{
    if (m_description != _description) {
        m_description = _description;

        changesNotStored();
    }
}

void Research::addDescription(const QString& _description)
{
    if (!_description.isEmpty()) {
        m_description.append(_description);

        changesNotStored();
    }
}

QString Research::url() const
{
    return m_url;
}

void Research::setUrl(const QString& _url)
{
    if (m_url != _url) {
        m_url = _url;

        changesNotStored();
    }
}

QPixmap Research::image() const
{
    if (m_image == nullptr) {
        return QPixmap();
    }

    return m_image->image(this);
}

void Research::setImage(const QPixmap& _image)
{
    if (m_image != nullptr
        && !ImageHelper::isImagesEqual(m_image->image(this), _image)) {
        m_image->setImage(_image, this);

        changesNotStored();
    }
}

QColor Research::color() const
{
    return m_color;
}

void Research::setColor(const QColor& _color)
{
    if (m_color != _color) {
        m_color = _color;

        changesNotStored();
    }
}

int Research::sortOrder() const
{
    return m_sortOrder;
}

void Research::setSortOrder(int _sortOrder)
{
    if (m_sortOrder != _sortOrder) {
        m_sortOrder = _sortOrder;

        changesNotStored();
    }
}

void Research::setImageWrapper(AbstractImageWrapper* _imageWrapper)
{
    if (m_image != _imageWrapper) {
        m_image = _imageWrapper;
    }
}

Research::Research(const Identifier& _id, Research* _parent, Research::Type _type, int _sortOrder,
    const QString& _name, const QString& _description, const QString& _url, const QColor& _color,
    AbstractImageWrapper* _imageWrapper)
    : DomainObject(_id),
      m_parent(_parent),
      m_type(_type),
      m_name(_name),
      m_description(_description),
      m_url(_url),
      m_color(_color),
      m_sortOrder(_sortOrder),
      m_image(_imageWrapper)
{
}


// ****


QString ResearchCharacter::realName() const
{
    QDomDocument xmlDocument;
    xmlDocument.setContent(description());
    QDomNode characterNode = xmlDocument.namedItem("character");
    QDomNode realNameNode = characterNode.namedItem("real_name");
    return realNameNode.toElement().text();
}

void ResearchCharacter::setRealName(const QString& _name)
{
    QString description;
    QXmlStreamWriter writer(&description);
    writer.writeStartDocument();
    writer.writeStartElement("character");
    writer.writeTextElement("real_name", _name);
    writer.writeStartElement("description");
    writer.writeCDATA(
                TextEditHelper::toHtmlEscaped(
                    TextEditHelper::removeDocumentTags(
                        descriptionText()
                        )
                    )
                );
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();

    setDescription(description);
}

QString ResearchCharacter::descriptionText() const
{
    QDomDocument xmlDocument;
    xmlDocument.setContent(description());
    QDomNode characterNode = xmlDocument.namedItem("character");
    QDomNode descriptionNode = characterNode.namedItem("description");
    return TextEditHelper::fromHtmlEscaped(descriptionNode.toElement().text());
}

void ResearchCharacter::setDescriptionText(const QString& _description)
{
    QString description;
    QXmlStreamWriter writer(&description);
    writer.writeStartDocument();
    writer.writeStartElement("character");
    writer.writeTextElement("real_name", realName());
    writer.writeStartElement("description");
    writer.writeCDATA(
                TextEditHelper::toHtmlEscaped(
                    TextEditHelper::removeDocumentTags(
                        _description
                        )
                    )
                );
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();

    setDescription(description);
}

void ResearchCharacter::addDescription(const QString& _description)
{
    //
    // Запоминаем текущие настоящее имя и описание
    //
    const QString sourceRealName = realName();
    const QString sourceDescription = descriptionText();
    //
    // Устанавливаем новые
    //
    setDescription(_description);
    const QString addedRealName = realName();
    QString addedDescription = descriptionText();
    //
    // И объединяем
    //
    if (!sourceRealName.isEmpty()) {
        setRealName(sourceRealName);
        addedDescription = QString("<p>%1</p>%2").arg(addedRealName, addedDescription);
    } else {
        setRealName(addedRealName);
    }
    //
    if (!sourceDescription.isEmpty()) {
        setDescriptionText(QString("%1%2").arg(sourceDescription, addedDescription));
    } else {
        setDescriptionText(addedDescription);
    }

    //
    // Помечаем изменённым
    //
    changesNotStored();
}

ResearchCharacter::ResearchCharacter(const Identifier& _id, Research* _parent, Research::Type _type, int _sortOrder,
    const QString& _name, const QString& _description, const QString& _url, const QColor& _color,
    AbstractImageWrapper* _imageWrapper)
    : Research(_id, _parent, _type, _sortOrder, _name, _description, _url, _color, _imageWrapper)
{
    Q_ASSERT_X(_type == Research::Character, Q_FUNC_INFO, "Character must have convenient type");
}


// ****


Research* ResearchBuilder::create(const Identifier& _id, Research* _parent, Research::Type _type,
    int _sortOrder, const QString& _name, const QString& _description, const QString& _url, const QColor& _color,
    AbstractImageWrapper* _imageWrapper)
{
    switch (_type) {
        case Research::Character: {
            return new ResearchCharacter(_id, _parent, _type, _sortOrder, _name, _description, _url, _color, _imageWrapper);
        }

        default: {
            return new Research(_id, _parent, _type, _sortOrder, _name, _description, _url, _color, _imageWrapper);
        }
    }
}


// ****


namespace {
    const int COLUMN_COUNT = 1;
}

ResearchTable::ResearchTable(QObject* _parent) :
    DomainObjectsItemModel(_parent)
{
}

int ResearchTable::columnCount(const QModelIndex&) const
{
    return COLUMN_COUNT;
}

QVariant ResearchTable::data(const QModelIndex& _index, int _role) const
{
    QVariant resultData;

    if (_role ==  Qt::DisplayRole
        || _role == Qt::EditRole) {
        DomainObject *domainObject = domainObjects().value(_index.row());
        Research* research = dynamic_cast<Research*>(domainObject);
        Column column = sectionToColumn(_index.column());
        switch (column) {
            case Name: {
                resultData = research->name();
                break;
            }

            default: {
                break;
            }
        }
    }

    return resultData;
}

ResearchTable::Column ResearchTable::sectionToColumn(int _section) const
{
    Column column = Undefined;

    switch (_section) {
        case 0: {
            column = Name;
            break;
        }

        default: {
            break;
        }
    }

    return column;
}
