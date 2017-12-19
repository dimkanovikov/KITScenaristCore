#include "ScenarioTextBlockInfo.h"

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QUuid>

using namespace BusinessLogic;


SceneHeadingBlockInfo::SceneHeadingBlockInfo(const QString& _uuid) :
    m_uuid(_uuid),
    m_sceneNumber(0)
{
    if (m_uuid.isEmpty()) {
        rebuildUuid();
    }
}

QString SceneHeadingBlockInfo::uuid() const
{
    return m_uuid;
}

void SceneHeadingBlockInfo::setUuid(const QString& _uuid)
{
    if (m_uuid != _uuid) {
        m_uuid = _uuid;
    }
}

void SceneHeadingBlockInfo::rebuildUuid()
{
    m_uuid = QUuid::createUuid().toString();
}

int SceneHeadingBlockInfo::sceneNumber() const
{
    return m_sceneNumber;
}

void SceneHeadingBlockInfo::setSceneNumber(int _number)
{
    if (m_sceneNumber != _number) {
        m_sceneNumber = _number;
    }
}

QString SceneHeadingBlockInfo::colors() const
{
    return m_colors;
}

void SceneHeadingBlockInfo::setColors(const QString& _colors)
{
    if (m_colors != _colors) {
        m_colors = _colors;
    }
}

QString SceneHeadingBlockInfo::stamp() const
{
    return m_stamp;
}

void SceneHeadingBlockInfo::setStamp(const QString& _stamp)
{
    if (m_stamp != _stamp) {
        m_stamp = _stamp;
    }
}

QString SceneHeadingBlockInfo::title() const
{
    return m_title;
}

void SceneHeadingBlockInfo::setTitle(const QString& _title)
{
    if (m_title != _title) {
        m_title = _title;
    }
}

QString SceneHeadingBlockInfo::description() const
{
    return m_description;
}

void SceneHeadingBlockInfo::setDescription(const QString& _description)
{
    //
    // Обновим описание, если он изменился
    //
    if (m_description != _description) {
        m_description = _description;
    }
}

SceneHeadingBlockInfo* SceneHeadingBlockInfo::clone() const
{
    SceneHeadingBlockInfo* copy = new SceneHeadingBlockInfo(m_uuid);
    copy->m_sceneNumber = m_sceneNumber;
    copy->m_colors = m_colors;
    copy->m_stamp = m_stamp;
    copy->m_description = m_description;
    return copy;
}


// ****


int CharacterBlockInfo::dialogueNumbder() const
{
    return m_dialogueNumber;
}

void CharacterBlockInfo::setDialogueNumber(int _number)
{
    if (m_dialogueNumber != _number) {
        m_dialogueNumber = _number;
    }
}
