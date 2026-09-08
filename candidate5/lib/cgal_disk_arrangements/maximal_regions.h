#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Maximal-region enumeration for an EQUAL-RADIUS disc set, without ever building
// the arrangement (maximal_region_pipeline.md, Stages 0–5).
//
// What we are actually after (Prop. 1 of the design note): for the FUT decider a
// translation t matters only through the index set S(t) = { k : |t − c_k| ≤ r }
// of discs containing it, and the decider is monotone in S.  The inclusion-
// maximal S(t) over t ∈ R² are exactly the inclusion-maximal simplices of the
// Čech complex of {D_k}, so no vertex/face enumeration is needed — deciding
// "is S a simplex?" suffices.
//
// Two facts collapse that decision to a tiny polynomial predicate system:
//   • Helly (Prop. 2): discs are convex in the plane ⇒ Helly number 3, so
//     ∩_{k∈S} D_k ≠ ∅ ⟺ every 3-subset of S has a common point.
//   • Equal radii (Prop. 3): ∩_{k∈S} D_k ≠ ∅ ⟺ MEC(centers of S) has radius ≤ r,
//     and for |S| ≤ 3 that is a sqrt-free polynomial inequality (P2 / P3).
//
// Consequences that matter for this code base: no sqrt in the predicates, no
// circle–circle intersection points, no coordinate-tolerance dedup, and exactly
// ONE emitted witness translation per maximal region instead of the ~18 boundary
// vertices per region the arrangement path emits (P0-b).
// ─────────────────────────────────────────────────────────────────────────────

#include "disc_arrangement_traversal.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cgal_disk_arrangements
{
namespace maxregion
{

using Mask = std::vector<std::uint64_t>;

// One inclusion-maximal region: the index set, plus a witness translation.
struct Region {
	Point         witness;  // MEC centre of the region's disc centres (Stage 5)
	Mask          mask;     // GLOBAL disc indices, aliases expanded (Stage 4)
	std::uint32_t ply;      // popcount(mask)
};

struct Params {
	// Relative leniency applied to r² (NOT to r): every predicate is evaluated at
	// radius r√(1+band).  The predicates are polynomial and sqrt-free, so the √ε
	// amplification that forced the arrangement path to 1e-7 is gone and 1e-12 is
	// ample.  Leniency only ever merges a region into a slightly larger one, so
	// the emitted family stays an antichain and stays covering.
	double      band     = 1e-12;
	// Components larger than this are handed back to the caller (`overflow`)
	// instead of being run through the 2^m downward-closed DP.
	std::size_t dp_limit = 20;
};

struct Stats {
	long long calls          = 0;  // enumerate() invocations
	long long discs          = 0;  // Σ n
	long long groups         = 0;  // Σ M (after coincident-centre merge)
	long long comps          = 0;  // Σ #connected components
	long long max_comp       = 0;  // largest component size seen
	long long dp_masks       = 0;  // Σ 2^m over DP'd components
	long long regions        = 0;  // Σ #maximal regions emitted
	long long singleton      = 0;  // regions coming from size-1 components
	long long overflow_comps = 0;  // components deferred to the caller
	long long mec_fail       = 0;  // MEC radius > r(1+band) after the DP said valid
	long long p3_acute       = 0;  // P3 evaluations that reached the circumcircle branch
	long long p3_sliver      = 0;  // …of those, how many are slivers (sin < 1e-4 at the
	                               // first-argument vertex) — the configuration a
	                               // fixed-apex P3 would evaluate ill-conditioned
	long long pre_ns  = 0, dp_ns = 0, mec_ns = 0;
};

struct Result {
	std::vector<Region>                   regions;
	// Global disc indices of each component too large for the DP.  The caller is
	// responsible for covering those (candidate4 falls back to the proven
	// arrangement-vertex path restricted to the component).
	std::vector<std::vector<std::size_t>> overflow;
};

// Enumerate all inclusion-maximal regions of `discs` (all radii assumed equal).
// `stats`, when non-null, is accumulated into.
Result enumerate(Discs const& discs, Params const& params, Stats* stats);

// Process-wide accumulator; printed as `[maxregion-stats]` at exit.
Stats& global_stats();

} // namespace maxregion
} // namespace cgal_disk_arrangements
