#include "maximal_regions.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
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
				" pre_ms=%.1f dp_ms=%.1f mec_ms=%.1f\n",
				s.calls, s.discs, s.groups, s.comps, s.max_comp, s.dp_masks,
				s.regions, s.singleton, s.overflow_comps, s.mec_fail,
				s.p3_acute, s.p3_sliver,
				s.pre_ns/1e6, s.dp_ns/1e6, s.mec_ns/1e6);
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
	double const band    = params.band;
	double const four_r2 = 4.0 * r2 * (1.0 + band);    // radius r√(1+band)

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
			if (pred2(gx[j] - gx[i], gy[j] - gy[i], four_r2)) {
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
				if (pred2(dx, dy, four_r2)) valid.set(Mm);
				continue;
			}
			if (p == 3) {
				std::uint32_t b = Mm;
				int const i = lowest_bit(b); b &= b - 1;
				int const j = lowest_bit(b); b &= b - 1;
				int const k = lowest_bit(b);
				// P3 ⇒ all three P2 (MEC ≤ r ⇒ every pair within 2r), so the pair
				// predicate does not need to be re-checked here.
				if (pred3(gx[comp[i]], gy[comp[i]], gx[comp[j]], gy[comp[j]],
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

} // namespace maxregion
} // namespace cgal_disk_arrangements
