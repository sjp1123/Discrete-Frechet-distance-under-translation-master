#include "fut_query.h"

#include "defs.h"
#include "filter.h"
#include "frechet_under_translation.h"
#include "parser.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

namespace
{

inline static bool isNear(FutTree::Point const& a, FutTree::Point const& b, distance_t distance)
{
	auto d = (a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]);
	if (d > distance*distance) { return false; }

	for (std::size_t i = 2; i < 4; ++i) {
		auto d = std::abs(a[i] - b[i]);
		if (d > distance) { return false; }
	}

	return true;
}

} // end anonymous namespace

FutQuery::FutQuery(std::string const& curve_directory)
	: curve_directory(curve_directory)
	, kd_tree(isNear)
{
}

void FutQuery::readCurveData(std::string const& curve_data_file)
{
	is_ready = false;

	// read filenames of curve files
	std::ifstream file(curve_data_file);
	std::vector<std::string> curve_filenames;
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			curve_filenames.push_back(line);
		}
	}
	else {
		ERROR("The curve data file could not be opened: " << curve_data_file);
	}

	// read curves
	curve_data.clear();
	curve_data.reserve(curve_filenames.size());

	for (auto const& curve_filename: curve_filenames) {
		std::ifstream curve_file(curve_directory + curve_filename);
		if (curve_file.is_open()) {
			curve_data.emplace_back();
			parser::readCurve(curve_file, curve_data.back());
			curve_data.back().filename = curve_filename;

			if (curve_data.back().empty()) { curve_data.pop_back(); }
		}
		else {
			ERROR("A curve file could not be opened: " << curve_directory + curve_filename);
		}
	}
}

void FutQuery::readQueryCurves(std::string const& query_curves_file)
{
	query_elements.clear();

	// read filenames of curve files and distances
	std::ifstream file(query_curves_file);
	std::vector<std::string> curve_filenames;
	if (file.is_open()) {
		std::stringstream ss;
		ss << file.rdbuf();

		std::string distance_string;
		std::string curve_filename;
		while (ss >> curve_filename >> distance_string) {
			curve_filenames.push_back(curve_filename);
			query_elements.emplace_back(Curve(), std::stod(distance_string));
		}
	}
	else {
		ERROR("The curve data file could not be opened: " << query_curves_file);
	}

	// read curves
	for (std::size_t i = 0; i < curve_filenames.size(); ++i) {
		std::ifstream curve_file(curve_directory + curve_filenames[i]);
		if (curve_file.is_open()) {
			parser::readCurve(curve_file, query_elements[i].curve);
	
			query_elements[i].curve.filename = curve_filenames[i];
		}
		else {
			ERROR("A curve file could not be opened: " << curve_directory + curve_filenames[i]);
		}
	}
}

void FutQuery::getReady()
{
	results.clear();

	// build all the data structures and make queries ready
	kd_tree.clear();
	for (CurveID id = 0; id < curve_data.size(); ++id) {
		auto const& curve = curve_data[id];
		kd_tree.add(toFutKdPoint(curve), id);
	}
	kd_tree.build();

	is_ready = true;
}

void FutQuery::run()
{
	assert(is_ready);
	results.clear();

	for (auto const& query_element: query_elements) {
		run_impl(query_element.curve, query_element.distance);
	}
}

void FutQuery::run(Curve const& curve, distance_t distance)
{
	assert(is_ready);
	results.clear();

	run_impl(curve, distance);
}

void FutQuery::run_impl(Curve const& curve, distance_t distance)
{
	assert(is_ready);

	// add new result for this query
	results.emplace_back();
	auto& result = results.back();

	// perform query
	candidates.clear();
	kd_tree.search(toFutKdPoint(curve), 2*distance, candidates);
	
	for (auto candidate: candidates) {
		if (frechet.lessThan(distance, curve, curve_data[candidate])) {
			result.add(candidate, frechet.getTranslation());
		}
	}

	// std::cout << result.curve_ids.size() << " " << candidates.size() << std::endl;
}

FutResults const& FutQuery::getResults() const
{
	return results;
}

void FutQuery::saveResults(std::string const& results_file) const
{
	std::ofstream file(results_file);
	if (file.is_open()) {
		for (auto const& result: results) {
			for (auto curve_id: result.curve_ids) {
				file << curve_data[curve_id].filename << " ";
			}
			file << "\n";
		}
	}
}

void FutQuery::printQueryInformation(std::size_t query_index) const
{
	auto const& query_element = query_elements[query_index];

	std::cout << "Query curve: " << query_element.curve.filename << "\n";
	std::cout << "Query distance: " << query_element.distance << "\n";
}

Curve const& FutQuery::getCurve(std::size_t curve_index) const
{
	return curve_data[curve_index];
}

Curves const& FutQuery::getCurves() const
{
	return curve_data;
}

distance_t FutQuery::getUpperBoundDistance() const
{
	if (curve_data.size() <= 1) { return 0.; }

	auto extreme = curve_data.front().getExtremePoints();
	Point min_point{extreme.min_x, extreme.min_y};
	Point max_point{extreme.max_x, extreme.max_y};

	for (auto const& curve: curve_data) {
		extreme = curve.getExtremePoints();
		min_point.x = std::min(min_point.x, extreme.min_x);
		min_point.y = std::min(min_point.y, extreme.min_y);
		max_point.x = std::max(max_point.x, extreme.max_x);
		max_point.y = std::max(max_point.y, extreme.max_y);
	}

	return min_point.dist(max_point);
}
