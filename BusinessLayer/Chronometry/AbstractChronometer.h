#ifndef ABSTRACTCHRONOMETER_H
#define ABSTRACTCHRONOMETER_H

#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

class QString;


namespace BusinessLogic
{
    /**
     * @brief Базовый класс рассчёта хронометража
     */
    class AbstractChronometer
    {
    public:
        virtual ~AbstractChronometer() {}

        /**
         * @brief Наименование хронометра
         */
        virtual QString name() const = 0;

        /**
         * @brief Подсчитать длительность заданного текста определённого типа
         */
        virtual qreal calculateFrom(const QTextBlock& _block, int _from, int _length) const = 0;
    };
}

#endif // ABSTRACTCHRONOMETER_H
