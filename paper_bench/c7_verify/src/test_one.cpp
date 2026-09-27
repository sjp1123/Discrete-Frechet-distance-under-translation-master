// E1-tests: replay one family-5 instance of test_exact_ref (same RNG draws) step by step,
// flushing before every library call so an abort or hang can be attributed.
//   test_one <seed> [skip_value]
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
struct P { double x, y; };
static double dfd_at(std::vector<P> const& a, std::vector<P> const& b, double tx, double ty) {
	std::size_t n = a.size(), m = b.size(); std::vector<double> D(n * m);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < m; ++j) {
		double dx = a[i].x - b[j].x - tx, dy = a[i].y - b[j].y - ty, d = dx*dx + dy*dy, best;
		if (i == 0 && j == 0) best = 0; else if (i == 0) best = D[j-1]; else if (j == 0) best = D[(i-1)*m];
		else best = std::min(D[(i-1)*m + j], std::min(D[(i-1)*m + j-1], D[i*m + j-1]));
		D[i*m + j] = std::max(best, d);
	}
	return std::sqrt(D[n*m - 1]);
}
static double ref_value(std::vector<P> const& a, std::vector<P> const& b) {
	std::vector<P> c; for (auto& p : a) for (auto& q : b) c.push_back({p.x - q.x, p.y - q.y});
	double best = 1e300; auto tryp = [&](double x, double y){ best = std::min(best, dfd_at(a, b, x, y)); };
	std::size_t n = c.size();
	for (std::size_t i = 0; i < n; ++i) tryp(c[i].x, c[i].y);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = i+1; j < n; ++j) tryp((c[i].x + c[j].x)/2, (c[i].y + c[j].y)/2);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = i+1; j < n; ++j) for (std::size_t k = j+1; k < n; ++k) {
		double ax = c[i].x, ay = c[i].y, bx = c[j].x, by = c[j].y, qx = c[k].x, qy = c[k].y;
		double d = 2*(ax*(by-qy) + bx*(qy-ay) + qx*(ay-by)); if (d == 0) continue;
		tryp(((ax*ax+ay*ay)*(by-qy) + (bx*bx+by*by)*(qy-ay) + (qx*qx+qy*qy)*(ay-by))/d,
		     ((ax*ax+ay*ay)*(qx-bx) + (bx*bx+by*by)*(ax-qx) + (qx*qx+qy*qy)*(bx-ax))/d);
	}
	return best;
}
int main(int argc, char** argv) {
	long s = atol(argv[1]); int fam = 5; bool skipv = argc > 2;
	std::cout << std::setprecision(17);
	std::mt19937_64 g(s * 7919 + fam); std::uniform_real_distribution<double> U(0, 1);
	std::vector<P> a, b;
	int K1 = 1 + (int)(U(g)*3), K2 = 2 + (int)(U(g)*4);
	for (int k = 0; k < K1; ++k) { double x = (int)(U(g)*6)/8., y = (int)(U(g)*6)/8.; int m = 1 + (int)(U(g)*5); for (int t = 0; t < m; ++t) a.push_back({x, y}); }
	for (int k = 0; k < K2; ++k) { double x = (int)(U(g)*6)/8., y = (int)(U(g)*6)/8.; int m = 1 + (int)(U(g)*5); for (int t = 0; t < m; ++t) b.push_back({x, y}); }
	std::cout << "pi:"; for (auto& p : a) std::cout << " (" << p.x << "," << p.y << ")"; std::cout << "\n";
	std::cout << "sigma:"; for (auto& p : b) std::cout << " (" << p.x << "," << p.y << ")"; std::cout << std::endl;
	Curve c1, c2; for (auto& p : a) c1.push_back({p.x, p.y}); for (auto& p : b) c2.push_back({p.x, p.y});
	double ref = ref_value(a, b);
	std::cout << "ref=" << ref << std::endl;
	if (!skipv) { std::cout << "calcDistance2 ..." << std::endl; FrechetUnderTranslation f; double v = f.calcDistance2(c1, c2); std::cout << "value=" << v << std::endl; }
	const double us[] = {1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1};
	for (double u : us) {
		if (ref * u <= 2e-7) continue;
		for (int sgn = -1; sgn <= 1; sgn += 2) {
			double d = ref * (1 + sgn*u);
			std::cout << "lessThan(" << d << ") u=" << sgn*u << " ..." << std::flush;
			FrechetUnderTranslation f; bool ans = f.lessThan(d, c1, c2);
			std::cout << " " << ans << std::endl;
		}
	}
	std::cout << "END" << std::endl;
}
