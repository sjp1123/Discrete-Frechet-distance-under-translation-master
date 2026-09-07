#include "defs.h"
#include "parser.h"
#include "range.h"
#include "frechet_under_translation.h"

#include <chrono>
#include <cmath>
#include <string>
#include <vector>
#include <random>

using hrc = std::chrono::high_resolution_clock;
using time_point = hrc::time_point;
using ns = std::chrono::nanoseconds;

struct MeanInterval {
	double mean;
	std::pair<double, double> interval;
};
template <typename T>
using DataRow = std::vector<T>;
template <typename T>
using DataRows = std::vector<DataRow<T> >;
using CounterEntry = MEASUREMENT::MeasurementTool::CounterEntry;
using TimeEntry = MEASUREMENT::MeasurementTool::TimeEntry;

struct TimeProfile {
	double preprocessing;
	double arrangement;
	double arrangement_construction;
	double arrangement_blackbox;
	double blackbox;
	double discselection;
};


void experiment1();
void deciderCheck();
void deciderExp(bool arrangement_estimation);
void valCompExp(bool valcompexp_usefullbenchmark, bool valcompexp_lmf, bool valcompexp_binsearch, bool valcompexp_lipschitz);
void writeArrangement();
void writeLipschitzHeatmapData();

namespace
{

using Strings = std::vector<std::string>;
Strings const curve_directories = {"../../benchmark/characters/data/",
	"../../benchmark/characters/data/",
	"../../benchmark/sigspatial/",
	"../../benchmark/Geolife Trajectories 1.3/data/"};
Strings const curve_data_files = {"../../benchmark/characters/data/dataset.txt",
	"../../benchmark/characters/data/dataset.txt",
	"../../benchmark/sigspatial/dataset.txt",
	"../../benchmark/Geolife Trajectories 1.3/data/dataset.txt"};

Strings names = {"same-characters", "all-characters", "sigspatial", "geolife"}; 

std::string output_dir = "../experiments/";

} // end anonymous namespace

int main()
{
	bool exp1 = true;
	bool checkexp = true;
	bool decexp = true;
	bool decexp_arrangement_estimation = true;
	bool valcompexp_small_benchmark = true; //use small benchmark (LMF, binary search, Lipschitz-only)
	bool valcompexp_large_benchmark = true; //use full benchmark (LMF und binary search)
	//bool valcompexp_lmf = true;
	//bool valcompexp_binsearch = true;
	//bool valcompexp_lipschitz = true;
	bool write_arrangement = false;
	bool lipschitz_heatmap = false;

	if (exp1) { experiment1(); }
	if (checkexp) { deciderCheck(); }
	if (decexp) { deciderExp(decexp_arrangement_estimation); }
	if (valcompexp_small_benchmark) {
		//TODO: make sure this is false, true, true, true! 
		bool fullbenchmark = false; bool lmf = true; bool binsearch = true; bool lipschitz = true;
	       	valCompExp(fullbenchmark, lmf, binsearch, lipschitz);
       	}
	if (valcompexp_large_benchmark) {
		//TODO: make sure this is true, true, true, false! 
		bool fullbenchmark = true; bool lmf = true; bool binsearch = true; bool lipschitz = false;
	       	valCompExp(fullbenchmark, lmf, binsearch, lipschitz);
       	}
	if (write_arrangement) { writeArrangement(); }
	if (lipschitz_heatmap) { writeLipschitzHeatmapData(); }
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

template <typename T>
std::pair<double,double> getPercentileInterval(std::vector<T> & vals, std::size_t percentage = 25) {
	assert(percentage > 0 and percentage < 50);
	std::size_t to_drop = (vals.size() * percentage) / 100;
	double lower, upper;
	auto lpt = vals.begin() + to_drop;
	std::nth_element(vals.begin(), lpt, vals.end()); 
	lower = (double) *lpt;
	auto upt = vals.begin() + vals.size() - 1 - to_drop;
	std::nth_element(vals.begin(), upt, vals.end()); 
	upper = (double) *upt;
	return std::make_pair(lower, upper);
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

DeciderQueries loadQueriesDistanceLess(std::string const& filename, std::string const& curve_directory,
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

	std::size_t query_count = 0;
	while (query_count < max_queries && f >> curve1_file >> curve2_file) {
		decider_queries.emplace_back();
		decider_queries.back().curve1 = parser::readCurve(curve_directory + curve1_file);
		decider_queries.back().curve2 = parser::readCurve(curve_directory + curve2_file);
		++query_count;
	}

	return decider_queries;
}

void dumpDeciderTable(DataRows<MeanInterval> const& minus, DataRows<MeanInterval> const& plus, std::string out_filename)
{
	assert(minus.size() == plus.size());


	std::ofstream f(out_filename);

	if (!f.is_open()) {
		std::cerr << "ERROR: Could not open query file: " << out_filename << "\n";
		std::exit(1);
	}


	for (int i = (int)minus.front().size()-1; i >= 0; --i) {
		for (std::size_t col_index = 0; col_index < minus.size(); ++col_index) {
			f << minus[col_index][i].mean << " " << 
			     minus[col_index][i].interval.first << " " <<
			     minus[col_index][i].interval.second << " ";
		}
		f << std::endl;
	}

	for (int i = 0; i < (int)plus.front().size(); ++i) {
		for (std::size_t col_index = 0; col_index < minus.size(); ++col_index) {
			f << plus[col_index][i].mean << " " << 
			     plus[col_index][i].interval.first << " " <<
			     plus[col_index][i].interval.second << " ";
		}
		f << std::endl;
	}

	f.close();

}


void printDeciderTable(DataRows<MeanInterval> const& minus, DataRows<MeanInterval> const& plus)
{
	assert(minus.size() == plus.size());


	for (std::size_t k_index = 0; k_index < minus.size(); ++k_index) {
		auto const& minus_row = minus[k_index];
		auto const& plus_row = plus[k_index];

		std::cout << "Mean: ";

		for (int i = (int)minus_row.size()-1; i >= 0; --i) {
			std::cout << minus_row[i].mean << " ";
		}
		for (int i = 0; i < (int)plus_row.size(); ++i) {
			std::cout << plus_row[i].mean << " ";
		}
		std::cout << "\n";

		
		/*std::cout << "Interval: " ;
		for (int i = (int)minus_row.size()-1; i >= 0; --i) {
			std::cout << "[" << minus_row[i].interval.first << "," << minus_row[i].interval.second << "] ";
		}
		for (int i = 0; i < (int)plus_row.size(); ++i) {
			std::cout << "[" << plus_row[i].interval.first << "," << plus_row[i].interval.second << "] ";
		}*/
		std::cout << "low: ";
		for (int i = (int)minus_row.size()-1; i >= 0; --i) {
			std::cout << minus_row[i].interval.first << " ";
		}
		for (int i = 0; i < (int)plus_row.size(); ++i) {
			std::cout << plus_row[i].interval.first << " ";
		}
		std::cout << "\n";

		std::cout << "high: " ;
		for (int i = (int)minus_row.size()-1; i >= 0; --i) {
			std::cout << minus_row[i].interval.second << " ";
		}
		for (int i = 0; i < (int)plus_row.size(); ++i) {
			std::cout << plus_row[i].interval.second << " ";
		}
		std::cout << "\n";

	}
	std::cout << "\n";

}

template <typename T>
double mean(std::vector<T> const& vec) {
	T sum = 0;
	for (auto it = vec.begin(); it != vec.end(); ++it) {
		sum += *it;
	}
	return sum / (double) vec.size();
}

template <typename T>
T sum(std::vector<T> const& vec) {
	T sum = 0;
	for (auto it = vec.begin(); it != vec.end(); ++it) {
		sum += *it;
	}
	return sum;
}


template <typename T>
T median(std::vector<T> & vec) {
	auto mid = vec.begin() + vec.size()/2; //for even vectors, settle for one of the two possible elements 
	std::nth_element(vec.begin(), mid, vec.end());
	return *mid;
}




template <typename Vec>
void printDeciderTable(Vec const& minus, Vec const& plus)
{
	assert(minus.size() == plus.size());

	for (std::size_t k_index = 0; k_index < minus.size(); ++k_index) {
		auto const& minus_row = minus[k_index];
		auto const& plus_row = plus[k_index];

		for (int i = (int)minus_row.size()-1; i >= 0; --i) {
			std::cout << minus_row[i] << " ";
		}
		for (int i = 0; i < (int)plus_row.size(); ++i) {
			std::cout << plus_row[i] << " ";
		}
		std::cout << "\n";
	}
	std::cout << "\n";
}

double estimateArrangementSize(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	// adapt this to be large enough to be meaningful but also permissive regarding running time
	std::size_t const number_of_samples = 100000;

	auto const seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::default_random_engine gen(seed);
	std::uniform_int_distribution<std::size_t> distribution1(0, curve1.size()-1);
	std::uniform_int_distribution<std::size_t> distribution2(0, curve2.size()-1);

	auto get_box = [&]() {
		auto center1 = curve1.front() - curve2.front();
		auto center2 = curve1.back() - curve2.back();

		// constrain by start and end points
		auto left = std::max(center1.x, center2.x) - distance;
		auto right = std::min(center1.x, center2.x) + distance;
		auto bottom = std::max(center1.y, center2.y) - distance;
		auto top = std::min(center1.y, center2.y) + distance;

		// constrain by extrema
		auto ep1 = curve1.getExtremePoints();
		auto ep2 = curve2.getExtremePoints();
		left = std::max(left, ep1.min_x - ep2.min_x - distance);
		right = std::min(right, ep1.max_x - ep2.max_x + distance);
		bottom = std::max(bottom, ep1.min_y - ep2.min_y - distance);
		top = std::min(top, ep1.max_y - ep2.max_y + distance);

		return SearchBox({left, bottom}, {right, top});
	};

	auto box = get_box();

	auto intersections_in_box = [&](Point const& c1, Point const& c2) {
		if (c1.dist_sqr(c2) > pow(2*distance, 2)) { return (std::size_t)0; }

		auto middle = c1+c2; middle /= 2.;
		auto const d = c1.dist(c2);
		auto const a = sqrt(pow(distance,2) - pow(d/2,2));
		auto intersection1 = Point{middle.x + a*(c1.y - c2.y)/d, middle.y + a*(c2.x - c1.x)/d};
		auto intersection2 = Point{middle.x - a*(c1.y - c2.y)/d, middle.y - a*(c2.x - c1.x)/d};

		std::size_t intersection_count = 0;
		if (box.contains(intersection1)) { ++intersection_count; }
		if (box.contains(intersection2)) { ++intersection_count; }
		return intersection_count;
	};

	std::size_t intersection_count = 0;
	for (std::size_t i = 0; i < number_of_samples; ++i) {
		std::size_t id1_1 = 0, id1_2 = 0, id2_1 = 0, id2_2 = 0;
		while (id1_1 == id1_2 && id2_1 == id2_2) {
			id1_1 = distribution1(gen);
			id2_1 = distribution2(gen);
			id1_2 = distribution1(gen);
			id2_2 = distribution2(gen);
		}

		auto const t1 = curve1[id1_1] - curve2[id2_1];
		auto const t2 = curve1[id1_2] - curve2[id2_2];

		intersection_count += intersections_in_box(t1, t2);
	}

	double avg_intersection_per_pair = (double)intersection_count/number_of_samples;
	assert(avg_intersection_per_pair >= 0 && avg_intersection_per_pair <= 2);

	std::size_t num_circles = curve1.size()*curve2.size();
	std::size_t num_pairs = (num_circles*(num_circles-1))/2;

	std::size_t const estimate = avg_intersection_per_pair*num_pairs;

	auto get_box_intersections = [&]() {
		std::size_t number_of_intersections = 0;
		for (std::size_t i = 0; i < curve1.size(); ++i) {
			for (std::size_t j = 0; j < curve2.size(); ++j) {
				auto center = curve1[i] - curve2[j];

				// check which corners are contained
				auto const dist_sqr = distance*distance;
				bool contains_tl = (box.topLeft().dist_sqr(center) <= dist_sqr);
				bool contains_tr = (box.topRight().dist_sqr(center) <= dist_sqr);
				bool contains_bl = (box.bottomLeft().dist_sqr(center) <= dist_sqr);
				bool contains_br = (box.bottomRight().dist_sqr(center) <= dist_sqr);

				// if the whole box is contained, this disc doesn't contribute to the arrangement
				if (contains_tl && contains_tr && contains_bl && contains_br) {
					continue;
				}

				// if the circle is completely contained we ignore it even though it
				// might contribute to the arrangement
				if (center.x + distance < box.max.x && center.x - distance > box.min.x &&
					center.y + distance < box.max.y && center.y - distance > box.min.y) {
					continue;
				}

				// if the disc does not intersect the box at all
				if (!contains_tl && !contains_tr && !contains_bl && !contains_br) {
					// check if bounding boxes of circle and box intersect
					if (center.x + distance <= box.min.x ||
						center.y + distance <= box.min.y ||
						center.x - distance >= box.max.x ||
						center.y - distance >= box.max.y) {

						continue;
					}

					// check if all nodes of rect are in one quadrant
					if ((box.bottomLeft().x >= center.x && box.bottomLeft().y >= center.y) ||
						(box.bottomRight().x <= center.x && box.bottomRight().y >= center.y) ||
						(box.topLeft().x >= center.x && box.topLeft().y <= center.y) ||
						(box.topRight().x <= center.x && box.topRight().y <= center.y)) {

						continue;
					}
				}

				// As we only use <=, >=, we only check for real intersections, even though
				// all of this is probably overshadowed by rounding errors.
				number_of_intersections += 2;
			}
		}

		return number_of_intersections;
	};

	return estimate + get_box_intersections();
}

inline void updateProfileDec(TimeProfile & profile)
{
	auto const& preprocessing_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_PREPROCESSING1);
	profile.preprocessing += preprocessing_entry.value;  

	auto const& blackbox_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_BLACKBOX1);
	profile.blackbox += blackbox_entry.value;  

	auto const& arrangement_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_ARRANGEMENT1);
	profile.arrangement += arrangement_entry.value;  

	auto const& arrangement_construction_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_N6_ARR);
	profile.arrangement_construction += arrangement_construction_entry.value;  
	auto const& arrangement_blackbox_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_N6_FRECHET);
	profile.arrangement_blackbox += arrangement_blackbox_entry.value;  

	auto const& discselection_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_DISCSELECTION1);
	profile.discselection += discselection_entry.value;  

}



void experiment1()
{

	std::cout << "I am experiment!" << std::endl;
	std::cout << "Nice to meet you, experiment!" << std::endl;

	// using TDEntry = MEASUREMENT::MeasurementTool::TimeDataEntry;
	// MEASUREMENT::start(EXP::DISTANCE_LESS_THAN);
	// std::size_t count = 0;
	// for (std::size_t i = 0; i < 1000000; ++i) {
	//     ++count;
	// }
	// MEASUREMENT::stop(EXP::DISTANCE_LESS_THAN);
    // 
	// auto const& entry = MEASUREMENT::getEntry<TDEntry>(EXP::DISTANCE_LESS_THAN);
	// std::cout << entry.mean() << std::endl;

	MEASUREMENT::start();
	// do something
	std::cout << "Time: " << MEASUREMENT::stop()/1000000. << " ms" << std::endl;
}

template <typename T>
inline MeanInterval generateMeanInterval(std::vector<T> & vals)
{
	MeanInterval entry;
	entry.mean = mean(vals);
	entry.interval = getPercentileInterval(vals);
	return entry;
}

void deciderExp(bool arrangement_estimation)
{
	std::cout << "***Starting decider experiment.***" << std::endl;

	std::size_t num_data_sets = 4;

	Strings query_file_prefixes = {"../test_data/fut_decider_benchmark_queries/characters_fut_decider_samechar",
		"../test_data/fut_decider_benchmark_queries/characters_fut_decider",
		"../test_data/fut_decider_benchmark_queries/sigspatial_fut_decider",
		"../test_data/fut_decider_benchmark_queries/geolife_fut_decider"};
	std::vector<ValueRange<int>> l_plus_ranges = { {-10, 3}, {-10, 3}, {-10, 3}, {-10, 3} };
	std::vector<ValueRange<int>> l_minus_ranges = { {-10, 0}, {-10, 0}, {-10, 0}, {-10, 0} };

	TimeProfile profile = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	for (std::size_t d = 0; d < num_data_sets; ++d) {
		std::cout << query_file_prefixes[d] << std::endl;

		DataRows<MeanInterval> table_plus;
		DataRows<MeanInterval> table_minus;

		DataRow<MeanInterval> row_plus(l_plus_ranges[d].size());
		DataRow<MeanInterval> row_minus(l_minus_ranges[d].size());

		DataRow<MeanInterval> row_bbcalls_plus(l_plus_ranges[d].size());
		DataRow<MeanInterval> row_bbcalls_minus(l_minus_ranges[d].size());


		DataRow<MeanInterval> row_arr_plus(l_plus_ranges[d].size());
		DataRow<MeanInterval> row_arr_minus(l_minus_ranges[d].size());

		double total_time = 0.0;
		std::size_t total_bbcalls = 0;
		std::size_t num_queries = 0;

		std::size_t plus_index = 0;
		for (auto l: l_plus_ranges[d]) {
			std::string filename = query_file_prefixes[d] + "_" + std::to_string(l) + "_plus.txt";
			auto queries = loadQueries(filename, curve_directories[d]);
			num_queries += queries.size();

			//TODO make measurement consistent with rest
			std::vector<size_t> cur_bbcalls;
			std::vector<double> cur_times;
			for (auto const& query: queries) {
				MEASUREMENT::reset();

				auto start = hrc::now();
				FrechetUnderTranslation frechet; 
				if (not frechet.lessThan(query.distance, query.curve1, query.curve2)) {
					std::cout << "ERROR: Wrong answer! Abort" << std::endl; //TODO: add this test to unit_tests 
				}
				auto time = std::chrono::duration_cast<ns>(hrc::now()-start).count(); 
				cur_times.push_back(time/1000000.);

				updateProfileDec(profile);
				total_time += time;

				auto const& bbentry = MEASUREMENT::getEntry<CounterEntry>(EXP::BBCALLS_COUNTER);
				cur_bbcalls.push_back(bbentry.value);

				total_bbcalls += bbentry.value;

			}
			row_plus[plus_index] = generateMeanInterval(cur_times);
			row_bbcalls_plus[plus_index] = generateMeanInterval(cur_bbcalls);

			if (arrangement_estimation) {
				std::vector<double> cur_arr_sizes;

				for (auto const& query: queries) {
					cur_arr_sizes.push_back(estimateArrangementSize(query.distance, query.curve1, query.curve2));
				}

				row_arr_plus[plus_index] = generateMeanInterval(cur_arr_sizes);
			}


			++plus_index;
		}
		std::size_t minus_index = 0;
		for (auto l: l_minus_ranges[d]) {
			std::string filename = query_file_prefixes[d] + "_" + std::to_string(l) + "_minus.txt";
			auto queries = loadQueries(filename, curve_directories[d]);
			num_queries += queries.size();

			std::vector<size_t> cur_bbcalls;
			std::vector<double> cur_times;
			for (auto const& query: queries) {
				MEASUREMENT::reset();

				auto start = hrc::now();
				FrechetUnderTranslation frechet; 
				if (frechet.lessThan(query.distance, query.curve1, query.curve2)) {
					std::cout << "ERROR: Wrong answer! Abort" << std::endl; //TODO: add this test to unit_tests 
				}
				auto time = std::chrono::duration_cast<ns>(hrc::now()-start).count(); 
				cur_times.push_back(time/1000000.);

				updateProfileDec(profile);
				total_time += time;

				auto const& bbentry = MEASUREMENT::getEntry<CounterEntry>(EXP::BBCALLS_COUNTER);
				cur_bbcalls.push_back(bbentry.value);


				total_bbcalls += bbentry.value;
			}
			row_minus[minus_index] = generateMeanInterval(cur_times);
			row_bbcalls_minus[minus_index] = generateMeanInterval(cur_bbcalls);

			if (arrangement_estimation) {
				std::vector<double> cur_arr_sizes;

				for (auto const& query: queries) {
					cur_arr_sizes.push_back(estimateArrangementSize(query.distance, query.curve1, query.curve2));
				}

				row_arr_minus[minus_index] = generateMeanInterval(cur_arr_sizes);
			}



			++minus_index;
		}

		table_plus.push_back(std::move(row_plus));
		table_minus.push_back(std::move(row_minus));
		table_plus.push_back(std::move(row_bbcalls_plus));
		table_minus.push_back(std::move(row_bbcalls_minus));
		if (arrangement_estimation) {
			table_plus.push_back(std::move(row_arr_plus));
			table_minus.push_back(std::move(row_arr_minus));
		}

		printDeciderTable(table_minus, table_plus);
		dumpDeciderTable(table_minus, table_plus, output_dir + names[d] + ".txt");
		

		//output profile information
		std::string table_out_filename = output_dir + names[d] + "_table.tex";

		std::ofstream f(table_out_filename);

		if (!f.is_open()) {
			std::cerr << "ERROR: Could not open query file: " << table_out_filename << "\n";
			std::exit(1);
		}

		f << "\\textsc{" << names[d] << "} & \\multicolumn{2}{c}{\\textbf{Time}} &  \\textbf{Black-Box Calls}  \\\\\n\\midrule\n"; 


		 f << "& \\multicolumn{2}{c}{" << total_time / 1000000.  << " ms} & " << total_bbcalls <<" \\\\\n" <<
	    "& \\multicolumn{2}{c}{(" << total_time / (1000000. * (double) num_queries) << " ms per instance)} & (" << total_bbcalls / (double) num_queries << " per instance)  \\\\\n" <<
	    "\\cmidrule(r){2-3} \n" <<
	    "& - Preprocessing & " << profile.preprocessing/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}\n" <<
	    "& - Black-box calls (Lipschitz) & " << profile.blackbox/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}" <<
	    "& - Arrangement estimation &" << profile.discselection/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}\n" <<
	   "& - Arrangement algorithm & " << profile.arrangement/1000000. << " ms  \\\\\n" <<
	   "& \\hphantom{bla} * Construction & " << profile.arrangement_construction/1000000. << " ms \\\\\n" <<
	   "& \\hphantom{bla} * Black-box calls & " << profile.arrangement_blackbox/1000000. << " ms \\\\\n" <<
	   "\\bottomrule \n";


	}

}

void deciderCheck()
{
	std::cout << "***Checking decider benchmark set.***" << std::endl;

	std::size_t num_data_sets = 4;

	Strings query_file_prefixes = {"../test_data/fut_decider_benchmark_queries/characters_fut_decider_samechar",
		"../test_data/fut_decider_benchmark_queries/characters_fut_decider",
		"../test_data/fut_decider_benchmark_queries/sigspatial_fut_decider",
		"../test_data/fut_decider_benchmark_queries/geolife_fut_decider"};

	const distance_t epsilon = 1e-7;

	for (std::size_t d = 0; d < num_data_sets; ++d) {
		std::cout << query_file_prefixes[d] << "...\n";

		std::string filename = query_file_prefixes[d] + "_computed_distances.check";
		auto queries = loadQueries(filename, curve_directories[d]);

		for (auto const& query: queries) {
			FrechetUnderTranslation frechet(epsilon); 
			//std::cout << datasets[set1] << datasets[set2] << ": " << q++ << std::endl;
			auto lmf_val = frechet.calcDistance2(query.curve1, query.curve2); 
			auto binsearch_val = frechet.calcDistance2(query.curve1, query.curve2); 
			if (std::abs(lmf_val - query.distance) > epsilon or
			    std::abs(binsearch_val - query.distance) > epsilon or
			    std::abs(lmf_val - binsearch_val) > epsilon)
		       	{
				std::cout << "Values do not agree: " << 
					query.curve1.filename << " " << query.curve2.filename << " " <<
					"(recorded: " << std::setprecision(20) << query.distance <<  
					", LMF: " << lmf_val <<  
					", binary search: " << binsearch_val << ")\n";
				return;
			}
		}
		std::cout << "...OK\n";
	}
}





void printDatasetTable(std::string caption, Strings datasets, DataRows<double> const& data)
{
	std::cout << caption << "| ";
	for (auto dataset : datasets) {
		std::cout << dataset << " ";
	}
	std::cout << std::endl;

	for (std::size_t set1 = 0; set1 < datasets.size(); ++set1) {

		std::cout << datasets[set1] <<  "| ";
		for (std::size_t set2 = 0; set2 < datasets.size(); ++set2) {
			std::cout << data[set1][set2] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

}

void dumpDatasetTable(Strings datasets, DataRows<double> const& data, std::string out_filename)
{

	std::ofstream f(out_filename);

	if (!f.is_open()) {
		std::cerr << "ERROR: Could not open query file: " << out_filename << "\n";
		std::exit(1);
	}

	for (std::size_t set1 = 0; set1 < datasets.size(); ++set1) {

		for (std::size_t set2 = 0; set2 < datasets.size(); ++set2) {
			f << data[set1][set2] << " ";
		}
		f << std::endl;
	}
	f << std::endl;
	
	f.close();
}


inline void updateProfileValComp(TimeProfile & profile)
{
	auto const& preprocessing_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_PREPROCESSING2);
	profile.preprocessing += preprocessing_entry.value;  

	auto const& blackbox_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_BLACKBOX2);
	profile.blackbox += blackbox_entry.value;  

	auto const& arrangement_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_ARRANGEMENT2);
	profile.arrangement += arrangement_entry.value;  

	auto const& arrangement_construction_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_N6_ARR);
	profile.arrangement_construction += arrangement_construction_entry.value;  
	auto const& arrangement_blackbox_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_N6_FRECHET);
	profile.arrangement_blackbox += arrangement_blackbox_entry.value;  

	auto const& discselection_entry = MEASUREMENT::getEntry<TimeEntry>(EXP::FUT_DISCSELECTION2);
	profile.discselection += discselection_entry.value;  

}




void valCompExp(bool valcompexp_usefullbenchmark, bool valcompexp_lmf, bool valcompexp_binsearch, bool valcompexp_lipschitz)
{
	std::cout << "***Starting value computation experiment.***" << std::endl;


	std::string query_file_prefix = valcompexp_usefullbenchmark ?
	    "../test_data/fut_val_computation_benchmark_queries/characters_full"
	  : "../test_data/fut_val_computation_benchmark_queries/characters_small";
	Strings datasets = {"a","b","c","d","e","g","h","l","m","n","o","p","q","r","s","u","v","w","y","z"};

	//std::size_t number_of_runs = 5;

	DataRows<double> values_under_translation;
	DataRows<double> values;
	DataRows<double> lmf_times;
	DataRows<double> binsearch_times;
	DataRows<double> lipschitz_times;
	DataRows<double> bbcalls_lmf;
	DataRows<double> bbcalls_binsearch;
	DataRows<double> bbcalls_lipschitz;

	TimeProfile profile = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	double totaltime_lmf = 0.0;
	double totaltime_binsearch = 0.0;
	double totaltime_lipschitz = 0.0;

	size_t totalbbcalls_lmf = 0;
	size_t totalbbcalls_binsearch = 0;
	size_t totalbbcalls_lipschitz = 0;

	std::string outprefix = valcompexp_usefullbenchmark ? "characters_valcomp_full_" : "characters_valcomp_small_";

	std::string scatter_lmf_filename = output_dir + outprefix + "scatter_lmf.dat";
	std::ofstream scatter_lmf_file(scatter_lmf_filename);

	std::string scatter_binsearch_filename = output_dir + outprefix + "scatter_binsearch.dat";
	std::ofstream scatter_binsearch_file(scatter_binsearch_filename);

	if (!scatter_lmf_file.is_open() or !scatter_binsearch_file.is_open()) {
		std::cerr << "ERROR: Could not open output files: " << scatter_lmf_filename << " and " << scatter_binsearch_filename << "\n";
		std::exit(1);
	}


	//TODO make this a function?
	//TODO build new frechet object for each call?

	std::size_t num_queries = 0;
	
	for (std::size_t set1 = 0; set1 < datasets.size(); ++set1) {

		DataRow<double> values_under_translation_row;
		DataRow<double> values_row;

		DataRow<double> lmf_times_row;
		DataRow<double> binsearch_times_row;
		DataRow<double> lipschitz_times_row;
		
		DataRow<double> bbcalls_lmf_row;
		DataRow<double> bbcalls_binsearch_row;
		DataRow<double> bbcalls_lipschitz_row;
		std::cout << "Row " << datasets[set1] << std::endl;

		//symmetrization
		for (std::size_t set2 = 0; set2 < set1; ++set2) {
		  	if (valcompexp_lmf) { 
			  lmf_times_row.push_back(lmf_times[set2][set1]);
			  bbcalls_lmf_row.push_back(bbcalls_lmf[set2][set1]);
			  values_under_translation_row.push_back(values_under_translation[set2][set1]);
			}
			if (valcompexp_binsearch) {
			  binsearch_times_row.push_back(binsearch_times[set2][set1]);
			  bbcalls_binsearch_row.push_back(bbcalls_binsearch[set2][set1]);
			}
			if (valcompexp_lipschitz) {
			  lipschitz_times_row.push_back(lipschitz_times[set2][set1]);
			  bbcalls_lipschitz_row.push_back(bbcalls_lipschitz[set2][set1]);
			}

			values_row.push_back(values[set2][set1]);
		}

		for (std::size_t set2 = set1; set2 < datasets.size(); ++set2) {
			std::string filename = query_file_prefix + "_" + datasets[set1] + "_" + datasets[set2] + ".txt";
			auto queries = loadQueriesDistanceLess(filename, curve_directories[0]);
			num_queries += queries.size();


			//measure (mean) times&calls for LMF and record computed median distances under translations
			//std::size_t q = 0;
			double total_val = 0.;
			if (valcompexp_lmf) {
				std::vector<double> cur_times;
				std::vector<std::size_t> cur_bbcalls;
				std::vector<distance_t> cur_vals;
				for (auto const& query: queries) {
					MEASUREMENT::reset();
					MEASUREMENT::start();

					FrechetUnderTranslation frechet; 
					//std::cout << datasets[set1] << datasets[set2] << ": " << q++ << std::endl;
					auto val = frechet.calcDistance2(query.curve1, query.curve2); 
					
					cur_times.push_back(MEASUREMENT::stop()/1000000.);  

					updateProfileValComp(profile);

					scatter_lmf_file << (double) cur_times.back() << std::endl;

					cur_vals.push_back(val);
					auto const& entry = MEASUREMENT::getEntry<CounterEntry>(EXP::BBCALLS_COUNTER);
					cur_bbcalls.push_back(entry.value);
				}
				lmf_times_row.push_back(mean(cur_times));
				//std::cout << "recorded median: " << lmf_times_row.back() << " (Mean: " << mean(cur_times) << ")\n\n";

				totaltime_lmf += sum(cur_times);
				values_under_translation_row.push_back(mean(cur_vals));
				bbcalls_lmf_row.push_back(mean(cur_bbcalls));
				totalbbcalls_lmf += sum(cur_bbcalls);
				total_val += sum(cur_vals);
			}

			//measure (mean) times&calls for binary search
			if (valcompexp_binsearch) {
				std::vector<double> cur_times;
				std::vector<std::size_t> cur_bbcalls;
				std::vector<distance_t> cur_vals;
				for (auto const& query: queries) {
					MEASUREMENT::reset(EXP::BBCALLS_COUNTER);
					MEASUREMENT::start();

					FrechetUnderTranslation frechet; 
					//std::cout << datasets[set1] << datasets[set2] << ": " << q++ << std::endl;
					auto val = frechet.calcDistance(query.curve1, query.curve2); 
					
					cur_times.push_back(MEASUREMENT::stop()/1000000.);  

					scatter_binsearch_file << (double) cur_times.back() << std::endl;

					cur_vals.push_back(val);
					auto const& entry = MEASUREMENT::getEntry<CounterEntry>(EXP::BBCALLS_COUNTER);
					cur_bbcalls.push_back(entry.value);
				}
				binsearch_times_row.push_back(mean(cur_times));
				totaltime_binsearch += sum(cur_times);
				bbcalls_binsearch_row.push_back(mean(cur_bbcalls));
				totalbbcalls_binsearch += sum(cur_bbcalls);

				if (valcompexp_lmf and abs(total_val - sum(cur_vals)) > 2e-7*queries.size()) {
					std::cout << "ERROR! precision requirement definitely not met" << std::endl;
				}
			}

			//measure (mean) times&calls for binary search
			if (valcompexp_lipschitz) {
				std::vector<double> cur_times;
				std::vector<std::size_t> cur_bbcalls;
				std::vector<distance_t> cur_vals;
				for (auto const& query: queries) {
					MEASUREMENT::reset(EXP::BBCALLS_COUNTER);
					MEASUREMENT::start();

					FrechetUnderTranslation frechet; 
					frechet.disableArrangement(); 
					//std::cout << datasets[set1] << datasets[set2] << ": " << q++ << std::endl;
					auto val = frechet.calcDistance2(query.curve1, query.curve2); 
					
					cur_times.push_back(MEASUREMENT::stop()/1000000.);  

					cur_vals.push_back(val);
					auto const& entry = MEASUREMENT::getEntry<CounterEntry>(EXP::BBCALLS_COUNTER);
					cur_bbcalls.push_back(entry.value);
				}
				lipschitz_times_row.push_back(mean(cur_times));
				totaltime_lipschitz += sum(cur_times);
				bbcalls_lipschitz_row.push_back(mean(cur_bbcalls));
				totalbbcalls_lipschitz += sum(cur_bbcalls);

				if (valcompexp_lmf and abs(total_val - sum(cur_vals)) > 2e-7*queries.size()) {
					std::cout << "ERROR! precision requirement definitely not met" << std::endl;
				}
			}


			//record computed average distances for untranslated curves
			DiscreteFrechetLight frechet; 
			std::vector<distance_t> cur_vals;
			for (auto const& query: queries) {
				cur_vals.push_back(frechet.calcDistance(query.curve1, query.curve2)); 
			}
			values_row.push_back(mean(cur_vals));

		}
		values_under_translation.push_back(values_under_translation_row);
		values.push_back(values_row);

		lmf_times.push_back(lmf_times_row);
		binsearch_times.push_back(binsearch_times_row);
		lipschitz_times.push_back(lipschitz_times_row);
		
		bbcalls_lmf.push_back(bbcalls_lmf_row);
		bbcalls_binsearch.push_back(bbcalls_binsearch_row);
		bbcalls_lipschitz.push_back(bbcalls_lipschitz_row);
	}


	printDatasetTable("FrechetuTval", datasets, values_under_translation);
	dumpDatasetTable(datasets, values_under_translation, output_dir + outprefix + "fut_val.dat");

	printDatasetTable("Frechet", datasets, values);
	dumpDatasetTable(datasets, values, output_dir + outprefix + "frechet_val.dat");

	std::string total_out_filename = output_dir + outprefix + "total_table.tex";;
	std::ofstream f(total_out_filename);

	if (!f.is_open()) {
		std::cerr << "ERROR: Could not open query file: " << total_out_filename << "\n";
		std::exit(1);
	}


	//f << "Algorithm & Time & Black-Box Calls\n";
	f << "\\textbf{Algorithm} & \\multicolumn{2}{c}{\\textbf{Time}} &  \\textbf{Black-Box Calls}  \\\\\n\\midrule\n"; 


	if (valcompexp_lmf) {
	  printDatasetTable("LMFcallsMedian", datasets, bbcalls_lmf);
	  dumpDatasetTable(datasets, bbcalls_lmf, output_dir + outprefix + "bbcalls_lmf.dat");
	  std::cout << "Total Black-Box Calls LMF: " << totalbbcalls_lmf << std::endl<< std::endl;

	  printDatasetTable("LMFtimesMedian", datasets, lmf_times);
	  dumpDatasetTable(datasets, lmf_times, output_dir + outprefix + "times_lmf.dat");
	  std::cout << "Total Time LMF: " << std::setprecision(1) << totaltime_lmf << " ms" << std::endl;
	  std::cout << "Profile:\n" <<
		  	"- Preprocessing: " << profile.preprocessing/1000000. <<  "\n" <<
		  	"- Black-Box Calls: " << profile.blackbox/1000000. <<  "\n" <<
		  	"- Arrangement: " << profile.arrangement/1000000. <<  "\n" <<
		  	"-- construction: " << profile.arrangement_construction/1000000. <<  "\n" <<
		  	"-- blackbox: " << profile.arrangement_blackbox/1000000. <<  "\n" <<
		  	"- DiscSelection: " << profile.discselection/1000000. <<  "\n"; 

	  //f << "LMF & " << totaltime_lmf << " & " << totalbbcalls_lmf << "\n";

	  f << "LMF & \\multicolumn{2}{c}{" << std::setprecision(1) << totaltime_lmf << " ms} & " << totalbbcalls_lmf <<" \\\\\n" <<
	    "& \\multicolumn{2}{c}{(" << totaltime_lmf / (double) num_queries << " ms per instance)} & (" << totalbbcalls_lmf / (double) num_queries << " per instance)  \\\\\n" <<
	    "\\cmidrule(r){2-3} \n" <<
	    "& - Preprocessing & " << profile.preprocessing/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}\n" <<
	    "& - Black-box calls (Lipschitz) & " << profile.blackbox/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}" <<
	    "& - Arrangement estimation &" << profile.discselection/1000000. << " ms  \\\\\n" <<
	    "\\cmidrule(r){2-3}\n" <<
	   "& - Arrangement algorithm & " << profile.arrangement/1000000. << " ms  \\\\\n" <<
	   "& \\hphantom{bla} * Construction & " << profile.arrangement_construction/1000000. << " ms \\\\\n" <<
	   "& \\hphantom{bla} * Black-box calls & " << profile.arrangement_blackbox/1000000. << " ms \\\\\n" <<
	   "\\midrule \n";


	}
	if (valcompexp_binsearch) {
	  printDatasetTable("BINSEARCHcallsMedian", datasets, bbcalls_binsearch);
	  dumpDatasetTable(datasets, bbcalls_binsearch, output_dir + outprefix + "times_binsearch.dat");
	  std::cout << "Total Black-Box Calls Binary Search: " << totalbbcalls_binsearch << std::endl<< std::endl;

	  printDatasetTable("BINSEARCHtimesMedian", datasets, binsearch_times);
	  dumpDatasetTable(datasets, binsearch_times, output_dir + outprefix + "bbcalls_binsearch.dat");
	  std::cout << "Total Time Binary Search: " << totaltime_binsearch << " ms" << std::endl;

	  //f << "Binary Search & " << totaltime_binsearch << " & " << totalbbcalls_binsearch << "\n";
	  f << "Binary Search & \\multicolumn{2}{c}{" << totaltime_binsearch << " ms} & " << totalbbcalls_binsearch << "\\\\\n" <<
	     "& \\multicolumn{2}{c}{( " << totaltime_binsearch / (double) num_queries << " ms per instance)} &  (" << totalbbcalls_binsearch / (double) num_queries << " per instance) \\\\\n";
	}
	if (valcompexp_lipschitz) {
	  printDatasetTable("LIPSCHITZcallsMedian", datasets, bbcalls_lipschitz);
	  dumpDatasetTable(datasets, bbcalls_lipschitz, output_dir + outprefix + "times_lipschitz.dat");
	  std::cout << "Total Black-Box Calls Lipschitz-only: " << totalbbcalls_lipschitz << std::endl<< std::endl;

	  printDatasetTable("LIPSCHITZtimesMedian", datasets, lipschitz_times);
	  dumpDatasetTable(datasets, lipschitz_times, output_dir + outprefix + "bbcalls_lipschitz.dat");
	  std::cout << "Total Time Lipschitz-only: " << totaltime_lipschitz << " ms" << std::endl;

	  //f << "Lipschitz-only & " << totaltime_lipschitz << " & " << totalbbcalls_lipschitz << "\n";
	  f << "Lipschitz-only & \\multicolumn{2}{c}{" << totaltime_lipschitz << " ms s} & " << totalbbcalls_lipschitz << "\\\\\n" <<
	    "& \\multicolumn{2}{c}{( " << totaltime_lipschitz / (double) num_queries << " ms per instance)} &  (" << totalbbcalls_lipschitz / (double) num_queries << " per instance) \\\\\n";

	}


	
	/*


		std::cout << "Frechet| ";
	for (auto dataset : datasets) {
		std::cout << dataset << " ";
	}
	std::cout << std::endl;

	for (std::size_t set1 = 0; set1 < datasets.size(); ++set1) {
		std::cout << datasets[set1] <<  "| ";
		for (std::size_t set2 = 0; set2 < set1; ++set2) {
			std::cout << "- "; 
		}
		for (std::size_t set2 = set1; set2 < datasets.size(); ++set2) {
			std::string filename = query_file_prefix + "_" + datasets[set1] + "_" + datasets[set2] + ".txt";
			auto queries = loadQueriesDistanceLess(filename, curve_directories[0]);

			DiscreteFrechetLight frechet; 
			distance_t sum = 0.;
			for (auto const& query: queries) {
				sum += frechet.calcDistance(query.curve1, query.curve2); 
			}
			sum /= (distance_t) queries.size();
			std::cout << sum << " ";
		}
		std::cout << std::endl;
	}*/
}

void writeArrangement()
{
	auto curve1 = parser::readCurve("../../benchmark/characters/data/1.txt");
	auto curve2 = parser::readCurve("../../benchmark/characters/data/10.txt");

	auto simplify_curve = [](Curve const& curve, std::size_t step) {
		Curve short_curve;
		for (std::size_t k = 0; k < curve.size()-1; k += step) {
			short_curve.push_back(curve[k]);
		}
		short_curve.push_back(curve.back());
		return short_curve;
	};

	auto short_curve1 = simplify_curve(curve1, 10);
	auto short_curve2 = simplify_curve(curve2, 10);

	for (std::size_t i = 0; i < short_curve1.size(); ++i) {
		std::cout << short_curve1[i].x << " " << short_curve1[i].y << std::endl;
	}
	std::cout << "===" << std::endl;
	for (std::size_t i = 0; i < short_curve1.size(); ++i) {
		std::cout << short_curve2[i].x << " " << short_curve2[i].y << std::endl;
	}

	// distance_t const radius = 5.;
	// std::cout << radius << std::endl;
	// for (std::size_t i = 0; i < short_curve1.size(); ++i) {
	//     for (std::size_t j = 0; j < short_curve2.size(); ++j) {
	//         auto center = short_curve1[i] - short_curve2[j];
	//         std::cout << center.x << " " << center.y << std::endl;
	//     }
	// }
}

void writeLipschitzHeatmapData()
{
	auto curve1 = parser::readCurve("../../benchmark/characters/data/1.txt");
	auto curve2 = parser::readCurve("../../benchmark/characters/data/100.txt");

	std::vector<std::vector<double>> distance_rows;

	distance_t const grid_size = 50.;
	auto const d = curve1.getUpperBoundDistance(curve2);
	auto const center1 = curve1.front() - curve2.front();
	auto const center2 = curve1.back() - curve2.back();
	auto const left = std::max(center1.x, center2.x) - d;
	auto const right = std::min(center1.x, center2.x) + d;
	auto const bottom = std::max(center1.y, center2.y) - d;
	auto const top = std::min(center1.y, center2.y) + d;
	auto const x_0 = left;
	auto const x_step = (right-left)/grid_size;
	auto const y_0 = bottom;
	auto const y_step = (top-bottom)/grid_size;

	DiscreteFrechetLight frechet;

	for (int j = 0; j <= grid_size; ++j) {
		distance_rows.emplace_back();
		auto& row = distance_rows.back();
		for (int i = 0; i <= grid_size; ++i) {
			curve2.translate({x_0 + i*x_step, y_0 + j*y_step});
			row.push_back(frechet.calcDistance(curve1, curve2));
		}
	}
	curve2.resetTranslation();

	std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(5);
	for (auto const& row: distance_rows) {
		for (std::size_t i = 0; i < row.size(); ++i) {
			std::cout << row[i] << " ";
		}
		std::cout << std::endl;
	}
}
