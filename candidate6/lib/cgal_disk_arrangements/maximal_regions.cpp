#include "maximal_regions.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <boost/multiprecision/cpp_int.hpp>
#include <utility>

namespace cgal_disk_arrangements
{
namespace maxregion
{

namespace
{

using clk = std::chrono::steady_clock;
using ns  = std::chrono::nanoseconds;

inline int popcnt64(std::uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_popcountll(x);
#else
	int r = 0; while (x) { x &= x - 1; ++r; } return r;
#endif
}

inline int popcnt32(std::uint32_t x) { return popcnt64((std::uint64_t)x); }

inline int lowest_bit(std::uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_ctz(x);
#else
	int i = 0; while (!((x >> i) & 1u)) ++i; return i;
#endif
}

// ── Stage 1 predicates ────────────────────────────────────────────────────────
// Both are sqrt-free and division-free polynomial inequalities in the input
// coordinates; `four_r2` carries the band as 4r²(1+band), i.e. the same code is
// reused verbatim for the proximity graph, the pair table and the triple table.

// P2: the two discs meet  ⟺  |c_i − c_j|² ≤ 4r².   (degree 2)
inline bool pred2(double dx, double dy, double four_r2) {
	return dx*dx + dy*dy <= four_r2;
}

// P3: the three discs have a common point  ⟺  MEC(A,B,C) has radius ≤ r.
// The MEC of three points is the longest-side diameter when the triangle is
// right/obtuse (c2 ≥ a2 + b2 with c the longest side) and the circumcircle when
// it is acute.  Written against squared quantities this is
//     obtuse : c2 ≤ 4r²
//     acute  : R² = a2·b2·c2 / (4·cross²) ≤ r²  ⟺  a2·b2·c2 ≤ 4r²·cross²
// which is sqrt- and division-free (degree 6).  Collinear points always land in
// the obtuse branch (c2 = a2 + b2 + 2√(a2·b2) ≥ a2 + b2), so cross = 0 never
// reaches a denominator.
//
// `cross` MUST be evaluated at the vertex opposite the longest side, i.e. at the
// LARGEST angle.  |cross| = |u|·|v|·sin θ is a 2×2 determinant of two terms of
// size |u|·|v|, so its relative error is ≈ ε/sin θ; and while an acute triangle's
// largest angle is bounded below by 60° (sin ≥ √3/2, no cancellation), its
// smallest angle is not bounded at all — (89°, 89°, 2°) is acute.  Taking the
// apex from a fixed argument position instead would put sin θ → 0 exactly in the
// configuration this data set produces in bulk: two centres nearly coincident
// (curve1[i] − curve2[j] with near-repeated curve points, which the bitwise
// coincidence merge does not catch) and the third far away.  Measured relative
// error of the implied 4R² over random slivers:
//
//     sin θ        apex = fixed arg      apex = largest angle
//     1e-3              2.8e-13                 7.5e-16
//     1e-4              2.5e-12                 7.4e-16
//     1e-5              2.9e-11                 6.6e-16
//
// i.e. the fixed-argument form breaks out of `band` = 1e-12 below sin θ ≈ 1e-4,
// and the error direction is the harmful one: rejecting a valid triple deletes
// every superset of it through the Helly recursion, so a genuine maximal region
// is LOST, not merely duplicated.  See test_maximal_regions.cpp `[P3 sliver]`.
bool pred3(double ax, double ay, double bx, double by, double cx, double cy,
           double four_r2, Stats& S)
{
	double const abx = bx - ax, aby = by - ay;   // A→B
	double const acx = cx - ax, acy = cy - ay;   // A→C
	double const bcx = cx - bx, bcy = cy - by;   // B→C

	double const a2 = bcx*bcx + bcy*bcy;         // |B−C|², opposite A
	double const b2 = acx*acx + acy*acy;         // |C−A|², opposite B
	double const c2 = abx*abx + aby*aby;         // |A−B|², opposite C

	// Longest side, the other two, and the two edges leaving the vertex opposite
	// it.  Negation is exact, so the edge vectors cost no extra rounding.
	double hi, s1, s2, ux, uy, vx, vy;
	if (a2 >= b2 && a2 >= c2) {                  // longest |B−C| → apex A
		hi = a2; s1 = b2; s2 = c2;
		ux =  abx; uy =  aby; vx =  acx; vy =  acy;
	} else if (b2 >= c2) {                       // longest |C−A| → apex B
		hi = b2; s1 = a2; s2 = c2;
		ux = -abx; uy = -aby; vx =  bcx; vy =  bcy;
	} else {                                     // longest |A−B| → apex C
		hi = c2; s1 = a2; s2 = b2;
		ux = -acx; uy = -acy; vx = -bcx; vy = -bcy;
	}

	if (hi >= s1 + s2)                           // right or obtuse → diameter
		return hi <= four_r2;

	// The branch test itself cancels when the triangle is nearly right, but the
	// two formulas agree exactly there (a right triangle's circumradius IS half
	// its hypotenuse), so a misclassification at the boundary is continuous.
	double const cross = ux*vy - uy*vx;          // 2·signed area, widest angle

	// Exposure counter for the conditioning issue above: how often would a
	// fixed-argument apex (vertex A) have been a sliver?  |cross_A| =
	// |AB|·|AC|·sin A and |cross| is label-independent, so sin A < 1e-4 ⟺
	// cross² < 1e-8·b2·c2.  Two multiplies, acute branch only.
	++S.p3_acute;
	if (cross*cross < 1e-8 * b2 * c2) ++S.p3_sliver;

	return a2*b2*c2 <= four_r2 * (cross*cross);
}

// ── Exact mode: filtered predicates with a rational fallback ──────────────────
// Every input is a double, so the exact value of each predicate is a rational
// number.  We evaluate in double first, bound the rounding error of the
// comparison rigorously (u = 2^-53 unit roundoff; fl(a∘b) = (a∘b)(1+d), |d| ≤ u,
// and the coordinate differences are correctly rounded so they carry relative
// error ≤ u), and fall back to boost::multiprecision::cpp_rational only when the
// bound does not certify the sign.  The rational path uses the exact radius r,
// never the rounded four_r2.
using Q = boost::multiprecision::cpp_rational;
constexpr double U = std::numeric_limits<double>::epsilon() / 2.0;   // 2^-53

inline bool pred2_exact(double xi, double yi, double xj, double yj, double r,
                        double four_r2, Stats& S)
{
	++S.p2_calls;
	double const dx = xj - xi, dy = yj - yi;
	double const lhs = dx*dx + dy*dy;
	// dx² and dy² carry ≤ 3u relative error, their sum ≤ 5u (no cancellation);
	// four_r2 = 4·fl(r·r) carries ≤ u.  8u·(lhs + four_r2) therefore bounds the
	// rounding error of the difference lhs − four_r2.
	double const err = 8.0 * U * (lhs + four_r2);
	if (std::abs(lhs - four_r2) > err) return lhs <= four_r2;
	++S.p2_exact;
	Q const qdx = Q(xj) - Q(xi), qdy = Q(yj) - Q(yi), qr = Q(r);
	return qdx*qdx + qdy*qdy <= 4*qr*qr;
}

// Exact P3 in rational arithmetic (MEC of three points has radius ≤ r).
// |cross| = 2·area is independent of the apex, so any vertex may serve.
inline bool pred3_rational(double ax, double ay, double bx, double by,
                           double cx, double cy, double r)
{
	Q const abx = Q(bx)-Q(ax), aby = Q(by)-Q(ay);
	Q const acx = Q(cx)-Q(ax), acy = Q(cy)-Q(ay);
	Q const bcx = Q(cx)-Q(bx), bcy = Q(cy)-Q(by);
	Q const a2 = bcx*bcx + bcy*bcy, b2 = acx*acx + acy*acy, c2 = abx*abx + aby*aby;
	Q hi = a2, s1 = b2, s2 = c2;
	if (b2 >= a2 && b2 >= c2) { hi = b2; s1 = a2; s2 = c2; }
	else if (c2 >= a2 && c2 >= b2) { hi = c2; s1 = a2; s2 = b2; }
	Q const four_r2 = 4 * Q(r) * Q(r);
	if (hi >= s1 + s2) return hi <= four_r2;                  // right/obtuse
	Q const cross = abx*acy - aby*acx;
	return a2*b2*c2 <= four_r2 * cross * cross;               // acute
}

inline bool pred3_exact(double ax, double ay, double bx, double by, double cx, double cy,
                        double r, double four_r2, Stats& S)
{
	++S.p3_calls;
	double const abx = bx - ax, aby = by - ay;
	double const acx = cx - ax, acy = cy - ay;
	double const bcx = cx - bx, bcy = cy - by;
	double const a2 = bcx*bcx + bcy*bcy;
	double const b2 = acx*acx + acy*acy;
	double const c2 = abx*abx + aby*aby;

	double hi, s1, s2, ux, uy, vx, vy;
	if (a2 >= b2 && a2 >= c2) { hi = a2; s1 = b2; s2 = c2; ux =  abx; uy =  aby; vx =  acx; vy =  acy; }
	else if (b2 >= c2)        { hi = b2; s1 = a2; s2 = c2; ux = -abx; uy = -aby; vx =  bcx; vy =  bcy; }
	else                      { hi = c2; s1 = a2; s2 = b2; ux = -acx; uy = -acy; vx = -bcx; vy = -bcy; }

	// Each squared length carries ≤ 5u relative error.  If the right/obtuse test
	// is within its rounding error, the branch itself is uncertain → rational.
	if (std::abs(hi - (s1 + s2)) <= 6.0 * U * (hi + s1 + s2)) {
		++S.p3_exact; return pred3_rational(ax, ay, bx, by, cx, cy, r);
	}
	if (hi >= s1 + s2) {                                      // diameter branch
		if (std::abs(hi - four_r2) > 8.0 * U * (hi + four_r2)) return hi <= four_r2;
		++S.p3_exact; return pred3_rational(ax, ay, bx, by, cx, cy, r);
	}
	++S.p3_acute;
	double const p = ux*vy, q = uy*vx;
	double const cr = p - q;                                  // 2·signed area
	if (cr*cr < 1e-8 * b2 * c2) ++S.p3_sliver;
	// cr: each product ≤ 3u relative, the difference adds ≤ u of |cr| — absolute
	// error ≤ 5u·(|p|+|q|).  lhs = a2·b2·c2 ≤ 18u relative.  rhs = four_r2·cr²:
	// propagate the absolute error of cr through the square, plus 4u relative.
	double const lhs = a2*b2*c2;
	double const rhs = four_r2 * (cr*cr);
	double const dc  = 5.0 * U * (std::abs(p) + std::abs(q));
	double const err = 18.0 * U * lhs + four_r2 * (2.0*std::abs(cr)*dc + dc*dc) + 4.0 * U * rhs;
	if (std::abs(lhs - rhs) > err) return lhs <= rhs;
	++S.p3_exact;
	return pred3_rational(ax, ay, bx, by, cx, cy, r);
}

// ── Stage 5 helper: minimum enclosing circle (Welzl, incremental) ─────────────
struct Disk2 { double x, y, r2; };

inline bool mec_contains(Disk2 const& d, double px, double py) {
	double const dx = px - d.x, dy = py - d.y;
	return dx*dx + dy*dy <= d.r2 * (1.0 + 1e-12);
}

inline Disk2 mec_from2(double ax, double ay, double bx, double by) {
	double const mx = 0.5*(ax + bx), my = 0.5*(ay + by);
	double const dx = ax - mx, dy = ay - my;
	return Disk2{mx, my, dx*dx + dy*dy};
}

Disk2 mec_from3(double ax, double ay, double bx, double by, double cx, double cy)
{
	double const bpx = bx - ax, bpy = by - ay;
	double const cpx = cx - ax, cpy = cy - ay;
	double const b2 = bpx*bpx + bpy*bpy;
	double const c2 = cpx*cpx + cpy*cpy;
	double const den = 2.0 * (bpx*cpy - bpy*cpx);

	// Near-collinear: the circumcircle is numerically meaningless.  Fall back to
	// the smallest two-point circle that still covers the third point.
	if (den*den <= 1e-28 * b2 * c2) {
		Disk2 best = mec_from2(ax, ay, bx, by);
		Disk2 cand = mec_from2(ax, ay, cx, cy);
		if (cand.r2 > best.r2) best = cand;
		cand = mec_from2(bx, by, cx, cy);
		if (cand.r2 > best.r2) best = cand;
		return best;
	}

	double const ux = (cpy*b2 - bpy*c2) / den;
	double const uy = (bpx*c2 - cpx*b2) / den;
	return Disk2{ax + ux, ay + uy, ux*ux + uy*uy};
}

// Deterministic shuffle: the incremental MEC is O(n) expected only on a random
// order, and a fixed seed keeps candidate4's output bit-reproducible.
inline std::uint32_t xorshift(std::uint32_t& s) {
	s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s;
}

Disk2 mec(std::vector<std::pair<double,double>>& p) {
	std::size_t const n = p.size();
	if (n == 0) return Disk2{0.0, 0.0, 0.0};
	std::uint32_t seed = 0x9E3779B9u ^ (std::uint32_t)n;
	for (std::size_t i = n; i-- > 1; )
		std::swap(p[i], p[xorshift(seed) % (std::uint32_t)(i + 1)]);

	Disk2 d{p[0].first, p[0].second, 0.0};
	for (std::size_t i = 1; i < n; ++i) {
		if (mec_contains(d, p[i].first, p[i].second)) continue;
		d = Disk2{p[i].first, p[i].second, 0.0};
		for (std::size_t j = 0; j < i; ++j) {
			if (mec_contains(d, p[j].first, p[j].second)) continue;
			d = mec_from2(p[i].first, p[i].second, p[j].first, p[j].second);
			for (std::size_t k = 0; k < j; ++k) {
				if (mec_contains(d, p[k].first, p[k].second)) continue;
				d = mec_from3(p[i].first, p[i].second,
				              p[j].first, p[j].second,
				              p[k].first, p[k].second);
			}
		}
	}
	return d;
}

// ── DP bit table over 2^m masks ───────────────────────────────────────────────
struct BitTable {
	std::vector<std::uint64_t> w;
	explicit BitTable(std::size_t bits) : w((bits + 63) / 64, 0ULL) {}
	inline bool get(std::uint32_t i) const { return (w[i >> 6] >> (i & 63)) & 1ULL; }
	inline void set(std::uint32_t i)       { w[i >> 6] |= 1ULL << (i & 63); }
};

} // anonymous namespace

Stats& global_stats() {
	struct Holder {
		Stats s;
		~Holder() {
			if (!s.calls) return;
			std::printf(
				"[maxregion-stats] calls=%lld discs=%lld groups=%lld comps=%lld"
				" max_comp=%lld dp_masks=%lld regions=%lld singleton=%lld"
				" overflow=%lld mec_fail=%lld p3_acute=%lld p3_sliver=%lld"
				" p2_calls=%lld p2_exact=%lld p3_calls=%lld p3_exact=%lld"
				" pre_ms=%.1f dp_ms=%.1f mec_ms=%.1f"
				" blk_calls=%lld blk_hull=%lld blk_drop=%lld blk_ms=%.1f max_excess=%.3e max_r=%.4g"
				" q1_calls=%lld q2_calls=%lld q2_exact=%lld wit_exact=%lld\n",
				s.calls, s.discs, s.groups, s.comps, s.max_comp, s.dp_masks,
				s.regions, s.singleton, s.overflow_comps, s.mec_fail,
				s.p3_acute, s.p3_sliver,
				s.p2_calls, s.p2_exact, s.p3_calls, s.p3_exact,
				s.pre_ns/1e6, s.dp_ns/1e6, s.mec_ns/1e6,
				s.blk_calls, s.blk_hull, s.blk_drop, s.blk_ns/1e6, s.max_excess, s.max_r,
				s.q1_calls, s.q2_calls, s.q2_exact, s.wit_exact);
		}
	};
	static Holder h;
	return h.s;
}

Result enumerate(Discs const& discs, Params const& params, Stats* stats)
{
	Result out;
	std::size_t const n = discs.size();
	if (!n) return out;

	Stats local;
	Stats& S = stats ? *stats : local;
	++S.calls;
	S.discs += (long long)n;

	double const r  = discs[0].radius;
	double const r2 = r * r;

#ifndef NDEBUG
	for (auto const& d : discs)
		assert(std::abs(d.radius - r) <= 1e-9 * (1.0 + std::abs(r)) &&
		       "maxregion::enumerate assumes all discs share a common radius");
#endif

	// One lenient threshold, used by P2, P3 and the proximity graph alike, so the
	// three stages agree on exactly which radius they are talking about.
	bool const   exact   = params.exact;
	double const band    = exact ? 0.0 : params.band;
	double const four_r2 = 4.0 * r2 * (1.0 + band);    // radius r√(1+band); exact mode: 4·fl(r²), filter only

	// The DP is Θ(m·2^m) while the arrangement fall-back is polynomial (~m⁴ for the
	// vertex sweep plus its filter), so the DP is only the cheaper *producer* below
	// m ≈ 10; above that it is bought for what it saves downstream (one witness per
	// region instead of every boundary vertex).  That trade stops paying somewhere
	// past m ≈ 16, and by m = 24 a single build is ~400 M bit-ops.  Hence the hard
	// ceiling: exceeding it is always safe — the component is handed back and the
	// arrangement path solves it exactly — whereas honouring a large dp_limit would
	// look like a hang.
	std::size_t dp_limit = params.dp_limit;
	if (dp_limit < 1)  dp_limit = 1;
	if (dp_limit > 24) dp_limit = 24;

	auto t0 = clk::now();

	// ── Stage 0.1: merge exactly-coincident centres ───────────────────────────
	// c_k = curve1[i] − curve2[j], so curves that share structure produce bitwise
	// identical centres in bulk.  Merging is not an optimisation detail: it is the
	// only thing that keeps the 2^m term honest, and unlike the arrangement path's
	// "skip near-coincident pairs" it cannot lose the relations those discs carry
	// (identical centre + identical radius ⇒ identical disc).
	std::vector<std::size_t> ord(n);
	for (std::size_t i = 0; i < n; ++i) ord[i] = i;
	std::sort(ord.begin(), ord.end(), [&](std::size_t a, std::size_t b) {
		if (discs[a].center.x != discs[b].center.x)
			return discs[a].center.x < discs[b].center.x;
		return discs[a].center.y < discs[b].center.y;
	});

	std::size_t const words = (n + 63) / 64;
	std::vector<double> gx, gy;      // group representative centres
	std::vector<Mask>   alias;       // group → global disc indices
	gx.reserve(n); gy.reserve(n); alias.reserve(n);
	for (std::size_t t = 0; t < n; ++t) {
		std::size_t const k = ord[t];
		double const x = discs[k].center.x, y = discs[k].center.y;
		if (alias.empty() || x != gx.back() || y != gy.back()) {
			gx.push_back(x); gy.push_back(y);
			alias.push_back(Mask(words, 0ULL));
		}
		alias.back()[k / 64] |= 1ULL << (k & 63);
	}
	std::size_t const M = gx.size();
	S.groups += (long long)M;

	// ── Stage 0.2: proximity graph + connected components ─────────────────────
	// Discs in different components are more than 2r apart, so they never share a
	// point: a maximal region never spans components and each one is solved
	// independently.  This is what keeps the 2^m term local instead of global.
	std::vector<std::size_t> parent(M);
	for (std::size_t i = 0; i < M; ++i) parent[i] = i;
	// iterative find with path halving
	auto find = [&](std::size_t x) {
		while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
		return x;
	};
	for (std::size_t i = 0; i < M; ++i)
		for (std::size_t j = i + 1; j < M; ++j)
			if (exact ? pred2_exact(gx[i], gy[i], gx[j], gy[j], r, four_r2, S)
			          : pred2(gx[j] - gx[i], gy[j] - gy[i], four_r2)) {
				std::size_t a = find(i), b = find(j);
				if (a != b) parent[a] = b;
			}

	std::vector<std::vector<std::size_t>> comps;
	{
		std::vector<std::size_t> slot(M, (std::size_t)-1);
		for (std::size_t i = 0; i < M; ++i) {
			std::size_t const root = find(i);
			if (slot[root] == (std::size_t)-1) { slot[root] = comps.size(); comps.push_back({}); }
			comps[slot[root]].push_back(i);
		}
	}
	S.comps += (long long)comps.size();

	auto t1 = clk::now();
	S.pre_ns += std::chrono::duration_cast<ns>(t1 - t0).count();

	std::vector<std::pair<double,double>> pts;   // scratch for the MEC

	for (auto const& comp : comps) {
		std::size_t const m = comp.size();
		if ((long long)m > S.max_comp) S.max_comp = (long long)m;

		// Size-1 component: the disc meets nothing, so {k} is trivially maximal.
		if (m == 1) {
			Region reg;
			reg.witness = Point{gx[comp[0]], gy[comp[0]]};
			reg.mask    = alias[comp[0]];
			reg.ply     = 0;
			for (auto w : reg.mask) reg.ply += (std::uint32_t)popcnt64(w);
			out.regions.push_back(std::move(reg));
			++S.singleton; ++S.regions;
			continue;
		}

		// Too large for the 2^m DP: hand the component back to the caller.
		// Warn once per process.  Overflow is exact but it turns the pipeline into
		// "arrangement path plus a wasted pre-pass", and it is otherwise invisible:
		// raising the driver's CUT_LIMIT past MAXREGION_DP_LIMIT (the shipped
		// sweeps use 24 and 48) silently disables everything candidate 4 does.
		if (m > dp_limit) {
			static bool warned = false;
			if (!warned) {
				warned = true;
				std::fprintf(stderr,
					"[maxregion] component of %zu discs exceeds MAXREGION_DP_LIMIT=%zu:"
					" falling back to the arrangement path for it. The pipeline is"
					" inactive for such components; see `overflow=` in"
					" [maxregion-stats].\n", m, dp_limit);
			}
			Mask covered(words, 0ULL);
			for (std::size_t g : comp)
				for (std::size_t wi = 0; wi < words; ++wi) covered[wi] |= alias[g][wi];
			std::vector<std::size_t> globals;
			for (std::size_t k = 0; k < n; ++k)
				if (covered[k / 64] & (1ULL << (k & 63))) globals.push_back(k);
			out.overflow.push_back(std::move(globals));
			++S.overflow_comps;
			continue;
		}

		auto tdp0 = clk::now();

		// ── Stage 2: downward-closed DP over the component's 2^m subsets ──────
		// `valid` is the exact Čech predicate evaluated at radius r√(1+band): P2
		// for pairs, P3 for triples, and for |Mm| ≥ 4 the Helly-3 recursion
		// "every (|Mm|−1)-subset is valid" — every 3-subset of Mm lives in at
		// least one of those, and all of them are already computed because
		// Mm \ {k} < Mm as an integer.
		//
		// One table, not a lenient/strict pair.  The arrangement path needs two
		// bands because its candidates SIT on the circles, so hi and lo disagree
		// at ~90% of its vertices; here the predicates are polynomial and the band
		// is 1e-12, and a second strict table only produces redundant output:
		// at an exact tangency d = 2r the pair is hi-valid but not lo-valid, so a
		// strict extension test fails to prune the singletons and {0}, {1}, {0,1}
		// are all emitted.  Using one lenient table keeps the family an antichain
		// and stays safe, because a genuinely feasible set is lenient-valid and
		// therefore sits inside some emitted set, whose witness misses that set's
		// discs by at most r·band/2 ≈ 5e-13·r.
		std::uint32_t const full = (m == 32) ? 0xFFFFFFFFu : ((1u << m) - 1u);
		std::size_t const  nmask = (std::size_t)full + 1;
		S.dp_masks += (long long)nmask;

		BitTable valid(nmask);
		valid.set(0);

		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			int const p = popcnt32(Mm);
			if (p == 1) { valid.set(Mm); continue; }
			if (p == 2) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b);
				double const dx = gx[comp[j]] - gx[comp[i]];
				double const dy = gy[comp[j]] - gy[comp[i]];
				if (exact ? pred2_exact(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]], r, four_r2, S)
				          : pred2(dx, dy, four_r2)) valid.set(Mm);
				continue;
			}
			if (p == 3) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b); b &= b - 1;
				int const k = lowest_bit(b);
				// P3 ⇒ all three P2 (MEC ≤ r ⇒ every pair within 2r), so the pair
				// predicate does not need to be re-checked here.
				if (exact ? pred3_exact(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]],
				                        gx[comp[k]], gy[comp[k]], r, four_r2, S)
				          : pred3(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]],
				                  gx[comp[k]], gy[comp[k]], four_r2, S)) valid.set(Mm);
				continue;
			}
			bool ok = true;
			for (std::uint32_t b = Mm; b; b &= b - 1) {
				if (!valid.get(Mm ^ (1u << lowest_bit(b)))) { ok = false; break; }
			}
			if (ok) valid.set(Mm);
		}

		auto tdp1 = clk::now();
		S.dp_ns += std::chrono::duration_cast<ns>(tdp1 - tdp0).count();

		// ── Stage 3: maximal extraction ───────────────────────────────────────
		// Downward closure makes maximality a one-step local test: Mm is maximal
		// iff it is valid and no single-element extension is.
		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			if (!valid.get(Mm)) continue;
			bool maximal = true;
			for (std::uint32_t b = full ^ Mm; b; b &= b - 1) {
				if (valid.get(Mm | (1u << lowest_bit(b)))) { maximal = false; break; }
			}
			if (!maximal) continue;

			// ── Stage 4: alias expansion back to global disc indices ──────────
			Region reg;
			reg.mask.assign(words, 0ULL);
			pts.clear();
			for (std::uint32_t b = Mm; b; b &= b - 1) {
				std::size_t const g = comp[lowest_bit(b)];
				for (std::size_t wi = 0; wi < words; ++wi) reg.mask[wi] |= alias[g][wi];
				pts.push_back({gx[g], gy[g]});
			}
			reg.ply = 0;
			for (auto w : reg.mask) reg.ply += (std::uint32_t)popcnt64(w);

			// ── Stage 5: witness translation = MEC centre ─────────────────────
			// Among all points of ∩_{k∈S} D_k the MEC centre minimises
			// max_k |p − c_k|, i.e. it maximises the margin to every bounding
			// circle — the single most robust representative of the region, which
			// is what makes one witness per region enough for the decider.
			auto tm0 = clk::now();
			Disk2 const d = mec(pts);
			S.mec_ns += std::chrono::duration_cast<ns>(clk::now() - tm0).count();
			if (d.r2 > r2 * (1.0 + band) * (1.0 + 1e-9)) ++S.mec_fail;

			reg.witness = Point{d.x, d.y};
			out.regions.push_back(std::move(reg));
			++S.regions;
		}
	}

	return out;
}

Result enumerate_block(Discs const& discs, std::vector<Point> const& block, double r,
                       Params const& params, Stats* stats)
{
	Result out;
	Stats local;
	Stats& S = stats ? *stats : local;
	auto tb0 = clk::now();
	++S.blk_calls;
	S.blk_hull += (long long)block.size();

	double const r2 = r * r;
	bool const   exact   = params.exact;
	double const band    = exact ? 0.0 : params.band;
	double const four_r2 = 4.0 * r2 * (1.0 + band);
	std::size_t dp_limit = params.dp_limit;
	if (dp_limit < 1)  dp_limit = 1;
	if (dp_limit > 24) dp_limit = 24;

	auto P2 = [&](double ax, double ay, double bx, double by) {
		return exact ? pred2_exact(ax, ay, bx, by, r, four_r2, S)
		             : pred2(bx - ax, by - ay, four_r2);
	};
	auto P3 = [&](double ax, double ay, double bx, double by, double cx, double cy) {
		return exact ? pred3_exact(ax, ay, bx, by, cx, cy, r, four_r2, S)
		             : pred3(ax, ay, bx, by, cx, cy, four_r2, S);
	};
	std::size_t const h = block.size();

	std::vector<std::pair<double,double>> pts;
	auto emit_block_only = [&]() {
		if (!h) return;
		pts.clear();
		for (auto const& b : block) pts.push_back({b.x, b.y});
		Disk2 const d = mec(pts);
		Region reg;
		reg.witness = Point{d.x, d.y};
		reg.ply = 0;
		out.regions.push_back(std::move(reg));
		++S.regions;
	};

	std::size_t const n = discs.size();
	if (!n) { emit_block_only(); S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count(); return out; }

	// Stage 0.1: merge exactly-coincident centres (as in enumerate()).
	std::vector<std::size_t> ord(n);
	for (std::size_t i = 0; i < n; ++i) ord[i] = i;
	std::sort(ord.begin(), ord.end(), [&](std::size_t a, std::size_t b) {
		if (discs[a].center.x != discs[b].center.x) return discs[a].center.x < discs[b].center.x;
		return discs[a].center.y < discs[b].center.y;
	});
	std::size_t const words = (n + 63) / 64;
	std::vector<double> gx, gy;
	std::vector<Mask>   alias;
	for (std::size_t t = 0; t < n; ++t) {
		std::size_t const k = ord[t];
		double const x = discs[k].center.x, y = discs[k].center.y;
		if (alias.empty() || x != gx.back() || y != gy.back()) {
			gx.push_back(x); gy.push_back(y);
			alias.push_back(Mask(words, 0ULL));
		}
		alias.back()[k / 64] |= 1ULL << (k & 63);
	}

	// Stage 0.15: drop groups that cannot meet ∩Block.
	{
		std::vector<double> kx, ky; std::vector<Mask> ka;
		for (std::size_t g = 0; g < gx.size(); ++g) {
			bool ok = true;
			for (std::size_t a = 0; a < h && ok; ++a)
				ok = P2(gx[g], gy[g], block[a].x, block[a].y);
			for (std::size_t a = 0; a < h && ok; ++a)
				for (std::size_t b = a + 1; b < h && ok; ++b)
					ok = P3(gx[g], gy[g], block[a].x, block[a].y, block[b].x, block[b].y);
			if (ok) { kx.push_back(gx[g]); ky.push_back(gy[g]); ka.push_back(alias[g]); }
			else ++S.blk_drop;
		}
		gx.swap(kx); gy.swap(ky); alias.swap(ka);
	}
	std::size_t const M = gx.size();
	if (!M) { emit_block_only(); S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count(); return out; }

	// Pair table: P2(i,j) ∧ ∀a P3(i,j,a).
	std::vector<char> pair_ok(M * M, 0);
	for (std::size_t i = 0; i < M; ++i)
		for (std::size_t j = i + 1; j < M; ++j) {
			bool ok = P2(gx[i], gy[i], gx[j], gy[j]);
			for (std::size_t a = 0; a < h && ok; ++a)
				ok = P3(gx[i], gy[i], gx[j], gy[j], block[a].x, block[a].y);
			pair_ok[i*M + j] = pair_ok[j*M + i] = ok;
		}

	// Components of the pair_ok graph.
	std::vector<std::size_t> parent(M);
	for (std::size_t i = 0; i < M; ++i) parent[i] = i;
	auto find = [&](std::size_t x) { while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; } return x; };
	for (std::size_t i = 0; i < M; ++i)
		for (std::size_t j = i + 1; j < M; ++j)
			if (pair_ok[i*M + j]) { std::size_t a = find(i), b = find(j); if (a != b) parent[a] = b; }
	std::vector<std::vector<std::size_t>> comps;
	{
		std::vector<std::size_t> slot(M, (std::size_t)-1);
		for (std::size_t i = 0; i < M; ++i) {
			std::size_t const root = find(i);
			if (slot[root] == (std::size_t)-1) { slot[root] = comps.size(); comps.push_back({}); }
			comps[slot[root]].push_back(i);
		}
	}

	auto emit = [&](std::vector<std::size_t> const& members) {
		Region reg;
		reg.mask.assign(words, 0ULL);
		pts.clear();
		for (std::size_t g : members) {
			for (std::size_t wi = 0; wi < words; ++wi) reg.mask[wi] |= alias[g][wi];
			pts.push_back({gx[g], gy[g]});
		}
		for (auto const& b : block) pts.push_back({b.x, b.y});
		reg.ply = 0;
		for (auto w : reg.mask) reg.ply += (std::uint32_t)popcnt64(w);
		Disk2 const d = mec(pts);
		if (d.r2 > r2 * (1.0 + band) * (1.0 + 1e-9)) ++S.mec_fail;
		reg.witness = Point{d.x, d.y};
		out.regions.push_back(std::move(reg));
		++S.regions;
	};

	std::vector<std::size_t> members;
	for (auto const& comp : comps) {
		std::size_t const m = comp.size();
		if (m == 1) { members.assign(1, comp[0]); emit(members); continue; }
		if (m > dp_limit) {
			Mask covered(words, 0ULL);
			for (std::size_t g : comp)
				for (std::size_t wi = 0; wi < words; ++wi) covered[wi] |= alias[g][wi];
			std::vector<std::size_t> globals;
			for (std::size_t k = 0; k < n; ++k)
				if (covered[k / 64] & (1ULL << (k & 63))) globals.push_back(k);
			out.overflow.push_back(std::move(globals));
			++S.overflow_comps;
			continue;
		}
		std::uint32_t const full = (m == 32) ? 0xFFFFFFFFu : ((1u << m) - 1u);
		BitTable valid((std::size_t)full + 1);
		valid.set(0);
		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			int const p = popcnt32(Mm);
			if (p == 1) { valid.set(Mm); continue; }
			if (p == 2) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b);
				if (pair_ok[comp[i]*M + comp[j]]) valid.set(Mm);
				continue;
			}
			bool ok = true;
			for (std::uint32_t b = Mm; b; b &= b - 1)
				if (!valid.get(Mm ^ (1u << lowest_bit(b)))) { ok = false; break; }
			if (ok && p == 3) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b); b &= b - 1;
				int const k = lowest_bit(b);
				ok = P3(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]], gx[comp[k]], gy[comp[k]]);
			}
			if (ok) valid.set(Mm);
		}
		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			if (!valid.get(Mm)) continue;
			bool maximal = true;
			for (std::uint32_t b = full ^ Mm; b; b &= b - 1)
				if (valid.get(Mm | (1u << lowest_bit(b)))) { maximal = false; break; }
			if (!maximal) continue;
			members.clear();
			for (std::uint32_t b = Mm; b; b &= b - 1) members.push_back(comp[lowest_bit(b)]);
			emit(members);
		}
	}
	S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count();
	return out;
}

namespace {
// max_i |p − c_i|² for p = (x, y)
inline double far2(std::vector<std::pair<double,double>> const& c, double x, double y) {
	double m = 0.0;
	for (auto const& q : c) { double dx = x - q.first, dy = y - q.second; double d = dx*dx + dy*dy; if (d > m) m = d; }
	return m;
}
inline double clampd(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Point of the box minimising max_i |p − c_i| (double).  On an axis-parallel
// line every |p − c_i|² is a parabola with the same leading coefficient, so the
// upper envelope's minimum is at an interval end, at a parabola vertex, or at a
// pairwise crossing.  `r2_accept` > 0 enables the clamped-MEC-centre shortcut.
double minimax_box(std::vector<std::pair<double,double>>& c, BoundingBox const& B, double& wx, double& wy, double r2_accept = 0.0)
{
	Disk2 const d = mec(c);
	if (d.x >= B.min.x && d.x <= B.max.x && d.y >= B.min.y && d.y <= B.max.y) {
		wx = d.x; wy = d.y; return far2(c, wx, wy);
	}
	if (r2_accept > 0) {
		double const qx = clampd(d.x, B.min.x, B.max.x), qy = clampd(d.y, B.min.y, B.max.y);
		double const v = far2(c, qx, qy);
		if (v <= r2_accept) { wx = qx; wy = qy; return v; }
	}
	double best = std::numeric_limits<double>::infinity();
	auto try_pt = [&](double x, double y) { double const v = far2(c, x, y); if (v < best) { best = v; wx = x; wy = y; } };
	std::size_t const m = c.size();
	for (double y0 : {B.min.y, B.max.y}) {
		try_pt(B.min.x, y0); try_pt(B.max.x, y0);
		for (std::size_t i = 0; i < m; ++i) {
			try_pt(clampd(c[i].first, B.min.x, B.max.x), y0);
			for (std::size_t j = i + 1; j < m; ++j) {
				double const xi = c[i].first, xj = c[j].first;
				if (xi == xj) continue;
				double const yi = y0 - c[i].second, yj = y0 - c[j].second;
				try_pt(clampd(((xj*xj - xi*xi) + (yj*yj - yi*yi)) / (2.0 * (xj - xi)), B.min.x, B.max.x), y0);
			}
		}
	}
	for (double x0 : {B.min.x, B.max.x}) {
		for (std::size_t i = 0; i < m; ++i) {
			try_pt(x0, clampd(c[i].second, B.min.y, B.max.y));
			for (std::size_t j = i + 1; j < m; ++j) {
				double const yi = c[i].second, yj = c[j].second;
				if (yi == yj) continue;
				double const xi = x0 - c[i].first, xj = x0 - c[j].first;
				try_pt(x0, clampd(((yj*yj - yi*yi) + (xj*xj - xi*xi)) / (2.0 * (yj - yi)), B.min.y, B.max.y));
			}
		}
	}
	return best;
}

// Exact version of minimax_box in rational arithmetic (witness fallback only).
// Every candidate — a pair midpoint or triple circumcentre for an interior
// optimum, a corner / clamped projection / clamped pairwise crossing on an edge —
// is a rational point, and the optimum is one of them.
bool minimax_box_exact(std::vector<std::pair<double,double>> const& c, BoundingBox const& B, double& wx, double& wy)
{
	std::size_t const m = c.size();
	std::vector<Q> X(m), Y(m);
	for (std::size_t i = 0; i < m; ++i) { X[i] = Q(c[i].first); Y[i] = Q(c[i].second); }
	Q const bx0 = Q(B.min.x), bx1 = Q(B.max.x), by0 = Q(B.min.y), by1 = Q(B.max.y);
	bool have = false; Q bestv, bx, by;
	auto consider = [&](Q const& px, Q const& py) {
		if (px < bx0 || px > bx1 || py < by0 || py > by1) return;
		Q v = 0;
		for (std::size_t k = 0; k < m; ++k) { Q dx = px - X[k], dy = py - Y[k]; Q d = dx*dx + dy*dy; if (d > v) v = d; }
		if (!have || v < bestv) { have = true; bestv = v; bx = px; by = py; }
	};
	auto qclamp = [](Q const& v, Q const& lo, Q const& hi) { return v < lo ? lo : (v > hi ? hi : v); };
	for (std::size_t i = 0; i < m; ++i) consider(X[i], Y[i]);
	for (std::size_t i = 0; i < m; ++i)
		for (std::size_t j = i + 1; j < m; ++j) consider((X[i] + X[j]) / 2, (Y[i] + Y[j]) / 2);
	for (std::size_t i = 0; i < m; ++i)
		for (std::size_t j = i + 1; j < m; ++j)
			for (std::size_t k = j + 1; k < m; ++k) {
				Q const bpx = X[j] - X[i], bpy = Y[j] - Y[i], cpx = X[k] - X[i], cpy = Y[k] - Y[i];
				Q const den = 2 * (bpx*cpy - bpy*cpx);
				if (den == 0) continue;
				Q const b2 = bpx*bpx + bpy*bpy, c2 = cpx*cpx + cpy*cpy;
				consider(X[i] + (cpy*b2 - bpy*c2) / den, Y[i] + (bpx*c2 - cpx*b2) / den);
			}
	for (Q const& y0 : {by0, by1}) {
		consider(bx0, y0); consider(bx1, y0);
		for (std::size_t i = 0; i < m; ++i) {
			consider(qclamp(X[i], bx0, bx1), y0);
			for (std::size_t j = i + 1; j < m; ++j) {
				if (X[i] == X[j]) continue;
				Q const di = y0 - Y[i], dj = y0 - Y[j];
				consider(qclamp(((X[j]*X[j] - X[i]*X[i]) + (dj*dj - di*di)) / (2 * (X[j] - X[i])), bx0, bx1), y0);
			}
		}
	}
	for (Q const& x0 : {bx0, bx1}) {
		for (std::size_t i = 0; i < m; ++i) {
			consider(x0, qclamp(Y[i], by0, by1));
			for (std::size_t j = i + 1; j < m; ++j) {
				if (Y[i] == Y[j]) continue;
				Q const di = x0 - X[i], dj = x0 - X[j];
				consider(x0, qclamp(((Y[j]*Y[j] - Y[i]*Y[i]) + (dj*dj - di*di)) / (2 * (Y[j] - Y[i])), by0, by1));
			}
		}
	}
	if (!have) return false;
	wx = bx.convert_to<double>(); wy = by.convert_to<double>();
	wx = clampd(wx, B.min.x, B.max.x); wy = clampd(wy, B.min.y, B.max.y);
	return true;
}

// ── Box-family predicates ─────────────────────────────────────────────────────
// |p − c|² ≤ r², exact in exact mode: pred2 tests |·|² ≤ 4h² with h = r/2, and
// halving is exact.
inline bool in_disc(double px, double py, double cx, double cy, double r, bool exact, double band, Stats& S)
{
	if (exact) return pred2_exact(cx, cy, px, py, 0.5 * r, r * r, S);
	double const dx = px - cx, dy = py - cy;
	return dx*dx + dy*dy <= r * r * (1.0 + band);
}

// sign(a + b − t2), t2 = 2t exactly; exact in exact mode.
inline int sign_sum_minus(double a, double b, double t2, bool exact)
{
	double const s = a + b, d = s - t2;
	if (!exact) return d > 0 ? 1 : (d < 0 ? -1 : 0);
	if (std::abs(d) > 2.0 * U * (std::abs(s) + std::abs(t2))) return d > 0 ? 1 : -1;
	Q const q = Q(a) + Q(b) - Q(t2);
	return q > 0 ? 1 : (q < 0 ? -1 : 0);
}

// On the line y = y0 (coordinates swapped by the caller for vertical edges) the
// chords of D(c_i, r) and D(c_j, r) overlap  ⟺  |x_i − x_j| ≤ √h_i + √h_j with
// h = r² − (y0 − y)² ≥ 0 (the caller has certified both chords exist).
// Case A: d² ≤ h_i + h_j.  Case B: (d² − h_i − h_j)² ≤ 4 h_i h_j.  sqrt-free.
bool chords_overlap(double xi, double yi, double xj, double yj, double y0, double r,
                    bool exact, double band, Stats& S)
{
	double const dx = xi - xj, di = y0 - yi, dj = y0 - yj;
	double const r2 = r * r * (exact ? 1.0 : (1.0 + band));
	double const d2 = dx*dx, hi = r2 - di*di, hj = r2 - dj*dj;
	double const A = d2 - (hi + hj);
	if (!exact) return A <= 0 || A*A <= 4.0 * hi * hj;
	++S.q2_calls;
	// Error bounds (u = 2^-53): |err(A)| ≤ 11u·m1, |err(E)| ≤ 25u·m2; margins 16u, 64u.
	double const m1 = d2 + 2.0*r2 + di*di + dj*dj;
	if (std::abs(A) > 16.0 * U * m1) {
		if (A <= 0) return true;
		double const E  = A*A - 4.0 * hi * hj;
		double const m2 = m1*m1 + 4.0 * (r2 + di*di) * (r2 + dj*dj);
		if (std::abs(E) > 64.0 * U * m2) return E <= 0;
	}
	++S.q2_exact;
	Q const qdx = Q(xi) - Q(xj), qdi = Q(y0) - Q(yi), qdj = Q(y0) - Q(yj), qr2 = Q(r) * Q(r);
	Q const qhi = qr2 - qdi*qdi, qhj = qr2 - qdj*qdj, qA = qdx*qdx - qhi - qhj;
	if (qA <= 0) return true;
	return qA*qA <= 4 * qhi * qhj;
}

// Lens D(c_i) ∩ D(c_j) meets the segment y = y0, x ∈ [x0, x1].  1-D Helly on the
// three intervals (chord_i, chord_j, segment): pairwise overlap suffices.
bool lens_meets_hseg(double xi, double yi, double xj, double yj, double y0, double x0, double x1,
                     double r, bool exact, double band, Stats& S)
{
	if (!in_disc(clampd(xi, x0, x1), y0, xi, yi, r, exact, band, S)) return false;
	if (!in_disc(clampd(xj, x0, x1), y0, xj, yj, r, exact, band, S)) return false;
	return chords_overlap(xi, yi, xj, yj, y0, r, exact, band, S);
}

// Q2: D(c_i) ∩ D(c_j) ∩ box ≠ ∅.  If the midpoint (the lens' deepest point) is in
// the box this is P2; otherwise the lens, being connected and containing the
// midpoint, meets the box iff it meets the box boundary.
bool lens_meets_box(double xi, double yi, double xj, double yj, BoundingBox const& B,
                    double r, double four_r2, bool exact, double band, Stats& S)
{
	bool const mid_in =
		sign_sum_minus(xi, xj, 2.0 * B.min.x, exact) >= 0 && sign_sum_minus(xi, xj, 2.0 * B.max.x, exact) <= 0 &&
		sign_sum_minus(yi, yj, 2.0 * B.min.y, exact) >= 0 && sign_sum_minus(yi, yj, 2.0 * B.max.y, exact) <= 0;
	if (mid_in)
		return exact ? pred2_exact(xi, yi, xj, yj, r, four_r2, S) : pred2(xj - xi, yj - yi, four_r2);
	return lens_meets_hseg(xi, yi, xj, yj, B.min.y, B.min.x, B.max.x, r, exact, band, S)
	    || lens_meets_hseg(xi, yi, xj, yj, B.max.y, B.min.x, B.max.x, r, exact, band, S)
	    || lens_meets_hseg(yi, xi, yj, xj, B.min.x, B.min.y, B.max.y, r, exact, band, S)
	    || lens_meets_hseg(yi, xi, yj, xj, B.max.x, B.min.y, B.max.y, r, exact, band, S);
}
} // anonymous namespace

Result enumerate_box(Discs const& discs, BoundingBox const& box, double r, double witness_tol,
                     Params const& params, Stats* stats)
{
	Result out;
	Stats local;
	Stats& S = stats ? *stats : local;
	auto tb0 = clk::now();
	++S.blk_calls; ++S.calls; S.discs += (long long)discs.size();

	double const r2 = r * r;
	bool const   exact   = params.exact;
	double const band    = exact ? 0.0 : params.band;
	double const four_r2 = 4.0 * r2 * (1.0 + band);
	// Witness acceptance: |w − c| ≤ r + witness_tol for every member, certified in
	// double (|err(|w−c|²)| ≤ 6u·|w−c|²), else recomputed exactly and rounded.
	double const rdec    = r + witness_tol;
	double const rdec2_c = rdec * rdec * (1.0 - 8.0 * U);
	std::size_t dp_limit = params.dp_limit;
	if (dp_limit < 1)  dp_limit = 1;
	if (dp_limit > 24) dp_limit = 24;

	std::size_t const n = discs.size();
	if (!n) { S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count(); return out; }

	// Stage 0.1: merge exactly-coincident centres.
	std::vector<std::size_t> ord(n);
	for (std::size_t i = 0; i < n; ++i) ord[i] = i;
	std::sort(ord.begin(), ord.end(), [&](std::size_t a, std::size_t b) {
		if (discs[a].center.x != discs[b].center.x) return discs[a].center.x < discs[b].center.x;
		return discs[a].center.y < discs[b].center.y;
	});
	std::size_t const words = (n + 63) / 64;
	std::vector<double> gx, gy;
	std::vector<Mask>   alias;
	for (std::size_t t = 0; t < n; ++t) {
		std::size_t const k = ord[t];
		double const x = discs[k].center.x, y = discs[k].center.y;
		if (alias.empty() || x != gx.back() || y != gy.back()) {
			gx.push_back(x); gy.push_back(y);
			alias.push_back(Mask(words, 0ULL));
		}
		alias.back()[k / 64] |= 1ULL << (k & 63);
	}

	// Q1: keep discs that meet the box (nearest box point within r).
	{
		std::vector<double> kx, ky; std::vector<Mask> ka;
		for (std::size_t g = 0; g < gx.size(); ++g) {
			++S.q1_calls;
			double const qx = clampd(gx[g], box.min.x, box.max.x), qy = clampd(gy[g], box.min.y, box.max.y);
			if (in_disc(qx, qy, gx[g], gy[g], r, exact, band, S)) { kx.push_back(gx[g]); ky.push_back(gy[g]); ka.push_back(alias[g]); }
			else ++S.blk_drop;
		}
		gx.swap(kx); gy.swap(ky); alias.swap(ka);
	}
	std::size_t const M = gx.size();
	S.groups += (long long)M;
	if (!M) { S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count(); return out; }

	std::vector<std::pair<double,double>> pts;
	double wx, wy;
	// Pair table: Q2 (implies P2).
	std::vector<char> pair_ok(M * M, 0);
	for (std::size_t i = 0; i < M; ++i)
		for (std::size_t j = i + 1; j < M; ++j) {
			bool const ok = lens_meets_box(gx[i], gy[i], gx[j], gy[j], box, r, four_r2, exact, band, S);
			pair_ok[i*M + j] = pair_ok[j*M + i] = ok;
		}

	std::vector<std::size_t> parent(M);
	for (std::size_t i = 0; i < M; ++i) parent[i] = i;
	auto find = [&](std::size_t x) { while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; } return x; };
	for (std::size_t i = 0; i < M; ++i)
		for (std::size_t j = i + 1; j < M; ++j)
			if (pair_ok[i*M + j]) { std::size_t a = find(i), b = find(j); if (a != b) parent[a] = b; }
	std::vector<std::vector<std::size_t>> comps;
	{
		std::vector<std::size_t> slot(M, (std::size_t)-1);
		for (std::size_t i = 0; i < M; ++i) {
			std::size_t const root = find(i);
			if (slot[root] == (std::size_t)-1) { slot[root] = comps.size(); comps.push_back({}); }
			comps[slot[root]].push_back(i);
		}
	}
	S.comps += (long long)comps.size();

	auto emit = [&](std::vector<std::size_t> const& members) {
		Region reg;
		reg.mask.assign(words, 0ULL);
		pts.clear();
		for (std::size_t g : members) {
			for (std::size_t wi = 0; wi < words; ++wi) reg.mask[wi] |= alias[g][wi];
			pts.push_back({gx[g], gy[g]});
		}
		reg.ply = 0;
		for (auto w : reg.mask) reg.ply += (std::uint32_t)popcnt64(w);
		auto tm0 = clk::now();
		double v = minimax_box(pts, box, wx, wy, r2);
		if (!(v * (1.0 + 8.0 * U) <= rdec2_c)) {
			// Not certified in double: recompute the optimum exactly, round, recheck.
			++S.wit_exact;
			if (minimax_box_exact(pts, box, wx, wy)) v = far2(pts, wx, wy);
			if (!(v * (1.0 + 8.0 * U) <= rdec2_c)) ++S.mec_fail;
		}
		S.mec_ns += std::chrono::duration_cast<ns>(clk::now() - tm0).count();
		{ double e = std::sqrt(v) - r; if (e > S.max_excess) S.max_excess = e; if (r > S.max_r) S.max_r = r; }
		reg.witness = Point{wx, wy};
		out.regions.push_back(std::move(reg));
		++S.regions;
	};

	std::vector<std::size_t> members;
	for (auto const& comp : comps) {
		std::size_t const m = comp.size();
		if ((long long)m > S.max_comp) S.max_comp = (long long)m;
		if (m == 1) { members.assign(1, comp[0]); emit(members); ++S.singleton; continue; }
		if (m > dp_limit) {
			Mask covered(words, 0ULL);
			for (std::size_t g : comp)
				for (std::size_t wi = 0; wi < words; ++wi) covered[wi] |= alias[g][wi];
			std::vector<std::size_t> globals;
			for (std::size_t k = 0; k < n; ++k)
				if (covered[k / 64] & (1ULL << (k & 63))) globals.push_back(k);
			out.overflow.push_back(std::move(globals));
			++S.overflow_comps;
			continue;
		}
		auto tdp0 = clk::now();
		std::uint32_t const full = (m == 32) ? 0xFFFFFFFFu : ((1u << m) - 1u);
		S.dp_masks += (long long)full + 1;
		BitTable valid((std::size_t)full + 1);
		valid.set(0);
		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			int const p = popcnt32(Mm);
			if (p == 1) { valid.set(Mm); continue; }
			if (p == 2) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b);
				if (pair_ok[comp[i]*M + comp[j]]) valid.set(Mm);
				continue;
			}
			bool ok = true;
			for (std::uint32_t b = Mm; b; b &= b - 1)
				if (!valid.get(Mm ^ (1u << lowest_bit(b)))) { ok = false; break; }
			if (ok && p == 3) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b); b &= b - 1;
				int const k = lowest_bit(b);
				ok = exact ? pred3_exact(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]], gx[comp[k]], gy[comp[k]], r, four_r2, S)
				           : pred3(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]], gx[comp[k]], gy[comp[k]], four_r2, S);
			}
			if (ok) valid.set(Mm);
		}
		S.dp_ns += std::chrono::duration_cast<ns>(clk::now() - tdp0).count();
		for (std::uint32_t Mm = 1; Mm <= full; ++Mm) {
			if (!valid.get(Mm)) continue;
			bool maximal = true;
			for (std::uint32_t b = full ^ Mm; b; b &= b - 1)
				if (valid.get(Mm | (1u << lowest_bit(b)))) { maximal = false; break; }
			if (!maximal) continue;
			members.clear();
			for (std::uint32_t b = Mm; b; b &= b - 1) members.push_back(comp[lowest_bit(b)]);
			emit(members);
		}
	}
	S.blk_ns += std::chrono::duration_cast<ns>(clk::now() - tb0).count();
	return out;
}

} // namespace maxregion
} // namespace cgal_disk_arrangements
