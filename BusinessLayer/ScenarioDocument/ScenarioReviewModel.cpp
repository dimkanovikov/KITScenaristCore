#include "ScenarioReviewModel.h"

#include "ScenarioTextDocument.h"
#include "ScenarioTemplate.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/PasswordStorage.h>

#include <QDateTime>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

using BusinessLogic::ScenarioReviewModel;
using BusinessLogic::ScenarioTextDocument;
using BusinessLogic::ScenarioBlockStyle;
using BusinessLogic::ScenarioTemplateFacade;

namespace {
    /**
     * @brief Сформировать заготовку формата редакторской заметки
     */
    static QTextCharFormat makeFormat(const QTextCursor& _cursor) {
        QTextCharFormat format;
        format.setProperty(ScenarioBlockStyle::PropertyIsReviewMark, true);
        //
        // Если это добавление заметки, а не смена цвета, добавим информацию о пользователе
        //
        if (!_cursor.charFormat().hasProperty(ScenarioBlockStyle::PropertyComments)) {
            format.setProperty(ScenarioBlockStyle::PropertyComments, QStringList() << "");
            format.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, QStringList() << DataStorageLayer::StorageFacade::userName());
            format.setProperty(ScenarioBlockStyle::PropertyCommentsDates, QStringList() << QDateTime::currentDateTime().toString(Qt::ISODate));
        }

        return format;
    }

    /**
     * @brief Сформировать формат выделения цвета текста
     */
    static QTextCharFormat makeForegroundFormat(const QTextCursor& _cursor, const QColor& _color) {
        QTextCharFormat format = makeFormat(_cursor);
        format.setForeground(_color);

        return format;
    }

    /**
     * @brief Сформировать формат заливки фона
     */
    static QTextCharFormat makeBackgroundFormat(const QTextCursor& _cursor, const QColor& _color) {
        QTextCharFormat format = makeFormat(_cursor);
        format.setBackground(_color);
        //
        // Принудительно стираем цвет текста
        //
        format.setForeground(Qt::black);

        return format;
    }

    /**
     * @brief Сформировать формат выделения фона
     */
    static QTextCharFormat makeHighlightFormat(const QTextCursor& _cursor, const QColor& _color) {
        QTextCharFormat format = makeBackgroundFormat(_cursor, _color);
        format.setProperty(ScenarioBlockStyle::PropertyIsHighlight, true);

        return format;
    }
}


ScenarioReviewModel::ScenarioReviewModel(ScenarioTextDocument *_parent) :
    QAbstractListModel(_parent),
    m_document(_parent)
{
    Q_ASSERT(_parent);

    connect(m_document, &ScenarioTextDocument::contentsChange, this, &ScenarioReviewModel::aboutUpdateReviewModel);
}

bool ScenarioReviewModel::isEmpty()
{
    return m_reviewMarks.isEmpty();
}

int ScenarioReviewModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent);

    return m_reviewMarks.size();
}

QVariant ScenarioReviewModel::data(const QModelIndex& _index, int _role) const
{
    QVariant result;

    if (_index.isValid()
        && _index.row() < m_reviewMarks.size()) {
        const ReviewMarkInfo& info = m_reviewMarks.at(_index.row());

        switch (_role) {
            case Qt::DecorationRole: {
                if (info.isDone) {
                    result = QColor(Qt::gray);
                } else {
                    result = info.background.isValid() ? info.background : info.foreground;
                }
                break;
            }
            case Qt::ForegroundRole: result = info.foreground; break;
            case Qt::BackgroundRole: result = info.background; break;
            case IsDoneRole: result = info.isDone; break;
            case CommentsRole: result = info.comments; break;
            case CommentsAuthorsRole: result = info.authors; break;
            case CommentsDatesRole: result = info.dates; break;
            default: break;
        }
    }

    return result;
}

bool ScenarioReviewModel::removeRows(int _row, int _count, const QModelIndex& _parent)
{
    bool result = false;

    int last = _row + _count - 1;
    if (m_reviewMarks.size() > last) {
        beginRemoveRows(_parent, _row, last);
        for (int replies = _count; replies > 0; --replies) {
            const ReviewMarkInfo review = m_reviewMarks.takeAt(_row);
            //
            // Восстановим формат
            //
            if (!m_document->isEmpty()) {
                QTextCursor cursor(m_document);
                cursor.beginEditBlock();
                cursor.setPosition(review.startPosition);
                //
                // Для каждого блока по отдельности
                //
                while (!cursor.atEnd()
                       && cursor.position() <= review.endPosition()) {
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    //
                    // Если ещё в текущем блоке, очищаем формат
                    //
                    if (!cursor.atBlockStart()) {
                        const ScenarioBlockStyle::Type type = ScenarioBlockStyle::forBlock(cursor.block());
                        const ScenarioBlockStyle style = ScenarioTemplateFacade::getTemplate().blockStyle(type);

                        ScenarioTextDocument::updateBlockRevision(cursor);
                        cursor.setCharFormat(style.charFormat());
                    }
                    //
                    // Если перешли в другой блок, снимаем выделение
                    //
                    else {
                        cursor.setPosition(cursor.selectionEnd());
                    }
                }
                cursor.endEditBlock();

                emit reviewChanged();
            }
        }
        endRemoveRows();

        result = true;
    }

    return result;
}

void ScenarioReviewModel::setReviewMarkTextColor(int _startPosition, int _length, const QColor& _color)
{
    QTextCursor cursor(m_document);
    cursor.setPosition(_startPosition);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _length);
    if (cursor.charFormat().foreground() != _color) {
        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(::makeForegroundFormat(cursor, _color));
        emit reviewChanged();
    }
}

void ScenarioReviewModel::setReviewMarkTextBgColor(int _startPosition, int _length, const QColor& _color)
{

    QTextCursor cursor(m_document);
    cursor.setPosition(_startPosition);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _length);
    if (cursor.charFormat().background() != _color) {
        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(::makeBackgroundFormat(cursor, _color));
        emit reviewChanged();
    }
}

void ScenarioReviewModel::setReviewMarkTextHighlight(int _startPosition, int _length, const QColor& _color)
{
    QTextCursor cursor(m_document);
    cursor.setPosition(_startPosition);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _length);
    if (cursor.charFormat().background() != _color) {
        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(::makeHighlightFormat(cursor, _color));
        emit reviewChanged();
    }
}

void ScenarioReviewModel::setReviewMarkComment(int _startPosition, int _length, const QString& _comment)
{

    QTextCursor cursor(m_document);
    cursor.setPosition(_startPosition);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, _length);
    if (cursor.charFormat().toolTip() != _comment) {
        QTextCharFormat format;
        format.setProperty(ScenarioBlockStyle::PropertyIsReviewMark, true);
        format.setProperty(ScenarioBlockStyle::PropertyComments, QStringList() << _comment);
        format.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, QStringList() << DataStorageLayer::StorageFacade::userName());
        format.setProperty(ScenarioBlockStyle::PropertyCommentsDates, QStringList() << QDateTime::currentDateTime().toString(Qt::ISODate));

        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(format);

        emit reviewChanged();
    }
}

void ScenarioReviewModel::setReviewMarkComment(const QModelIndex& _index, const QString& _comment)
{
    if (_index.isValid()) {
        const ReviewMarkInfo mark = m_reviewMarks.at(_index.row());
        setReviewMarkComment(mark.startPosition, mark.length, _comment);
    }
}

void ScenarioReviewModel::addReviewMarkComment(const QModelIndex& _index, const QString& _comment)
{
    if (_index.isValid()) {
        const ReviewMarkInfo mark = m_reviewMarks.at(_index.row());

        QTextCursor cursor(m_document);
        cursor.setPosition(mark.startPosition);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mark.length);

        QTextCharFormat format = cursor.charFormat();
        QStringList comments = format.property(ScenarioBlockStyle::PropertyComments).toStringList();
        QStringList authors = format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
        QStringList dates = format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();

        comments << _comment;
        authors << DataStorageLayer::StorageFacade::userName();
        dates << QDateTime::currentDateTime().toString(Qt::ISODate);

        format.setProperty(ScenarioBlockStyle::PropertyComments, comments);
        format.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, authors);
        format.setProperty(ScenarioBlockStyle::PropertyCommentsDates, dates);

        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(format);

        emit reviewChanged();
    }
}

void ScenarioReviewModel::updateReviewMarkComment(const QModelIndex& _index, int _commentIndex, const QString& _comment)
{
    if (_index.isValid()) {
        const ReviewMarkInfo mark = m_reviewMarks.at(_index.row());

        QTextCursor cursor(m_document);
        cursor.setPosition(mark.startPosition);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mark.length);

        QTextCharFormat format = cursor.charFormat();
        QStringList comments = format.property(ScenarioBlockStyle::PropertyComments).toStringList();
        QStringList authors = format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
        QStringList dates = format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();

        if (comments.size() > _commentIndex) {
            comments[_commentIndex] = _comment;
            dates[_commentIndex] = QDateTime::currentDateTime().toString(Qt::ISODate);
        }

        format.setProperty(ScenarioBlockStyle::PropertyComments, comments);
        format.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, authors);
        format.setProperty(ScenarioBlockStyle::PropertyCommentsDates, dates);

        ScenarioTextDocument::updateBlockRevision(cursor);
        cursor.mergeCharFormat(format);

        emit reviewChanged();
    }
}

void ScenarioReviewModel::setReviewMarkIsDone(int _cursorPosition, bool _isDone)
{
    const QModelIndex indexForUpdate = indexForPosition(_cursorPosition);
    setReviewMarkIsDone(indexForUpdate, _isDone);
}

void ScenarioReviewModel::setReviewMarkIsDone(const QModelIndex& _index, bool _isDone)
{
    if (_index.isValid()) {
        const ReviewMarkInfo mark = m_reviewMarks.at(_index.row());

        QTextCursor cursor(m_document);
        cursor.setPosition(mark.startPosition);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mark.length);
        if (cursor.charFormat().boolProperty(ScenarioBlockStyle::PropertyIsDone) != _isDone) {
            QTextCharFormat format;
            format.setProperty(ScenarioBlockStyle::PropertyIsReviewMark, true);
            format.setProperty(ScenarioBlockStyle::PropertyIsDone, _isDone);

            ScenarioTextDocument::updateBlockRevision(cursor);
            cursor.mergeCharFormat(format);

            emit reviewChanged();
        }
    }
}

void ScenarioReviewModel::removeMarks(int _fromCursorPosition, int _toCursorPosition)
{
    if (_fromCursorPosition == _toCursorPosition) {
        removeMark(_fromCursorPosition);
    } else {
        //
        // Удаляем все заметки входящие в заданный интервал
        //
        QList<QModelIndex> indexesToDelete;
        for (int position = _fromCursorPosition; position <= _toCursorPosition; ++position) {
            const QModelIndex index = indexForPosition(position);
            if (!indexesToDelete.contains(index)) {
                indexesToDelete.prepend(index);
            }
        }
        foreach (const QModelIndex& index, indexesToDelete) {
            removeMark(index);
        }
    }
}

void ScenarioReviewModel::removeMark(int _cursorPosition)
{
    removeMark(indexForPosition(_cursorPosition));
}

void ScenarioReviewModel::removeMark(const QModelIndex& _index, int _commentIndex)
{
    if (_index.isValid()) {
        if (_commentIndex == 0) {
            removeRow(_index.row());
        } else {
            const ReviewMarkInfo mark = m_reviewMarks.at(_index.row());

            QTextCursor cursor(m_document);
            cursor.setPosition(mark.startPosition);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, mark.length);

            QTextCharFormat format = cursor.charFormat();
            QStringList comments = format.property(ScenarioBlockStyle::PropertyComments).toStringList();
            QStringList authors = format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
            QStringList dates = format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();

            if (comments.size() > _commentIndex) {
                comments.removeAt(_commentIndex);
                authors.removeAt(_commentIndex);
                dates.removeAt(_commentIndex);
            }

            format.setProperty(ScenarioBlockStyle::PropertyComments, comments);
            format.setProperty(ScenarioBlockStyle::PropertyCommentsAuthors, authors);
            format.setProperty(ScenarioBlockStyle::PropertyCommentsDates, dates);
            cursor.mergeCharFormat(format);

            emit reviewChanged();
        }
    }
}

int ScenarioReviewModel::markStartPosition(const QModelIndex& _index) const
{
    int startPosition = 0;
    if (_index.isValid()) {
        startPosition = m_reviewMarks.at(_index.row()).startPosition;
    }
    return startPosition;
}

int ScenarioReviewModel::markLength(const QModelIndex& _index) const
{
    int length = 0;
    if (_index.isValid()) {
        length = m_reviewMarks.at(_index.row()).length;
    }
    return length;
}

QModelIndex ScenarioReviewModel::indexForPosition(int _position)
{
    QModelIndex result;
    QMap<int, int>::const_iterator iterator = m_reviewMap.lowerBound(_position);
    if (iterator != m_reviewMap.end()) {
        const int reviewIndex = iterator.value();
        const ReviewMarkInfo info = m_reviewMarks.value(reviewIndex);
        if (info.startPosition <= _position
            && (info.startPosition + info.length) >= _position) {
            result = index(reviewIndex, 0);
        }
    }
    return result;
}

void ScenarioReviewModel::aboutUpdateReviewModel(int _position, int _removed, int _added)
{
    //
    // Ищем начало изменения
    //
    int startMarkIndex = 0;
    for (; startMarkIndex < m_reviewMarks.size(); ++startMarkIndex) {
        if (m_reviewMarks.at(startMarkIndex).endPosition() >= _position) {
            break;
        }
    }


    //
    // Если есть заметки на обработку
    //
    if (startMarkIndex < m_reviewMarks.size()) {
        //
        // Прорабатываем удаление
        //
        if (_removed > 0) {
            const int removeEndPosition = _position + _removed;
            for (int markIndex = startMarkIndex; markIndex < m_reviewMarks.size(); ++markIndex) {
                ReviewMarkInfo& mark = m_reviewMarks[markIndex];
                //
                // Удалить заметки полностью входящие в удалённый текст
                //
                if (mark.startPosition >= _position
                    && mark.endPosition() <= removeEndPosition) {
                    //
                    // Уведомляем клиента об удалении элементов модели
                    //
                    beginRemoveRows(QModelIndex(), markIndex, markIndex);
                    m_reviewMarks.removeAt(markIndex);
                    endRemoveRows();
                    --markIndex;
                }
                //
                // Скорректировать размер заметки, чъё начало попадает под удаление
                //
                else if (mark.startPosition >= _position
                         && mark.startPosition < removeEndPosition
                         && mark.endPosition() > removeEndPosition) {
                    mark.length = mark.endPosition() - removeEndPosition;
                    mark.startPosition = _position;
                }
                //
                // Скорректировать размер заметки, чей конец попадает под удаление
                //
                else if (mark.startPosition < _position
                         && mark.endPosition() > _position
                         && mark.endPosition() <= removeEndPosition) {
                    mark.length = _position - mark.startPosition;
                }
                //
                // Скорректировать размер заметки, чъё тело попадает под удаление
                //
                else if (mark.startPosition <= _position
                         && mark.endPosition() >= removeEndPosition) {
                    mark.length -= _removed;
                }
                //
                // Скорректировать позицию заметки следующей за удалённым текстом
                //
                else {
                    mark.startPosition -= _removed;
                }
            }
        }

        //
        // Прорабатываем добавление
        //
        int searchPosition = _position;
        int searchPositionEnd = _position + _added;
        if (_added > 0) {
            for (int markIndex = startMarkIndex; markIndex < m_reviewMarks.size(); ++markIndex) {
                ReviewMarkInfo& mark = m_reviewMarks[markIndex];
                //
                // Скорректировать позицию заметки после вставленного текст
                //
                if (mark.startPosition > _position) {
                    mark.startPosition += _added;
                }
                //
                // Все остальные заметки удаляем, т.к. вставиться мог в том числе
                // и текст содержащий внутри себя форматирование, поэтому нужно его разобрать целиком
                //
                else {
                    //
                    // Корректируем начальную позицию поиска и количество добавленных символов,
                    // чтобы включить удалённый комментарий целиком
                    //
                    if (searchPosition > mark.startPosition) {
                        searchPosition = mark.startPosition;
                    }
                    if (searchPositionEnd < mark.endPosition()) {
                        searchPositionEnd = mark.endPosition();
                    }

                    //
                    // Уведомляем клиента об удалении элементов модели
                    //
                    beginRemoveRows(QModelIndex(), markIndex, markIndex);
                    m_reviewMarks.removeAt(markIndex);
                    endRemoveRows();
                    --markIndex;
                }
            }
        }

        //
        // Расширяем границы поиска
        //
        _position = searchPosition;
        _added = searchPositionEnd - searchPosition;
    }


    //
    // Корректируем карту текущих элементов
    //
    m_reviewMap.clear();
    for (int markIndex = 0; markIndex < m_reviewMarks.size(); ++markIndex) {
        const ReviewMarkInfo& mark = m_reviewMarks[markIndex];
        m_reviewMap.insert(mark.startPosition, markIndex);
        m_reviewMap.insert(mark.beforeEndPosition(), markIndex);
    }


    //
    // Поиск и добавление новых
    //
    if (_added > 0) {
        QTextCursor cursor(m_document);
        cursor.setPosition(_position);
        while (cursor.position() < (_position + _added)
               && !cursor.atEnd()) {
            const QTextBlock currentBlock = cursor.block();
            auto textFormats = currentBlock.textFormats();
            if (textFormats.isEmpty()) {
                textFormats.append({0, 0, currentBlock.charFormat()});
            }

            for (const QTextLayout::FormatRange& range : textFormats) {
                if (range.format.boolProperty(ScenarioBlockStyle::PropertyIsReviewMark)) {
                    //
                    // Если такой заметки не сохранено, добавляем
                    //
                    const int startPosition = currentBlock.position() + range.start;
                    if (!m_reviewMap.contains(startPosition)) {
                        int insertPosition = m_reviewMarks.size();
                        QMap<int, int>::iterator iter = m_reviewMap.lowerBound(startPosition);
                        if (iter != m_reviewMap.end()) {
                            insertPosition = iter.value();
                            if (m_reviewMarks[insertPosition].startPosition < startPosition) {
                                ++insertPosition;
                            }
                        }

                        ReviewMarkInfo newMark;
                        if (range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                            newMark.background = range.format.background().color();
                        }
                        if (range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                            newMark.foreground = range.format.foreground().color();
                        }
                        newMark.startPosition = currentBlock.position() + range.start;
                        newMark.length = range.length;
                        newMark.isDone = range.format.boolProperty(ScenarioBlockStyle::PropertyIsDone);
                        newMark.comments = range.format.property(ScenarioBlockStyle::PropertyComments).toStringList();
                        newMark.authors = range.format.property(ScenarioBlockStyle::PropertyCommentsAuthors).toStringList();
                        newMark.dates = range.format.property(ScenarioBlockStyle::PropertyCommentsDates).toStringList();

                        //
                        // Если это не первая заметка
                        //
                        if (insertPosition > 0) {
                            const int prevMarkIndex = insertPosition - 1;
                            ReviewMarkInfo& prevMark = m_reviewMarks[prevMarkIndex];
                            //
                            // Если стили одинаковы
                            //
                            if (prevMark.foreground == newMark.foreground
                                && prevMark.background == newMark.background
                                && prevMark.comments == newMark.comments
                                && prevMark.authors == newMark.authors
                                && prevMark.dates == newMark.dates) {
                                //
                                // Проверяем, можно ли её объединить с предыдущей
                                //
                                if (
                                    // в одном блоке
                                    prevMark.endPosition() == newMark.startPosition
                                    // в разных блоках
                                    || prevMark.endPosition() == newMark.startPosition - 1) {
                                    //
                                    // Обновляем сохранённую заметку
                                    //
                                    const int oldEndPos = prevMark.endPosition();
                                    prevMark.length += 1 + newMark.length;
                                    prevMark.isDone = newMark.isDone;
                                    //
                                    // Обновляем карту
                                    //
                                    m_reviewMap.remove(oldEndPos);
                                    m_reviewMap.insert(prevMark.beforeEndPosition(), prevMarkIndex);

                                    //
                                    // Переходим к обработке следующего элемента
                                    //
                                    continue;
                                }
                                //
                                // А может быть она итак уже в неё входит
                                //
                                else if (prevMark.startPosition < newMark.startPosition
                                         && prevMark.endPosition() >= newMark.endPosition()) {
                                    //
                                    // Тогда просто игнорируем эту заметку
                                    //
                                    continue;
                                }
                            }
                        }


                        beginInsertRows(QModelIndex(), insertPosition, insertPosition);
                        m_reviewMarks.insert(insertPosition, newMark);
                        //
                        m_reviewMap.insert(newMark.startPosition, insertPosition);
                        m_reviewMap.insert(newMark.beforeEndPosition(), insertPosition);
                        while (iter != m_reviewMap.end()) {
                            *iter = iter.value() + 1;
                            ++iter;
                        }
                        endInsertRows();
                    }
                }
            }

            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        }
    }
}


int ScenarioReviewModel::ReviewMarkInfo::endPosition() const
{
    return startPosition + length;
}

int ScenarioReviewModel::ReviewMarkInfo::beforeEndPosition() const
{
    return endPosition() - 1;
}
