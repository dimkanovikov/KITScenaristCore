#include "ScenarioTextBlockInfo.h"

#include <3rd_party/Helpers/TextEditHelper.h>

#include <QUuid>

using namespace BusinessLogic;

namespace {
    /**
     * @brief Счётчик идентификаторов блоков, используется в качестве хэша для быстрого поиска блоков
     */
    static quint64 g_idCounter = 0;
}


TextBlockInfo::TextBlockInfo()
{
    updateId();
}

quint64 TextBlockInfo::id() const
{
    return m_id;
}

void TextBlockInfo::updateId()
{
    m_id = ++g_idCounter;
}

bool TextBlockInfo::hasBookmark() const
{
    return m_hasBookmark;
}

void TextBlockInfo::setHasBookmark(bool _hasBookmark)
{
    if (m_hasBookmark != _hasBookmark) {
        m_hasBookmark = _hasBookmark;
        updateId();
    }
}

QString TextBlockInfo::bookmark() const
{
    return m_bookmark;
}

void TextBlockInfo::setBookmark(const QString& _bookmark)
{
    if (m_bookmark != _bookmark) {
        m_bookmark = _bookmark;
        updateId();
    }
}

QColor TextBlockInfo::bookmarkColor() const
{
    return m_bookmarkColor;
}

void TextBlockInfo::setBookmarkColor(const QColor& _color)
{
    if (m_bookmarkColor != _color) {
        m_bookmarkColor = _color;
        updateId();
    }
}

QColor TextBlockInfo::diffColor() const
{
    return m_diffColor;
}

void TextBlockInfo::setDiffType(TextBlockInfo::DiffType _type)
{
    switch (_type) {
        case kDiffAdded: {
            m_diffColor = Qt::green;
            break;
        }

        case kDiffRemoved: {
            m_diffColor = Qt::red;
            break;
        }

        default: break;
    }

    if (m_diffColor.isValid()) {
        m_diffColor.setAlpha(120);
    }

    updateId();
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
        updateId();
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
    if (m_fixed != _fixed) {
        m_fixed = _fixed;
        updateId();
    }
}

int SceneHeadingBlockInfo::sceneNumberFixNesting() const
{
    return m_sceneNumberFixNesting;
}

void SceneHeadingBlockInfo::setSceneNumberFixNesting(int _sceneNumberFixNesting)
{
    if (m_sceneNumberFixNesting != _sceneNumberFixNesting) {
        m_sceneNumberFixNesting = _sceneNumberFixNesting;
        updateId();
    }
}

int SceneHeadingBlockInfo::sceneNumberSuffix() const
{
    return m_sceneNumberSuffix;
}

void SceneHeadingBlockInfo::setSceneNumberSuffix(int sceneNumberSuffix)
{
    if (m_sceneNumberSuffix != sceneNumberSuffix) {
        m_sceneNumberSuffix = sceneNumberSuffix;
        updateId();
    }
}

QString SceneHeadingBlockInfo::sceneNumber() const
{
    return m_sceneNumber;
}

void SceneHeadingBlockInfo::setSceneNumber(const QString& _number)
{
    if (m_sceneNumber != _number) {
        m_sceneNumber = _number;
        updateId();
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
        updateId();
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
        updateId();
    }
}

QString SceneHeadingBlockInfo::name() const
{
    return m_name;
}

void SceneHeadingBlockInfo::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;
        updateId();
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
        updateId();
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
    copy->m_name = m_name;
    copy->m_description = m_description;
    return copy;
}


// ****


int CharacterBlockInfo::dialogueNumber() const
{
    return m_dialogueNumber;
}

void CharacterBlockInfo::setDialogueNumber(int _number)
{
    if (m_dialogueNumber != _number) {
        m_dialogueNumber = _number;
        updateId();
    }
}

CharacterBlockInfo* CharacterBlockInfo::clone() const
{
    CharacterBlockInfo* copy = new CharacterBlockInfo;
    copy->m_dialogueNumber = m_dialogueNumber;
    return copy;
}
