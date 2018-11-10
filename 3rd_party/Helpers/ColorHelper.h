#ifndef COLORHELPER_H
#define COLORHELPER_H

#include <QColor>
#include <QSet>

/**
 * @brief Вспомогательные функции для работы с цветами
 */
class ColorHelper {
public:
    /**
     * @brief Получить цвет для курсора соавтора
     */
    static QColor cursorColor(int _index) {
        QColor color;
        switch (_index % 8) {
            case 0: color = Qt::red; break;
            case 1: color = Qt::darkGreen; break;
            case 2: color = Qt::blue; break;
            case 3: color = Qt::darkCyan; break;
            case 4: color = Qt::magenta; break;
            case 5: color = Qt::darkMagenta; break;
            case 6: color = Qt::darkRed; break;
            case 7: color = Qt::darkYellow; break;
        }
        return color;
    }

    /**
     * @brief Определить цвет текста контрастирующий с заданным цветом фона
     */
    static QColor textColor(const QColor& _forBackgroundColor) {
        static QSet<QString> s_lightTextColorBackgrounds = {
            "#000000",
            "#434343",
            "#666666",
            "#980000",
            "#ff0000",
            "#4a86e8",
            "#0000ff",
            "#9900ff",
            "#ff00ff",
            "#c63f24",
            "#e06666",
            "#8e7cc3",
            "#c27ba0",
            "#a21b00",
            "#cc0000",
            "#45818e",
            "#3c78d8",
            "#3d85c6",
            "#674ea7",
            "#a64d79",
            "#831f0c",
            "#990000",
            "#b45f06",
            "#bf9000",
            "#38761d",
            "#134f5c",
            "#1155cc",
            "#0b5394",
            "#351c75",
            "#741b47",
            "#590f00",
            "#650000",
            "#783f04",
            "#7f6000",
            "#274e13",
            "#0c343d",
            "#1c4485",
            "#073762",
            "#1f124c",
            "#4b112f"
        };
        if (s_lightTextColorBackgrounds.contains(_forBackgroundColor.name())) {
            return QColor("#ebebeb");
        }
        return QColor("#38393a");
    }
};

#endif //COLORHELPER_H
