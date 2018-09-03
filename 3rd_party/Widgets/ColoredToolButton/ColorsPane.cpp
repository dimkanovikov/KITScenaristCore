#include "ColorsPane.h"


ColorsPane::ColorsPane(QWidget* _parent)
    : QWidget(_parent)
{
}

void ColorsPane::disableColor(const QColor& _color)
{
    for (auto& colorInfo : m_colorsInfos) {
        if (colorInfo.color == _color) {
            colorInfo.isEnabled = false;
            break;
        }
    }

    update();
}

QColor ColorsPane::currentColor() const
{
    return m_currentColorInfo.color;
}

bool ColorsPane::contains(const QColor& _color) const
{
    for (const auto& colorInfo : m_colorsInfos) {
        if (colorInfo.color == _color) {
            return true;
        }
    }

    return false;
}

void ColorsPane::setCurrentColor(const QColor& _color)
{
    for (const auto& colorInfo : m_colorsInfos) {
        if (colorInfo.color == _color) {
            m_currentColorInfo = colorInfo;
            break;
        }
    }

    update();
}

QList<ColorKeyInfo>& ColorsPane::colorsInfos()
{
    return m_colorsInfos;
}

ColorKeyInfo& ColorsPane::currentColorInfo()
{
    return m_currentColorInfo;
}
