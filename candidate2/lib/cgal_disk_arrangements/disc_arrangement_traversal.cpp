#include "disc_arrangement_traversal.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>   // cpp_rational for --maximal=exact

namespace cgal_disk_arrangements
{

struct CGALData {
	std::vector<Point> points;
	std::size_t current = 0;
	// C_q–ply instrumentation (parallel to points; filled only when X3Q_DUMP set)
	std::vector<std::uint32_t> ply;
	std::vector<std::uint8_t>  ismax;
	std::uint32_t last_ply = 0;
	std::uint8_t  last_ismax = 0;
};

namespace
{

using clk = std::chrono::steady_clock;
using ns  = std::chrono::nanoseconds;
using Mask = std::vector<uint64_t>;

// ── X1 runtime switch (experiment_design_X1_X2.md §2.1) ────────────────────────
// The maximal filter is the *only* thing that differs between arms D1 and D0, so
// it is selected by one runtime knob — the MAXIMAL_MODE environment variable —
// with everything else (binary, decider code path, dedup, arrangement_cut_limit)
// held identical, per invalidation-condition #1.  Read once and cached.
//
//   on         (default)  compute masks, run maximal filter  → D1 (unchanged)
//   mask-only             compute masks, skip filter         → cost-decomposition
//   off                   skip masks AND filter, emit all m  → D0
//   exact                 verification: also run exact (rational) maximal
//                         enumeration and log K_exact / degeneracy (NEXT §3)
//
// Masks are consumed only by filter_maximal(), so `off` legitimately skips the
// mask pass entirely; `mask-only` keeps it to isolate mask cost from filter cost.
enum class MaxMode { On, Off, MaskOnly, Exact };
MaxMode maximal_mode() {
	static MaxMode cached = []{
		const char* e = std::getenv("MAXIMAL_MODE");
		if (!e) return MaxMode::On;                       // default preserves D1
		std::string s(e);
		if (s == "off")                        return MaxMode::Off;
		if (s == "mask-only" || s == "mask_only") return MaxMode::MaskOnly;
		if (s == "exact")                      return MaxMode::Exact;
		return MaxMode::On;
	}();
	return cached;
}

// ── Bitmask utilities ─────────────────────────────────────────────────────────
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

// Each candidate carries two containment masks, computed with a boundary band:
//   mask_hi — boundary discs counted as INSIDE  (most inclusive set)
//   mask_lo — boundary discs counted as OUTSIDE (most conservative set)
// count is the mask_hi popcount, used only to order the maximal-filter pass.
struct Candidate { Point point; Mask mask_hi; Mask mask_lo; std::size_t count; };

// Inclusion-wise maximal filter that is robust to floating-point boundary
// ambiguity.  A candidate c is dropped only when even its most inclusive set
// (mask_hi) is contained in some survivor's most conservative set (mask_lo):
// then c is genuinely dominated regardless of how the boundary discs are
// classified, so no candidate that could uniquely enable a matching is lost.
// In general position mask_hi == mask_lo and this reduces to the plain filter.
void filter_maximal(std::vector<Candidate>& cands) {
	std::sort(cands.begin(), cands.end(),
		[](Candidate const& a, Candidate const& b){ return a.count > b.count; });
	std::vector<Candidate> result;
	for (auto const& c : cands) {
		bool dominated = false;
		for (auto const& r : result) {
			if (subset_mask(c.mask_hi, r.mask_lo))
				{ dominated = true; break; }
		}
		if (!dominated) result.push_back(c);
	}
	cands.swap(result);
}

// Whether X3Q per-query instrumentation is active (env X3Q_DUMP set once).
bool x3q_enabled() {
	static bool on = (std::getenv("X3Q_DUMP") != nullptr);
	return on;
}

// Like filter_maximal but non-destructive: returns keep[i]=1 iff cands[i] is
// inclusion-maximal, aligned with the input order (for C_q–ply tagging).
std::vector<char> mark_maximal(std::vector<Candidate> const& cands) {
	std::vector<std::size_t> idx(cands.size());
	for (std::size_t i = 0; i < idx.size(); ++i) idx[i] = i;
	std::sort(idx.begin(), idx.end(),
		[&](std::size_t a, std::size_t b){ return cands[a].count > cands[b].count; });
	std::vector<char> keep(cands.size(), 0);
	std::vector<std::size_t> survivors;
	for (std::size_t i : idx) {
		bool dominated = false;
		for (std::size_t s : survivors)
			if (subset_mask(cands[i].mask_hi, cands[s].mask_lo)) { dominated = true; break; }
		if (!dominated) { keep[i] = 1; survivors.push_back(i); }
	}
	return keep;
}

// ── Instrumentation ───────────────────────────────────────────────────────────
struct SweepStats {
	long long calls=0, before=0, after=0;
	long long sweep_ns=0, mask_ns=0, filter_ns=0;
	long long npts=0, degen=0, queries=0;   // INSTRUMENTATION (revert after measuring)
	~SweepStats() {
		if (!calls) return;
		std::printf(
			"[sweep-stats] calls=%lld before=%lld after=%lld"
			" gen_ms=%.1f mask_ms=%.1f filter_ms=%.1f"
			" npts=%lld degen=%lld queries=%lld\n",
			calls, before, after,
			sweep_ns/1e6, mask_ns/1e6, filter_ns/1e6,
			npts, degen, queries);
	}
} g_stats;

long long g_build_id = 0;   // per-arrangement-build counter for --maximal=exact CSV

// Compute containment bitmask of point (px,py) against all discs.
Mask compute_mask(double px, double py,
                  Discs const& discs, double r2eps)
{
	Mask m((discs.size() + 63) / 64, 0ULL);
	for (std::size_t k = 0; k < discs.size(); ++k) {
		double dx = px - discs[k].center.x;
		double dy = py - discs[k].center.y;
		if (dx*dx + dy*dy <= r2eps)
			m[k/64] |= 1ULL << (k & 63);
	}
	return m;
}

// ── Exact maximal enumeration (--maximal=exact, verification only) ─────────────
// Reference: NEXT_P0_X3_X5.md §3.1 + Appendix A, validated in _x3_exact_ref.py.
// A vertex p = ∂D_i∩∂D_j equals m + σ√s·w with rational m,w,s; containment
// |p−c_k|²−r² = A + σ√s·B (A,B rational) → exact sign via a one-root compare.
// Hybrid: decide the sign in double, fall back to rational only inside a tiny
// relative band, so the exact cost concentrates on the near-boundary vertices
// that are the whole point of this mode.
namespace exact_ns {

using Z = boost::multiprecision::cpp_int;
using Q = boost::multiprecision::cpp_rational;

inline int sgnQ(Q const& x){ return x>0 ? 1 : (x<0 ? -1 : 0); }

Q dbl_to_Q(double x){
	if (x == 0.0) return Q(0);
	int e; double mm = std::frexp(x, &e);          // x = mm·2^e, 0.5≤|mm|<1
	long long mi = (long long)std::ldexp(mm, 53);  // 53-bit integer mantissa
	e -= 53;
	Z num(mi);
	if (e >= 0){ num <<= e; return Q(num); }
	Z den(1); den <<= (-e); return Q(num, den);
}

// sign(A + C√s), s ≥ 0
int sign_one_root(Q const& A, Q const& C, Q const& s){
	if (s == 0 || C == 0) return sgnQ(A);
	if (A >= 0 && C > 0) return 1;
	if (A <= 0 && C < 0) return -1;
	Q D = A*A - C*C*s;                             // opposite signs → compare squares
	int sd = sgnQ(D);
	return A > 0 ? sd : -sd;
}

struct Ctx { std::vector<Q> cx, cy; std::vector<double> cxd, cyd; Q r2; double r2d; };

struct Vertex {
	Mask mask;
	std::vector<std::size_t> active;               // constraints with sign exactly 0
	std::vector<std::pair<Q,Q>> gact;              // g_k for active k
	Q s, wx, wy; int sigma;
	double px, py;                                 // double representative
	bool ok=false;
};

Vertex vertex_at(Ctx const& c, std::size_t n, std::size_t i, std::size_t j, int sigma){
	Vertex V;
	Q dx = c.cx[j]-c.cx[i], dy = c.cy[j]-c.cy[i];
	Q d2 = dx*dx + dy*dy;
	if (d2 == 0 || d2 > 4*c.r2) return V;
	if (d2 == 4*c.r2 && sigma == -1) return V;      // tangent: σ duplicate
	Q s = (c.r2 - d2/Q(4)) / d2;
	Q mx = (c.cx[i]+c.cx[j])/Q(2), my = (c.cy[i]+c.cy[j])/Q(2);
	Q wx = -dy, wy = dx;
	double sd = (double)s, sq = sd>0? std::sqrt(sd):0.0;
	double mxd=(double)mx, myd=(double)my, wxd=(double)wx, wyd=(double)wy, d2d=(double)d2;
	(void)d2d; (void)wxd; (void)wyd;
	V.px = mxd + sigma*sq*wxd; V.py = myd + sigma*sq*wyd;
	V.mask.assign((n+63)/64, 0ULL);
	// Double distance pre-filter: a disc whose centre is clearly farther/nearer
	// than r (by a 2e-6 relative band, ≫ double rounding at the vertex) is
	// classified directly; only the ambiguous near-boundary band pays the exact
	// one-root cost. Reduces the inner loop from O(N) to local density.
	double const px = V.px, py = V.py;
	double const band_hi = c.r2d*(1.0 + 2e-6), band_lo = c.r2d*(1.0 - 2e-6);
	for (std::size_t k = 0; k < n; ++k){
		double ddx = px - c.cxd[k], ddy = py - c.cyd[k];
		double dist2 = ddx*ddx + ddy*ddy;
		if (dist2 > band_hi) continue;                 // clearly outside
		if (dist2 < band_lo){ V.mask[k/64] |= 1ULL << (k&63); continue; }  // clearly inside
		// ambiguous → exact one-root sign of |p−c_k|²−r² = A + σ√s·B
		Q gx = mx - c.cx[k], gy = my - c.cy[k];
		Q A = gx*gx + gy*gy + s*d2 - c.r2;
		Q B = 2*(gx*wx + gy*wy);
		int t = sign_one_root(A, sigma>0?B:Q(-B), s);
		if (t <= 0) V.mask[k/64] |= 1ULL << (k&63);
		if (t == 0){ V.active.push_back(k); V.gact.push_back({gx,gy}); }
	}
	V.s=s; V.wx=wx; V.wy=wy; V.sigma=sigma; V.ok=true;
	return V;
}

// ∩S has empty interior ⟺ active outward normals n_k=g_k+σ√s·w positively span
// R² (0 strictly interior of their hull). cross(n_a,n_b)=Ac+σ√s·Bc, Ac,Bc rational.
bool normals_span(Vertex const& V){
	std::size_t n = V.active.size();
	if (n < 3) return false;
	auto cross_sign = [&](std::size_t a, std::size_t b)->int{
		Q const& gax=V.gact[a].first;  Q const& gay=V.gact[a].second;
		Q const& gbx=V.gact[b].first;  Q const& gby=V.gact[b].second;
		Q Ac = gax*gby - gay*gbx;                    // g_a × g_b
		Q Bc = (gax-gbx)*V.wy - (gay-gby)*V.wx;      // (g_a−g_b) × w
		return sign_one_root(Ac, V.sigma>0?Bc:Q(-Bc), V.s);
	};
	for (std::size_t a=0;a<n;++a){
		bool pos=false, neg=false;
		for (std::size_t b=0;b<n;++b){ if(b==a) continue; int csn=cross_sign(a,b);
			if(csn>0)pos=true; else if(csn<0)neg=true; }
		if(!(pos&&neg)) return false;                // others lie in one closed halfplane
	}
	return true;
}

struct Result { long long V_pairs=0, K_exact_distinct=0, n_degen_geom=0, K_exact_pts=0, n_isolated=0; };

// analyse discs exactly; return counts + one representative point per distinct
// inclusion-maximal mask (appended to reps).
Result analyze(Discs const& discs, std::vector<Point>& reps){
	Result R;
	std::size_t n = discs.size();
	Ctx c; c.cx.resize(n); c.cy.resize(n); c.cxd.resize(n); c.cyd.resize(n);
	double r = discs.empty()?0.0:discs[0].radius;
	c.r2 = dbl_to_Q(r)*dbl_to_Q(r); c.r2d = r*r;
	for (std::size_t k=0;k<n;++k){
		c.cx[k]=dbl_to_Q(discs[k].center.x); c.cy[k]=dbl_to_Q(discs[k].center.y);
		c.cxd[k]=discs[k].center.x; c.cyd[k]=discs[k].center.y;
	}
	// key = mask bytes -> (degenerate, representative point)
	std::map<Mask, std::pair<bool,Point>> seen;
	// R6: a disc is "isolated" (Claim V |S|=1 exception) iff no OTHER disc lies
	// within 2r of it — it makes no arrangement vertex, so it is absent from the
	// candidate list. touched[k]=1 once any other disc is within 2r (d2<=4r²,
	// including the d2==0 coincident case).
	std::vector<char> touched(n, 0);
	Q four_r2 = 4*c.r2;
	for (std::size_t i=0;i<n;++i){
		Q cix=c.cx[i], ciy=c.cy[i];
		for (std::size_t j=i+1;j<n;++j){
			Q dx=c.cx[j]-cix, dy=c.cy[j]-ciy; Q d2=dx*dx+dy*dy;
			if (d2<=four_r2){ touched[i]=1; touched[j]=1; }
			if (d2==0 || d2>four_r2) continue;
			++R.V_pairs;
			for (int sigma : {+1,-1}){
				Vertex V = vertex_at(c, n, i, j, sigma);
				if (!V.ok) continue;
				++R.K_exact_pts;
				bool deg = normals_span(V);
				auto it = seen.find(V.mask);
				if (it==seen.end()) seen.emplace(V.mask, std::make_pair(deg, Point{V.px,V.py}));
				else it->second.first = it->second.first || deg;
			}
		}
	}
	// inclusion-maximal filter over distinct masks
	std::vector<Mask const*> ms; ms.reserve(seen.size());
	for (auto const& kv : seen) ms.push_back(&kv.first);
	for (auto const& kv : seen){
		Mask const& m = kv.first;
		bool dominated=false;
		for (Mask const* m2 : ms){ if (m2==&m) continue;
			if (subset_mask(m, *m2) && m!=*m2){ dominated=true; break; } }
		if (!dominated){ ++R.K_exact_distinct; if (kv.second.first) ++R.n_degen_geom;
			reps.push_back(kv.second.second); }
	}
	for (std::size_t k=0;k<n;++k) if(!touched[k]) ++R.n_isolated;
	return R;
}

} // namespace exact_ns

// Append one X3 CSV row (path label from env X3_PATH; file from env X3_DUMP).
void x3_dump(long long build_id, std::size_t N, exact_ns::Result const& R,
             long long m, long long K_band, long long K_band_distinct,
             long long K_band_distinct_lo, long long n_degen_band, double t_exact_ms){
	static const char* path = std::getenv("X3_DUMP");
	if (!path) return;
	static bool header = false;
	std::FILE* f = std::fopen(path, "a");
	if (!f) return;
	if (!header){
		std::fseek(f, 0, SEEK_END);
		if (std::ftell(f)==0)
			std::fprintf(f, "build_id,path,N,V_pairs,m,K_band,K_band_distinct,"
			                "K_band_distinct_lo,K_exact,K_exact_distinct,"
			                "n_degen_band,n_degen_geom,n_isolated,t_exact_ms\n");
		header = true;
	}
	const char* lbl = std::getenv("X3_PATH"); if(!lbl) lbl="?";
	std::fprintf(f, "%lld,%s,%zu,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%.3f\n",
		build_id, lbl, N, R.V_pairs, m, K_band, K_band_distinct, K_band_distinct_lo,
		R.K_exact_pts, R.K_exact_distinct, n_degen_band, R.n_degen_geom,
		R.n_isolated, t_exact_ms);
	std::fclose(f);
}

// ── Disc-arrangement candidate generation (CGAL-free) ──────────────────────────
//
// Strategy: enumerate every vertex of the disc arrangement — i.e. every pairwise
// circle–circle intersection point — together with all disc centers, then keep
// only the inclusion-wise maximal candidates (by containment bitmask).
//
// This is *provably complete*: every face of the arrangement is represented by
// at least one of these points (a circle–circle intersection on its boundary,
// or the disc center for a lone single-disc interior), so no maximal-depth
// candidate translation can be missed.  We deliberately drop the earlier
// angular-depth "local-max run" heuristic, which could silently lose a hidden
// deeper vertex in degenerate (coincident-event) configurations, and which was
// sensitive to event tie-breaking and to the angle-0 boundary count.
//
// All discs are assumed to share one radius r — true in the FUT decider, where
// every disc has radius = the current distance threshold.  This is asserted in
// debug builds.  For equal radii the two intersection points of circles C_i,C_j
// at centers a,b (distance d, 0 < d < 2r) are the chord midpoint ± h along the
// perpendicular direction, with h = sqrt(r² − (d/2)²).
//
// Complexity: O(n²) candidate vertices, O(n) per containment mask, O(m²) for
// the 2-D proximity dedup and the maximal filter, where m = #candidates.  In the
// divide-and-conquer FUT context n ≤ arrangement_cut_limit, so each call is
// effectively constant time.

std::unique_ptr<CGALData> build_candidates(Discs const& discs)
{
	auto data = std::make_unique<CGALData>();
	std::size_t n = discs.size();
	if (!n) return data;

	double r    = discs[0].radius;
	double r2   = r * r;
	// Relative boundary band: a point within band·r of a circle is "on the
	// boundary".  Two thresholds bracket the ambiguity — r2_hi counts boundary
	// discs as inside, r2_lo as outside — and the maximal filter uses both so a
	// near-triple-point is never wrongly pruned.  A relative band scales with r,
	// so it never exceeds the radius itself as r shrinks during the binary
	// search on the distance bound.
	double const band = 1e-7;
	double r2_hi = r2 * (1.0 + band);
	double r2_lo = r2 * (1.0 - band);

#ifndef NDEBUG
	for (auto const& d : discs)
		assert(std::abs(d.radius - r) <= 1e-9 * (1.0 + std::abs(r)) &&
		       "build_candidates assumes all discs share a common radius");
#endif

	auto t0 = clk::now();

	// All raw candidate points, collected as (x, y) pairs.
	std::vector<std::pair<double,double>> raw;
	raw.reserve(n * n);

	// (1) Disc centers — guarantees a representative for every single-disc
	//     interior cell, even one whose disc meets no other disc.
	for (auto const& d : discs)
		raw.push_back({d.center.x, d.center.y});

	// (2) All pairwise circle–circle intersection points (arrangement vertices).
	for (std::size_t i = 0; i < n; ++i) {
		double ax = discs[i].center.x, ay = discs[i].center.y;
		for (std::size_t j = i + 1; j < n; ++j) {
			double bx = discs[j].center.x, by = discs[j].center.y;
			double dx = bx - ax, dy = by - ay;
			double d2 = dx*dx + dy*dy;
			// Disjoint (d ≥ 2r) or (near-)coincident centers.  The coincidence
			// threshold is RELATIVE to r² so it stays correct as r shrinks during
			// the binary search; an absolute threshold would swallow real
			// intersections once r gets small.
			if (d2 >= 4.0*r2 || d2 <= r2 * 1e-18) continue;

			double d    = std::sqrt(d2);
			double half = 0.5 * d;
			double h2   = r2 - half*half;
			double h    = h2 > 0.0 ? std::sqrt(h2) : 0.0;

			double mx = 0.5 * (ax + bx), my = 0.5 * (ay + by);  // chord midpoint
			double ux = -dy / d, uy = dx / d;                   // unit perpendicular

			raw.push_back({mx + h*ux, my + h*uy});
			raw.push_back({mx - h*ux, my - h*uy});
		}
	}

	auto t1 = clk::now();

	// ---------- deduplicate candidates by 2-D proximity ----------
	// Sort lexicographically by (x, y), then a new point is a duplicate only of
	// already-kept points whose x is within tol — a short backward window.  This
	// keeps the test fully 2-D (Euclidean, not a 1-D x-merge) while running in
	// O(m log m) instead of O(m²), so it no longer blows up when the disc count
	// grows beyond the small D&C cut limit.
	double tol  = 1e-9 * (1.0 + std::abs(r));
	double tol2 = tol * tol;
	std::sort(raw.begin(), raw.end());
	std::vector<std::pair<double,double>> unique_pts;
	for (auto const& p : raw) {
		bool dup = false;
		for (std::size_t k = unique_pts.size(); k-- > 0; ) {
			if (p.first - unique_pts[k].first > tol) break;  // sorted: rest are farther in x
			double ex = p.first - unique_pts[k].first;
			double ey = p.second - unique_pts[k].second;
			if (ex*ex + ey*ey <= tol2) { dup = true; break; }
		}
		if (!dup) unique_pts.push_back(p);
	}

	MaxMode const mode = maximal_mode();

	// ---------- compute containment masks (skipped entirely in `off`) ----------
	// Masks feed only the maximal filter, so `off` avoids the O(m·n) mask pass and
	// its setup cost collapses toward gen-only — this is exactly the 5.1 vs 79.6 ms
	// ambiguity the design's three-mode split is meant to resolve.
	std::vector<Candidate> cands;
	long long degen_band_local = 0;
	if (mode != MaxMode::Off || x3q_enabled()) {   // X3Q needs masks even under `off`
		cands.reserve(unique_pts.size());
		for (auto const& p : unique_pts) {
			Mask mhi = compute_mask(p.first, p.second, discs, r2_hi);
			Mask mlo = compute_mask(p.first, p.second, discs, r2_lo);
			++g_stats.npts; if (mhi != mlo) { ++g_stats.degen; ++degen_band_local; }
			cands.push_back({Point{p.first, p.second}, mhi, mlo, mask_count(mhi)});
		}
	}

	auto t2 = clk::now();

	// ---------- inclusion-wise maximal filter (`on` and `exact`) ----------
	// X3Q emits ALL candidates (tagged with ply + maximal flag), so it must not
	// filter here — the maximal set is marked non-destructively below instead.
	if ((mode == MaxMode::On || mode == MaxMode::Exact) && !x3q_enabled())
		filter_maximal(cands);

	auto t3 = clk::now();

	// ---------- C_q–ply mode: emit ALL candidates tagged (NEXT §3.4) ----------
	if (x3q_enabled()) {
		std::vector<char> keep = mark_maximal(cands);
		data->points.reserve(cands.size());
		data->ply.reserve(cands.size());
		data->ismax.reserve(cands.size());
		for (std::size_t i = 0; i < cands.size(); ++i) {
			data->points.push_back(cands[i].point);
			data->ply.push_back((std::uint32_t)mask_count(cands[i].mask_hi));
			data->ismax.push_back(keep[i] ? 1 : 0);
		}
		++g_stats.calls; g_stats.before += (long long)cands.size();
		g_stats.after += (long long)std::count(keep.begin(), keep.end(), (char)1);
		g_stats.sweep_ns += std::chrono::duration_cast<ns>(t1 - t0).count();
		g_stats.mask_ns  += std::chrono::duration_cast<ns>(t2 - t1).count();
		data->current = 0;
		return data;
	}

	// ---------- exact verification mode (NEXT §3): log counts, emit exact reps ----
	if (mode == MaxMode::Exact) {
		long long const m_ex   = (long long)unique_pts.size();
		long long const K_band = (long long)cands.size();       // band survivors (points)
		// R5: distinct maximal families under the lenient (hi) vs strict (lo) band.
		// mask_hi counts boundary discs IN, mask_lo counts them OUT; families that
		// coincide under lo but split under hi are the ε-degenerate (boundary-
		// dependent) maximal families — the algorithmic analog of M(D)-R(D).
		std::vector<Mask> hi; hi.reserve(cands.size());
		std::vector<Mask> lo; lo.reserve(cands.size());
		for (auto const& c : cands){ hi.push_back(c.mask_hi); lo.push_back(c.mask_lo); }
		std::sort(hi.begin(), hi.end());
		std::sort(lo.begin(), lo.end());
		long long K_band_distinct    = (long long)(std::unique(hi.begin(), hi.end()) - hi.begin());
		long long K_band_distinct_lo = (long long)(std::unique(lo.begin(), lo.end()) - lo.begin());
		auto te0 = clk::now();
		std::vector<Point> reps;
		exact_ns::Result R = exact_ns::analyze(discs, reps);
		auto te1 = clk::now();
		double t_exact_ms = std::chrono::duration_cast<ns>(te1 - te0).count()/1e6;
		x3_dump(++g_build_id, n, R, m_ex, K_band, K_band_distinct, K_band_distinct_lo,
		        degen_band_local, t_exact_ms);
		(void)reps;  // exact reps are measurement only; not fed to the decider
		// Decider uses the proven band-on survivor set (= arm D1) so the answer is
		// guaranteed correct; the exact enumeration above is pure verification and
		// is logged to the X3 CSV only. (Feeding one point per distinct maximal
		// MASK would drop the other maximal vertices — distinct translations the
		// decider still needs — so we never do that.)
		++g_stats.calls; g_stats.before += m_ex; g_stats.after += (long long)cands.size();
		g_stats.sweep_ns += std::chrono::duration_cast<ns>(t1 - t0).count();
		for (auto const& c : cands) data->points.push_back(c.point);
		data->current = 0;
		return data;
	}

	// ---------- instrumentation ----------
	// before = m (candidates after dedup); after = survivors K.  In off/mask-only
	// no filtering happens so K == m — the design's K == m invariant for those arms.
	std::size_t const m = unique_pts.size();
	std::size_t const K = (mode == MaxMode::On) ? cands.size() : m;
	++g_stats.calls;
	g_stats.before    += (long long)m;
	g_stats.after     += (long long)K;
	g_stats.sweep_ns  += std::chrono::duration_cast<ns>(t1 - t0).count();
	g_stats.mask_ns   += std::chrono::duration_cast<ns>(t2 - t1).count();
	g_stats.filter_ns += std::chrono::duration_cast<ns>(t3 - t2).count();

	// ---------- emit candidate translations ----------
	// `on` emits the K survivors; `off`/`mask-only` emit all m deduped points, so
	// the decider is called on the full candidate set (arm D0).
	if (mode == MaxMode::On) {
		for (auto const& c : cands)
			data->points.push_back(c.point);
	} else {
		data->points.reserve(unique_pts.size());
		for (auto const& p : unique_pts)
			data->points.push_back(Point{p.first, p.second});
	}
	data->current = 0;
	return data;
}

} // anonymous namespace

// ── Public ArrangementTraversal interface ─────────────────────────────────────
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

bool ArrangementTraversal::hasNext()
	{ return data->current < data->points.size(); }

Point ArrangementTraversal::getNext()
{
	++g_stats.queries;   // INSTRUMENTATION
	std::size_t i = data->current++;
	if (i < data->ply.size()) { data->last_ply = data->ply[i]; data->last_ismax = data->ismax[i]; }
	return data->points[i];
}

std::size_t ArrangementTraversal::getSize() const { return size; }

std::uint32_t ArrangementTraversal::lastPly() const { return data->last_ply; }
bool ArrangementTraversal::lastIsMaximal() const { return data->last_ismax != 0; }

} // namespace cgal_disk_arrangements
