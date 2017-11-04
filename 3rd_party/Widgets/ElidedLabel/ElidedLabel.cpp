#include "ElidedLabel.h"

#include <QDebug>
#include <QPainter>
#include <QResizeEvent>

//---------------------------------------------------------------------

ElidedLabel::ElidedLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
    , elideMode_(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(const QString &txt, QWidget *parent, Qt::WindowFlags f)
    : QLabel(txt, parent, f)
    , elideMode_(Qt::ElideRight)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(const QString &txt,
                         Qt::TextElideMode elideMode,
                         QWidget *parent,
                         Qt::WindowFlags f)
    : QLabel(txt, parent, f)
    , elideMode_(elideMode)
{
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

//---------------------------------------------------------------------

void ElidedLabel::setText(const QString &txt) {
    sourceText_ = txt;
    cacheElidedText(geometry().width());
//    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

//---------------------------------------------------------------------

void ElidedLabel::cacheElidedText(int w) {
    cachedElidedText_ = fontMetrics().elidedText(sourceText_, elideMode_, w, Qt::TextShowMnemonic);
    if (elideMode_ != Qt::ElideNone
        && cachedElidedText_ != text()) {
        QLabel::setText(cachedElidedText_);
    }
}

//---------------------------------------------------------------------

void ElidedLabel::resizeEvent(QResizeEvent *e) {
    QLabel::resizeEvent(e);
    cacheElidedText(e->size().width());
}
