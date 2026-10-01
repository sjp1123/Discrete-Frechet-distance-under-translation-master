// scratch driver: value of every entry point for each pair: calcDistance (binary search baseline),
// calcDistance2 (LMF, the paper's), and lessThan at the LMF value (+1e-7) as a consistency check.
#include "defs.h"
#include "curves.h"
#include "parser.h"
#include "frechet_under_translation.h"
#include "measurement_tool.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc, char* argv[]) {
	if (argc != 3) { std::cerr << "entry <pairs> <dir>\n"; return 1; }
	std::ifstream in(argv[1]); std::string dir(argv[2]); if (dir.back() != '/') dir += '/';
	std::string a, b, line;
	std::cout << std::setprecision(17);
	while (std::getline(in, line)) {
		std::istringstream ls(line); if (!(ls >> a >> b)) continue;
		auto c1 = parser::readCurve(dir + a), c2 = parser::readCurve(dir + b);
		MEASUREMENT::reset();
		FrechetUnderTranslation f1; double v1 = f1.calcDistance(c1, c2);
		FrechetUnderTranslation f2; double v2 = f2.calcDistance2(c1, c2);
		std::cout << a << " " << b << " " << v1 << " " << v2 << "\n" << std::flush;
	}
}
