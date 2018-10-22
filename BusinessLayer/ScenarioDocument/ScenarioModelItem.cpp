#include "ScenarioModelItem.h"

#include <QPainter>

using namespace BusinessLogic;


namespace {
    const int MAX_TEXT_LENGTH = 300;
}

ScenarioModelItem::ScenarioModelItem(int _position) :
    m_position(_position),
    m_fixed(false),
    m_textLength(0),
    m_duration(0),
    m_type(Scene),
    m_hasNote(false),
    m_parent(0)
{
}

ScenarioModelItem::~ScenarioModelItem()
{
    qDeleteAll(m_children);
}

QString ScenarioModelItem::uuid() const
{
    return m_uuid;
}

void ScenarioModelItem::setUuid(const QString& _uuid)
{
    if (m_uuid != _uuid) {
        m_uuid = _uuid;
    }
}

int ScenarioModelItem::position() const
{
    return m_position;
}

void ScenarioModelItem::setPosition(int _position)
{
    if (m_position != _position) {
        m_position = _position;
    }
}

int ScenarioModelItem::endPosition() const
{
    return position() + length();
}

int ScenarioModelItem::length() const
{
    int length = 0;
    //
    // Единица тут добавляется, за символ переноса строки после предыдущего блока
    // это может быть заголовок элемента, или описание сцены перед текстом
    //
    length += m_header.length();
    if (!m_description.isNull()) {
        length += 1 + m_description.length();
    }
    //
    // Ещё один перенос перед каждым вложенного элемента
    //
    if (hasChildren()) {
        foreach (ScenarioModelItem* child, m_children) {
            length += child->length() + 1;
        }
    }
    //
    // И перед текстом
    //
    if (!m_text.isNull()) {
        length += 1 + m_textLength;
    }
    //
    // + 1 перенос перед закрывающим элементом
    //
    if (m_type == Folder) {
        length += 1 + m_footer.length();
    }


    return length;
}

QString ScenarioModelItem::sceneNumber() const
{
    return m_sceneNumber;
}

bool ScenarioModelItem::setSceneNumber(const QString& _number)
{
    if (m_sceneNumber != _number) {
        m_sceneNumber = _number;
        return true;
    }
    return false;
}

bool ScenarioModelItem::isFixed() const
{
    return m_fixed;
}

void ScenarioModelItem::setFixed(bool _fixed)
{
    m_fixed = _fixed;
}

int ScenarioModelItem::fixNesting() const
{
    return m_fixNesting;
}

void ScenarioModelItem::setFixNesting(int _fixNesting)
{
    m_fixNesting = _fixNesting;
}

int ScenarioModelItem::numberSuffix() const
{
    return m_numberSuffix;
}

void ScenarioModelItem::setNumberSuffix(int _numberSuffix)
{
    m_numberSuffix = _numberSuffix;
}

QString ScenarioModelItem::header() const
{
    return m_header;
}

void ScenarioModelItem::setHeader(const QString& _header)
{
    if (m_header != _header) {
        m_header = _header;
    }
}

QString ScenarioModelItem::footer() const
{
    return m_footer;
}

void ScenarioModelItem::setFooter(const QString& _footer)
{
    if (m_footer != _footer) {
        m_footer = _footer;
    }
}

QString ScenarioModelItem::colors() const
{
    return m_colors;
}

void ScenarioModelItem::setColors(const QString& _colors)
{
    if (m_colors != _colors) {
        m_colors = _colors;
    }
}

QString ScenarioModelItem::stamp() const
{
    return m_stamp;
}

void ScenarioModelItem::setStamp(const QString& _stamp)
{
    if (m_stamp != _stamp) {
        m_stamp = _stamp;
    }
}

QString ScenarioModelItem::name() const
{
    return m_name;
}

void ScenarioModelItem::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;
    }
}

QString ScenarioModelItem::description() const
{
    return m_description;
}

void ScenarioModelItem::setDescription(const QString& _description)
{
    if (m_description.isNull() != _description.isNull()
        || m_description != _description) {
        m_description = _description;
    }
}

QString ScenarioModelItem::text() const
{
    return m_text;
}

QString ScenarioModelItem::fullText() const
{
    return m_fullText;
}

void ScenarioModelItem::setText(const QString& _text)
{
    m_textLength = _text.length();
    QString newText = _text.left(MAX_TEXT_LENGTH);
    newText.replace("\n", " ");
    if (m_text.isNull() != newText.isNull()
        || m_text != newText) {
        m_text = newText;
        m_fullText = _text;
    }
}

qreal ScenarioModelItem::duration() const
{
    return m_duration;
}

void ScenarioModelItem::setDuration(qreal _duration)
{
    if (m_duration != _duration) {
        m_duration = _duration;
        updateParentDuration();
    }
}

ScenarioModelItem::Type ScenarioModelItem::type() const
{
    return m_type;
}

void ScenarioModelItem::setType(ScenarioModelItem::Type _type)
{
    if (m_type != _type) {
        m_type = _type;
    }
}

QIcon ScenarioModelItem::icon() const
{
    static QHash<Type, QIcon> s_iconsCache;
    if (!s_iconsCache.contains(m_type)) {
        QString iconPath;
        switch (m_type) {
            case Folder: {
                iconPath = ":/Graphics/Iconset/folder.svg";
                break;
            }

            default: {
                iconPath = ":/Graphics/Iconset/file-document-box.svg";
                break;
            }
        }
        s_iconsCache[m_type] = QIcon(iconPath);
    }

    return s_iconsCache[m_type];
}

bool ScenarioModelItem::hasNote() const
{
    return m_hasNote;
}

void ScenarioModelItem::setHasNote(bool _hasNote)
{
    if (m_hasNote != _hasNote) {
        m_hasNote = _hasNote;
    }
}

Counter ScenarioModelItem::counter() const
{
    return m_counter;
}

void ScenarioModelItem::setCounter(const Counter& _counter)
{
    if (m_counter != _counter) {
        m_counter = _counter;
        updateParentCounter();
    }
}

void ScenarioModelItem::updateParentDuration()
{
    //
    // Если есть дети - обновляем свою длительность
    //
    if (hasChildren()) {
        int duration = 0;
        foreach (ScenarioModelItem* child, m_children) {
            duration += qRound(child->duration());
        }
        setDuration(duration);
    }

    //
    // Обновляем родителя
    //
    if (hasParent()) {
        parent()->updateParentDuration();
    }
}

void ScenarioModelItem::updateParentCounter()
{
    //
    // Если есть дети - обновляем свои счётчики
    //
    if (hasChildren()) {
        Counter counter;
        foreach (ScenarioModelItem* child, m_children) {
            counter.setWords(counter.words() + child->counter().words());
            counter.setCharactersWithSpaces(
                counter.charactersWithSpaces() + child->counter().charactersWithSpaces()
                );
            counter.setCharactersWithoutSpaces(
                counter.charactersWithoutSpaces() + child->counter().charactersWithoutSpaces()
                );
        }
        setCounter(counter);
    }

    //
    // Обновляем родителя
    //
    if (hasParent()) {
        parent()->updateParentCounter();
    }
}

void ScenarioModelItem::clear()
{
    m_header.clear();
    m_text.clear();
    m_footer.clear();

    m_duration = 0;
    updateParentDuration();
}

//! Вспомогательные методы для организации работы модели

void ScenarioModelItem::prependItem(ScenarioModelItem* _item)
{
    //
    // Устанавливаем себя родителем
    //
    _item->m_parent = this;

    //
    // Добавляем элемент в список детей
    //
    m_children.prepend(_item);
}

void ScenarioModelItem::appendItem(ScenarioModelItem* _item)
{
    //
    // Устанавливаем себя родителем
    //
    _item->m_parent = this;

    //
    // Добавляем элемент в список детей
    //
    m_children.append(_item);
}

void ScenarioModelItem::insertItem(int _index, ScenarioModelItem* _item)
{
    _item->m_parent = this;
    m_children.insert(_index, _item);
}

void ScenarioModelItem::removeItem(ScenarioModelItem* _item)
{
    _item->clear();

    //
    // removeOne - удаляет объект при помощи delete, так что потом самому удалять не нужно
    //
    m_children.removeOne(_item);
    _item = 0;
}

bool ScenarioModelItem::hasParent() const
{
    return m_parent != 0;
}

ScenarioModelItem* ScenarioModelItem::parent() const
{
    return m_parent;
}

ScenarioModelItem* ScenarioModelItem::childAt(int _index) const
{
    return m_children.value(_index, 0);
}

int ScenarioModelItem::rowOfChild(ScenarioModelItem* _child) const
{
    return m_children.indexOf(_child);
}

int ScenarioModelItem::childCount() const
{
    return m_children.count();
}

bool ScenarioModelItem::hasChildren() const
{
    return !m_children.isEmpty();
}

bool ScenarioModelItem::childOf(ScenarioModelItem* _parent) const
{
    ScenarioModelItem* parent = m_parent;
    while (parent != nullptr) {
        if (parent == _parent) {
            return true;
        }

        parent = parent->parent();
    }
    return false;
}
