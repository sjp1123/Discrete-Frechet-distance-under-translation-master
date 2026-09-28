// Large-value family (report item D): the single-point family scaled so that
// delta* is 1e7..1e8.  Above ~3.4e7 one ulp exceeds the base case's epsilon/2
// (5e-9), so a binary search on [min, max] can stop making progress.  Each LMF
// call runs under a 20 s alarm; a hang prints HANG and exits 3.
//
//   test_large_values <seed0> <count> <scale>   (scale 1e8 -> delta* ~ 1e7..6e7)
//   prints "S seed value exact_MEC_radius relative_error"
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <csignal>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <unistd.h>
static unsigned alarm_secs(unsigned d) { const char* e = std::getenv("TEST_ALARM"); return e ? (unsigned)atoi(e) : d; }
struct P { double x, y; };
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
static long g_seed = -1;
static void on_alarm(int) { std::printf("HANG seed=%ld\n", g_seed); std::fflush(stdout); _exit(3); }
int main(int argc, char** argv) {
	long s0 = atol(argv[1]), cnt = atol(argv[2]); double scale = atof(argv[3]);
	std::signal(SIGALRM, on_alarm);
	for (long s = s0; s < s0 + cnt; ++s) {
		g_seed = s;
		std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0, 1);
		Curve c1, c2; c1.push_back({0, 0});
		std::vector<P> cen;
		for (int k = 0; k < 4; k++) {
			double x = std::round(U(g) * 1e4) / 1e4 * scale, y = std::round(U(g) * 1e4) / 1e4 * scale;
			c2.push_back({-x, -y}); cen.push_back({x, y});
		}
		alarm(alarm_secs(20));
		FrechetUnderTranslation f; double v = f.calcDistance2(c1, c2);
		alarm(0);
		double const r = mec(cen);
		std::printf("S %ld %.17g %.17g %.3g\n", s, v, r, (v - r) / r);
	}
	std::printf("DONE\n");
}
