#include "Transition.h"

using namespace Domain;


Transition::Transition(const Identifier& _id, const QString& _name) :
    DomainObject(_id),
    m_name(_name)
{
}

QString Transition::name() const
{
    return m_name;
}

void Transition::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;

        changesNotStored();
    }
}

bool Transition::equal(const QString& _name) const
{
    return m_name == _name;
}

// ****

namespace {
    const int kColumnCount = 1;
}

TransitionsTable::TransitionsTable(QObject* _parent) :
    DomainObjectsItemModel(_parent)
{
}

int TransitionsTable::columnCount(const QModelIndex&) const
{
    return kColumnCount;
}

QVariant TransitionsTable::data(const QModelIndex& _index, int _role) const
{
    QVariant resultData;

    if (_role ==  Qt::DisplayRole
        || _role == Qt::EditRole) {
        DomainObject *domainObject = domainObjects().value(_index.row());
        Transition* transition = dynamic_cast<Transition*>(domainObject);
        Column column = sectionToColumn(_index.column());
        switch (column) {
            case Name: {
                resultData = transition->name();
                break;
            }

            default: {
                break;
            }
        }
    }

    return resultData;
}

TransitionsTable::Column TransitionsTable::sectionToColumn(int _section) const
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
