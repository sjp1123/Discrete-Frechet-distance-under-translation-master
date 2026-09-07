#include "disc_arrangement_traversal.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include <CGAL/Iso_rectangle_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Arr_circle_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>

namespace cgal_disk_arrangements
{

using Kernel = CGAL::Epeck;
using Traits_2 = CGAL::Arr_circle_segment_traits_2<Kernel>;
using Point_2 = Kernel::Point_2;
using Iso_rectangle_2 = Kernel::Iso_rectangle_2;
using Curve_2 = Traits_2::Curve_2;
using Circle_2 = Traits_2::Kernel::Circle_2;
using Arrangement_2 = CGAL::Arrangement_2<Traits_2>;

struct CGALData {
	Arrangement_2 arr;
	std::vector<Point> points;
	std::size_t current = 0;
};

namespace
{

using clk = std::chrono::steady_clock;
using ns  = std::chrono::nanoseconds;

// ── Compile-time feature flags ────────────────────────────────────────────────
static const bool ENABLE_MAXIMAL_FILTER              = true;
static const bool ENABLE_FACE_REPRESENTATIVES        = false;
static const bool USE_DIRECT_INTERSECTION_CANDIDATES = false;
// When true, uses CGAL arrangement + BFS face-depth propagation to extract
// inclusion-wise maximal vertices without O(n^6) containment computation.
// Replaces both the direct-intersection path and the old CGAL+mask path.
static const bool USE_CGAL_DEPTH_MAXIMAL             = true;

// ── CGAL-arrangement-path instrumentation ─────────────────────────────────────
// Printed only when USE_DIRECT_INTERSECTION_CANDIDATES = false (calls > 0).
struct ArrInstrumentation {
	long long insert_ns  = 0;
	long long vertex_ns  = 0;
	long long face_ns    = 0;
	long long filter_ns  = 0;
	long long raw_total  = 0;
	long long filt_total = 0;
	long long calls      = 0;

	~ArrInstrumentation() {
		if (calls == 0) return;
		std::printf(
			"[arr-stats] calls=%lld raw=%lld filtered=%lld"
			" insert_ms=%.1f vtx_containment_ms=%.1f"
			" face_containment_ms=%.1f filter_ms=%.1f\n",
			calls, raw_total, filt_total,
			insert_ns / 1e6,
			vertex_ns / 1e6,
			face_ns   / 1e6,
			filter_ns / 1e6);
	}
} g_arr_inst;

// ── Direct-path instrumentation ───────────────────────────────────────────────
// Printed only when USE_DIRECT_INTERSECTION_CANDIDATES = true (calls > 0).
struct DirectInstrumentation {
	long long calls        = 0;
	long long disks_total  = 0;
	long long pair_tests   = 0;
	long long raw_points   = 0;
	long long unique_masks = 0;
	long long maximal      = 0;
	long long gen_ns       = 0;
	long long cont_ns      = 0;
	long long filt_ns      = 0;

	~DirectInstrumentation() {
		if (calls == 0) return;
		std::printf(
			"[direct-stats] calls=%lld disks=%lld pair_tests=%lld raw_points=%lld"
			" unique_masks=%lld maximal=%lld gen_ms=%.1f containment_ms=%.1f filter_ms=%.1f\n",
			calls, disks_total, pair_tests, raw_points,
			unique_masks, maximal,
			gen_ns  / 1e6,
			cont_ns / 1e6,
			filt_ns / 1e6);
	}
} g_direct_inst;

using Mask = std::vector<unsigned long long>;

bool intersect(Circle_2 const& circle, Iso_rectangle_2 const& rect)
{
	bool const contains0 = circle.has_on_bounded_side(rect[0]);
	bool const contains1 = circle.has_on_bounded_side(rect[1]);
	bool const contains2 = circle.has_on_bounded_side(rect[2]);
	bool const contains3 = circle.has_on_bounded_side(rect[3]);

	if (contains0 && contains1 && contains2 && contains3) {
		return false;
	}

	if (!contains0 && !contains1 && !contains2 && !contains3) {
		if (!CGAL::do_overlap(circle.bbox(), rect.bbox())) {
			return false;
		}

		if ((rect[0].x() > circle.center().x() && rect[0].y() > circle.center().y()) ||
			(rect[1].x() < circle.center().x() && rect[1].y() > circle.center().y()) ||
			(rect[3].x() > circle.center().x() && rect[3].y() < circle.center().y()) ||
			(rect[2].x() < circle.center().x() && rect[2].y() < circle.center().y())) {
			return false;
		}
	}

	return true;
}

// Containment check: is `point` inside (or on the boundary of) `disc`?
//
// Uses floating-point arithmetic with a relative epsilon that scales with r².
// A fixed absolute epsilon (e.g. 1e-10) fails for large radii: for r=1000,
// r²=10⁶ and CGAL::to_double() rounding in dist² can reach ~10⁻⁸, far
// exceeding 1e-10.  The relative form 1e-9*(1+r²) covers all practical scales.
//
// Boundary points (dist² ≈ r²) are conservatively counted as inside, so a
// vertex that lies exactly on a disc's circumference is never incorrectly
// excluded from that disc's containment set.
//
// Note: Arrangement_2::Point_2 is an algebraic _One_root_point_2 type and
// cannot be passed to Kernel::Circle_2::has_on_unbounded_side, which only
// accepts Kernel::Point_2.  An exact path for arrangement vertices would
// require squaring _One_root_number objects, which CGAL does not expose as a
// public operation.  The relative-epsilon float path is therefore the only
// sound option; it is correct because to_double() rounding at arrangement
// vertices is several orders of magnitude smaller than the relative epsilon.
bool contains_disc(Disc const& disc, Point const& point)
{
	double dx = point.x - disc.center.x;
	double dy = point.y - disc.center.y;
	double dist2 = dx * dx + dy * dy;
	double radius2 = disc.radius * disc.radius;
	// eps scales linearly with radius (not with radius²).
	// rounding error in dist2 ≈ 2 * |coord| * r * machine_eps ≈ 4.4e-16 * r,
	// so 1e-9 * r provides a large safety margin for boundary classification.
	// The old form 1e-9*(1+r²) gave a false-positive zone of ~1e-9/(2r) in
	// distance units; for tiny discs (r≈3e-4) this was ~1.7e-6, large enough
	// to misclassify nearby disk centers and corrupt the containment masks.
	double eps = 1e-9 * disc.radius;

	return dist2 <= radius2 + eps;
}

Mask make_mask(Discs const& discs, Point const& point)
{
	Mask mask((discs.size() + 63) / 64, 0);

	for (std::size_t i = 0; i < discs.size(); ++i) {
		if (contains_disc(discs[i], point)) {
			mask[i / 64] |= (1ULL << (i % 64));
		}
	}

	return mask;
}

bool equal_mask(Mask const& a, Mask const& b)
{
	return a == b;
}

bool subset_mask(Mask const& a, Mask const& b)
{
	for (std::size_t i = 0; i < a.size(); ++i) {
		if ((a[i] & ~b[i]) != 0) {
			return false;
		}
	}

	return true;
}

bool proper_subset_mask(Mask const& a, Mask const& b)
{
	return subset_mask(a, b) && !equal_mask(a, b);
}

std::size_t mask_count(Mask const& mask)
{
	std::size_t result = 0;

	for (auto x : mask) {
		while (x != 0) {
			x &= (x - 1);
			++result;
		}
	}

	return result;
}

struct Candidate {
	Point point;
	Mask mask;
	std::size_t count;
};

void filter_maximal_candidates(std::vector<Candidate>& candidates)
{
	std::sort(candidates.begin(), candidates.end(),
		[](Candidate const& a, Candidate const& b) {
			return a.count > b.count;
		});

	std::vector<Candidate> result;

	for (auto const& candidate : candidates) {
		bool dominated = false;
		bool duplicate = false;

		for (auto const& chosen : result) {
			if (equal_mask(candidate.mask, chosen.mask)) {
				duplicate = true;
				break;
			}

			if (proper_subset_mask(candidate.mask, chosen.mask)) {
				dominated = true;
				break;
			}
		}

		if (!dominated && !duplicate) {
			result.push_back(candidate);
		}
	}

	candidates.swap(result);
}

// ── Depth-based instrumentation ───────────────────────────────────────────────
struct DepthInstrumentation {
	long long calls       = 0;
	long long maximal_sum = 0;
	long long max_depth_sum = 0;
	long long arr_ns      = 0;
	long long bfs_ns      = 0;
	long long collect_ns  = 0;

	~DepthInstrumentation() {
		if (calls == 0) return;
		std::printf(
			"[depth-stats] calls=%lld maximal=%lld avg_max_depth=%.1f"
			" arr_ms=%.1f bfs_ms=%.1f collect_ms=%.1f\n",
			calls, maximal_sum,
			calls > 0 ? (double)max_depth_sum / calls : 0.0,
			arr_ns     / 1e6,
			bfs_ns     / 1e6,
			collect_ns / 1e6);
	}
} g_depth_inst;

// ── CGAL arrangement + BFS face-depth → maximal vertices ─────────────────────
// Builds the arrangement exactly as the original does, then propagates face
// depths via BFS (inner CCB crossing: +1, outer CCB crossing: -1) and
// collects vertices adjacent to the maximum-depth faces.  These are exactly
// the inclusion-wise maximal vertices, obtained in O(n^4) instead of O(n^6).
// ── Bitmask propagation via BFS ───────────────────────────────────────────────
// Propagates the exact containment bitmask from face to face.
// Crossing a halfedge h on circle C_i:
//   cross > 0 (CCW arc): h->face() is inside C_i → neighbour is outside → clear bit i
//   cross < 0 (CW  arc): h->face() is outside C_i → neighbour is inside → set bit i
CGALData* build_cgal_depth(
    std::vector<Curve_2> const& to_insert,
    Discs const& discs)
{
	using Face_handle     = Arrangement_2::Face_handle;
	using Halfedge_handle = Arrangement_2::Halfedge_handle;

	CGALData* data = new CGALData();

	// 1. Build CGAL arrangement ────────────────────────────────────────────────
	auto t_arr0 = clk::now();
	CGAL::insert(data->arr, to_insert.begin(), to_insert.end());
	auto t_arr1 = clk::now();

	// Map circle center (double) → disc index
	std::size_t n = discs.size();
	std::size_t mask_words = (n + 63) / 64;
	std::map<std::pair<double,double>, int> center_to_idx;
	for (std::size_t i = 0; i < n; ++i)
		center_to_idx[{discs[i].center.x, discs[i].center.y}] = (int)i;

	auto disc_of = [&](Halfedge_handle h) -> int {
		if (h->curve().is_linear()) return -1;
		auto circle = h->curve().supporting_circle();
		double cx = CGAL::to_double(circle.center().x());
		double cy = CGAL::to_double(circle.center().y());
		auto it = center_to_idx.find({cx, cy});
		return (it != center_to_idx.end()) ? it->second : -1;
	};

	// Cross product sign: determines whether crossing h enters or leaves circle.
	// cross > 0 → CCW arc → h->face() is interior → neighbour is exterior
	// cross < 0 → CW  arc → h->face() is exterior → neighbour is interior
	auto cross_sign = [](Halfedge_handle h) -> int {
		auto circle = h->curve().supporting_circle();
		double cx = CGAL::to_double(circle.center().x());
		double cy = CGAL::to_double(circle.center().y());
		double sx = CGAL::to_double(h->source()->point().x());
		double sy = CGAL::to_double(h->source()->point().y());
		double tx = CGAL::to_double(h->target()->point().x());
		double ty = CGAL::to_double(h->target()->point().y());
		double cross = (tx - sx) * (cy - sy) - (ty - sy) * (cx - sx);
		if (cross >  1e-10) return +1;
		if (cross < -1e-10) return -1;
		return 0;  // degenerate
	};

	// 2. BFS with bitmask propagation ─────────────────────────────────────────
	auto t_bfs0 = clk::now();
	// face → containment bitmask (which disc interiors contain this face)
	std::map<Face_handle, Mask> face_masks;
	std::queue<Face_handle> bfs_queue;

	auto unbounded = data->arr.unbounded_face();
	face_masks[unbounded] = Mask(mask_words, 0ULL);
	bfs_queue.push(unbounded);

	while (!bfs_queue.empty()) {
		Face_handle face = bfs_queue.front();
		bfs_queue.pop();
		Mask const& cur = face_masks.at(face);

		auto visit = [&](Halfedge_handle h) {
			Face_handle nb = h->twin()->face();
			if (face_masks.count(nb)) return;

			Mask nb_mask = cur;  // copy current face's mask
			int idx = disc_of(h);
			if (idx >= 0) {
				int cs = cross_sign(h);
				if (cs > 0)       // CCW: h->face() inside → neighbour outside → clear
					nb_mask[idx/64] &= ~(1ULL << (idx % 64));
				else if (cs < 0)  // CW:  h->face() outside → neighbour inside → set
					nb_mask[idx/64] |=  (1ULL << (idx % 64));
				// cs == 0: degenerate, leave mask unchanged (conservative)
			}
			face_masks[nb] = std::move(nb_mask);
			bfs_queue.push(nb);
		};

		if (face->has_outer_ccb()) {
			auto h = face->outer_ccb();
			do { visit(h); ++h; } while (h != face->outer_ccb());
		}
		for (auto hole = face->inner_ccbs_begin();
		     hole != face->inner_ccbs_end(); ++hole) {
			auto h = *hole;
			do { visit(h); ++h; } while (h != *hole);
		}
	}
	auto t_bfs1 = clk::now();

	// 3. For each arrangement vertex: union of adjacent face masks + boundary arcs
	auto t_col0 = clk::now();
	std::vector<Candidate> candidates;

	for (auto vit = data->arr.vertices_begin();
	     vit != data->arr.vertices_end(); ++vit) {
		if (vit->is_at_open_boundary()) continue;

		Mask mask(mask_words, 0ULL);

		// Union adjacent face masks and mark circles passing through this vertex
		auto h = vit->incident_halfedges();
		auto h0 = h;
		do {
			// Face to the left of h (h->face())
			if (face_masks.count(h->face())) {
				Mask const& fm = face_masks.at(h->face());
				for (std::size_t w = 0; w < mask_words; ++w)
					mask[w] |= fm[w];
			}
			// Circle whose boundary passes through this vertex
			int idx = disc_of(h);
			if (idx >= 0)
				mask[idx/64] |= (1ULL << (idx % 64));
			++h;
		} while (h != h0);

		Candidate cand;
		cand.point = {
			CGAL::to_double(vit->point().x()),
			CGAL::to_double(vit->point().y())
		};
		cand.mask  = mask;
		cand.count = mask_count(mask);
		candidates.push_back(cand);
	}

	// 4. Inclusion-wise maximal filter ────────────────────────────────────────
	if (ENABLE_MAXIMAL_FILTER)
		filter_maximal_candidates(candidates);
	auto t_col1 = clk::now();

	// Instrumentation
	++g_depth_inst.calls;
	g_depth_inst.maximal_sum   += static_cast<long long>(candidates.size());
	g_depth_inst.arr_ns     += std::chrono::duration_cast<ns>(t_arr1 - t_arr0).count();
	g_depth_inst.bfs_ns     += std::chrono::duration_cast<ns>(t_bfs1 - t_bfs0).count();
	g_depth_inst.collect_ns += std::chrono::duration_cast<ns>(t_col1 - t_col0).count();

	for (auto const& c : candidates)
		data->points.push_back(c.point);
	data->current = 0;
	return data;
}

CGALData* build(std::vector<Curve_2> const& to_insert, Discs const& discs, bool maximal_only)
{
	CGALData* data = new CGALData();

	// ── 1. Arrangement construction ──────────────────────────────────────────
	auto t_ins0 = clk::now();
	CGAL::insert(data->arr, to_insert.begin(), to_insert.end());
	auto t_ins1 = clk::now();

	std::vector<Candidate> candidates;

	// ── 2a. Containment sets for arrangement vertices ─────────────────────────
	// A vertex is at the exact intersection of two arrangement curves.
	// We convert to double for the mask check; see contains_disc for why the
	// relative-epsilon float path is sufficient even for boundary points.
	auto t_vtx0 = clk::now();
	for (auto vertex = data->arr.vertices_begin(); vertex != data->arr.vertices_end(); ++vertex) {
		auto x = CGAL::to_double(vertex->point().x());
		auto y = CGAL::to_double(vertex->point().y());

		Point point;
		point.x = x;
		point.y = y;

		auto mask = make_mask(discs, point);
		auto count = mask_count(mask);

		Candidate candidate;
		candidate.point = point;
		candidate.mask = mask;
		candidate.count = count;

		candidates.push_back(candidate);
	}
	auto t_vtx1 = clk::now();

	// ── 2b. Containment sets for bounded face representatives ─────────────────
	// Gated by ENABLE_FACE_REPRESENTATIVES.  When false the compiler eliminates
	// this block entirely; face_containment_ms will read 0 in the stats output.
	//
	// Conservative guarantee: S(face) ⊆ S(adjacent vertex) always holds (a disc
	// whose interior contains the face also contains the face's closure and hence
	// all boundary vertices).  Face candidates are therefore always dominated by
	// some vertex candidate and filtered out by filter_maximal_candidates().
	// This pass was removed because it doubles the containment cost with zero
	// benefit to the final filtered set.
	auto t_face0 = clk::now();
	if (ENABLE_FACE_REPRESENTATIVES) {
		for (auto face = data->arr.faces_begin(); face != data->arr.faces_end(); ++face) {
			if (face->is_unbounded()) continue;

			auto ccb = face->outer_ccb();
			auto he = ccb;
			double sx = 0.0;
			double sy = 0.0;
			std::size_t cnt = 0;

			do {
				sx += CGAL::to_double(he->source()->point().x());
				sy += CGAL::to_double(he->source()->point().y());
				++cnt;
				++he;
			} while (he != ccb);

			if (cnt == 0) continue;

			Point rep;
			rep.x = sx / static_cast<double>(cnt);
			rep.y = sy / static_cast<double>(cnt);

			auto mask = make_mask(discs, rep);
			auto count = mask_count(mask);

			Candidate candidate;
			candidate.point = rep;
			candidate.mask = mask;
			candidate.count = count;

			candidates.push_back(candidate);
		}
	}
	auto t_face1 = clk::now();

	long long raw = static_cast<long long>(candidates.size());

	// ── 3. Maximal filter ────────────────────────────────────────────────────
	// Gated by ENABLE_MAXIMAL_FILTER.  When false, all raw candidates are passed
	// to the Fréchet oracle (useful as a correctness baseline).
	auto t_flt0 = clk::now();
	if (maximal_only && ENABLE_MAXIMAL_FILTER) {
		filter_maximal_candidates(candidates);
	}
	auto t_flt1 = clk::now();

	long long filtered = static_cast<long long>(candidates.size());

	// ── Accumulate instrumentation ────────────────────────────────────────────
	g_arr_inst.insert_ns  += std::chrono::duration_cast<ns>(t_ins1  - t_ins0).count();
	g_arr_inst.vertex_ns  += std::chrono::duration_cast<ns>(t_vtx1  - t_vtx0).count();
	g_arr_inst.face_ns    += std::chrono::duration_cast<ns>(t_face1 - t_face0).count();
	g_arr_inst.filter_ns  += std::chrono::duration_cast<ns>(t_flt1  - t_flt0).count();
	g_arr_inst.raw_total  += raw;
	g_arr_inst.filt_total += filtered;
	++g_arr_inst.calls;

	data->points.reserve(candidates.size());

	for (auto const& candidate : candidates) {
		data->points.push_back(candidate.point);
	}

	data->current = 0;

	return data;
}

// ── Direct candidate generation (no CGAL arrangement) ────────────────────────
// Generates candidates from circle-circle intersections and disk centers.
// box may be nullptr (no filtering); when provided, only in-box points are kept.
CGALData* build_direct(Discs const& discs, BoundingBox const* box, bool maximal_only)
{
	CGALData* data = new CGALData();
	// data->arr is intentionally left empty; the CGAL arrangement is not built.

	std::size_t const n = discs.size();
	long long pair_tests_count = 0;

	// ── 1. Generate candidate points ──────────────────────────────────────────
	auto t_gen0 = clk::now();
	std::vector<Point> raw_points;

	// Every disk center: covers isolated disks that intersect no other disk.
	for (auto const& disc : discs) {
		if (!box || box->contains(disc.center)) {
			raw_points.push_back(disc.center);
		}
	}

	// Circle-circle intersection points for every ordered pair (i < j).
	for (std::size_t i = 0; i < n; ++i) {
		for (std::size_t j = i + 1; j < n; ++j) {
			++pair_tests_count;

			double cx1 = discs[i].center.x, cy1 = discs[i].center.y, r1 = discs[i].radius;
			double cx2 = discs[j].center.x, cy2 = discs[j].center.y, r2 = discs[j].radius;

			double dx = cx2 - cx1;
			double dy = cy2 - cy1;
			double d2 = dx * dx + dy * dy;
			double d  = std::sqrt(d2);

			if (d < 1e-12) continue;           // concentric or duplicate centers

			double sum_r  = r1 + r2;
			double diff_r = std::abs(r1 - r2);

			if (d > sum_r + 1e-9 || d < diff_r - 1e-9) continue; // no intersection

			// a: signed distance from c1 to the chord midpoint along the line c1→c2
			double a  = (r1 * r1 - r2 * r2 + d2) / (2.0 * d);
			double h2 = r1 * r1 - a * a;
			if (h2 < 0.0) h2 = 0.0;
			double h  = std::sqrt(h2);

			// Chord midpoint
			double px = cx1 + a * dx / d;
			double py = cy1 + a * dy / d;

			if (h < 1e-9) {
				// Tangent: one intersection point
				raw_points.push_back({px, py});
			} else {
				// Two symmetric intersection points
				// Note: box filter intentionally omitted here.  The original CGAL
				// arrangement inserts full circles (not clipped to the box) and therefore
				// includes circle-circle intersections that lie outside the bounding box.
				// Filtering them out caused the candidate to miss optimal translations
				// that sit just outside the box boundary.  Out-of-box points have
				// empty or small containment sets and are discarded by the maximal filter.
				raw_points.push_back({px + h * dy / d, py - h * dx / d});
				raw_points.push_back({px - h * dy / d, py + h * dx / d});
			}
		}
	}

	// ── Box corners and circle-box-edge intersections ─────────────────────────
	// The original CGAL arrangement inserted 4 box-boundary line segments along
	// with the circle arcs.  CGAL automatically created vertices at:
	//   ① box corners  (4 points — segment-segment intersections)
	//   ② circle-box-edge crossings  (where each circle boundary crosses a box edge)
	// The circle-circle-only approach above misses these, causing the candidate to
	// report a Fréchet distance that is too large when the optimal translation
	// lies on or near the box boundary.
	if (box) {
		double const bx0 = box->min.x, bx1 = box->max.x;
		double const by0 = box->min.y, by1 = box->max.y;

		// ① Box corners
		raw_points.push_back({bx0, by0});
		raw_points.push_back({bx1, by0});
		raw_points.push_back({bx1, by1});
		raw_points.push_back({bx0, by1});

		// ② Circle-box-edge crossings
		for (auto const& disc : discs) {
			double const cx = disc.center.x, cy = disc.center.y, r = disc.radius;

			// Vertical box edges:  x = bx0  and  x = bx1
			for (int ei = 0; ei < 2; ++ei) {
				double const ex = (ei == 0) ? bx0 : bx1;
				double const h2 = r*r - (ex - cx)*(ex - cx);
				if (h2 < 0.0) continue;
				double const h = std::sqrt(h2);
				Point p1{ex, cy + h};
				Point p2{ex, cy - h};
				if (box->contains(p1)) raw_points.push_back(p1);
				if (h > 1e-9 && box->contains(p2)) raw_points.push_back(p2);
			}

			// Horizontal box edges:  y = by0  and  y = by1
			for (int ei = 0; ei < 2; ++ei) {
				double const ey = (ei == 0) ? by0 : by1;
				double const h2 = r*r - (ey - cy)*(ey - cy);
				if (h2 < 0.0) continue;
				double const h = std::sqrt(h2);
				Point p1{cx + h, ey};
				Point p2{cx - h, ey};
				if (box->contains(p1)) raw_points.push_back(p1);
				if (h > 1e-9 && box->contains(p2)) raw_points.push_back(p2);
			}
		}
	}
	auto t_gen1 = clk::now();

	long long raw = static_cast<long long>(raw_points.size());

	// ── 2. Compute containment mask for each candidate point ──────────────────
	auto t_cont0 = clk::now();
	std::vector<Candidate> candidates;
	candidates.reserve(raw_points.size());

	for (auto const& point : raw_points) {
		auto mask  = make_mask(discs, point);
		auto count = mask_count(mask);

		Candidate candidate;
		candidate.point = point;
		candidate.mask  = mask;
		candidate.count = count;
		candidates.push_back(candidate);
	}
	auto t_cont1 = clk::now();

	// ── 3. Deduplicate by mask, then inclusion-maximal filter ─────────────────
	// std::vector<unsigned long long> has lexicographic operator<, so sorting by
	// mask groups identical masks together for std::unique to remove duplicates.
	auto t_flt0 = clk::now();

	std::sort(candidates.begin(), candidates.end(),
		[](Candidate const& a, Candidate const& b) { return a.mask < b.mask; });
	auto uniq_end = std::unique(candidates.begin(), candidates.end(),
		[](Candidate const& a, Candidate const& b) { return equal_mask(a.mask, b.mask); });
	candidates.erase(uniq_end, candidates.end());

	long long unique_masks_count = static_cast<long long>(candidates.size());

	if (maximal_only && ENABLE_MAXIMAL_FILTER) {
		filter_maximal_candidates(candidates);
	}

	long long maximal_count = static_cast<long long>(candidates.size());
	auto t_flt1 = clk::now();

	// ── Accumulate instrumentation ────────────────────────────────────────────
	++g_direct_inst.calls;
	g_direct_inst.disks_total  += static_cast<long long>(n);
	g_direct_inst.pair_tests   += pair_tests_count;
	g_direct_inst.raw_points   += raw;
	g_direct_inst.unique_masks += unique_masks_count;
	g_direct_inst.maximal      += maximal_count;
	g_direct_inst.gen_ns       += std::chrono::duration_cast<ns>(t_gen1  - t_gen0).count();
	g_direct_inst.cont_ns      += std::chrono::duration_cast<ns>(t_cont1 - t_cont0).count();
	g_direct_inst.filt_ns      += std::chrono::duration_cast<ns>(t_flt1  - t_flt0).count();

	data->points.reserve(candidates.size());
	for (auto const& candidate : candidates) {
		data->points.push_back(candidate.point);
	}
	data->current = 0;

	return data;
}

}

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox box)
	: ArrangementTraversal(discs, box, false)
{
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs)
	: ArrangementTraversal(discs, false)
{
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox box, bool maximal_only)
{
	std::vector<Curve_2> to_insert;
	for (auto const& disc: discs) {
		auto point_2 = Point_2(disc.center.x, disc.center.y);
		auto circle_2 = Circle_2(point_2, disc.radius * disc.radius);
		auto rect_2 = Iso_rectangle_2({box.min.x, box.min.y}, {box.max.x, box.max.y});
		if (intersect(circle_2, rect_2))
			to_insert.push_back(Curve_2(circle_2));
	}
	to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.min.x, box.max.y});
	to_insert.emplace_back(Point_2{box.min.x, box.max.y}, Point_2{box.max.x, box.max.y});
	to_insert.emplace_back(Point_2{box.max.x, box.max.y}, Point_2{box.max.x, box.min.y});
	to_insert.emplace_back(Point_2{box.max.x, box.min.y}, Point_2{box.min.x, box.min.y});

	if (USE_CGAL_DEPTH_MAXIMAL && maximal_only)
		data = build_cgal_depth(to_insert, discs);
	else if (USE_DIRECT_INTERSECTION_CANDIDATES)
		data = build_direct(discs, &box, maximal_only);
	else
		data = build(to_insert, discs, maximal_only);
	size = data->points.size();
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs, bool maximal_only)
{
	std::vector<Curve_2> to_insert;
	for (auto const& disc: discs) {
		auto point_2 = Point_2(disc.center.x, disc.center.y);
		auto circle_2 = Circle_2(point_2, disc.radius * disc.radius);
		to_insert.push_back(Curve_2(circle_2));
	}

	if (USE_CGAL_DEPTH_MAXIMAL && maximal_only)
		data = build_cgal_depth(to_insert, discs);
	else if (USE_DIRECT_INTERSECTION_CANDIDATES)
		data = build_direct(discs, nullptr, maximal_only);
	else
		data = build(to_insert, discs, maximal_only);
	size = data->points.size();
}

ArrangementTraversal::~ArrangementTraversal()
{
	delete data;
}

bool ArrangementTraversal::hasNext()
{
	return data->current < data->points.size();
}

Point ArrangementTraversal::getNext()
{
	auto point = data->points[data->current];
	++data->current;
	return point;
}

std::size_t ArrangementTraversal::getSize() const
{
	return size;
}

}