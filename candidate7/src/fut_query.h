#pragma once

#include "frechet_under_translation.h"
#include "geometry_basics.h"
#include "query_helper.h"
#include "times.h"
#include "curves.h"

#include <string>

namespace unit_tests { void testFrechetUnderTranslationQuery(); }

class FutQuery
{
public:
	FutQuery(std::string const& curve_directory);

	void readCurveData(std::string const& curve_data_file);
	void readQueryCurves(std::string const& query_curves_file);
	void getReady();

	void run();
	void run(Curve const& curve, distance_t distance);

	FutResults const& getResults() const;
	void saveResults(std::string const& results_file) const;

	void printQueryInformation(std::size_t query_index) const;
	Curve const& getCurve(std::size_t curve_index) const;
	Curves const& getCurves() const;

	// get an upper bound on the fréchet distance of all curves in the data set
	distance_t getUpperBoundDistance() const;

private:
	bool is_ready = false;

	std::string const curve_directory;

	FrechetUnderTranslation frechet;
	QueryElements query_elements;
	Curves curve_data;
	CurveIDs candidates;
	FutResults results;

	FutTree kd_tree;

	void run_impl(Curve const& curve, distance_t distance);

	friend void unit_tests::testFrechetUnderTranslationQuery();
};
