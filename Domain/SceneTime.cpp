#include "SceneTime.h"

using namespace Domain;


SceneTime::SceneTime(const Identifier& _id, const QString& _name) :
	DomainObject(_id),
	m_name(_name)
{
}

QString SceneTime::name() const
{
    return m_name;
}

void SceneTime::setName(const QString& _name)
{
	if (m_name != _name) {
		m_name = _name;

		changesNotStored();
	}
}

// ****

namespace {
	const int COLUMN_COUNT = 1;
}

SceneTimesTable::SceneTimesTable(QObject* _parent) :
	DomainObjectsItemModel(_parent)
{
}

int SceneTimesTable::columnCount(const QModelIndex&) const
{
	return COLUMN_COUNT;
}

QVariant SceneTimesTable::data(const QModelIndex& _index, int _role) const
{
	QVariant resultData;

	if (_role ==  Qt::DisplayRole
		|| _role == Qt::EditRole) {
		DomainObject *domainObject = domainObjects().value(_index.row());
		SceneTime* time = dynamic_cast<SceneTime*>(domainObject);
		Column column = sectionToColumn(_index.column());
		switch (column) {
			case Name: {
				resultData = time->name();
				break;
			}

			default: {
				break;
			}
		}
	}

	return resultData;
}

SceneTimesTable::Column SceneTimesTable::sectionToColumn(int _section) const
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
