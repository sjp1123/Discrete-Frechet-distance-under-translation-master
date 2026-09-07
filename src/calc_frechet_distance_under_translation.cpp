#include "defs.h"
#include "frechet_under_translation.h"
#include "freespace_light_vis.h"
#include "fut_n6_algorithm.h"
#include "parser.h"

#include <string>

void printUsage()
{
	std::cout <<
		"Usage: ./calc_frechet_distance_under_translation <curve_file1> <curve_file2> [<alg> <svg_file_prefix>]\n"
		"\n"
		"The parameter <alg> can either be \"n6\", \"fut_binary\", \"fut_lmf\", or \"fut\" (default).\n"
		"For \"fut\" both of the others (\"fut_binary\", \"fut_lmf\") are executed.\n"
		"If only three arguments are passed, then no svg file\n"
		"is exported. More information regarding the format of the curve files can\n"
		"be found in README.\n"
		"\n";
}

int main(int argc, char* argv[])
{
	if (argc <= 2 || argc >= 6) {
		printUsage();
		ERROR("Wrong number of arguments passed.");
	}

	std::string curve_file1(argv[1]);
	std::string curve_file2(argv[2]);
	std::string alg = (argc >= 4 ? argv[3] : "fut");
	std::string vis_prefix = (argc >= 5 ? argv[4] : "");

	auto curve1 = parser::readCurve(curve_file1);
	auto curve2 = parser::readCurve(curve_file2);

	distance_t distance;
	if (alg == "n6") {
		N6Alg n6_alg;
		distance = n6_alg.calcDistance(curve1, curve2);
		MEASUREMENT::print();
	}
	else if (alg == "fut" || alg == "fut_binary" || alg == "fut_lmf" || alg == "fut_lipschitz") {
		FrechetUnderTranslation frechet;
		if (alg == "fut" || alg == "fut_binary") {
			distance = frechet.calcDistance(curve1, curve2);
			std::cout << "The Fréchet distance for calcDistance (Binary Search) is: " << std::setprecision(20) << distance << "\n";
			MEASUREMENT::print();
			MEASUREMENT::reset();
			std::cout << std::endl;
		}

		if (alg == "fut" || alg == "fut_lmf") {
			distance = frechet.calcDistance2(curve1, curve2);
			std::cout << "The Fréchet distance for calcDistance2 (LMF) is: " << std::setprecision(20) << distance << "\n";
			MEASUREMENT::print();
			MEASUREMENT::reset();
			std::cout << std::endl;
		}

		if (alg == "fut" || alg == "fut_lipschitz") {
			frechet.disableArrangement();
			distance = frechet.calcDistance2(curve1, curve2);
			std::cout << "The Fréchet distance for calcDistance2 (Lipschitz-only) is: " << std::setprecision(20) << distance << "\n";
			MEASUREMENT::print();
		}


		// if (!vis_prefix.empty()) {
		//     FrechetLight frechet_light;
		//     curve2.translate(frechet.getTranslation());
		//     frechet_light.buildFreespaceDiagram(distance+frechet.getEpsilon(), curve1, curve2);
		// 
		//     curve2.resetTranslation();
		//     FreespaceLightVis vis_curves(frechet_light);
		//     vis_curves.onlyDrawCurves();
		//     vis_curves.exportToSvg(vis_prefix + "_curves.svg");
		// 
		//     curve2.translate(frechet.getTranslation());
		//     FreespaceLightVis vis_translated(frechet_light);
		//     vis_translated.exportToSvg(vis_prefix + "_translated.svg");
		// }
	}
	else {
		printUsage();
		ERROR("Invalid value for <alg> passed: " << alg);
	}
}
