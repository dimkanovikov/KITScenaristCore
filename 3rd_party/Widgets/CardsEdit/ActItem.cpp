#include "ActItem.h"

#include <3rd_party/Helpers/ColorHelper.h>
#include <3rd_party/Helpers/StyleSheetHelper.h>
#include <3rd_party/Helpers/TextUtils.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

namespace {
    /**
     * @brief Флаги для отрисовки текста в зависимости от локали
     */
    static Qt::AlignmentFlag textDrawAlign() {
        if (QLocale().textDirection() == Qt::LeftToRight) {
            return Qt::AlignLeft;
        } else {
            return Qt::AlignRight;
        }
    }
}


ActItem::ActItem(QGraphicsItem* _parent) :
    QGraphicsObject(_parent),
    m_shadowEffect(new QGraphicsDropShadowEffect)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setCacheMode(QGraphicsItem::NoCache);

    m_shadowEffect->setBlurRadius(StyleSheetHelper::dpToPx(12));
    m_shadowEffect->setColor(QColor(63, 63, 63, 210));
    m_shadowEffect->setXOffset(0);
    m_shadowEffect->setYOffset(StyleSheetHelper::dpToPx(1));
    setGraphicsEffect(m_shadowEffect.data());
}

void ActItem::setUuid(const QString& _uuid)
{
    if (m_uuid != _uuid) {
        m_uuid = _uuid;
    }
}

QString ActItem::uuid() const
{
    return m_uuid;
}

void ActItem::setTitle(const QString& _title)
{
    if (m_title != _title) {
        m_title = _title;
        update();
    }
}

QString ActItem::title() const
{
    return m_title;
}

void ActItem::setDescription(const QString& _description)
{
    if (m_description != _description) {
        m_description = _description;
        update();
    }
}

QString ActItem::description() const
{
    return m_description;
}

void ActItem::setColors(const QString& _colors)
{
    if (m_colors != _colors) {
        m_colors = _colors;
        update();
    }
}

QString ActItem::colors() const
{
    return m_colors;
}

int ActItem::type() const
{
    return Type;
}

void ActItem::setBoundingRect(const QRectF& _boundingRect)
{
    if (m_boundingRect != _boundingRect) {
            prepareGeometryChange();
            QRectF boundingRectCorrected = _boundingRect;
            boundingRectCorrected.setWidth(qMax(static_cast<qreal>(StyleSheetHelper::dpToPx(120.0)),
                                                boundingRectCorrected.width()));
            m_boundingRect = boundingRectCorrected;
        }
}

QRectF ActItem::boundingRect() const
{
    return m_boundingRect;
}

void ActItem::setCanMoveX(bool _canMove)
{
    if (m_canMoveX != _canMove) {
        m_canMoveX = _canMove;
    }
}

void ActItem::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    Q_UNUSED(_option);
    Q_UNUSED(_widget);

    _painter->save();

    {
        _painter->setClipRect(m_boundingRect);

        QRectF actRect = m_boundingRect;
        if (scene() != nullptr
            && !scene()->views().isEmpty()) {
            const QGraphicsView* view = scene()->views().first();
            const QPointF viewTopLeftPoint = view->mapToScene(QPoint(0, 0));
            const int scrollDelta = view->verticalScrollBar()->isVisible() ? view->verticalScrollBar()->width() : 0;
            const QPointF viewTopRightPoint = view->mapToScene(QPoint(view->width() - scrollDelta, 0));

            const qreal actWidth = viewTopRightPoint.x() - viewTopLeftPoint.x();
            if (actWidth < actRect.width()) {
                actRect.setLeft(viewTopLeftPoint.x());
                actRect.setWidth(actWidth);
            }
        }
        const QPalette palette = QApplication::palette();
        const QStringList colors = m_colors.split(";", QString::SkipEmptyParts);

        //
        // Рисуем фон
        //
        // ... заданным цветом, если он задан
        //
        if (!colors.isEmpty()) {
            _painter->setBrush(QColor(colors.first()));
            _painter->setPen(QColor(colors.first()));
        }
        //
        // ... или стандартным цветом
        //
        else {
            _painter->setBrush(palette.base());
            _painter->setPen(palette.base().color());
        }
        _painter->drawRect(actRect);

        //
        // Рисуем дополнительные цвета
        //
        if (!m_colors.isEmpty()) {
            QStringList colorsNamesList = m_colors.split(";", QString::SkipEmptyParts);
            colorsNamesList.removeFirst();
            //
            // ... если они есть
            //
            if (!colorsNamesList.isEmpty()) {
                const qreal additionalColorWidth = StyleSheetHelper::dpToPx(20);
                const int colorsCount = colorsNamesList.size() - 1;
                const int actXPos = QLocale().textDirection() == Qt::LeftToRight
                                    ? actRect.left()
                                    : actRect.right();
                QRectF colorRect(actXPos, actRect.top(), additionalColorWidth, actRect.height());
                for (int colorIndex = colorsCount; colorIndex >= 0; --colorIndex) {
                    const int moveDelta = (colorsCount - colorIndex + 1)*additionalColorWidth;
                    if (QLocale().textDirection() == Qt::LeftToRight) {
                        colorRect.moveLeft(actRect.right() - moveDelta);
                    } else {
                        colorRect.moveRight(actRect.left() + moveDelta);
                    }
                    _painter->fillRect(colorRect, QColor(colorsNamesList.value(colorIndex)));
                }
            }
        }

        //
        // Рисуем заголовок
        //
        QTextOption textoption;
        textoption.setAlignment(Qt::AlignTop | ::textDrawAlign());
        textoption.setWrapMode(QTextOption::NoWrap);
        QFont font = _painter->font();
        font.setBold(true);
        _painter->setFont(font);
        if (!colors.isEmpty()) {
            _painter->setPen(ColorHelper::textColor(QColor(colors.first())));
        } else {
            _painter->setPen(palette.text().color());
        }
        const int titleWidth = _painter->fontMetrics().width(m_title);
        const int titleHeight = _painter->fontMetrics().height();
        const int titleXPos = QLocale().textDirection() == Qt::LeftToRight
                              ? actRect.left() + StyleSheetHelper::dpToPx(7)
                              : actRect.right() - StyleSheetHelper::dpToPx(7) - titleWidth;
        const QRectF titleRect(titleXPos, StyleSheetHelper::dpToPx(9), titleWidth, titleHeight);
        _painter->drawText(titleRect, m_title, textoption);

        //
        // Рисуем описание
        //
        font.setBold(false);
        _painter->setFont(font);
        const int spacing = titleRect.height() / 2;
        const int descriptionXPos = QLocale().textDirection() == Qt::LeftToRight
                                    ? titleRect.right() + spacing
                                    : actRect.left();
        const int descriptionWidth = QLocale().textDirection() == Qt::LeftToRight
                                     ? actRect.size().width() - titleRect.width() - spacing - StyleSheetHelper::dpToPx(7)
                                     : actRect.size().width() - titleRect.width() - spacing*2 - StyleSheetHelper::dpToPx(7);
        const QRectF descriptionRect(descriptionXPos, StyleSheetHelper::dpToPx(9), descriptionWidth, titleHeight);
        _painter->drawText(descriptionRect, m_description, textoption);

        //
        // Рисуем рамку выделения
        //
        if (isSelected()) {
            _painter->setBrush(Qt::transparent);
            _painter->setPen(QPen(palette.highlight(), StyleSheetHelper::dpToPx(1), Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
            _painter->drawRect(actRect);
        }
    }

    _painter->restore();
}

QVariant ActItem::itemChange(QGraphicsItem::GraphicsItemChange _change, const QVariant& _value)
{
    if (!m_canMoveX) {
        switch (_change) {
            case ItemPositionChange: {
                QPointF newPositionCorrected = _value.toPointF();
                newPositionCorrected.setX(0);
                return QGraphicsItem::itemChange(_change, newPositionCorrected);
            }

            default: {
                break;
            }
        }
    }

    return QGraphicsItem::itemChange(_change, _value);
}
