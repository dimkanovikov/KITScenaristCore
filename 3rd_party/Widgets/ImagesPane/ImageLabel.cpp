#include "ImageLabel.h"

#include <QApplication>
#include <QFileDialog>
#include <QPainter>
#include <QStyle>
#include <QToolButton>
#include <QVariant>


ImageLabel::ImageLabel(QWidget* _parent) :
    QWidget(_parent),
    m_sortOrder(0),
    m_clearButton(new QToolButton(this)),
    m_isReadOnly(false)
{
    setCursor(Qt::PointingHandCursor);

    m_clearButton->setIcon(QIcon(":/Graphics/Icons/Editing/red_cross.png"));
    m_clearButton->setStyleSheet("border: none;");
    m_clearButton->setAttribute(Qt::WA_OpaquePaintEvent);
    m_clearButton->resize(m_clearButton->sizeHint());
    m_clearButton->hide();

    connect(m_clearButton, &QToolButton::clicked, this, &ImageLabel::removeRequested);
}

void ImageLabel::setImage(const QPixmap& _image)
{
    m_image = _image;

    update();
}

QPixmap ImageLabel::image() const
{
    return m_image;
}

void ImageLabel::setSortOrder(int _sortOrder)
{
    if (m_sortOrder != _sortOrder) {
        m_sortOrder = _sortOrder;
    }
}

int ImageLabel::sortOrder() const
{
    return m_sortOrder;
}

void ImageLabel::setReadOnly(bool _readOnly)
{
    if (m_isReadOnly != _readOnly) {
        m_isReadOnly = _readOnly;

        setCursor(_readOnly ? Qt::ArrowCursor : Qt::PointingHandCursor);
    }
}

void ImageLabel::enterEvent(QEvent* _event)
{
    //
    // Если фотография установлена покажем кнопку удаления
    //
    if (!m_isReadOnly && !m_image.isNull()) {
        m_clearButton->move(this->width() - m_clearButton->width(), 0);
        m_clearButton->show();
    }

    QWidget::enterEvent(_event);
}

void ImageLabel::leaveEvent(QEvent* _event)
{
    //
    // Скроем кнопку удаления
    //
    m_clearButton->hide();

    QWidget::leaveEvent(_event);
}

void ImageLabel::mousePressEvent(QMouseEvent* _event)
{
    emit clicked();

    QWidget::mousePressEvent(_event);
}

void ImageLabel::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    if (!m_image.isNull()) {
        QPainter painter;
        painter.begin(this);
        const QPixmap scaledPhoto = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap((width() - scaledPhoto.width()) / 2,
                           (height() - scaledPhoto.height()) / 2,
                           scaledPhoto);
        painter.end();
    }
}
