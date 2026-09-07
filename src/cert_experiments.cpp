#include "defs.h"
#include "frechet_light.h"
#include "query.h"
#include "times.h"
#include "parser.h"
#include "range.h"

#include <array>
#include <chrono>
#include <string>
#include <vector>

using hrc = std::chrono::high_resolution_clock;
using time_point = hrc::time_point;
using ns = std::chrono::nanoseconds;
using TimesRow = std::vector<double>;
using TimesRows = std::vector<TimesRow>;

void comparisonExp();
void partsMeasurementExp();
void datasetStatsExp();
void boxesVsRuntimeExp();
void certificatesRuntime();
void certificatesYesNoRuntime();
void omitRulesExp();
void deciderComparisonExp();
void deciderCountFiltered();

namespace
{

using Strings = std::vector<std::string>;
Strings const curve_directories = {"../../benchmark/sigspatial/",
	"../../benchmark/characters/data/", "../../benchmark/Geolife Trajectories 1.3/data/"};
Strings const curve_data_files = {"../../benchmark/sigspatial/dataset.txt",
	"../../benchmark/characters/data/dataset.txt",
	"../../benchmark/Geolife Trajectories 1.3/data/dataset.txt"};

} // end anonymous namespace

int main()
{
	bool certificates_runtime = true;
	bool certificates_yes_no_runtime = true;

	if (certificates_runtime) { certificatesRuntime(); }
	if (certificates_yes_no_runtime) { certificatesYesNoRuntime(); }
}

void printRow(std::vector<double> const& row, int precision = 3)
{
	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(precision);
	for (std::size_t i = 0; i < row.size()-1; ++i) {
		std::cout << row[i] << " & ";
	}
	std::cout << row.back() << " \\\\\n";
}

template <typename A, typename B>
void printPairs(std::vector<std::pair<A,B>> const& data)
{
	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(3);
	for (auto const& data_point: data) {
		std::cout << data_point.first << " " << data_point.second << "\n";
	}
}

struct DeciderQuery
{
	Curve curve1;
	Curve curve2;
	distance_t distance;
};
using DeciderQueries = std::vector<DeciderQuery>;

DeciderQueries loadQueries(std::string const& filename, std::string const& curve_directory,
	std::size_t max_queries = std::numeric_limits<std::size_t>::max())
{
	DeciderQueries decider_queries;
	std::ifstream f(filename);

	if (!f.is_open()) {
		std::cerr << "ERROR: Could not open query file: " << filename << "\n";
		std::exit(1);
	}

	std::string curve1_file;
	std::string curve2_file;
	distance_t distance;

	std::size_t query_count = 0;
	while (query_count < max_queries && f >> curve1_file >> curve2_file >> distance) {
		decider_queries.emplace_back();
		decider_queries.back().curve1 = parser::readCurve(curve_directory + curve1_file);
		decider_queries.back().curve2 = parser::readCurve(curve_directory + curve2_file);
		decider_queries.back().distance = distance;
		++query_count;
	}

	return decider_queries;
}

void certificatesRuntime()
{
	std::cout << "Starting certificates runtime experiment." << std::endl;

	std::size_t num_data_sets = 3;
	Strings query_file_prefixes = {"../test_data/benchmark_queries/sigspatial_query",
		"../test_data/benchmark_queries/characters_query",
		"../test_data/benchmark_queries/geolife_query"};

	std::vector<int> ks = {0, 1, 10, 100, 1000};

	std::vector<double> cert_overall_times;
	std::vector<double> cert_certifying_times;
	std::vector<double> yes_times;
	std::vector<double> no_times;
	std::vector<double> cert_creation_times;
	std::vector<double> cert_check_sum_times;
	std::vector<double> cert_check_filter_times;
	std::vector<double> cert_check_complete_times;

	for (std::size_t d = 0; d < num_data_sets; ++d) {
		Query query(curve_directories[d]);
		query.readCurveData(curve_data_files[d]);
		query.setAlgorithm("light");
		query.getReady();

		for (std::size_t i = 0; i < ks.size(); ++i) {
			auto k = ks[i];

			query.readQueryCurves(query_file_prefixes[d] + std::to_string(k) + ".txt");

			auto start = hrc::now();
			query.run();
			auto cert_overall_time = std::chrono::duration_cast<ns>(hrc::now()-start).count();

			auto cert_creation_time = (double)global::times.certcomp_sum;
			auto cert_certifying_time = (double)global::times.certifyingcomp_sum;
			auto cert_yes_times = (double)global::times.certcompyes_sum;
			auto cert_no_times = (double)global::times.certcompno_sum;

			auto cert_check_filter_time = (double)global::times.certcheck_sum[Times::FILTER];
			auto cert_check_complete_time = (double)global::times.certcheck_sum[Times::COMPLETE];
			cert_overall_times.push_back(cert_overall_time/1000000.);
			cert_certifying_times.push_back(cert_certifying_time/1000000.);
			cert_creation_times.push_back(cert_creation_time/1000000.);
			yes_times.push_back(cert_yes_times/1000000.);
			no_times.push_back(cert_no_times/1000000.);
			cert_check_sum_times.push_back((cert_check_filter_time+cert_check_complete_time)/1000000.);
			cert_check_filter_times.push_back(cert_check_filter_time/1000000.);
			cert_check_complete_times.push_back(cert_check_complete_time/1000000.);
			global::times.reset();
		}
	}

	printRow(cert_overall_times, 1);
	printRow(cert_certifying_times, 1);
	printRow(cert_creation_times, 1);
	printRow(yes_times, 1);
	printRow(no_times, 1);
	printRow(cert_check_sum_times, 1);
	printRow(cert_check_filter_times, 1);
	printRow(cert_check_complete_times, 1);
}

void certificatesYesNoRuntime()
{
	std::cout << "Starting certificates runtime yes/no experiment." << std::endl;

	std::size_t num_data_sets = 3;
	Strings query_file_prefixes = {"../test_data/benchmark_queries/sigspatial_query",
		"../test_data/benchmark_queries/characters_query",
		"../test_data/benchmark_queries/geolife_query"};

	std::vector<int> ks = {0, 1, 10, 100, 1000};

	std::vector<double> yes_times;
	std::vector<double> unfiltered_yes_curves;
	std::vector<double> no_times;
	std::vector<double> unfiltered_no_curves;

	for (std::size_t d = 0; d < num_data_sets; ++d) {
		Query query(curve_directories[d]);
		query.readCurveData(curve_data_files[d]);
		query.setAlgorithm("light");
		query.getReady();

		for (std::size_t i = 0; i < ks.size(); ++i) {
			auto k = ks[i];

			query.readQueryCurves(query_file_prefixes[d] + std::to_string(k) + ".txt");
			query.run();

			auto time_per_yes = (double)global::times.certcompyes_sum;
			auto time_per_no = (double)global::times.certcompno_sum;
			yes_times.push_back(time_per_yes/1000000.);
			no_times.push_back(time_per_no/1000000.);

			unfiltered_yes_curves.push_back(global::times.numYesChecked[Times::COMPLETE]);
			unfiltered_no_curves.push_back(global::times.numNoChecked[Times::COMPLETE]);
			global::times.reset();
		}
	}

	printRow(yes_times, 1);
	printRow(unfiltered_yes_curves, 4);
	printRow(no_times, 1);
	printRow(unfiltered_no_curves, 4);
}
