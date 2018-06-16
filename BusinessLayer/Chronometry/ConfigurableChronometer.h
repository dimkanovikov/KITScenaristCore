#ifndef CONFIGURABLECHRONOMETER_H
#define CONFIGURABLECHRONOMETER_H

#include "AbstractChronometer.h"


namespace BusinessLogic
{
    /**
     * @brief Расчёт хронометража а-ля Софокл
     */
    class ConfigurableChronometer : public AbstractChronometer
    {
    public:
        ConfigurableChronometer();

        /**
         * @brief Наименование хронометра
         */
        QString name() const override;

        /**
         * @brief Подсчитать длительность заданного текста определённого типа
         */
        qreal calculateFrom(const QTextBlock& _block, int _from, int _length) const override;
    };
}

#endif // CONFIGURABLECHRONOMETER_H
