#include "ScriptVersion.h"

#include <3rd_party/Helpers/TextUtils.h>

#include <QApplication>

using Domain::ScriptVersion;
using Domain::ScriptVersionsTable;


ScriptVersion::ScriptVersion(const Domain::Identifier& _id, const QString& _username, const QDateTime& _datetime,
                             const QColor& _color, const QString& _name, const QString& _description,
                             const QString& _scriptText)
    : DomainObject(_id),
      m_username(_username),
      m_datetime(_datetime),
      m_color(_color),
      m_name(_name),
      m_description(_description),
      m_scriptText(_scriptText)
{
}

QString ScriptVersion::username() const
{
    return m_username;
}

void ScriptVersion::setUsername(const QString& _username)
{
    if (m_username != _username) {
        m_username = _username;

        changesNotStored();
    }
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

QString ScriptVersion::scriptText() const
{
    return m_scriptText;
}

void ScriptVersion::setScriptText(const QString& _scriptText)
{
    if (m_scriptText != _scriptText) {
        m_scriptText = _scriptText;

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

Domain::ScriptVersionsTable::ScriptVersionsTable(QObject* _parent)
    : DomainObjectsItemModel(_parent)
{
}

int ScriptVersionsTable::columnCount(const QModelIndex&) const
{
    return kColumnCount;
}

QVariant ScriptVersionsTable::data(const QModelIndex& _index, int _role) const
{
    QVariant resultData;

    if (_role ==  Qt::DisplayRole
        || _role == Qt::EditRole) {
        DomainObject *domainObject = domainObjects().value(_index.row());
        ScriptVersion* version = dynamic_cast<ScriptVersion*>(domainObject);
        const Column column = static_cast<Column>(_index.column());
        switch (column) {
            case kUsername: {
                resultData = version->username();
                break;
            }

            case kDatetime: {
                resultData = version->datetime();
                break;
            }

            case kColor: {
                resultData = version->color();
                break;
            }

            case kName: {
                if (domainObject == domainObjects().last()) {
                    resultData
                            = QString("%1 %2")
                              .arg(version->name())
                              .arg(TextUtils::directedText(
                                       QApplication::translate("Domain::ScriptVersionsTable", "current version"),
                                       '[', ']')
                                   );
                } else {
                    resultData = version->name();
                }
                break;
            }

            case kDescription: {
                resultData = version->description();
                break;
            }

            case kScriptText: {
                resultData = version->scriptText();
                break;
            }

            default: {
                break;
            }
        }
    }

    return resultData;
}
