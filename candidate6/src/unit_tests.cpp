// Make sure to always print in the unit tests
#ifdef NVERBOSE
#	undef NVERBOSE
#endif
#include "unit_tests.h"

#include <cmath>
#include <random>
#include <unordered_set>

#include "defs.h"
#include "discrete_frechet_light.h"
#include "discrete_frechet_naive.h"
#include "frechet_under_translation.h"
#include "frechet_light.h"
#include "fut_n6_algorithm.h"
#include "fut_query.h"
#include "parser.h"
#include "priority_search_tree.h"
#include "range_tree.h"

#ifdef CERTIFY
#include "freespace_light_vis.h"
#endif

//
// Define some helpers
//

#define TEST_INFO(x, y)                                                        \
	do {                                                                       \
		if (!(x)) {                                                            \
			std::cout << "\n";                                                 \
			std::cout << "TEST_FAILED!\n";                                     \
			std::cout << "File: " << __FILE__ << "\n";                         \
			std::cout << "Line: " << __LINE__ << "\n";                         \
			std::cout << "Function: " << __func__ << "\n";                     \
			std::cout << "Test: " << #x << "\n";                               \
			std::cout << "Info: " << y << "\n";                                \
			std::cout << "\n";                                                 \
			std::cout << std::flush;                                           \
			std::abort();                                                      \
		}                                                                      \
	} while (0)

#define TEST(x) TEST_INFO(x, "")

namespace
{

Curve getCurve1() {
	Curve curve;
	curve.push_back({0., 0.});
	curve.push_back({2., 0.});

	return curve;
}

Curve getCurve2() {
	Curve curve;
	curve.push_back({0., 1.});
	curve.push_back({1., 1.5});
	curve.push_back({2., 1.});

	return curve;
}

Curve getCurve3() {
	Curve curve;
	curve.push_back({0., 0.});
	curve.push_back({1., 0.});
	curve.push_back({2., 0.});
	curve.push_back({3., 1.});
	curve.push_back({2., 2.});
	curve.push_back({1., 2});
	curve.push_back({1., 1.9});
	curve.push_back({1., 1.8});
	curve.push_back({1., 1.7});

	return curve;
}

bool roughlyEqual(distance_t a, distance_t b, distance_t eps = 1e-6)
{
	return std::abs(a-b) < eps;
}

} // end anonymous

//
// Unit Tests
//

void unit_tests::testAll()
{
	unit_tests::testFrechetUnderTranslation();
	// unit_tests::testN6Algorithm();
	// unit_tests::testFrechetUnderTranslationQuery();
	// unit_tests::testDiscreteFrechetNaive();
	// unit_tests::testDiscreteFrechetLight();
//     unit_tests::testPrioritySearchTree();
//     unit_tests::testGeometricBasics();
// #ifdef CERTIFY
//     unit_tests::testFreespaceLightVis();
// #endif
//     unit_tests::testLightCertificate();
//     unit_tests::testRangeTree();
}

void unit_tests::testDiscreteFrechetNaive()
{
	PRINT("Start discrete Frechet Naive test.");

	DiscreteFrechetNaive frechet;

	Curve curve1({Point{0.0, 0.0}, Point{1.0, 0.0}, Point{2.0, 0.0}});
	Curve curve2({Point{0.0, 1.0}, Point{2.0, 1.0}});

	auto distance = frechet.calcDistance(curve1, curve2);
	TEST(roughlyEqual(distance, sqrt(2)));

	PRINT("Finished discrete Frechet Naive test.");
}

void unit_tests::testDiscreteFrechetLight()
{
	PRINT("Start discrete Frechet Light test.");

	{
		DiscreteFrechetLight frechet;

		Curve curve1({Point{0.0, 0.0}, Point{1.0, 0.0}, Point{2.0, 0.0}});
		Curve curve2({Point{0.0, 1.0}, Point{2.0, 1.0}});

		auto distance = frechet.calcDistance(curve1, curve2);
		TEST(roughlyEqual(distance, sqrt(2)));
	}

	{
		DiscreteFrechetNaive frechet_naive;
		DiscreteFrechetLight frechet_light;

		std::string const path = "../../benchmark/characters/data/";

		auto curve1 = parser::readCurve(path + "1.txt");
		for (std::size_t i = 1; i < 2858; ++i) {
			std::string const curve2_filename = std::to_string(i) + ".txt";
			auto curve2 = parser::readCurve(path + curve2_filename);

			auto distance_naive = frechet_naive.calcDistance(curve1, curve2);
			auto distance_light = frechet_light.calcDistance(curve1, curve2);

			TEST_INFO(roughlyEqual(distance_naive, distance_light),
			          "Filename of offending curve is " + curve2_filename);
		}
	}

	PRINT("Finished discrete Frechet Light test.");
}

void unit_tests::testGeometricBasics()
{
	// Test Points
	Point p1{0., 0.};
	Point p2{2., 0.};
	Point p3{3., 4.};

	TEST(p1.dist(p2) == 2);
	TEST(p1.dist(p3) == 5);

	// Test Curves
	auto curve1 = getCurve1();
	auto curve2 = getCurve2();

	TEST(curve1.size() == 2 && curve2.size() == 3);
	TEST(curve1.curve_length(0, 1) == 2);
}

#ifdef CERTIFY
void unit_tests::testFreespaceLightVis()
{
	auto curve2 = getCurve2();
	auto curve3 = getCurve3();
	distance_t distance = 1.5;

	FrechetLight frechet;
	frechet.lessThan(distance, curve2, curve3);

	FreespaceLightVis vis(frechet);
	vis.exportToSvg("freespace_light_vis.svg");
}
#endif

void unit_tests::testLightCertificate() {
	//unit_tests::testLightCertificate("../../testdaten/simple-curve1.txt", "../../testdaten/simple-curve2.txt", 5);
	//unit_tests::testLightCertificate("../../testdaten/simple-curve1.txt", "../../testdaten/simple-curve2.txt", 2);

	// unit_tests::testLightCertificate("../../testdaten/simple-curve3.txt", "../../testdaten/simple-curve4.txt", 0.75);

	unit_tests::testLightCertificate("../../testdaten/file-012086.dat", "../../testdaten/file-019953.dat", 1272.84);
}


void unit_tests::testLightCertificate(std::string curve1file, std::string curve2file, distance_t distance) {
	auto curve1 = parser::readCurve(curve1file);
	auto curve2 = parser::readCurve(curve2file);

	FrechetLight frechet;
	bool output = frechet.lessThan(distance, curve1, curve2);
	Certificate& c = frechet.computeCertificate();
	TEST(c.isValid()); 
	TEST(c.isYes() == output);
	TEST(c.check());

	c.dump_certificate();

}

void unit_tests::testRangeTree()
{
	using Tree = RangeTree<double, int>;

	bool manual_test = false;
	bool randomized_test = true;

	auto matches_naive = [](Tree::Point query, Tree::Points const& points,
	                        std::vector<bool> const& deleted, Tree::Values& result) {
		Tree::Values naive_result;

		for (std::size_t i = 0; i < points.size(); ++i) {
			auto const& point = points[i];
			if (point.x >= query.x && point.y <= query.y && !deleted[i]) {
				naive_result.push_back(i);
			}
		}

		std::sort(result.begin(), result.end());
		std::sort(naive_result.begin(), naive_result.end());

		return result.size() == naive_result.size() &&
			std::equal(result.begin(), result.end(), naive_result.begin());
	};

	Tree range_tree;

	if (manual_test) {
		range_tree.add({0., 0.}, 0);
		range_tree.add({1., 0.}, 1);
		range_tree.add({2., 0.}, 2);
		range_tree.add({0., 1.}, 3);
		range_tree.add({1., 2.}, 4);
		range_tree.add({2., 3.}, 5);

		range_tree.build();
		std::cout << range_tree << "\n";

		std::vector<int> result;
		range_tree.searchAndDelete({.5, 2.5}, result);

		// should be: 1, 2, 4
		for (auto value: result) { std::cout << value << ", "; } std::cout << "\n";
	}

	if (randomized_test) {
		int number_of_points = 10000;
		int number_of_queries = 100;
		int number_of_runs = 1000;
	
		for (int run = 0; run < number_of_runs; ++run) {
			range_tree.clear();

			Tree::Points points;
		
			std::random_device r;
			std::default_random_engine e(r());
			std::uniform_real_distribution<double> rand(-100., 100.);
		
			for (int i = 0; i < number_of_points; ++i) {
				Tree::Point random_point = {rand(e), rand(e)};
				points.push_back(random_point);
		
				range_tree.add(random_point, i);
			}
		
			range_tree.build();
		
			std::vector<int> result;
			std::vector<bool> deleted(number_of_points, false);
			for (int i = 0; i < number_of_queries; ++i) {
				Tree::Point random_query = {rand(e), rand(e)};
		
				result.clear();
				range_tree.searchAndDelete(random_query, result);
		
				TEST(matches_naive(random_query, points, deleted, result));
				for (auto id: result) { deleted[id] = true; }
			}
		}
	}
}

void unit_tests::testPrioritySearchTree()
{
	using Tree = PrioritySearchTree<double, int>;

	bool manual_test = false;
	bool randomized_test = true;

	auto matches_naive = [](Tree::Point query, Tree::Points const& points,
	                        std::vector<bool> const& deleted, Tree::Values& result) {
		Tree::Values naive_result;

		for (std::size_t i = 0; i < points.size(); ++i) {
			auto const& point = points[i];
			if (point.x >= query.x && point.y <= query.y && !deleted[i]) {
				naive_result.push_back(i);
			}
		}

		std::sort(result.begin(), result.end());
		std::sort(naive_result.begin(), naive_result.end());

		return result.size() == naive_result.size() &&
			std::equal(result.begin(), result.end(), naive_result.begin());
	};

	Tree pst;

	if (manual_test) {
		pst.add({0., 0.}, 0);
		pst.add({1., 0.}, 1);
		pst.add({2., 0.}, 2);
		pst.add({0., 1.}, 3);
		pst.add({1., 2.}, 4);
		pst.add({2., 3.}, 5);

		pst.build();
		std::cout << pst << "\n";

		std::vector<int> result;
		pst.searchAndDelete({.5, 2.5}, result);

		// should be: 1, 2, 4
		for (auto value: result) { std::cout << value << ", "; } std::cout << "\n";
	}

	if (randomized_test) {
		int number_of_points = 10000;
		int number_of_queries = 100;
		int number_of_runs = 1000;
	
		for (int run = 0; run < number_of_runs; ++run) {
			pst.clear();

			Tree::Points points;
		
			std::random_device r;
			std::default_random_engine e(r());
			std::uniform_real_distribution<double> rand(-100., 100.);
		
			for (int i = 0; i < number_of_points; ++i) {
				Tree::Point random_point = {rand(e), rand(e)};
				points.push_back(random_point);
		
				pst.add(random_point, i);
			}
		
			pst.build();
		
			std::vector<int> result;
			std::vector<bool> deleted(number_of_points, false);
			for (int i = 0; i < number_of_queries; ++i) {
				Tree::Point random_query = {rand(e), rand(e)};
		
				result.clear();
				pst.searchAndDelete(random_query, result);
		
				TEST(matches_naive(random_query, points, deleted, result));
				for (auto id: result) { deleted[id] = true; }
			}
		}
	}
}

void unit_tests::testFrechetUnderTranslation()
{
	PRINT("Start Fréchet under translation test.");

	// small test
	{
		Curve curve1({Point{0.0, 0.0}, Point{1.0, 0.0}});
		Curve curve2({Point{0.0, -1.0}, Point{0.0, 1.0}});
		Curve curve3({Point{5.0, -13.0}, Point{2.0, -12.0}, Point{5.0, -11.0}});

		FrechetUnderTranslation frechet;

		N6Alg n6_alg;
		auto dist1 = n6_alg.calcDistance(curve1, curve2);
		auto dist2 = frechet.calcDistance(curve1, curve2);
		TEST_INFO(roughlyEqual(dist1, sqrt(5)/2.), "n^6 algorithm seems broken");
		TEST_INFO(roughlyEqual(dist2, sqrt(5)/2.), "fut algorithm seems broken");
		dist1 = n6_alg.calcDistance(curve1, curve3);
		dist2 = frechet.calcDistance(curve1, curve3);
		TEST_INFO(roughlyEqual(dist1, sqrt(10)/2.),
			"n^6 algorithm seems broken: " << dist1 << " should be " << sqrt(10)/2.);
		TEST_INFO(roughlyEqual(dist2, sqrt(10)/2.),
			"fut algorithm seems broken: " << dist2 << " should be " << sqrt(10)/2.);
		dist1 = n6_alg.calcDistance(curve2, curve3);
		dist2 = frechet.calcDistance(curve2, curve3);
		TEST_INFO(roughlyEqual(dist1, dist2), "small test 3 failed: " << dist1 << " " << dist2);
	}

	auto const to_sigspatial_filename = [](std::size_t i) {
		std::string const i_str = std::to_string(i);
		std::string const num = std::string(6 - i_str.length(), '0') + i_str;
		return "../../benchmark/sigspatial/file-" + num + ".dat";
	};
	auto const to_characters_filename = [](std::size_t i) {
		std::string const i_str = std::to_string(i+1); // Note, +1 as indices start at 1
		return "../../benchmark/characters/data/" + i_str + ".txt";
	};
	// might be unused
	(void) to_sigspatial_filename;
	(void) to_characters_filename;

	Curves curves;
	for (std::size_t i = 0; i < 100; ++i) {
		auto const filename = to_sigspatial_filename(i);
		// auto const filename = to_characters_filename(i);

		curves.push_back(parser::readCurve(filename));
	}

	for (std::size_t i = 0; i < curves.size(); ++i) {
		for (std::size_t j = i; j < curves.size(); ++j) {
			PRINT(i << " " << j);
			auto const& curve1 = curves[i];
			auto const& curve2 = curves[j];

			FrechetUnderTranslation fut;
			auto const eps = fut.getEpsilon();

			bool enable_d = true;
			bool enable_d2 = true;
			assert(enable_d || enable_d2);

			distance_t d, d2;
			Point d_translation, d2_translation;
			if (enable_d) {
				MEASUREMENT::start();
				d = fut.calcDistance(curve1, curve2);
				d_translation = fut.getTranslation();
				// DEBUG(d_translation);
				MEASUREMENT::stopAndPrint("calcDistance");
			}
			if (enable_d2) {
				MEASUREMENT::start();
				d2 = fut.calcDistance2(curve1, curve2);
				d2_translation = fut.getTranslation();
				// DEBUG(d2_translation);
				MEASUREMENT::stopAndPrint("calcDistance2");
			}
			// PRINT("");

			if (enable_d && enable_d2) {
				TEST_INFO(roughlyEqual(d, d2, eps), std::setprecision(20) << d << " " << d2);
			}

			DiscreteFrechetLight frechet(eps/10.);

			// check translation for d
			if (enable_d) {
				curve2.translate(d_translation);
				// PRINT(std::setprecision(20) <<  d << " " << frechet.calcDistance(curve1, curve2));
				TEST(frechet.lessThanWithFilters(d+eps, curve1, curve2));
				TEST(!frechet.lessThanWithFilters(std::max(0., d-eps), curve1, curve2) || roughlyEqual(d, 0., eps));
			}
			// check translation for d2
			if (enable_d2) {
				curve2.translate(d2_translation);
				// PRINT(std::setprecision(20) <<  d2 << " " << frechet.calcDistance(curve1, curve2));
				TEST(frechet.lessThanWithFilters(d2+eps, curve1, curve2));
				TEST(!frechet.lessThanWithFilters(std::max(0., d2-eps), curve1, curve2) || roughlyEqual(d2, 0., eps));
			}

			auto dist = enable_d ? d : d2;
			curve2.resetTranslation();

			// check negative
			distance_t const grid_size = 1000.;
			auto center1 = curve1.front() - curve2.front();
			auto center2 = curve1.back() - curve2.back();
			auto left = std::max(center1.x, center2.x) - dist;
			auto right = std::min(center1.x, center2.x) + dist;
			auto bottom = std::max(center1.y, center2.y) - dist;
			auto top = std::min(center1.y, center2.y) + dist;
			auto x_0 = left;
			auto x_step = (right-left)/grid_size;
			auto y_0 = bottom;
			auto y_step = (top-bottom)/grid_size;

			for (int i = 0; i <= grid_size; ++i) {
				for (int j = 0; j <= grid_size; ++j) {
					curve2.translate({x_0 + i*x_step, y_0 + j*y_step});
					TEST_INFO(!frechet.lessThanWithFilters(std::max(0.,dist-eps), curve1, curve2) || roughlyEqual(dist, 0., eps), frechet.calcDistance(curve1, curve2));
					curve2.resetTranslation();
				}
			}

			PRINT("");
		}
	}

	PRINT("Finished Fréchet under translation test.");
}

void unit_tests::testFrechetUnderTranslationQuery()
{
	std::cout << "Start Fréchet distance under translation test." << std::endl;

	// do small query and check result a bit more naively
	std::string curve_directory = "../../benchmark/sigspatial/";
	std::string curve_data_file = "../../benchmark/sigspatial/dataset.txt";
	std::string query_curves_file = "../test_data/queries_small.txt";
	
	FutQuery query(curve_directory);
	query.readCurveData(curve_data_file);
	query.readQueryCurves(query_curves_file);
	query.getReady();
	query.run();

	TEST(query.results.size() == query.query_elements.size());
	for (std::size_t result_index = 0; result_index < query.results.size(); ++result_index) {
		FrechetUnderTranslation frechet;
		auto epsilon = frechet.getEpsilon();

		auto const& query_element = query.query_elements[result_index];
		auto const& curve1 = query_element.curve;
		auto& result = query.results[result_index];
		auto& curve_ids = result.curve_ids;
		auto& translations = result.translations;

		// check positive
		for (std::size_t j = 0; j < curve_ids.size(); ++j) {
			auto const& curve2 = query.curve_data[curve_ids[j]];
			curve2.translate(translations[j]);
			TEST(frechet.frechet.lessThanFixedTranslation(query_element.distance+20*epsilon, curve1, curve2));
			curve2.resetTranslation();
		}

		// check negative
		std::unordered_set<std::size_t> close_ids(curve_ids.begin(), curve_ids.end());
		for (CurveID id = 0; id < query.curve_data.size(); ++id) {
			if (close_ids.count(id)) { continue; }

			auto const& curve2 = query.curve_data[id];
			distance_t d = query_element.distance;

			distance_t grid_size = 200.;
			auto center1 = curve1.front() - curve2.front();
			auto center2 = curve1.back() - curve2.back();
			auto left = std::max(center1.x, center2.x) - d;
			auto right = std::min(center1.x, center2.x) + d;
			auto bottom = std::max(center1.y, center2.y) - d;
			auto top = std::min(center1.y, center2.y) + d;
			auto x_0 = left;
			auto x_step = (right-left)/grid_size;
			auto y_0 = bottom;
			auto y_step = (top-bottom)/grid_size;

			for (int i = 0; i <= grid_size; ++i) {
				for (int j = 0; j <= grid_size; ++j) {
					curve2.translate({x_0 + i*x_step, y_0 + j*y_step});
					TEST(!frechet.frechet.lessThanFixedTranslation(d-10*epsilon, curve1, curve2));
				}
			}
			curve2.resetTranslation();
		}
	}

	std::cout << "All tests of Fréchet distance under translation succeeded." << std::endl;
}

void unit_tests::testN6Algorithm()
{
	PRINT("Start n6_alg test.");

	auto const to_sigspatial_filename = [](std::size_t i) {
		std::string const i_str = std::to_string(i);
		std::string const num = std::string(6 - i_str.length(), '0') + i_str;
		return "../../benchmark/sigspatial/file-" + num + ".dat";
	};

	// larger test
	{
		std::size_t const number_of_curves = 50;
		std::size_t const subsample_size = 10;

		for (std::size_t i = 0; i < number_of_curves; ++i) {
			auto curve1_long = parser::readCurve(to_sigspatial_filename(i));
			Curve curve1_short;
			for (std::size_t k = 0; k < std::min<std::size_t>(curve1_long.size(), subsample_size); ++k) {
				curve1_short.push_back(curve1_long[k]);
			}

			for (std::size_t j = 0; j < number_of_curves; ++j) {
				PRINT(i << " " << j);

				auto curve2_long = parser::readCurve(to_sigspatial_filename(j));
				Curve curve2_short;
				for (std::size_t j = 0; j < std::min<std::size_t>(curve2_long.size(), subsample_size); ++j) {
					curve2_short.push_back(curve2_long[j]);
				}
		
				N6Alg n6_alg;
				auto const d = n6_alg.calcDistance(curve1_short, curve2_short);
				auto const eps = n6_alg.epsilon;

				DiscreteFrechetLight frechet(eps/10.);

				// check positive
				curve2_short.translate(n6_alg.getTranslation());
				TEST(frechet.lessThanWithFilters(d+eps, curve1_short, curve2_short));
				TEST(!frechet.lessThanWithFilters(std::max(0.,d-eps), curve1_short, curve2_short) || roughlyEqual(d, 0.));
				curve2_short.resetTranslation();

				// check negative
				distance_t grid_size = 200.;
				auto center1 = curve1_short.front() - curve2_short.front();
				auto center2 = curve1_short.back() - curve2_short.back();
				auto left = std::max(center1.x, center2.x) - d;
				auto right = std::min(center1.x, center2.x) + d;
				auto bottom = std::max(center1.y, center2.y) - d;
				auto top = std::min(center1.y, center2.y) + d;
				auto x_0 = left;
				auto x_step = (right-left)/grid_size;
				auto y_0 = bottom;
				auto y_step = (top-bottom)/grid_size;

				for (int k = 0; k <= grid_size; ++k) {
					for (int j = 0; j <= grid_size; ++j) {
						curve2_short.translate({x_0 + k*x_step, y_0 + j*y_step});
						TEST(!frechet.lessThanWithFilters(std::max(0.,d-eps), curve1_short, curve2_short) || roughlyEqual(d, 0.));
						curve2_short.resetTranslation();
					}
				}
			}
		}
	}

	PRINT("Finished n6_alg test.");
}

// just in case anyone does anything stupid with this file...
#undef TEST_INFO
#undef TEST
