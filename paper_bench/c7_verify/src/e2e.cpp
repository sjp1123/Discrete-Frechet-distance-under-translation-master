// F09 end-to-end: LMF (calcDistance2) on concyclic lattice points far from origin.
// e2e <log2 X> <R> <stride> [closed=1]
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
int main(int argc, char** argv) {
	int e = atoi(argv[1]); long R = atol(argv[2]); int stride = atoi(argv[3]);
	int closed = argc > 4 ? atoi(argv[4]) : 1;
	double X = std::ldexp(1.0, e);
	std::vector<std::pair<long,long>> P;
	for (long a = -R; a <= R; ++a) { long b2 = R*R - a*a; long b = (long)std::llround(std::sqrt((double)b2));
		for (long bb = b-1; bb <= b+1; ++bb) if (bb >= 0 && bb*bb == b2) { P.push_back({a, bb}); if (bb) P.push_back({a, -bb}); } }
	std::sort(P.begin(), P.end()); P.erase(std::unique(P.begin(), P.end()), P.end());
	std::sort(P.begin(), P.end(), [](auto const& u, auto const& v){ return std::atan2((double)u.second,(double)u.first) < std::atan2((double)v.second,(double)v.first); });
	Curve c1, c2;
	std::size_t n = 0;
	for (std::size_t i = 0; i < P.size(); i += stride) { c1.push_back({X + P[i].first, X + P[i].second}); ++n; }
	if (closed) c1.push_back(c1[0]);
	c2.push_back({0, 0});
	std::printf("lattice pts=%zu used=%zu X=2^%d ulp=%g R=%ld ; expected value = R\n", P.size(), n, e, std::nextafter(X, 1e300) - X, R);
	std::fflush(stdout);
	FrechetUnderTranslation f;
	double v = f.calcDistance2(c1, c2);
	std::printf("calcDistance2 = %.17g  err = %g\nDONE\n", v, v - (double)R);
	return 0;
}
