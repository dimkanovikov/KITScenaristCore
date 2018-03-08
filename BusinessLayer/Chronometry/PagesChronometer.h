#ifndef PAGESCHRONOMETER_H
#define PAGESCHRONOMETER_H

#include "AbstractChronometer.h"


namespace BusinessLogic
{
    /**
     * @brief Посчёт хронометража по страницам текста
     */
    class PagesChronometer : public AbstractChronometer
    {
    public:
        PagesChronometer();

        /**
         * @brief Наименование хронометра
         */
        QString name() const;

        /**
         * @brief Подсчитать длительность заданного текста определённого типа
         */
        qreal calculateFrom(const QTextBlock& _block, int _from, int _lenght) const override;
    };
}

#endif // PAGESCHRONOMETER_H
