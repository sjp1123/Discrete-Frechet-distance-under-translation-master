// Tangency-window family (report item A).  Both curves sit at Sigspatial-like
// raw coordinates (offset 1.36e7, so the decider rounds at ~1e-9), pi has 2-3
// points and sigma 3-4.  With v the LMF value, the decider is queried at every
// radius where a pair of discs becomes tangent (|c_i - c_j|/2) or a triple
// becomes concurrent (circumradius), restricted to [v + 1e-6, 1.05 v], and one ulp
// either side.  Those are the radii at which a new maximal set appears whose
// only common points are near-tangent, i.e. whose witness has no margin.
// Every answer must be YES (delta > delta*).
//
//   test_tangency_windows <seed0> <count> [far] [scale]   ->  "S seed v queries no"
//     scale multiplies the raw-coordinate offset (default 1: Sigspatial, ~1.4e7)
//     far = 1: pi near the origin and sigma at the offset, so the translation
//     (and every disc centre) is ~1.4e7 as well, not only the raw coordinates.
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
int main(int argc, char** argv) {
	long s0 = atol(argv[1]), cnt = atol(argv[2]); bool const far = argc > 3 && atoi(argv[3]) == 1;
	double const scale = argc > 4 ? atof(argv[4]) : 1.0;   // multiplies the raw-coordinate offset
	long total_no = 0, total_q = 0;
	for (long s = s0; s < s0 + cnt; ++s) {
		std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0, 1);
		double const ox = -13.6e6 * scale, oy = 4.56e6 * scale;
		int n1 = 2 + (int)(U(g) * 2), n2 = 3 + (int)(U(g) * 2);
		Curve c1, c2;
		double const px = far ? 0.0 : ox, py = far ? 0.0 : oy;
		for (int i = 0; i < n1; i++) c1.push_back({px + std::round(U(g) * 1e7) / 100, py + std::round(U(g) * 1e7) / 100});
		for (int i = 0; i < n2; i++) c2.push_back({ox + std::round(U(g) * 1e7) / 100, oy + std::round(U(g) * 1e7) / 100});
		FrechetUnderTranslation f; double v = f.calcDistance2(c1, c2);
		std::vector<double> cx, cy;
		for (int i = 0; i < n1; i++) for (int j = 0; j < n2; j++) { auto c = c1[i] - c2[j]; cx.push_back(c.x); cy.push_back(c.y); }
		std::vector<double> rad;
		int const m = cx.size();
		for (int i = 0; i < m; i++) for (int j = i + 1; j < m; j++) rad.push_back(std::hypot(cx[i] - cx[j], cy[i] - cy[j]) / 2);
		for (int i = 0; i < m; i++) for (int j = i + 1; j < m; j++) for (int k = j + 1; k < m; k++) {
			double a = std::hypot(cx[j] - cx[k], cy[j] - cy[k]), b = std::hypot(cx[i] - cx[k], cy[i] - cy[k]), c = std::hypot(cx[i] - cx[j], cy[i] - cy[j]);
			double area2 = std::fabs((cx[j] - cx[i]) * (cy[k] - cy[i]) - (cy[j] - cy[i]) * (cx[k] - cx[i]));
			if (area2 > 0) rad.push_back(a * b * c / (2 * area2));
		}
		int q = 0, no = 0;
		for (double R : rad) {
			if (R < v + 1e-6 || R > 1.05 * v) continue;
			for (double d : {std::nextafter(R, 0.0), R, std::nextafter(R, 1e300)}) {
				FrechetUnderTranslation dq; ++q;
				if (!dq.lessThan(d, c1, c2)) ++no;
			}
		}
		total_q += q; total_no += no;
		std::printf("S %ld %.17g %d %d\n", s, v, q, no);
	}
	std::printf("DONE queries=%ld no=%ld\n", total_q, total_no);
}
