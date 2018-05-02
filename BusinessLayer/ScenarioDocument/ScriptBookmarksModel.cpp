#include "ScriptBookmarksModel.h"

#include "ScenarioTextBlockInfo.h"
#include "ScenarioTextDocument.h"

#include <3rd_party/Helpers/ImageHelper.h>

#include <QIcon>
#include <QTextBlock>

using BusinessLogic::TextBlockInfo;
using BusinessLogic::ScenarioTextDocument;
using BusinessLogic::ScriptBookmarksModel;


ScriptBookmarksModel::ScriptBookmarksModel(BusinessLogic::ScenarioTextDocument* _parent) :
    QAbstractListModel(_parent),
    m_document(_parent)
{
    Q_ASSERT(_parent);

    connect(m_document, &ScenarioTextDocument::contentsChange, this, &ScriptBookmarksModel::aboutUpdateBookmarksModel);
}

int ScriptBookmarksModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent);

    return m_bookmarks.size();
}

QVariant ScriptBookmarksModel::data(const QModelIndex& _index, int _role) const
{
    QVariant result;

    if (_index.isValid()
        && _index.row() < m_bookmarks.size()) {
        const BookmarkInfo& info = m_bookmarks.at(_index.row());

        switch (_role) {
            case Qt::DecorationRole: {
                QIcon icon(":/Graphics/Iconset/bookmark.svg");
                ImageHelper::setIconColor(icon, info.color);
                result = icon;
                break;
            }

            case Qt::DisplayRole: {
                result = info.text;
                break;
            }

            default: break;
        }
    }

    return result;
}

bool ScriptBookmarksModel::removeRows(int _row, int _count, const QModelIndex& _parent)
{
    bool result = false;

    int last = _row + _count - 1;
    if (m_bookmarks.size() > last) {
        beginRemoveRows(_parent, _row, last);
        for (int replies = _count; replies > 0; --replies) {
            const BookmarkInfo bookmark = m_bookmarks.takeAt(_row);
            //
            // Убираем закладки из текста
            //
            if (!m_document->isEmpty()) {
                QTextBlock bookmarkBlock = m_document->findBlock(bookmark.position);
                if (bookmarkBlock.userData() == nullptr)
                    continue;

                TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(bookmarkBlock.userData());
                blockInfo->setHasBookmark(false);
                bookmarkBlock.setUserData(blockInfo);

                ScenarioTextDocument::updateBlockRevision(bookmarkBlock);

                emit modelChanged();
            }
        }
        endRemoveRows();

        result = true;
    }

    return result;
}

void ScriptBookmarksModel::addBookmark(int _position, const QString& _text, const QColor& _color)
{
    if (m_bookmarksMap.contains(_position)) {
        const int bookmarkRow = m_bookmarksMap[_position];
        BookmarkInfo& bookmark = m_bookmarks[bookmarkRow];
        bookmark.text = _text;
        bookmark.color = _color;
        //
        emit dataChanged(index(bookmarkRow), index(bookmarkRow));
    } else {
        int insertPosition = m_bookmarks.size();
        QMap<int, int>::iterator iter = m_bookmarksMap.lowerBound(_position);
        if (iter != m_bookmarksMap.end()) {
            insertPosition = iter.value();
            if (m_bookmarks[insertPosition].position < _position) {
                ++insertPosition;
            }
        }
        //
        BookmarkInfo bookmark;
        bookmark.position = _position;
        bookmark.color = _color;
        bookmark.text = _text;
        //
        beginInsertRows(QModelIndex(), insertPosition, insertPosition);
        m_bookmarks.insert(insertPosition, bookmark);
        //
        m_bookmarksMap.insert(bookmark.position, insertPosition);
        while (iter != m_bookmarksMap.end()) {
            *iter += 1;
            ++iter;
        }
        endInsertRows();
    }
}

void ScriptBookmarksModel::removeBookmark(int _position)
{
    auto bookmarksIter = m_bookmarksMap.find(_position);
    if (bookmarksIter == m_bookmarksMap.end()) {
        return;
    }

    const int bookmarkRow = bookmarksIter.value();
    beginRemoveRows(QModelIndex(), bookmarkRow, bookmarkRow);
    m_bookmarks.removeAt(bookmarksIter.value());
    bookmarksIter = m_bookmarksMap.erase(bookmarksIter);
    //
    while (bookmarksIter != m_bookmarksMap.end()) {
        *bookmarksIter -= 1;
        ++bookmarksIter;
    }
    endRemoveRows();
}

void ScriptBookmarksModel::aboutUpdateBookmarksModel(int _position, int _removed, int _added)
{
    //
    // Ищем начало изменения
    //
    int startMarkIndex = 0;
    for (; startMarkIndex < m_bookmarks.size(); ++startMarkIndex) {
        if (m_bookmarks.at(startMarkIndex).position > _position) {
            break;
        }
    }


    //
    // Если есть закладки на обработку
    //
    if (startMarkIndex < m_bookmarks.size()) {
        //
        // Смена формата
        //
        if (_removed == _added) {
            //
            // Если есть закладка, которая попадает в изменение формата, удаляем её
            //
            const int endPosition = _position + _added;
            for (int markIndex = startMarkIndex; markIndex < m_bookmarks.size(); ++markIndex) {
                const BookmarkInfo& mark = m_bookmarks[markIndex];
                if (mark.position >= _position
                    && mark.position < endPosition) {
                    beginRemoveRows(QModelIndex(), markIndex, markIndex);
                    m_bookmarks.removeAt(markIndex);
                    endRemoveRows();
                    --markIndex;
                }
            }
        }

        //
        // Прорабатываем удаление
        //
        if (_removed > 0) {
            const int removeEndPosition = _position + _removed;
            for (int markIndex = startMarkIndex; markIndex < m_bookmarks.size(); ++markIndex) {
                BookmarkInfo& mark = m_bookmarks[markIndex];
                //
                // Удалить закладки полностью входящие в удалённый текст
                //
                if (mark.position > _position
                    && mark.position <= removeEndPosition) {
                    beginRemoveRows(QModelIndex(), markIndex, markIndex);
                    m_bookmarks.removeAt(markIndex);
                    endRemoveRows();
                    --markIndex;
                }
                //
                // Скорректировать позицию заметки следующей за удалённым текстом
                //
                else {
                    mark.position -= _removed;
                }
            }
        }

        //
        // Прорабатываем добавление
        //
        if (_added > 0) {
            for (int markIndex = startMarkIndex; markIndex < m_bookmarks.size(); ++markIndex) {
                BookmarkInfo& mark = m_bookmarks[markIndex];
                //
                // Скорректировать позицию заметки после вставленного текст
                //
                if (mark.position > _position) {
                    mark.position += _added;
                }
            }
        }
    }


    //
    // Корректируем карту текущих элементов
    //
    m_bookmarksMap.clear();
    for (int markIndex = 0; markIndex < m_bookmarks.size(); ++markIndex) {
        const BookmarkInfo& mark = m_bookmarks[markIndex];
        m_bookmarksMap.insert(mark.position, markIndex);
    }


    //
    // Поиск и добавление новых
    //
    if (_added > 0) {
        QTextBlock currentBlock = m_document->findBlock(_position);
        while (currentBlock.isValid()
               && currentBlock.position() <= _position + _added) {

            do {
                if (currentBlock.userData() == nullptr) {
                    break;
                }

                TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(currentBlock.userData());
                if (!blockInfo->hasBookmark()) {
                    break;
                }

                const int startPosition = currentBlock.position();
                if (m_bookmarksMap.contains(startPosition)) {
                    break;
                }

                //
                // Если такой заметки не сохранено, добавляем
                //
                int insertPosition = m_bookmarks.size();
                QMap<int, int>::iterator iter = m_bookmarksMap.lowerBound(startPosition);
                if (iter != m_bookmarksMap.end()) {
                    insertPosition = iter.value();
                    if (m_bookmarks[insertPosition].position < startPosition) {
                        ++insertPosition;
                    }
                }
                //
                BookmarkInfo newMark;
                newMark.position = startPosition;
                newMark.color = blockInfo->bookmarkColor();
                newMark.text = blockInfo->bookmark();
                //
                beginInsertRows(QModelIndex(), insertPosition, insertPosition);
                m_bookmarks.insert(insertPosition, newMark);
                //
                m_bookmarksMap.insert(newMark.position, insertPosition);
                while (iter != m_bookmarksMap.end()) {
                    *iter = *iter + 1;
                    ++iter;
                }
                endInsertRows();
            } while (false);

            currentBlock = currentBlock.next();
        }
    }
}
