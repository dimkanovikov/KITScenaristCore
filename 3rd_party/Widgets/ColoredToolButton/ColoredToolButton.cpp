#include "ColoredToolButton.h"

#include "ColorsPane.h"
#include "GoogleColorsPane.h"
#include "WordHighlightColorsPane.h"

#include <3rd_party/Helpers/ImageHelper.h>
#include <3rd_party/Widgets/SlidingPanel/SlidingPanel.h>
#include <3rd_party/Widgets/WAF/Animation/Animation.h>

#include <QBitmap>
#include <QEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QWidgetAction>


ColoredToolButton::ColoredToolButton(const QIcon& _icon, QWidget* _parent, QWidget* _topLevelParent) :
    QToolButton(_parent),
    m_icon(_icon),
    m_colorNotChoosedYet(true),
    m_colorsPane(nullptr)
{
    setIcon(_icon);
    setFocusPolicy(Qt::NoFocus);
    aboutUpdateIcon(palette().text().color());

#ifdef MOBILE_OS
    m_colorsPanel = new SlidingPanel(_topLevelParent);
    m_colorsPanel->hide();
#endif

    connect(this, static_cast<void (QToolButton::*)(bool)>(&QToolButton::clicked), this, &ColoredToolButton::aboutClicked);
}

ColoredToolButton::ColoredToolButton(QWidget* _parent, QWidget* _topLevelParent) :
    QToolButton(_parent),
    m_colorNotChoosedYet(true),
    m_colorsPane(nullptr)
{
    setFocusPolicy(Qt::NoFocus);

#ifdef MOBILE_OS
    m_colorsPanel = new SlidingPanel(_topLevelParent);
    m_colorsPanel->hide();

    connect(this, static_cast<void (QToolButton::*)(bool)>(&QToolButton::clicked), this, &ColoredToolButton::aboutClicked);
#endif
}

ColoredToolButton::~ColoredToolButton()
{
#ifndef MOBILE_OS
    if (m_colorsPane != nullptr) {
        m_colorsPane->deleteLater();
    }
#endif
}

void ColoredToolButton::setColorsPane(ColoredToolButton::ColorsPaneType _pane)
{
    //
    // Удаляем предыдущую панель и меню, если была
    //
    if (m_colorsPane != nullptr) {
        m_colorsPane->close();
        disconnect(m_colorsPane, &ColorsPane::selected, this, &ColoredToolButton::setColor);
        m_colorsPane->deleteLater();
        m_colorsPane = nullptr;
    }
#ifndef MOBILE_OS
    if (menu() != nullptr) {
        menu()->deleteLater();
        setMenu(nullptr);
    }
#endif

    //
    // Создаём новую панель
    //
    switch (_pane) {
        default:
        case NonePane: {
            break;
        }

        case Google: {
            m_colorsPane = new GoogleColorsPane;
            break;
        }

        case WordHighlight: {
            m_colorsPane = new WordHighlightColorsPane;
            break;
        }
    }

    //
    // Настраиваем новую панель
    //
    setPopupMode(QToolButton::DelayedPopup);
    if (m_colorsPane != nullptr) {
#ifdef MOBILE_OS
        QHBoxLayout* layout = static_cast<QHBoxLayout*>(m_colorsPanel->layout());
        if (layout == nullptr) {
            layout = new QHBoxLayout(m_colorsPanel);
            layout->setContentsMargins(QMargins());
            layout->setSpacing(0);
        }
        layout->addWidget(m_colorsPane);
#else
        setPopupMode(QToolButton::MenuButtonPopup);

        QMenu* menu = new QMenu(this);
        QWidgetAction* wa = new QWidgetAction(menu);
        wa->setDefaultWidget(m_colorsPane);
        menu->addAction(wa);
        setMenu(menu);
#endif

        connect(m_colorsPane, &ColorsPane::selected, this, &ColoredToolButton::setColor);
    }
}

void ColoredToolButton::disableColor(const QColor& _color)
{
    m_colorsPane->disableColor(_color);
}

QColor ColoredToolButton::currentColor() const
{
    QColor result;
    if (m_colorNotChoosedYet == false) {
        result = m_colorsPane->currentColor();
    }
    return result;
}

void ColoredToolButton::selectFirstEnabledColor()
{
    m_colorsPane->selectFirstEnabledColor();

    updateColor(m_colorsPane->currentColor());
}

void ColoredToolButton::setColor(const QColor& _color)
{
    updateColor(_color);

    emit clicked(_color);
}

void ColoredToolButton::updateColor(const QColor& _color)
{
    QColor newColor;
    if (_color.isValid()) {
        m_colorNotChoosedYet = false;
        newColor = _color;
    } else {
        m_colorNotChoosedYet = true;
        newColor = palette().text().color();
    }

    if (m_colorsPane != nullptr
        && m_colorsPane->contains(_color)) {
        m_colorsPane->setCurrentColor(newColor);
#ifdef MOBILE_OS
        if (m_colorsPanel->isVisible()) {
            WAF::Animation::slideOut(m_colorsPanel, WAF::FromBottomToTop, true, true);
        }
#else
        menu()->close();
#endif
    }

    aboutUpdateIcon(_color);

    emit colorChanged(_color);
}

bool ColoredToolButton::event(QEvent* _event)
{
    if (m_colorNotChoosedYet) {
        if (_event->type() == QEvent::PaletteChange) {
            aboutUpdateIcon(palette().text().color());
        }
    }

    return QToolButton::event(_event);
}

void ColoredToolButton::aboutUpdateIcon(const QColor& _color)
{
    //
    // Если иконка ещё не была сохранена, делаем это
    //
    if (m_icon.isNull()) {
        m_icon = icon();
    }

    QIcon newIcon = m_icon;
    ImageHelper::setIconColor(newIcon, iconSize(), _color);
    setIcon(newIcon);
}

void ColoredToolButton::aboutClicked()
{
    if (m_colorsPane == nullptr) {
        return;
    }

#ifdef MOBILE_OS
    m_colorsPanel->raise();
    m_colorsPanel->resize(m_colorsPanel->sizeHint());
    m_colorsPanel->setFixedCornerPos(mapTo(m_colorsPanel->parentWidget(), pos()), Qt::BottomLeftCorner);
    WAF::Animation::slideIn(m_colorsPanel, WAF::FromBottomToTop, true, true);
#endif

    emit clicked(m_colorsPane->currentColor());
}
