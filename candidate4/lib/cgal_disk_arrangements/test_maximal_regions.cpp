// Regression suite for the Čech maximal-region pipeline
// (maximal_region_pipeline.md §9).  Self-contained: no CGAL, no curve I/O.
//
//   ctest -R maximal_regions          (or run the binary directly)

#include "maximal_regions.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <set>
#include <vector>

using namespace cgal_disk_arrangements;

namespace {

int g_failures = 0;

void check(bool ok, char const* what) {
	if (!ok) { std::printf("  FAIL  %s\n", what); ++g_failures; }
	else       std::printf("  ok    %s\n", what);
}

Discs make(std::vector<std::pair<double,double>> const& c, double r) {
	Discs d;
	for (auto const& p : c) d.push_back(Disc{Point{p.first, p.second}, r});
	return d;
}

maxregion::Result run(Discs const& d, double band = 1e-12, std::size_t dp = 20) {
	maxregion::Params p; p.band = band; p.dp_limit = dp;
	return maxregion::enumerate(d, p, nullptr);
}

std::set<std::vector<std::uint64_t>> mask_set(maxregion::Result const& r) {
	std::set<std::vector<std::uint64_t>> s;
	for (auto const& reg : r.regions) s.insert(reg.mask);
	return s;
}

bool has_bit(maxregion::Mask const& m, std::size_t k) {
	return (m[k / 64] >> (k & 63)) & 1ULL;
}

bool subset(maxregion::Mask const& a, maxregion::Mask const& b) {
	for (std::size_t i = 0; i < a.size(); ++i) if (a[i] & ~b[i]) return false;
	return true;
}

bool nonzero(maxregion::Mask const& m) {
	for (auto w : m) if (w) return true;
	return false;
}

// ── §9 row 5: Rips ≠ Čech ─────────────────────────────────────────────────────
// Equilateral triangle of side exactly 2r: every PAIR of discs touches, so the
// proximity (Vietoris–Rips) graph is a triangle, but the circumradius is
// 2r/√3 ≈ 1.1547 r > r, so the three discs have NO common point.  An
// implementation that skips the Helly/P3 check reports one region {0,1,2}; the
// correct answer is the three pairs.  This is the decisive regression.
void test_rips_not_cech() {
	std::printf("[rips != cech]\n");
	double const r = 1.0, s = 2.0 * r;
	Discs d = make({{0.0, 0.0}, {s, 0.0}, {s/2.0, s*std::sqrt(3.0)/2.0}}, r);
	auto res = run(d);
	check(res.regions.size() == 3, "three maximal regions (the three tangent pairs)");
	for (auto const& reg : res.regions)
		check(reg.ply == 2, "each region has exactly two discs (not the triple)");
}

// Shrinking the same triangle until the circumradius drops below r must flip it
// into a single triple region — the P3 boundary is where it belongs.
void test_equilateral_boundary() {
	std::printf("[equilateral P3 boundary]\n");
	double const r = 1.0;
	// circumradius of an equilateral triangle of side s is s/√3, so s = r√3 is
	// the exact threshold.
	double const s_crit = r * std::sqrt(3.0);
	for (double f : {0.999, 1.001}) {
		double const s = s_crit * f;
		Discs d = make({{0.0, 0.0}, {s, 0.0}, {s/2.0, s*std::sqrt(3.0)/2.0}}, r);
		auto res = run(d);
		if (f < 1.0) {
			check(res.regions.size() == 1 && res.regions[0].ply == 3,
			      "s = 0.999·r√3 -> one triple region");
		} else {
			check(res.regions.size() == 3, "s = 1.001·r√3 -> three pair regions");
		}
	}
}

// ── P3 conditioning: sliver triangles ─────────────────────────────────────────
// Two centres nearly coincident, the third far away — the shape the Minkowski
// difference produces whenever a curve has near-repeated points, and which the
// bitwise coincidence merge deliberately does NOT collapse.  Such a triangle is
// acute with a tiny angle at the far vertex, so evaluating the cross product
// there loses ~1/sin θ digits.  Both instances below have r set 1e-11 (relative,
// on r²) ABOVE the exact circumradius, so the triple genuinely has a common
// point; taking the apex from a fixed argument position rejects them.
//
// Witnesses produced against an exact rational oracle; the second is at the
// geolife working scale (r = 0.025, centres 6e-8 apart, third 0.05 away).
//
// Both are laid out so the FAR vertex sorts first: Stage 0 orders centres by
// (x, y) and the DP feeds the three lowest set bits to P3 in that order, so the
// apex a fixed-argument implementation would pick is the lowest-x centre.  The
// second case is the first one's exact mirror in x (a sign flip, so the
// circumradius is bit-identical) — without it the sort would hand the buggy code
// a well-conditioned vertex and the case would pass for the wrong reason.
void test_p3_sliver() {
	std::printf("[P3 sliver: two centres nearly coincident, third far]\n");
	struct Case { double r, x0, y0, x1, y1, x2, y2; char const* what; };
	Case const cases[] = {
		{18.500000000092498,
		 -26.979012919344402, -25.32060153106756,
		 -0.0, 0.0,
		 -7.5753726555651137e-05, 8.0715077517280683e-05,
		 "sin(theta) = 3.0e-06, r = 18.5"},
		{0.025000000000125002,
		 -0.0447524259404551, 0.022298438789378806,
		 -0.0, 0.0,
		 -2.7155213710244256e-08, -5.449974436292583e-08,
		 "sin(theta) = 1.2e-06, r = 0.025 (geolife scale)"},
	};
	for (auto const& t : cases) {
		Discs d = make({{t.x0, t.y0}, {t.x1, t.y1}, {t.x2, t.y2}}, t.r);
		auto res = run(d);
		bool ok = res.regions.size() == 1 && res.regions[0].ply == 3;
		check(ok, t.what);
		if (!ok)
			std::printf("        got %zu region(s), ply %s\n", res.regions.size(),
			            res.regions.empty() ? "-" :
			            (res.regions[0].ply == 1 ? "1" : "2"));
	}
}

// ── §9 row 1: coincident centres ──────────────────────────────────────────────
void test_coincident_merge() {
	std::printf("[coincident centres]\n");
	double const r = 1.0;
	Discs d = make({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {5.0, 0.0}}, r);
	auto res = run(d);
	check(res.regions.size() == 2, "two regions: the triple-alias group and the lone disc");
	bool found_alias = false;
	for (auto const& reg : res.regions)
		if (reg.ply == 3 && has_bit(reg.mask, 0) && has_bit(reg.mask, 1) && has_bit(reg.mask, 2))
			found_alias = true;
	check(found_alias, "alias expansion restores all three coincident indices");
}

// ── §9 row 2: exact tangency, closed convention ───────────────────────────────
void test_exact_tangency() {
	std::printf("[exact tangency d = 2r, closed convention]\n");
	double const r = 1.0;
	Discs d = make({{0.0, 0.0}, {2.0, 0.0}}, r);
	auto res = run(d);
	check(res.regions.size() == 1 && res.regions[0].ply == 2,
	      "tangent discs share their tangent point -> one region {0,1}");
	if (!res.regions.empty()) {
		double const x = res.regions[0].witness.x, y = res.regions[0].witness.y;
		check(std::abs(x - 1.0) < 1e-9 && std::abs(y) < 1e-9,
		      "witness is the tangent point (1,0)");
	}
}

// ── §9 row 3: triple common point, no duplicate output ────────────────────────
void test_triple_common_point() {
	std::printf("[triple with a common point]\n");
	double const r = 1.0;
	Discs d = make({{0.0, 0.0}, {0.4, 0.0}, {0.2, 0.35}}, r);
	auto res = run(d);
	check(res.regions.size() == 1 && res.regions[0].ply == 3, "single region {0,1,2}");
	check(mask_set(res).size() == res.regions.size(), "no duplicate masks emitted");
}

// ── Isolated discs (Claim V |S| = 1) ──────────────────────────────────────────
void test_isolated() {
	std::printf("[isolated discs]\n");
	double const r = 0.5;
	Discs d = make({{0.0, 0.0}, {10.0, 0.0}, {20.0, 0.0}}, r);
	auto res = run(d);
	check(res.regions.size() == 3, "three singleton regions");
	for (auto const& reg : res.regions) check(reg.ply == 1, "each is a lone disc");
}

// ── §9 row 6/7: grid oracle ───────────────────────────────────────────────────
// Independent completeness check: sample the plane densely, collect S(p) for
// every sample, and require that every observed index set is contained in some
// emitted region.  Prop. 1 says the maximal S(p) are exactly the maximal Čech
// simplices, so a sampled set the pipeline cannot cover is a real loss.
// Also checks the two invariants the decider relies on: the emitted family is an
// antichain, and each witness really lies in every disc of its own region.
void test_grid_oracle() {
	std::printf("[grid oracle vs pipeline]\n");
	std::mt19937 rng(20260902u);
	std::uniform_real_distribution<double> U(0.0, 4.0);

	int uncovered_total = 0, comparable_total = 0, witness_bad = 0;
	for (int trial = 0; trial < 12; ++trial) {
		std::size_t const n = 8;
		double const r = 1.0;
		Discs d;
		for (std::size_t i = 0; i < n; ++i)
			d.push_back(Disc{Point{U(rng), U(rng)}, r});
		auto res = run(d);

		// antichain + witness containment
		for (std::size_t a = 0; a < res.regions.size(); ++a) {
			for (std::size_t b = 0; b < res.regions.size(); ++b)
				if (a != b && subset(res.regions[a].mask, res.regions[b].mask))
					++comparable_total;
			for (std::size_t k = 0; k < n; ++k) {
				if (!has_bit(res.regions[a].mask, k)) continue;
				double const dx = res.regions[a].witness.x - d[k].center.x;
				double const dy = res.regions[a].witness.y - d[k].center.y;
				if (dx*dx + dy*dy > r*r*(1.0 + 1e-9)) ++witness_bad;
			}
		}

		// grid sweep
		double const lo = -1.5, hi = 5.5;
		int const G = 260;
		std::set<std::vector<std::uint64_t>> observed;
		for (int ix = 0; ix <= G; ++ix) {
			double const px = lo + (hi - lo) * ix / G;
			for (int iy = 0; iy <= G; ++iy) {
				double const py = lo + (hi - lo) * iy / G;
				maxregion::Mask m((n + 63) / 64, 0ULL);
				for (std::size_t k = 0; k < n; ++k) {
					double const dx = px - d[k].center.x, dy = py - d[k].center.y;
					if (dx*dx + dy*dy <= r*r) m[k / 64] |= 1ULL << (k & 63);
				}
				if (nonzero(m)) observed.insert(m);
			}
		}
		for (auto const& m : observed) {
			bool covered = false;
			for (auto const& reg : res.regions) if (subset(m, reg.mask)) { covered = true; break; }
			if (!covered) ++uncovered_total;
		}
	}
	check(uncovered_total == 0, "every grid-observed index set is covered by a region");
	check(comparable_total == 0, "emitted regions form an antichain");
	check(witness_bad == 0, "every witness lies in every disc of its own region");
}

// The DP overflow path must hand oversized components back, not drop them.
void test_overflow_handoff() {
	std::printf("[dp_limit overflow hand-off]\n");
	double const r = 1.0;
	Discs d;
	for (int i = 0; i < 10; ++i) d.push_back(Disc{Point{0.1 * i, 0.0}, r});
	auto res = run(d, 1e-12, /*dp_limit=*/4);
	check(res.regions.empty() && res.overflow.size() == 1,
	      "one oversized component returned to the caller");
	if (res.overflow.size() == 1)
		check(res.overflow[0].size() == 10, "hand-off lists all ten global disc indices");
}

} // namespace

int main() {
	test_rips_not_cech();
	test_p3_sliver();
	test_equilateral_boundary();
	test_coincident_merge();
	test_exact_tangency();
	test_triple_common_point();
	test_isolated();
	test_overflow_handoff();
	test_grid_oracle();
	std::printf(g_failures ? "\nFAILED (%d)\n" : "\nPASSED\n", g_failures);
	return g_failures ? 1 : 0;
}
