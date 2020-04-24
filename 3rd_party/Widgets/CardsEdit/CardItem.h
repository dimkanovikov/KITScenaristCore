#ifndef CARDITEM_H
#define CARDITEM_H

#include <QGraphicsObject>

#include <QGraphicsDropShadowEffect>
#include <QParallelAnimationGroup>

/**
 * @brief Элемент сцены являющий собой сцену или папку сценария
 */
class CardItem : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal zValue READ zValue WRITE setZValue)

public:
    /**
     * @brief Тип карточки
     */
    static const int Type = UserType + 15;

    /**
     * @brief Майм тип карточки
     */
    static const QString MimeType;

public:
    explicit CardItem(QGraphicsItem* _parent = nullptr);
    explicit CardItem(const QByteArray& mimeData, QGraphicsItem* _parent = nullptr);

    /**
     * @brief Идентификатор
     */
    /** @{ */
    void setUuid(const QString& _uuid);
    QString uuid() const;
    /** @} */

    /**
     * @brief Является ли карточка группирующей
     */
    /** @{ */
    void setIsFolder(bool _isFolder);
    bool isFolder() const;
    /** @} */

    /**
     * @brief Номер сцены
     */
    /** @{ */
    void setNumber(const QString& _number);
    QString number() const;
    /** @} */

    /**
     * @brief Заголовок
     */
    /** @{ */
    void setTitle(const QString& _title);
    QString title() const;
    /** @} */

    /**
     * @brief Описание
     */
    /** @{ */
    void setDescription(const QString& _description);
    QString description() const;
    /** @} */

    /**
     * @brief Штамп
     */
    /** @{ */
    void setStamp(const QString& _stamp);
    QString stamp() const;
    /** @} */

    /**
     * @brief Цвета
     */
    /** @{ */
    void setColors(const QString& _colors);
    QString colors() const;
    /** @} */

    /**
     * @brief Вложена ли карточка в папку
     */
    /** @{ */
    void setIsEmbedded(bool _embedded);
    bool isEmbedded() const;
    /** @} */

    /**
     * @brief Размер карточки
     */
    /** @{ */
    void setSize(const QSizeF& _size);
    QSizeF size() const;
    /** @} */

    /**
     * @brief Установить флаг, что папка готова принять карточку
     */
    void setIsReadyForEmbed(bool _isReady);

    /**
     * @brief Установить режим перемещения между сценами (true) или по сцене (false)
     */
    void setInDragOutMode(bool _inDragOutMode);

    /**
     * @brief Переопределяем метод, чтобы работал qgraphicsitem_cast
     */
    int type() const override;

    /**
     * @brief Область занимаемая карточкой
     */
    QRectF boundingRect() const override;

    /**
     * @brief Область занимаемая карточкой с учётом резервной области под декорацию папки
     */
    QRectF boundingRectCorrected() const;

    /**
     * @brief Отрисовка карточки
     */
    void paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget) override;

    /**
     * @brief Взять карточку с доски
     */
    void takeFromBoard();

    /**
     * @brief Положить карточку обратно на доску
     */
    void putOnBoard();

    /**
     * @brief Определить идентификатор папки, в которую будет вкладываться карточка
     */
    QString cardForEmbedUuid() const;

    /**
     * @brief Настроить внешний вид карточки, когда она отфильтрована
     */
    void setFiltered(bool _filtered);

protected:
    /**
     * @brief Переопределяем для проверки возможности вложить карточку в папку
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent* _event) override;

    /**
     * @brief Переопределяем для реализации возможности перетаскивания сцен между двумя сценами, а так же для анимации выделения
     */
    void mousePressEvent(QGraphicsSceneMouseEvent* _event) override;

    /**
     * @brief Переопределяем для реализации анимации снятия выделения
     */
    /** @{ */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* _event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* _event) override;
    /** @} */

private:
    /**
     * @brief Идентификатор
     */
    QString m_uuid;

    /**
     * @brief Является ли карточка папкой (true) или сценой (false)
     */
    bool m_isFolder = false;

    /**
     * @brief Номер сцены
     */
    QString m_number;

    /**
     * @brief Заголовок карточки
     */
    QString m_title;

    /**
     * @brief Описание карточки
     */
    QString m_description;

    /**
     * @brief Штамп на карточке
     */
    QString m_stamp;

    /**
     * @brief Цвета карточки
     */
    QString m_colors;

    /**
     * @brief Вложена ли карточка в папку
     */
    bool m_isEmbedded = false;

    /**
     * @brief Размер карточки
     */
    QSizeF m_size = QSizeF(200, 150);

    /**
     * @brief Находится ли папка в состоянии готовности вставки карточки
     */
    bool m_isReadyForEmbed = false;

    /**
     * @brief Находится ли карточка в состояния переноса между сценами
     */
    bool m_isInDragOutMode = false;

    /**
     * @brief Эффект отбрасывания тени
     */
    QScopedPointer<QGraphicsDropShadowEffect> m_shadowEffect;

    /**
     * @brief Анимация карточки
     */
    QScopedPointer<QParallelAnimationGroup> m_animation;
};

#endif // CARDITEM_H
