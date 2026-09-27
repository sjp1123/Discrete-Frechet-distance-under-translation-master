// E1-tests: sub-Frechet decider false YES when delta equals the end-pair distance (seed-4697 instance).
#include "defs.h"
#include "curves.h"
#include "discrete_frechet_queries.h"
#include <iostream>
#include <iomanip>
#include <cmath>
int main() {
	std::cout << std::setprecision(17);
	Curve c1, c2;
	for (int i = 0; i < 5; ++i) c1.push_back({0.625, 0.125});
	for (int i = 0; i < 2; ++i) c1.push_back({0.625, 0});
	c2.push_back({0.375, 0.375}); for (int i = 0; i < 3; ++i) c2.push_back({0.375, 0.5});
	for (int i = 0; i < 5; ++i) c2.push_back({0.125, 0}); for (int i = 0; i < 2; ++i) c2.push_back({0.25, 0.125});
	Point center1 = c1.front() - c2.front();
	c2.translate(center1);
	double e = c1.back().dist(c2[c2.size()-1]);
	std::cout << "end-pair distance at center1 = " << e << " ; true DFD = " << std::sqrt(0.125) << "\n";
	for (double d : {std::nextafter(e, 0.), e, std::nextafter(e, 1.), e*1.01, 0.2, 0.3}) { DiscreteFrechetQueries q(1e-8); std::cout << "  lt(" << d << ") = " << q.lessThanFixedTranslation(d, c1, c2) << "\n"; }
	// minimal variants
	Curve a, b; a.push_back({0,0}); a.push_back({0,0}); a.push_back({1,0});
	b.push_back({0,0}); b.push_back({0,5}); b.push_back({1,1});
	for (double d : {1.0, 0.999, 1.001, 2.0}) { DiscreteFrechetQueries q(1e-8); std::cout << "  minimal lt(" << d << ") = " << q.lessThanFixedTranslation(d, a, b) << " (true DFD = 5)\n"; }
}
