/*
Copyright 2014 John Bailey

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "MetricDumper.hpp"
#include "MetricOptions.hpp"

#include <iomanip>

#define IS_OUTPUT_TREE_FORM( _fmt ) (((_fmt) == METRIC_DUMP_FORMAT_TREE ) || ((_fmt) == METRIC_DUMP_FORMAT_SPARSE_TREE ))

const std::string MetricDumper::m_namePrefix[METRIC_UNIT_MAX] = {
	"",
	"File: ",
	"\tFunction: ",
	"\tMethod: "
};
const std::string MetricDumper::m_dumpPrefix[METRIC_UNIT_MAX] = {
	"",
	"\t",
	"\t\t",
	"\t\t"
};

void MetricDumper::dumpMetric(std::ostream& out, const MetricUnit* const p_unit, const MetricType_e p_metric, const MetricDumpFormat_e p_fmt, const std::string& p_sep, const bool p_recurse, const std::string& p_suffix,bool p_useShortNames)
{
	MetricUnit::counter_t val = p_unit->getCounter(p_metric, p_recurse);
	const MetricUnitType_e metricType = p_unit->GetType();

	if ((p_fmt != METRIC_DUMP_FORMAT_SPARSE_TREE) ||
		(val != 0))
	{
		if (IS_OUTPUT_TREE_FORM(p_fmt)) {
			std::string metricName;

			if (p_useShortNames)
			{
				metricName = MetricUnit::getMetricShortName(p_metric);
			}
			else
			{
				metricName = MetricUnit::getMetricName(p_metric);
			}

			out << m_dumpPrefix[metricType] << metricName << p_suffix << ": ";
		}

		uint32_t scaling = MetricUnit::getMetricScaling(p_metric);

		if (scaling == 1)
		{
			out << val << p_sep;
		}
		else
		{
			out << std::setprecision(6) << ((float)(val)) / scaling << p_sep;
		}
	}
}

void MetricDumper::dump(std::ostream& out, const MetricUnit* const p_topLevel, const bool p_output[METRIC_UNIT_MAX], const MetricDumpFormat_e p_fmt, const MetricOptions* const p_options)
{
	std::string sep;
	const MetricUnitType_e metricUnitType = p_topLevel->GetType();
	const bool useShortNames = (p_options != NULL) && p_options->getUseShortNames();

	if (p_output[metricUnitType])
	{
		switch (p_fmt) {
		case METRIC_DUMP_FORMAT_TREE:
		case METRIC_DUMP_FORMAT_SPARSE_TREE:
			// TODO: Should be endl
			sep = "\r\n";
			break;
		case METRIC_DUMP_FORMAT_TSV:
			sep = "\t";
			break;
		case METRIC_DUMP_FORMAT_CSV:
			sep = ",";
			break;
		}

		if ((p_fmt == METRIC_DUMP_FORMAT_TSV) ||
			(p_fmt == METRIC_DUMP_FORMAT_CSV)) {

			if (metricUnitType == METRIC_UNIT_GLOBAL)
			{
				out << "Name" << sep;
				int loop;
				for (loop = 0;
					loop < METRIC_TYPE_MAX;
					loop++)
				{
					const MetricType_e metric = static_cast<MetricType_e>(loop);
					const bool localAndCumulativeOutputs = MetricUnit::isMetricCumulative(metric) && MetricUnit::isMetricLocalAndCumulative(metric);

					/* TODO: Output a warning in the case that anything in p_options.OutputMetrics isn't understood */
					if (SHOULD_INCLUDE_METRIC(p_options, MetricUnit::getMetricShortName(metric)))
					{
						std::string metricName;
						
						if (useShortNames)
						{
							metricName = MetricUnit::getMetricShortName(metric);
						}
						else
						{
							metricName = MetricUnit::getMetricName(metric);
						}

						out << metricName;

						if (localAndCumulativeOutputs)
						{
							out << "(local)" << sep << metricName << "(cumulative)";
						}
						out << sep;
					}
				}
				out << std::endl;
			}
		}

		if (IS_OUTPUT_TREE_FORM(p_fmt)) {
			out << m_namePrefix[metricUnitType];
		}

		out << p_topLevel->getUnitName() << sep;

		unsigned loop;
		for (loop = 0;
			loop < METRIC_TYPE_MAX;
			loop++)
		{
			const MetricType_e metric = static_cast<MetricType_e>(loop);
			const bool metricIsCumulative = MetricUnit::isMetricCumulative(metric);
			/* TODO: duped above */
			const bool localAndCumulativeOutputs = metricIsCumulative && MetricUnit::isMetricLocalAndCumulative(metric);

			/* Filter out metrics which only apply at file/method level */
			if (SHOULD_INCLUDE_METRIC(p_options, MetricUnit::getMetricShortName(metric)) && MetricUnit::doesMetricApplyForUnit(metric, metricUnitType))
			{
				std::string sfx = "";
				bool output_sep = false;
				if (localAndCumulativeOutputs)
				{
					if (metricUnitType == METRIC_UNIT_FILE)
					{
						dumpMetric(out, p_topLevel, (MetricType_e)loop, p_fmt, sep, false, "(local)",useShortNames);
						sfx = "(cumulative)";
					}
					else if (!IS_OUTPUT_TREE_FORM(p_fmt))
					{
						output_sep = true;
					}
				}
				dumpMetric(out, p_topLevel, (MetricType_e)loop, p_fmt, sep, metricIsCumulative, sfx,useShortNames);
				if (output_sep)
				{
					out << sep;
				}
			}
		}

		if ((p_fmt == METRIC_DUMP_FORMAT_TSV) ||
			(p_fmt == METRIC_DUMP_FORMAT_CSV)) {
			out << std::endl;
		}
	}

	const MetricUnit::SubUnitMap_t* const subUnits = p_topLevel->getSubUnits();

	for (MetricUnit::SubUnitMap_t::const_iterator unitIt = subUnits->begin();
		 unitIt != subUnits->end();
		 ++unitIt)
	{
		dump(out, (*unitIt).second, p_output, p_fmt, p_options);
	}
}

