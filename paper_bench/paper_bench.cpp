// paper_bench: in-process benchmark driver for original vs candidate5 on the
// data sets of "When Lipschitz Walks Your Dog" (BKN, ESA 2020).
//
// It is compiled once per arm against that arm's own sources (see
// CMakeLists.txt / build.sh), so both arms run exactly the code path of the
// paper's harness (src/fut_paper_experiments.cpp): `trans` objects, -DTRANSLATION,
// FrechetUnderTranslation with default parameters, timed around construction + call
// like the paper's harness, but with steady_clock instead of its high_resolution_clock
// (system_clock in libstdc++, which WSL2 steps; experiment_log 6.12).
//
//   paper_bench lmf     <pairs.txt>   <curve_dir> <out.csv>
//        value computation, calcDistance2 (LMF), one row per pair
//   paper_bench decider <queries.txt> <curve_dir> <out.csv>
//        decision problem, lessThan(distance), one row per query
//   paper_bench gen     <pairs.txt>   <curve_dir> <out_prefix>
//        replicates src/fut_create_benchmark_decider.cpp: delta* at precision 1e-7,
//        <out_prefix>_computed_distances.check and the 23 _l_plus/_l_minus files
//
// Input line formats: "<file1> <file2>" (lmf/gen) and "<file1> <file2> <distance>"
// (decider); file names are relative to <curve_dir>, exactly as in the authors'
// test_data/fut_*_benchmark_queries files.
#include "defs.h"
#include "curves.h"
#include "parser.h"
#include "frechet_under_translation.h"
#include "measurement_tool.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

using hrc = std::chrono::steady_clock;   // monotonic; high_resolution_clock = system_clock steps under WSL2

std::map<std::string, Curve> curve_cache;

Curve const& getCurve(std::string const& dir, std::string const& name)
{
	auto it = curve_cache.find(name);
	if (it != curve_cache.end()) { return it->second; }
	auto curve = parser::readCurve(dir + name);
	curve.filename = name;
	return curve_cache.emplace(name, std::move(curve)).first->second;
}

double timerMs(EXP id)
{
	auto const& e = MEASUREMENT::getEntry<MEASUREMENT::MeasurementTool::TimeEntry>(id);
	return e.value / 1000000.;
}

std::size_t counter(EXP id)
{
	return MEASUREMENT::getEntry<MEASUREMENT::MeasurementTool::CounterEntry>(id).value;
}

void usage()
{
	std::cerr << "Usage: paper_bench <lmf|decider|gen> <query_file> <curve_dir> <out>\n";
	std::exit(1);
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc != 5) { usage(); }
	std::string mode(argv[1]), query_file(argv[2]), curve_dir(argv[3]), out(argv[4]);
	if (!curve_dir.empty() && curve_dir.back() != '/') { curve_dir += '/'; }

	std::ifstream in(query_file);
	if (!in.is_open()) { std::cerr << "cannot open " << query_file << "\n"; return 1; }

	if (mode == "lmf") {
		std::ofstream csv(out);
		csv << "file1,file2,n1,n2,value,time_ms,bbcalls,n6_arr_ms,n6_fre_ms,pre2_ms,bb2_ms,disc2_ms,arr2_ms\n";
		csv << std::setprecision(20);
		std::string f1, f2, rest;
		std::string line;
		std::size_t q = 0;
		while (std::getline(in, line)) {
			std::istringstream ls(line);
			if (!(ls >> f1 >> f2)) { continue; }
			auto const& c1 = getCurve(curve_dir, f1);
			auto const& c2 = getCurve(curve_dir, f2);

			MEASUREMENT::reset();
			auto start = hrc::now();
			FrechetUnderTranslation frechet;
			auto val = frechet.calcDistance2(c1, c2);
			auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(hrc::now() - start).count();

			csv << f1 << "," << f2 << "," << c1.size() << "," << c2.size() << ","
			    << val << "," << ns / 1000000. << "," << counter(EXP::BBCALLS_COUNTER) << ","
			    << timerMs(EXP::FUT_N6_ARR) << "," << timerMs(EXP::FUT_N6_FRECHET) << ","
			    << timerMs(EXP::FUT_PREPROCESSING2) << "," << timerMs(EXP::FUT_BLACKBOX2) << ","
			    << timerMs(EXP::FUT_DISCSELECTION2) << "," << timerMs(EXP::FUT_ARRANGEMENT2) << "\n";
			csv.flush();   // keep the row if the process is killed later (OOM on a later pair)
			if (++q % 200 == 0) { std::cerr << "  " << q << " queries\n"; }
		}
		std::cerr << "lmf: " << q << " queries -> " << out << "\n";
	}
	else if (mode == "decider") {
		std::ofstream csv(out);
		csv << "file1,file2,distance,answer,time_ms,bbcalls,n6_arr_ms,n6_fre_ms,pre1_ms,bb1_ms,disc1_ms,arr1_ms\n";
		csv << std::setprecision(20);
		std::string f1, f2, line;
		distance_t distance;
		std::size_t q = 0;
		while (std::getline(in, line)) {
			std::istringstream ls(line);
			if (!(ls >> f1 >> f2 >> distance)) { continue; }
			auto const& c1 = getCurve(curve_dir, f1);
			auto const& c2 = getCurve(curve_dir, f2);

			MEASUREMENT::reset();
			auto start = hrc::now();
			FrechetUnderTranslation frechet;
			bool answer = frechet.lessThan(distance, c1, c2);
			auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(hrc::now() - start).count();

			csv << f1 << "," << f2 << "," << distance << "," << (answer ? 1 : 0) << ","
			    << ns / 1000000. << "," << counter(EXP::BBCALLS_COUNTER) << ","
			    << timerMs(EXP::FUT_N6_ARR) << "," << timerMs(EXP::FUT_N6_FRECHET) << ","
			    << timerMs(EXP::FUT_PREPROCESSING1) << "," << timerMs(EXP::FUT_BLACKBOX1) << ","
			    << timerMs(EXP::FUT_DISCSELECTION1) << "," << timerMs(EXP::FUT_ARRANGEMENT1) << "\n";
			csv.flush();   // keep the row if the process is killed later (OOM on a later pair)
			if (++q % 2000 == 0) { std::cerr << "  " << q << " queries\n"; }
		}
		std::cerr << "decider: " << q << " queries -> " << out << "\n";
	}
	else if (mode == "gen") {
		// Distance factors follow the PAPER (ESA 2020, section 6, "Decider experiments"):
		//   NO  queries: (1 - 4^l) * delta_LB,  l = -10 .. -1
		//   YES queries: (1 + 4^l) * delta_UB,  l = -10 ..  2
		// with delta_LB = delta* - eps, delta_UB = delta* + eps, eps = 1e-7, so the
		// interval [delta_LB, delta_UB] has width 2e-7 as the paper requires.
		// (The tree's own fut_create_benchmark_decider.cpp uses 2^l instead, which makes
		// the hardest NO query 1 - 9.8e-4 instead of the paper's 1 - 9.5e-7.)
		const distance_t precision = 1e-7;
		std::vector<int> ls_plus, ls_minus;
		for (int l = -10; l <= 2; ++l) { ls_plus.push_back(l); }
		for (int l = -10; l <= -1; ++l) { ls_minus.push_back(l); }
		std::vector<std::ofstream> fplus, fminus;
		for (int l: ls_plus) { fplus.emplace_back(out + "_" + std::to_string(l) + "_plus.txt"); fplus.back() << std::setprecision(20); }
		for (int l: ls_minus) { fminus.emplace_back(out + "_" + std::to_string(l) + "_minus.txt"); fminus.back() << std::setprecision(20); }
		std::ofstream check(out + "_computed_distances.check");
		check << std::setprecision(20);

		std::string f1, f2, line;
		std::size_t q = 0;
		while (std::getline(in, line)) {
			std::istringstream ls(line);
			if (!(ls >> f1 >> f2)) { continue; }
			auto const& c1 = getCurve(curve_dir, f1);
			auto const& c2 = getCurve(curve_dir, f2);
			FrechetUnderTranslation frechet(precision);
			auto delta_star = frechet.calcDistance2(c1, c2);
			check << f1 << " " << f2 << " " << delta_star << "\n";
			for (std::size_t i = 0; i < ls_minus.size(); ++i) {
				auto distance = (delta_star - precision) * (1. - std::pow(4, ls_minus[i]));
				fminus[i] << f1 << " " << f2 << " " << distance << "\n";
			}
			for (std::size_t i = 0; i < ls_plus.size(); ++i) {
				auto distance = (delta_star + precision) * (1. + std::pow(4, ls_plus[i]));
				fplus[i] << f1 << " " << f2 << " " << distance << "\n";
			}
			if (++q % 200 == 0) { std::cerr << "  " << q << " pairs\n"; }
		}
		std::cerr << "gen: " << q << " pairs -> " << out << "_*\n";
	}
	else { usage(); }
	return 0;
}
