#ifndef COMPARESCRIPTVERSIONSTOOL_H
#define COMPARESCRIPTVERSIONSTOOL_H

class QString;


namespace BusinessLogic
{
    /**
     * @brief Инструмент сравнения сценариев
     */
    class CompareScriptVersionsTool
    {
    public:
        /**
         * @brief Сравнить два сценария
         */
        static QString compareScripts(const QString& _firstScript, const QString& _secondScript);
    };
}

#endif // COMPARESCRIPTVERSIONSTOOL_H
