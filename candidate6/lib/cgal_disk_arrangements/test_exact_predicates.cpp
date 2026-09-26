// Build: g++ -O2 -std=c++17 -I. -I../../src test_exact_predicates.cpp -o test_exact_predicates
// Result (seed 42): P3 400000 boundary cases, 399978 rational fallbacks, 0 wrong-when-certified;
//                   P2 400000 boundary cases, 400000 rational fallbacks, 0 wrong-when-certified.
// Empirical check of the filter bounds: whenever pred2_exact/pred3_exact decide
// in double (no rational fallback), the answer must equal the rational answer.
#include "maximal_regions.cpp"
#include <random>
using namespace cgal_disk_arrangements::maxregion;
int main() {
	std::mt19937_64 rng(42);
	std::uniform_real_distribution<double> coord(-5000.0, 5000.0), scale(1e-3, 1e3), tiny(-1.0, 1.0);
	long long n3 = 0, bad3 = 0, ex3 = 0, n2 = 0, bad2 = 0, ex2 = 0;
	for (int it = 0; it < 400000; ++it) {
		Stats S;
		double ax = coord(rng), ay = coord(rng), bx = coord(rng), by = coord(rng), cx = coord(rng), cy = coord(rng);
		if (it % 4 == 1) { // near-collinear / near-right / sliver families
			double t = tiny(rng); bx = ax + (cx-ax)*0.37 + 1e-9*t; by = ay + (cy-ay)*0.37 + 1e-9*t;
		}
		if (it % 4 == 2) { double s = scale(rng); bx = ax + s; by = ay; cx = ax; cy = ay + s * (1 + 1e-13*tiny(rng)); }
		// choose r AT the boundary: r² = exact MEC radius² (rational) rounded to double, ± ulps
		Q qa2 = (Q(cx)-Q(bx))*(Q(cx)-Q(bx)) + (Q(cy)-Q(by))*(Q(cy)-Q(by));
		Q qb2 = (Q(cx)-Q(ax))*(Q(cx)-Q(ax)) + (Q(cy)-Q(ay))*(Q(cy)-Q(ay));
		Q qc2 = (Q(bx)-Q(ax))*(Q(bx)-Q(ax)) + (Q(by)-Q(ay))*(Q(by)-Q(ay));
		Q hi = qa2, s1 = qb2, s2 = qc2;
		if (qb2 >= qa2 && qb2 >= qc2) { hi = qb2; s1 = qa2; s2 = qc2; } else if (qc2 >= qa2 && qc2 >= qb2) { hi = qc2; s1 = qa2; s2 = qb2; }
		Q cross = (Q(bx)-Q(ax))*(Q(cy)-Q(ay)) - (Q(by)-Q(ay))*(Q(cx)-Q(ax));
		Q four_r2_exact = (hi >= s1 + s2 || cross == 0) ? hi : qa2*qb2*qc2 / (cross*cross);
		double r = std::sqrt(four_r2_exact.convert_to<double>() / 4.0);
		int k = (int)(rng() % 7) - 3;  // -3..3 ulps
		for (int u = 0; u < std::abs(k); ++u) r = std::nextafter(r, k > 0 ? 1e300 : -1e300);
		double four_r2 = 4.0 * (r * r);
		bool got = pred3_exact(ax, ay, bx, by, cx, cy, r, four_r2, S);
		bool ref = pred3_rational(ax, ay, bx, by, cx, cy, r);
		++n3; ex3 += S.p3_exact; if (S.p3_exact == 0 && got != ref) ++bad3;
		// P2 at the boundary
		double dx = bx - ax, dy = by - ay; double r2p = std::sqrt((dx*dx + dy*dy)) / 2.0;
		for (int u = 0; u < std::abs(k); ++u) r2p = std::nextafter(r2p, k > 0 ? 1e300 : -1e300);
		Stats S2; bool g2 = pred2_exact(ax, ay, bx, by, r2p, 4.0*(r2p*r2p), S2);
		Q qr = Q(r2p); bool ref2 = (Q(bx)-Q(ax))*(Q(bx)-Q(ax)) + (Q(by)-Q(ay))*(Q(by)-Q(ay)) <= 4*qr*qr;
		++n2; ex2 += S2.p2_exact; if (S2.p2_exact == 0 && g2 != ref2) ++bad2;
	}
	std::printf("P3: %lld boundary cases, rational fallback %lld, wrong-when-certified %lld\n", n3, ex3, bad3);
	std::printf("P2: %lld boundary cases, rational fallback %lld, wrong-when-certified %lld\n", n2, ex2, bad2);
	return (bad2 || bad3) ? 1 : 0;
}
