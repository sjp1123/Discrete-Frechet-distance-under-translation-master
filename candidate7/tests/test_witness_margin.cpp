// Witness margin against the decider's own rounding (audit item A and its follow-up).
//
// Three discs of radius r meet in (almost) one point: r is the circumradius of an acute triangle of
// centres, rounded up by a few ulps.  Each centre is c = p - q with p near the origin and q at raw
// coordinates around `offset` (default 1.36e7, Sigspatial), exactly as the decider sees a
// translation t: it tests |p - fl(q + t)| <= fl(r + slack).  For the region holding all three discs
// we replay that test in double on the witness enumerate_box emits, and count the regions sent to
// the box-arrangement fallback instead.
//
// candidate7 accepts a double witness only within r and otherwise rounds the exact optimum.  The
// _nofix build keeps the whole slack as the tolerance of that rounded witness (candidate7 as first
// merged); the default build subtracts the decider's rounding bound 2u(|p|max + R) + 6uR (follow-up), so
// a witness, double or rounded exact, without that margin also sends its discs to the fallback.  At
// Sigspatial scale both give 0 rejections and identical witnesses; the difference shows once
// |p|max + R passes ~4e7, where the bound exceeds the slack.
//
//   test_witness_margin <count> [edge] [offset] [same_city]
//     edge = 1: the concurrency point lies just outside the box, so the witness is an edge point.
#include "maximal_regions.cpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
using namespace cgal_disk_arrangements;
using namespace cgal_disk_arrangements::maxregion;
int main(int argc, char** argv) {
	long const cnt = atol(argv[1]);
	bool const edge_mode = argc > 2 && atoi(argv[2]) == 1;
	double const offset = argc > 3 ? atof(argv[3]) : 1.36e7;
	// same_city = 1: both curves at the offset and the disc centres (translations) small, as for two
	// Sigspatial trajectories; 0: p near the origin, so the translations are ~ -offset.
	bool const same_city = argc > 4 && atoi(argv[4]) == 1;
	double const slack = 9e-9;
	std::mt19937_64 g(12345); std::uniform_real_distribution<double> U(0, 1);
	long nw = 0, rej = 0, cases = 0, fallback = 0;
	for (long it = 0; it < cnt; ++it) {
		// acute triangle of centres, scale ~4e4, positioned at ~ -1.36e7
		double const R0 = 2e4 + 4e4 * U(g), a0 = 2 * M_PI * U(g);
		double ang[3] = {a0, a0 + 2.0 + 0.3 * U(g), a0 + 4.0 + 0.3 * U(g)};
		double const qx0 = offset + 1e5 * U(g), qy0 = offset / 3 + 1e5 * U(g);
		double px[3], py[3], qx[3], qy[3]; Discs d;
		double const ccx = 2e3 * U(g), ccy = 2e3 * U(g);      // p near the origin, so centres (translations) are ~ -1.36e7
		for (int k = 0; k < 3; ++k) {
			qx[k] = qx0 + 1e3 * U(g); qy[k] = qy0 + 1e3 * U(g);
			// p = c + q with c the k-th centre around the circumcentre (ccx - qx0, ccy - qy0)
			px[k] = (same_city ? ccx : ccx - qx0) + R0 * std::cos(ang[k]) + qx[k];
			py[k] = (same_city ? ccy : ccy - qy0) + R0 * std::sin(ang[k]) + qy[k];
		}
		// the centres the algorithm sees are the rounded differences
		double cx[3], cy[3];
		for (int k = 0; k < 3; ++k) { cx[k] = px[k] - qx[k]; cy[k] = py[k] - qy[k]; }
		// exact-ish circumradius (long double) of the rounded centres, then + a few ulps
		long double ax = cx[0], ay = cy[0], bx = cx[1], by = cy[1], ex = cx[2], ey = cy[2];
		long double A = std::hypot((long double)(bx - ex), (long double)(by - ey)), B = std::hypot((long double)(ax - ex), (long double)(ay - ey)), C = std::hypot((long double)(ax - bx), (long double)(ay - by));
		long double area2 = std::fabs((bx - ax) * (ey - ay) - (by - ay) * (ex - ax));
		double r = (double)(A * B * C / (2 * area2));
		for (int t = 0; t < 1 + (int)(U(g) * 4); ++t) r = std::nextafter(r, 1e300);
		for (int k = 0; k < 3; ++k) d.push_back({{cx[k], cy[k]}, r});
		// box around the concurrency point
		long double D = 2 * (ax * (by - ey) + bx * (ey - ay) + ex * (ay - by));
		long double ux = ((ax*ax + ay*ay) * (by - ey) + (bx*bx + by*by) * (ey - ay) + (ex*ex + ey*ey) * (ay - by)) / D;
		long double uy = ((ax*ax + ay*ay) * (ex - bx) + (bx*bx + by*by) * (ax - ex) + (ex*ex + ey*ey) * (bx - ax)) / D;
		double const h = 1e-3 * (0.2 + U(g));
		BoundingBox box{{(double)ux - h * U(g), (double)uy - h * U(g)}, {(double)ux + h * U(g) + 1e-6, (double)uy + h * U(g) + 1e-6}};
		if (edge_mode) {
			// the concurrency point just outside the box: the optimum is on an edge
			double const gap = 1e-7 * U(g);
			box = BoundingBox{{(double)ux + gap, (double)uy - h}, {(double)ux + gap + h, (double)uy + h}};
			if (it & 1) box = BoundingBox{{(double)ux - h, (double)uy + gap}, {(double)ux + h, (double)uy + gap + h}};
			r = std::nextafter(r * (1 + 1e-11 * U(g)), 1e300);            // enough room to reach the edge
			for (auto& dd : d) dd.radius = r;
		}
		double tol = slack;                                              // candidate7 as first merged
#ifdef WITH_DECIDER_ERR
		double const u = std::numeric_limits<double>::epsilon() / 2;
		double Mp = 0; for (int k = 0; k < 3; ++k) Mp = std::max(Mp, std::max(std::abs(px[k]), std::abs(py[k])));
		tol -= 2.0 * u * (Mp + r + slack) + 6.0 * u * (r + slack);      // follow-up: decider rounding bound (as N6Alg)
#endif
		Params prm; prm.exact = true; Stats st;
		Result res = enumerate_box(d, box, r, tol, prm, &st);
		++cases;
		double const Rd = r + slack, Rd2 = Rd * Rd;
		bool any_ok = true; long here = 0;
		for (auto const& reg : res.regions) {
			if (reg.ply != 3) continue;                                   // the region holding all three discs
			Point w = reg.witness; ++here; ++nw;
			bool ok = true;
			for (int k = 0; k < 3; ++k) {
				double tx = qx[k] + w.x, ty = qy[k] + w.y;          // fl(q + t), as curve_fut does
				double dx = px[k] - tx, dy = py[k] - ty;
				if (!(dx*dx + dy*dy <= Rd2)) ok = false;
			}
			if (!ok) any_ok = false;
		}
		if (!res.overflow.empty()) ++fallback;
		if (here && !any_ok) ++rej;
	}
	std::printf("cases=%ld three-disc_witnesses=%ld rejected_by_decider=%ld sent_to_fallback=%ld\n", cases, nw, rej, fallback);
}
