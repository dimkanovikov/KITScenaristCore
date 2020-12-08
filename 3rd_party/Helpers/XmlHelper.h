#pragma once

#include <QString>

namespace XmlHelper
{

/**
 * @brief Является ли строка тэгом
 */
inline bool isTag(const QString& _tag) {
    return !_tag.isEmpty() && _tag.startsWith("<") && _tag.endsWith(">");
}

} // namespace XmlHelper
