#include "ScenarioTextBlockInfo.h"

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QUuid>

using namespace BusinessLogic;


bool TextBlockInfo::hasBookmark() const
{
    return m_hasBookmark;
}

void TextBlockInfo::setHasBookmark(bool _hasBookmark)
{
    m_hasBookmark = _hasBookmark;
}

QString TextBlockInfo::bookmark() const
{
    return m_bookmark;
}

void TextBlockInfo::setBookmark(const QString& _bookmark)
{
    m_bookmark = _bookmark;
}

QColor TextBlockInfo::bookmarkColor() const
{
    return m_bookmarkColor;
}

void TextBlockInfo::setBookmarkColor(const QColor& _color)
{
    m_bookmarkColor = _color;
}


// ****


SceneHeadingBlockInfo::SceneHeadingBlockInfo(const QString& _uuid) :
    m_uuid(_uuid)
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

bool SceneHeadingBlockInfo::isSceneNumberFixed() const
{
    return m_fixed;
}

void SceneHeadingBlockInfo::setSceneNumberFixed(bool _fixed)
{
    m_fixed = _fixed;
}

unsigned SceneHeadingBlockInfo::sceneNumberFixNesting() const
{
    return m_sceneNumberFixNesting;
}

void SceneHeadingBlockInfo::setSceneNumberFixNesting(unsigned sceneNumberFixNesting)
{
    m_sceneNumberFixNesting = sceneNumberFixNesting;
}

int SceneHeadingBlockInfo::sceneNumberSuffix() const
{
    return m_sceneNumberSuffix;
}

void SceneHeadingBlockInfo::setSceneNumberSuffix(int sceneNumberSuffix)
{
    m_sceneNumberSuffix = sceneNumberSuffix;
}

QString SceneHeadingBlockInfo::sceneNumber() const
{
    return m_sceneNumber;
}

void SceneHeadingBlockInfo::setSceneNumber(const QString& _number)
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
    copy->m_fixed = m_fixed;
    copy->m_sceneNumberFixNesting = m_sceneNumberFixNesting;
    copy->m_sceneNumberSuffix = m_sceneNumberSuffix;
    copy->m_colors = m_colors;
    copy->m_stamp = m_stamp;
    copy->m_title = m_title;
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

CharacterBlockInfo* CharacterBlockInfo::clone() const
{
    CharacterBlockInfo* copy = new CharacterBlockInfo;
    copy->m_dialogueNumber = m_dialogueNumber;
    return copy;
}
