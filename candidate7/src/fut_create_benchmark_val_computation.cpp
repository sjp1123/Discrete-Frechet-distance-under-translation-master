#include "defs.h"
#include "curves.h"
#include "parser.h"
#include "frechet_under_translation.h"
#include "discrete_frechet_light.h"

#include <fstream>
#include <limits>
#include <random>
#include <cmath>
#include <chrono>

void printUsage()
{
	std::cout <<
		"Usage: ./fut_create_benchmark_val_computation <meta_dataset> <meta_directory> <curve_directory> <out_prefix> <number_of_queries>\n"
		"\n";
}

struct BenchmarkQuery {
	Curve const& curve1;
	Curve const& curve2;
	distance_t distance;

	BenchmarkQuery(Curve const& curve1, Curve const& curve2, distance_t distance)
		: curve1(curve1), curve2(curve2), distance(distance) {}
};
using BenchmarkQueries = std::vector<BenchmarkQuery>;
using BenchmarkQueriesVec = std::vector<BenchmarkQueries>;

using Datasets = std::vector<Curves>;


void readDatasets(std::string const& meta_file, std::string const& meta_dir, std::string const& curve_directory, Datasets& datasets, std::vector<std::string>& dataset_names)
{
	// read filenames of curve files
	std::ifstream file(meta_file);
	std::vector<std::string> dataset_filenames;
	if (file.is_open()) {
		std::string filename, set_name;
		while (file >> filename >> set_name) {
			std::cout << filename << ": " << set_name << std::endl;
			dataset_filenames.push_back(filename);
			dataset_names.push_back(set_name);
		}
	}
	else {
		ERROR("The meta data file could not be opened: " << meta_file);
	}

	// read datasets
	datasets.clear();
	datasets.reserve(dataset_filenames.size());

	for (auto const& dataset_filename: dataset_filenames) {
		std::vector<std::string> curve_filenames;

		std::ifstream dataset_file(meta_dir + dataset_filename);
		if (dataset_file.is_open()) {
			std::string line;
			while (std::getline(dataset_file, line)) {
				curve_filenames.push_back(line);
			}
		}
		else {
			ERROR("The data set file could not be opened: " << meta_dir + dataset_filename);
		}
		datasets.emplace_back();
		auto& cur_dataset = datasets.back();
		cur_dataset.reserve(curve_filenames.size());

		for (auto const& curve_filename: curve_filenames) {

			std::ifstream curve_file(curve_directory + curve_filename);
			if (curve_file.is_open()) {
				cur_dataset.emplace_back();
				parser::readCurve(curve_file, cur_dataset.back());
				cur_dataset.back().filename = curve_filename;

				if (cur_dataset.back().empty()) { cur_dataset.pop_back(); }
			}
			else {
				ERROR("A curve file could not be opened: " << curve_directory + curve_filename);
			}
		}
	}
}


CurveID getRandomCurveID(Curves const& curves)
{
	static const auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	static std::default_random_engine gen(seed);

	std::uniform_int_distribution<std::size_t> distribution(0, curves.size()-1);
	return distribution(gen);
}

int main(int argc, char* argv[])
{
	if (argc != 6) {
		printUsage();
		ERROR("Wrong number of arguments passed.");
	}

	std::string meta_file(argv[1]);
	std::string meta_dir(argv[2]);
	std::string curve_directory(argv[3]);
	std::string out_prefix = argv[4];
	std::size_t number_of_queries = std::stoi(argv[5]);

	Datasets datasets;
	std::vector<std::string> dataset_names;
	readDatasets(meta_file, meta_dir, curve_directory, datasets, dataset_names);
	std::cout << "Loaded " << datasets.size() << " datasets:" << std::endl;
	for (auto dataset : datasets) {
		std::cout << dataset.size() << " curves" << std::endl; 
	}

	//
	// calculate benchmark
	//

	for (std::size_t set1 = 0; set1 < datasets.size(); ++set1) {
		for (std::size_t set2 = set1; set2 < datasets.size(); ++set2) {  
			auto const& curve_set1 = datasets[set1];
			auto const& curve_set2 = datasets[set2];



			std::string filename = out_prefix + "_" + dataset_names[set1] + "_" + 
			  dataset_names[set2] + ".txt";


			std::ofstream f(filename);
			if (!f.is_open()) { std::cerr << "Error opening file.\n"; std::exit(1); }

			f << std::setprecision(20);

			for (std::size_t q = 0; q < number_of_queries; ++q) {

				auto curve1_id = getRandomCurveID(curve_set1);
				auto curve2_id = getRandomCurveID(curve_set2);
				auto const& curve1 = curve_set1[curve1_id];
				auto const& curve2 = curve_set2[curve2_id];


				if ( (set1 == set2 and curve2_id == curve1_id)) { //trivial instance
					q--; continue;
				}


				f << curve1.filename << " " << curve2.filename << "\n";
			}

			f.close();
		}
	}

}
