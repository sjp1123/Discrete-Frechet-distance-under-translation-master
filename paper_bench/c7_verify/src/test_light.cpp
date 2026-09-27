// E1-tests: sub-Frechet decider (DiscreteFrechetQueries / DiscreteFrechetLight, unchanged from
// original) against a brute-force DP, with delta equal to (or one ulp around) a point-pair distance.
//   test_light <seed0> <count> <grid_denominator> <max_repeat>
// Counts only unambiguous mismatches: |DFD - delta| > 1e-9.  Also replays the FUT decider on
// family-5 seed 4697 at delta = sqrt(2)/8.
#include "defs.h"
#include "curves.h"
#include "discrete_frechet_queries.h"
#include "frechet_under_translation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
static double dfd(Curve const& a, Curve const& b) {   // b already translated through operator[]
	std::size_t n = a.size(), m = b.size(); std::vector<double> D(n * m);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < m; ++j) {
		double d = a[i].dist(b[j]), best;
		if (i == 0 && j == 0) best = 0; else if (i == 0) best = D[j-1]; else if (j == 0) best = D[(i-1)*m];
		else best = std::min(D[(i-1)*m + j], std::min(D[(i-1)*m + j-1], D[i*m + j-1]));
		D[i*m + j] = std::max(best, d);
	}
	return D[n*m - 1];
}
int main(int argc, char** argv) {
	long s0 = atol(argv[1]), cnt = atol(argv[2]); int den = atoi(argv[3]); int rep = atoi(argv[4]);
	std::cout << std::setprecision(17);
	long q = 0, bad_yes = 0, bad_no = 0, shown = 0;
	for (long s = s0; s < s0 + cnt; ++s) {
		std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0, 1);
		Curve c1, c2;
		int k1 = 1 + (int)(U(g)*4), k2 = 1 + (int)(U(g)*4);
		for (int k = 0; k < k1; ++k) { double x = (int)(U(g)*den)/(double)den, y = (int)(U(g)*den)/(double)den; int m = 1 + (int)(U(g)*rep); for (int t = 0; t < m; ++t) c1.push_back({x, y}); }
		for (int k = 0; k < k2; ++k) { double x = (int)(U(g)*den)/(double)den, y = (int)(U(g)*den)/(double)den; int m = 1 + (int)(U(g)*rep); for (int t = 0; t < m; ++t) c2.push_back({x, y}); }
		Point tau{(int)(U(g)*2*den - den)/(double)den, (int)(U(g)*2*den - den)/(double)den};
		c2.translate(tau);
		double truth = dfd(c1, c2);
		std::vector<double> ds;
		for (std::size_t i = 0; i < c1.size(); ++i) for (std::size_t j = 0; j < c2.size(); ++j) { double d = c1[i].dist(c2[j]); ds.push_back(d); ds.push_back(std::nextafter(d, 0.)); ds.push_back(std::nextafter(d, 10.)); ds.push_back(d/2); ds.push_back(d*2); }
		std::sort(ds.begin(), ds.end()); ds.erase(std::unique(ds.begin(), ds.end()), ds.end());
		for (double d : ds) {
			if (std::fabs(truth - d) <= 1e-9) continue;
			DiscreteFrechetQueries fq(1e-8); bool ans = fq.lessThanFixedTranslation(d, c1, c2); ++q;
			bool t = truth <= d;
			if (ans != t) { if (ans) ++bad_yes; else ++bad_no;
				if (shown++ < 6) { std::cout << "MISMATCH seed=" << s << " delta=" << d << " truth_dfd=" << truth << " light=" << ans << "\n  pi:";
					for (std::size_t i = 0; i < c1.size(); ++i) std::cout << " (" << c1[i].x << "," << c1[i].y << ")";
					std::cout << "\n  sigma+tau:"; for (std::size_t j = 0; j < c2.size(); ++j) std::cout << " (" << c2[j].x << "," << c2[j].y << ")"; std::cout << "\n"; } }
		}
	}
	std::cout << "DONE queries=" << q << " false_yes=" << bad_yes << " false_no=" << bad_no << "\n";
	// FUT decider replay on family-5 seed 4697 (delta* = 0.22534695471649932)
	Curve p, r;
	for (int i = 0; i < 5; ++i) p.push_back({0.625, 0.125}); for (int i = 0; i < 2; ++i) p.push_back({0.625, 0});
	r.push_back({0.375, 0.375}); for (int i = 0; i < 3; ++i) r.push_back({0.375, 0.5});
	for (int i = 0; i < 5; ++i) r.push_back({0.125, 0}); for (int i = 0; i < 2; ++i) r.push_back({0.25, 0.125});
	for (double d : {std::sqrt(2.)/8., 0.17677669529663689, 0.17677669529663687, 0.2, 0.22}) {
		FrechetUnderTranslation f; bool a = f.lessThan(d, p, r);
		std::cout << "FUT lessThan(" << d << ") on seed-4697 curves (delta*=0.225347) = " << a;
		if (a) { Point t = f.getTranslation(); r.translate(t); std::cout << "  witness=(" << t.x << "," << t.y << ") dfd(witness)=" << dfd(p, r); r.resetTranslation(); }
		std::cout << "\n";
	}
}
