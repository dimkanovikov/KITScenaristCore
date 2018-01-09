#ifndef FDXEXPORTER_H
#define FDXEXPORTER_H

#include "AbstractExporter.h"

class QXmlStreamWriter;

namespace BusinessLogic
{
	class ScenarioDocument;


	/**
	 * @brief Экспортер в FDX
	 */
	class FdxExporter : public AbstractExporter
	{
	public:
		FdxExporter();

		/**
		 * @brief Экспорт заданного документа в указанный файл
		 */
        void exportTo(ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const override;

        /**
         * @brief Экспорт заданной модели разработки с указанными параметрами
         */
        void exportTo(const ResearchModelCheckableProxy* _researchModel,
                      const ExportParameters& _exportParameters) const override;

	private:
		/**
		 * @brief Записать текст сценария
		 */
		void writeContent(QXmlStreamWriter& _writer, ScenarioDocument* _scenario, const ExportParameters& _exportParameters) const;

		/**
		 * @brief Записать параметры сценария
		 */
		void writeSettings(QXmlStreamWriter& _writer) const;
	};
}

#endif // FDXEXPORTER_H
