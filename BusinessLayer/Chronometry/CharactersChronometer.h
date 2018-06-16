#ifndef CHARACTERSCHRONOMETER_H
#define CHARACTERSCHRONOMETER_H

#include "AbstractChronometer.h"


namespace BusinessLogic
{
    /**
     * @brief Расчёт хронометража по количеству символов
     */
    class CharactersChronometer : public AbstractChronometer
    {
    public:
        CharactersChronometer();

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

#endif // CHARACTERSCHRONOMETER_H
