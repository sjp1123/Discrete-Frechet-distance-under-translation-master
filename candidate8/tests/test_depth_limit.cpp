// Depth-limit family (report items B and C): pi = one point, sigma = K clusters
// of m exactly repeated points.  Near delta* more than CUT_LIMIT (12) cut discs
// stay alive in every box (the duplicates count separately), so the decider
// descends to the depth limit (40).  Truth: YES iff MEC-radius(centres) <= delta.
//
//   test_depth_limit <seed0> <count> <m> [scale]   (m < 0: cluster-major order)
//     delta = r*(1+u), u log-uniform in [1e-9, 1e-5]; prints FAIL on a NO.
//     Each query runs under a 5 s alarm (TEST_ALARM=<s> overrides); a hang prints HANG and exits 3.
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <csignal>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <unistd.h>
static unsigned alarm_secs(unsigned d) { const char* e = std::getenv("TEST_ALARM"); return e ? (unsigned)atoi(e) : d; }
#include <vector>
struct P { double x, y; };
static long g_seed = -1;
static void on_alarm(int) { std::printf("HANG seed=%ld\n", g_seed); std::fflush(stdout); _exit(3); }
static bool inAll(std::vector<P> const& c, double cx, double cy, double r2) {
	for (auto& p : c) { double dx = p.x - cx, dy = p.y - cy; if (dx*dx + dy*dy > r2 * (1 + 1e-12)) return false; }
	return true;
}
static double mec(std::vector<P> const& c) {
	double best = 1e300; int n = c.size();
	for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) {
		double cx = (c[i].x + c[j].x) / 2, cy = (c[i].y + c[j].y) / 2, r2 = (c[i].x - cx)*(c[i].x - cx) + (c[i].y - cy)*(c[i].y - cy);
		if (r2 < best*best && inAll(c, cx, cy, r2)) best = std::sqrt(r2);
	}
	for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) for (int k = j + 1; k < n; k++) {
		double ax = c[i].x, ay = c[i].y, bx = c[j].x, by = c[j].y, qx = c[k].x, qy = c[k].y;
		double d = 2 * (ax*(by - qy) + bx*(qy - ay) + qx*(ay - by)); if (std::fabs(d) < 1e-15) continue;
		double ux = ((ax*ax + ay*ay)*(by - qy) + (bx*bx + by*by)*(qy - ay) + (qx*qx + qy*qy)*(ay - by)) / d;
		double uy = ((ax*ax + ay*ay)*(qx - bx) + (bx*bx + by*by)*(ax - qx) + (qx*qx + qy*qy)*(bx - ax)) / d;
		double r2 = (ax - ux)*(ax - ux) + (ay - uy)*(ay - uy); if (r2 < best*best && inAll(c, ux, uy, r2)) best = std::sqrt(r2);
	}
	return best;
}
int main(int argc, char** argv) {
	long s0 = atol(argv[1]), cnt = atol(argv[2]); int m = atoi(argv[3]); double scale = argc > 4 ? atof(argv[4]) : 1.0;
	// m < 0: |m| copies of every cluster, cluster-major order (no shuffle), so the
	// first CUT_LIMIT+1 cut discs can all be copies of one cluster.
	bool const shuffle = m > 0; if (m < 0) m = -m;
	std::signal(SIGALRM, on_alarm);
	long fails = 0;
	for (long s = s0; s < s0 + cnt; ++s) {
		g_seed = s;
		std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0, 1);
		int K = 3 + (int)(U(g) * 3);
		std::vector<P> cen(K);
		for (auto& p : cen) { p.x = std::round(U(g) * 1e4) / 1e4 * scale; p.y = std::round(U(g) * 1e4) / 1e4 * scale; }
		Curve c1, c2; c1.push_back({0, 0});
		std::vector<int> order; for (int k = 0; k < K; k++) for (int t = 0; t < m; t++) order.push_back(k);
			if (shuffle) std::shuffle(order.begin(), order.end(), g);
		for (int k : order) c2.push_back({-cen[k].x, -cen[k].y});
		double r = mec(cen), u = std::pow(10., -9 + 4 * U(g)), delta = r * (1 + u);
		alarm(alarm_secs(5));
		FrechetUnderTranslation f; bool ans = f.lessThan(delta, c1, c2);
		alarm(0);
		if (!ans) { ++fails; std::printf("FAIL seed=%ld K=%d u=%.3g\n", s, K, u); }
	}
	std::printf("DONE fails=%ld of %ld\n", fails, cnt);
}
