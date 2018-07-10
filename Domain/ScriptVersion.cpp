#include "ScriptVersion.h"

using Domain::ScriptVersion;


ScriptVersion::ScriptVersion(const Domain::Identifier& _id, const QDateTime& _datetime,
                             const QColor& _color, const QString& _name, const QString& _description)
    : DomainObject(_id),
      m_datetime(_datetime),
      m_color(_color),
      m_name(_name),
      m_description(_description)
{
}

QDateTime ScriptVersion::datetime() const
{
    return m_datetime;
}

void ScriptVersion::setDatetime(const QDateTime& _datetime)
{
    if (m_datetime != _datetime) {
        m_datetime = _datetime;

        changesNotStored();
    }
}

QString ScriptVersion::name() const
{
    return m_name;
}

void ScriptVersion::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;

        changesNotStored();
    }
}

QString ScriptVersion::description() const
{
    return m_description;
}

void ScriptVersion::setDescription(const QString& _description)
{
    if (m_description != _description) {
        m_description = _description;

        changesNotStored();
    }
}

QColor ScriptVersion::color() const
{
    return m_color;
}

void ScriptVersion::setColor(const QColor& _color)
{
    if (m_color != _color) {
        m_color = _color;

        changesNotStored();
    }
}

// ****

namespace {
    const int kColumnCount = 3;
}

Domain::ScriptVersionsTable::ScriptVersionsTable(QObject* _parent)
    : DomainObjectsItemModel(_parent)
{
}

int Domain::ScriptVersionsTable::columnCount(const QModelIndex&) const
{
    return kColumnCount;
}

QVariant Domain::ScriptVersionsTable::data(const QModelIndex& _index, int _role) const
{
    QVariant resultData;

    if (_role ==  Qt::DisplayRole
        || _role == Qt::EditRole) {
        DomainObject *domainObject = domainObjects().value(_index.row());
        ScriptVersion* version = dynamic_cast<ScriptVersion*>(domainObject);
        const Column column = sectionToColumn(_index.column());
        switch (column) {
            case Datetime: {
                resultData = version->datetime();
                break;
            }

            case Color: {
                resultData = version->color();
                break;
            }

            case Name: {
                resultData = version->name();
                break;
            }

            case Description: {
                resultData = version->description();
                break;
            }

            default: {
                break;
            }
        }
    }

    return resultData;
}

Domain::ScriptVersionsTable::Column Domain::ScriptVersionsTable::sectionToColumn(int _section) const
{
    switch (_section) {
        case 0: return Datetime;
        case 1: return Color;
        case 2: return Name;
        case 3: return Description;
        default: return Undefined;
    }
}
