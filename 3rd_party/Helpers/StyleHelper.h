#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QProxyStyle>


/**
 * @brief Стиль для отрисовки крупных иконок у меню
 */
class BigMenuIconStyle: public QProxyStyle {
    Q_OBJECT

public:
    BigMenuIconStyle(QStyle* _style = 0) : QProxyStyle(_style) {}
    BigMenuIconStyle(const QString& _key) : QProxyStyle(_key) {}

    /**
     * @brief Переопределяем, чтобы задать собственный размер иконки
     */
    virtual int pixelMetric(QStyle::PixelMetric _metric, const QStyleOption* _option = nullptr, const QWidget* _widget = nullptr) const {
        switch (_metric) {
            case QStyle::PM_SmallIconSize:
                return 20;
            default:
                return QProxyStyle::pixelMetric(_metric, _option, _widget);
        }
    }
};

#endif // STYLEHELPER_H
