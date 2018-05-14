#include "ResearchModelItem.h"

#include <Domain/Research.h>

using BusinessLogic::ResearchModelItem;
using Domain::Research;


ResearchModelItem::ResearchModelItem(Domain::Research* _research) :
    m_research(_research),
    m_parent(0)
{
}

ResearchModelItem::~ResearchModelItem()
{
    m_research = 0;
    qDeleteAll(m_children);
}

QString ResearchModelItem::name() const
{
    return
            m_research == 0
            ? QString()
            : m_research->name();
}

QPixmap ResearchModelItem::icon() const
{
    QString iconPath;
    if (m_research != 0) {
        switch (m_research->type()) {
            case Research::ResearchRoot: {
                iconPath = ":/Graphics/Iconset/file-tree.svg";
                break;
            }

            case Research::Folder: {
                iconPath = ":/Graphics/Iconset/folder.svg";
                break;
            }

            case Research::Url: {
                iconPath = ":/Graphics/Iconset/web.svg";
                break;
            }

            case Research::ImagesGallery: {
                iconPath = ":/Graphics/Iconset/image-multiple.svg";
                break;
            }

            case Research::Image: {
                iconPath = ":/Graphics/Iconset/image.svg";
                break;
            }

            case Research::MindMap: {
                iconPath = ":/Graphics/Iconset/mind-map.svg";
                break;
            }

            case Research::CharactersRoot: {
                iconPath = ":/Graphics/Iconset/account-multiple.svg";
                break;
            }

            case Research::Character: {
                iconPath = ":/Graphics/Iconset/account.svg";
                break;
            }

            case Research::LocationsRoot: {
                iconPath = ":/Graphics/Iconset/home-multiple.svg";
                break;
            }

            case Research::Location: {
                iconPath = ":/Graphics/Iconset/home.svg";
                break;
            }

            case Research::TitlePage: {
                iconPath = ":/Graphics/Iconset/book-open-variant.svg";
                break;
            }

            default: {
                iconPath = ":/Graphics/Iconset/file-document-box.svg";
                break;
            }
        }
    }

    return QPixmap(iconPath);
}

Domain::Research* ResearchModelItem::research() const
{
    return m_research;
}

//! Вспомогательные методы для организации работы модели

void ResearchModelItem::prependItem(ResearchModelItem* _item)
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

void ResearchModelItem::appendItem(ResearchModelItem* _item)
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

void ResearchModelItem::insertItem(int _index, ResearchModelItem* _item)
{
    _item->m_parent = this;
    m_children.insert(_index, _item);
}

void ResearchModelItem::removeItem(ResearchModelItem* _item)
{
    //
    // removeOne - удаляет объект при помощи delete, так что потом самому удалять не нужно
    //
    m_children.removeOne(_item);
    _item = 0;
}

bool ResearchModelItem::hasParent() const
{
    return m_parent != 0;
}

ResearchModelItem* ResearchModelItem::parent() const
{
    return m_parent;
}

ResearchModelItem* ResearchModelItem::childAt(int _index) const
{
    return m_children.value(_index, 0);
}

int ResearchModelItem::rowOfChild(ResearchModelItem* _child) const
{
    return m_children.indexOf(_child);
}

int ResearchModelItem::childCount() const
{
    return m_children.count();
}

bool ResearchModelItem::hasChildren() const
{
    return !m_children.isEmpty();
}
