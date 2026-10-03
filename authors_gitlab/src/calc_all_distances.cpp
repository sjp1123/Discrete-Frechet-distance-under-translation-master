#include "defs.h"
#include "discrete_frechet_light.h"
#include "frechet_abstract.h"
#include "frechet_light.h"
#include "parser.h"

#include <string>

void printUsage()
{
	std::cout << "Usage: ./calc_all_distances <alg_name>\n";
	std::cout << "       <alg_name> can be light or discrete_light\n";
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printUsage();
		ERROR("Wrong number of parameters.");
	}

	std::string const path = "../../benchmark/Geolife Trajectories 1.3/data/";

	Curves curves;
	for (std::size_t i = 0; i < 2858; ++i) {
		std::string const filename = std::to_string(i+1) + ".txt";
		curves.push_back(parser::readCurve(path + filename));
	}

	std::size_t k = 100;

	//FIXME: Change back to newer C++ standard...
	//std::unique_ptr<FrechetAbstract> frechet;
	FrechetAbstract* frechet;
	if (std::string(argv[1]) == "discrete_light") {
	  	//FIXME ...and here...
		//frechet = std::unique_ptr<DiscreteFrechetLight>(new DiscreteFrechetLight());
	  	frechet = new DiscreteFrechetLight();
	}
	else if (std::string(argv[1]) == "light") {
	  	//FIXME ...and here...
		//frechet = std::unique_ptr<FrechetLight>(new FrechetLight());
		frechet = new FrechetLight();
	}
	else {
		printUsage();
		ERROR("Unknown <alg_name> parameter provided.");
	}

	frechet->setRules({true, true, true, true, true});

	distance_t sum = 0.;
	for (std::size_t i = 0; i < k; ++i) {
		for (std::size_t j = 0; j < k; ++j) {
			auto distance = frechet->calcDistance(curves[i], curves[j]);
			sum += distance;
		}
	}

	//FIXME ...and here.
	delete frechet;
	std::cout << sum << std::endl;
}
