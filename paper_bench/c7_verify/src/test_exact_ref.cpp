// E1-tests: exact brute-force reference for DFD under translation on small inputs.
// delta* = min over candidate translations tau (every distinct centre c = pi_i - sigma_j,
// every pair midpoint, every triple circumcentre) of DFD(pi, sigma + tau).  (For a fixed
// coupling the optimum is the MEC of its centres, determined by <= 3 centres.)
//
//   test_exact_ref <family> <seed0> <count> [v]
//   family: 1 single-point clusters (6,6)   2 clustered family (as test_clustered_family)
//           3 random curves n1,n2 in [1,8] on 1e-4 grid   4 coarse grid (ties, co-circularity)
//           5 clustered both sides, coarse grid
// Prints VALERR when |calcDistance2 - ref| > 1e-7, DYES when lessThan(ref(1-u)) is YES with
// ref*u > 2e-7, DNO when lessThan(ref(1+u)) is NO with ref*u > 2e-7, WITBAD when a YES
// translation has DFD > delta + 1e-8.
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
	std::size_t n = a.size(), m = b.size();
	std::vector<double> D(n * m);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < m; ++j) {
		double dx = a[i].x - b[j].x - tx, dy = a[i].y - b[j].y - ty;
		double d = dx*dx + dy*dy, best;
		if (i == 0 && j == 0) best = 0;
		else if (i == 0) best = D[j-1];
		else if (j == 0) best = D[(i-1)*m];
		else best = std::min(D[(i-1)*m + j], std::min(D[(i-1)*m + j-1], D[i*m + j-1]));
		D[i*m + j] = std::max(best, d);
	}
	return std::sqrt(D[n*m - 1]);
}
static double ref_value(std::vector<P> const& a, std::vector<P> const& b, double& bx, double& by) {
	std::vector<P> c;
	for (auto& p : a) for (auto& q : b) c.push_back({p.x - q.x, p.y - q.y});
	std::sort(c.begin(), c.end(), [](P const& u, P const& v){ return u.x < v.x || (u.x == v.x && u.y < v.y); });
	c.erase(std::unique(c.begin(), c.end(), [](P const& u, P const& v){ return u.x == v.x && u.y == v.y; }), c.end());
	double best = 1e300;
	auto tryp = [&](double x, double y){ double v = dfd_at(a, b, x, y); if (v < best) { best = v; bx = x; by = y; } };
	std::size_t n = c.size();
	for (std::size_t i = 0; i < n; ++i) tryp(c[i].x, c[i].y);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = i+1; j < n; ++j) tryp((c[i].x + c[j].x)/2, (c[i].y + c[j].y)/2);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = i+1; j < n; ++j) for (std::size_t k = j+1; k < n; ++k) {
		double ax = c[i].x, ay = c[i].y, bx2 = c[j].x, by2 = c[j].y, qx = c[k].x, qy = c[k].y;
		double d = 2*(ax*(by2-qy) + bx2*(qy-ay) + qx*(ay-by2)); if (d == 0) continue;
		double ux = ((ax*ax+ay*ay)*(by2-qy) + (bx2*bx2+by2*by2)*(qy-ay) + (qx*qx+qy*qy)*(ay-by2))/d;
		double uy = ((ax*ax+ay*ay)*(qx-bx2) + (bx2*bx2+by2*by2)*(ax-qx) + (qx*qx+qy*qy)*(bx2-ax))/d;
		tryp(ux, uy);
	}
	return best;
}
int main(int argc, char** argv) {
	int fam = atoi(argv[1]); long s0 = atol(argv[2]), cnt = atol(argv[3]); bool verbose = argc > 4;
	std::cout << std::setprecision(17);
	long valerr = 0, dyes = 0, dno = 0, witbad = 0, dq = 0; double maxdv = 0;
	const double us[] = {1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1};
	for (long s = s0; s < s0 + cnt; ++s) {
		std::mt19937_64 g(s * 7919 + fam); std::uniform_real_distribution<double> U(0, 1);
		std::vector<P> a, b;
		if (fam == 1) {
			a.push_back({0, 0}); int K = 3 + (int)(U(g)*4);
			for (int k = 0; k < K; ++k) { double x = std::round(U(g)*1e4)/1e4, y = std::round(U(g)*1e4)/1e4; int m = 1 + (int)(U(g)*6); for (int t = 0; t < m; ++t) b.push_back({-x, -y}); }
			std::shuffle(b.begin(), b.end(), g);
		} else if (fam == 2) {
			int n1 = 2 + (int)(U(g)*3); double spread = 0.05 + 0.4*U(g);
			for (int i = 0; i < n1; ++i) a.push_back({std::round(U(g)*spread*1e4)/1e4, std::round(U(g)*spread*1e4)/1e4});
			int K = 3 + (int)(U(g)*4);
			for (int k = 0; k < K; ++k) { double x = std::round(U(g)*1e4)/1e4, y = std::round(U(g)*1e4)/1e4; int m = 1 + (int)(U(g)*6); for (int t = 0; t < m; ++t) b.push_back({-x, -y}); }
		} else if (fam == 3) {
			int n1 = 1 + (int)(U(g)*8), n2 = 1 + (int)(U(g)*8);
			for (int i = 0; i < n1; ++i) a.push_back({std::round(U(g)*1e4)/1e4, std::round(U(g)*1e4)/1e4});
			for (int i = 0; i < n2; ++i) b.push_back({std::round(U(g)*1e4)/1e4, std::round(U(g)*1e4)/1e4});
		} else if (fam == 4) {
			int n1 = 1 + (int)(U(g)*7), n2 = 1 + (int)(U(g)*7);
			for (int i = 0; i < n1; ++i) a.push_back({(double)(int)(U(g)*6)/8, (double)(int)(U(g)*6)/8});
			for (int i = 0; i < n2; ++i) b.push_back({(double)(int)(U(g)*6)/8, (double)(int)(U(g)*6)/8});
		} else {
			int K1 = 1 + (int)(U(g)*3), K2 = 2 + (int)(U(g)*4);
			for (int k = 0; k < K1; ++k) { double x = (int)(U(g)*6)/8., y = (int)(U(g)*6)/8.; int m = 1 + (int)(U(g)*5); for (int t = 0; t < m; ++t) a.push_back({x, y}); }
			for (int k = 0; k < K2; ++k) { double x = (int)(U(g)*6)/8., y = (int)(U(g)*6)/8.; int m = 1 + (int)(U(g)*5); for (int t = 0; t < m; ++t) b.push_back({x, y}); }
		}
		Curve c1, c2; for (auto& p : a) c1.push_back({p.x, p.y}); for (auto& p : b) c2.push_back({p.x, p.y});
		double tx, ty; double ref = ref_value(a, b, tx, ty);
		bool bad = false;
		{ FrechetUnderTranslation f; double v = f.calcDistance2(c1, c2); double dv = v - ref; if (std::fabs(dv) > maxdv) maxdv = std::fabs(dv);
		  if (std::fabs(dv) > 1e-7) { ++valerr; bad = true; std::cout << "VALERR seed=" << s << " ref=" << ref << " v=" << v << " d=" << dv << "\n"; } }
		for (double u : us) {
			if (ref * u <= 2e-7) continue;
			for (int sgn = -1; sgn <= 1; sgn += 2) {
				double d = ref * (1 + sgn*u); ++dq;
				FrechetUnderTranslation f; bool ans = f.lessThan(d, c1, c2);
				if (ans) { Point t = f.getTranslation(); double w = dfd_at(a, b, t.x, t.y);
					if (w > d + 1e-8) { ++witbad; bad = true; std::cout << "WITBAD seed=" << s << " delta=" << d << " dfd(t)=" << w << "\n"; } }
				if (sgn < 0 && ans) { ++dyes; bad = true; std::cout << "DYES seed=" << s << " ref=" << ref << " u=" << u << "\n"; }
				if (sgn > 0 && !ans) { ++dno; bad = true; std::cout << "DNO seed=" << s << " ref=" << ref << " u=" << u << "\n"; }
			}
		}
		if (bad && verbose) {
			std::cout << "  pi:"; for (auto& p : a) std::cout << " (" << p.x << "," << p.y << ")"; std::cout << "\n";
			std::cout << "  sigma:"; for (auto& p : b) std::cout << " (" << p.x << "," << p.y << ")"; std::cout << "\n";
			std::cout << "  ref_tau=(" << tx << "," << ty << ")\n";
		}
	}
	std::cout << "DONE fam=" << fam << " count=" << cnt << " valerr=" << valerr << " dyes=" << dyes << " dno=" << dno
	          << " witbad=" << witbad << " dqueries=" << dq << " max|v-ref|=" << maxdv << "\n";
}
