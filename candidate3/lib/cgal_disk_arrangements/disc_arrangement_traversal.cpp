#include "disc_arrangement_traversal.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace cgal_disk_arrangements
{

struct CGALData {
	std::vector<Point> points;
	std::size_t current = 0;
};

namespace
{

using clk = std::chrono::steady_clock;
using ns  = std::chrono::nanoseconds;
using Mask = std::vector<uint64_t>;

using EK = CGAL::Exact_predicates_exact_constructions_kernel;   // lazy-exact, filtered
using FT = EK::FT;

std::size_t mask_count(Mask const& m) {
	std::size_t r = 0;
	for (auto w : m) { auto x = w; while (x) { x &= x-1; ++r; } }
	return r;
}
bool subset_mask(Mask const& a, Mask const& b) {
	for (std::size_t i = 0; i < a.size(); ++i)
		if (a[i] & ~b[i]) return false;
	return true;
}

// Two masks per candidate (boundary band): mask_hi counts a disc as containing
// the point if the point is within radius·(1+band); mask_lo within radius·(1−band).
struct Candidate { Point point; Mask mask_hi; Mask mask_lo; std::size_t count; };

// Boundary-robust inclusion-maximal filter (same rule as the float variant):
// drop c only if its most-inclusive mask is inside some survivor's most-
// conservative mask.  NOTE: the band here is NOT about floating-point error
// (masks are computed exactly); it is needed because the maximal filter runs on
// a SUBSET of discs (the cut centers), so plain inclusion-maximal can drop a
// translation the full Fréchet decider still needs.  Keeping near-boundary
// candidates compensates — exactly as in candidate2.
void filter_maximal(std::vector<Candidate>& cands) {
	std::sort(cands.begin(), cands.end(),
		[](Candidate const& a, Candidate const& b){ return a.count > b.count; });
	std::vector<Candidate> result;
	for (auto const& c : cands) {
		bool dominated = false;
		for (auto const& r : result)
			if (subset_mask(c.mask_hi, r.mask_lo)) { dominated = true; break; }
		if (!dominated) result.push_back(c);
	}
	cands.swap(result);
}

struct SweepStats {
	long long calls=0, before=0, after=0, gen_ns=0, filt_ns=0;
	~SweepStats() {
		if (!calls) return;
		std::printf("[exact-stats] calls=%lld before=%lld after=%lld gen_ms=%.1f filt_ms=%.1f\n",
			calls, before, after, gen_ns/1e6, filt_ns/1e6);
	}
} g_stats;

// EXACT predicate sign of (G4x ≥ σ|T|), σ = sg, with |T|² = W2²·R/d2 (scaled).
//   sg==0 : G4x ≥ 0
//   sg>0  : G4x ≥ 0  AND  G4x²·d2 ≥ 4·R·W2²
//   sg<0  : G4x ≥ 0  OR   4·R·W2² ≥ G4x²·d2
inline bool inside_pred(FT const& G4x, FT const& W2, FT const& R, FT const& d2, int sg) {
	if (sg == 0) return (G4x >= 0);
	FT L  = FT(4) * R * W2 * W2;
	FT Rr = G4x * G4x * d2;
	return (sg > 0) ? ((G4x >= 0) && (Rr >= L)) : ((G4x >= 0) || (L >= Rr));
}

// ── Fast EXACT disc-arrangement candidate generation ───────────────────────────
// Direct enumeration (pairwise circle–circle vertices + disc centers) with EXACT
// rational containment predicates — no CGAL Arrangement_2 (no DCEL/BFS) and no
// exact sqrt construction of the vertices.  See inside_pred for the algebra.
std::unique_ptr<CGALData> build_candidates(Discs const& discs)
{
	auto data = std::make_unique<CGALData>();
	std::size_t n = discs.size();
	if (!n) return data;

	double rd  = discs[0].radius;
	FT     r2  = FT(rd) * FT(rd);
	FT     band = FT(1) / FT(10000000);     // 1e-7 relative boundary band
	FT     r2_hi = r2 * (FT(1) + band);
	FT     r2_lo = r2 * (FT(1) - band);
	FT     delta = FT(4) * r2 * band;       // G4_hi = G4 + delta, G4_lo = G4 − delta

	std::vector<FT> cx(n), cy(n);
	for (std::size_t i = 0; i < n; ++i) { cx[i] = FT(discs[i].center.x); cy[i] = FT(discs[i].center.y); }

	auto t0 = clk::now();
	std::vector<Candidate> cands;
	std::size_t words = (n + 63) / 64;

	// (1) disc centers
	for (std::size_t i = 0; i < n; ++i) {
		Mask mhi(words, 0ULL), mlo(words, 0ULL);
		for (std::size_t k = 0; k < n; ++k) {
			FT dx = cx[i] - cx[k], dy = cy[i] - cy[k];
			FT dd = dx*dx + dy*dy;
			if (dd <= r2_hi) mhi[k/64] |= 1ULL << (k & 63);
			if (dd <= r2_lo) mlo[k/64] |= 1ULL << (k & 63);
		}
		cands.push_back({Point{discs[i].center.x, discs[i].center.y}, mhi, mlo, mask_count(mhi)});
	}

	// (2) pairwise circle–circle intersection vertices
	for (std::size_t i = 0; i < n; ++i) {
		double axd = discs[i].center.x, ayd = discs[i].center.y;
		for (std::size_t j = i + 1; j < n; ++j) {
			double bxd = discs[j].center.x, byd = discs[j].center.y;

			FT ex = cx[j] - cx[i], ey = cy[j] - cy[i];
			FT d2 = ex*ex + ey*ey;
			if (d2 <= 0) continue;
			FT R  = FT(4) * r2 - d2;
			if (R < 0) continue;                       // disjoint
			FT perpx = -ey, perpy = ex;
			FT sumx = cx[i] + cx[j], sumy = cy[i] + cy[j];
			int eps_lo = (R == 0) ? +1 : -1;

			for (int eps = +1; eps >= eps_lo; eps -= 2) {
				Mask mhi(words, 0ULL), mlo(words, 0ULL);
				for (std::size_t k = 0; k < n; ++k) {
					FT Qx = sumx - FT(2)*cx[k], Qy = sumy - FT(2)*cy[k];
					FT G4 = d2 - (Qx*Qx + Qy*Qy);
					FT W2 = Qx*perpx + Qy*perpy;
					int sgnW = (W2 > 0) ? 1 : ((W2 < 0) ? -1 : 0);
					int sg = eps * sgnW;
					if (inside_pred(G4 + delta, W2, R, d2, sg)) mhi[k/64] |= 1ULL << (k & 63);
					if (inside_pred(G4 - delta, W2, R, d2, sg)) mlo[k/64] |= 1ULL << (k & 63);
				}
				double exd = bxd - axd, eyd = byd - ayd;
				double d2d = exd*exd + eyd*eyd;
				double dd  = std::sqrt(d2d);
				double h2  = rd*rd - 0.25*d2d;
				double hh  = h2 > 0.0 ? std::sqrt(h2) : 0.0;
				double sc  = (dd > 0.0) ? eps * hh / dd : 0.0;
				double px  = 0.5*(axd + bxd) + sc * (-eyd);
				double py  = 0.5*(ayd + byd) + sc * ( exd);
				cands.push_back({Point{px, py}, mhi, mlo, mask_count(mhi)});
			}
		}
	}

	auto t1 = clk::now();
	std::size_t before = cands.size();
	filter_maximal(cands);
	auto t2 = clk::now();

	++g_stats.calls;
	g_stats.before += (long long)before;
	g_stats.after  += (long long)cands.size();
	g_stats.gen_ns  += std::chrono::duration_cast<ns>(t1 - t0).count();
	g_stats.filt_ns += std::chrono::duration_cast<ns>(t2 - t1).count();

	for (auto const& c : cands) data->points.push_back(c.point);
	data->current = 0;
	return data;
}

} // anonymous namespace

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox)
	: ArrangementTraversal(discs, false) {}
ArrangementTraversal::ArrangementTraversal(Discs const& discs)
	: ArrangementTraversal(discs, false) {}
ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox, bool maximal_only)
	: ArrangementTraversal(discs, maximal_only) {}
ArrangementTraversal::ArrangementTraversal(Discs const& discs, bool /*maximal_only*/)
{
	data = build_candidates(discs);
	size = data->points.size();
}
ArrangementTraversal::~ArrangementTraversal() = default;
bool ArrangementTraversal::hasNext() { return data->current < data->points.size(); }
Point ArrangementTraversal::getNext() { return data->points[data->current++]; }
std::size_t ArrangementTraversal::getSize() const { return size; }

} // namespace cgal_disk_arrangements
