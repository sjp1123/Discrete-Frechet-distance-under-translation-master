#include "disc_arrangement_traversal.h"
#include "maximal_regions.h"

#include <CGAL/Iso_rectangle_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <algorithm>
// X2 kernel contrast (experiment_design_X1_X2.md §3.1): compile-time switch
// between Epeck (exact constructions, default = A-arms) and Epick (double
// constructions, = B-arms). Only the kernel changes; the DCEL arrangement code,
// the traits template, and the no-maximal vertex enumeration are identical, so
// A0 vs B0 isolates the kernel-construction effect (contrast K_0).
#if defined(USE_EPICK)
  #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#else
  #include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#endif
#include <CGAL/Arr_circle_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace cgal_disk_arrangements
{

#if defined(USE_EPICK)
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;  // B-arms
#else
using Kernel = CGAL::Epeck;                                          // A-arms
#endif
using Traits_2 = CGAL::Arr_circle_segment_traits_2<Kernel>;
using Point_2 = Kernel::Point_2;
using Iso_rectangle_2 = Kernel::Iso_rectangle_2;
using Curve_2 = Traits_2::Curve_2;
using Circle_2 = Traits_2::Kernel::Circle_2;
using Arrangement_2 = CGAL::Arrangement_2<Traits_2>;

// Two emitters share one traversal object.
//
//   legacy — the arrangement is kept and its vertices are iterated lazily,
//            exactly as in `original` (no materialisation, no extra copy).
//   cech   — the maximal-region pipeline has already produced one witness per
//            region, so the points are materialised up front and `arr` is never
//            allocated.  Components too large for the pipeline's DP are covered
//            by running the *Epeck* arrangement on that component and appending
//            its vertices here (the fallback is exact, and identical in kind to
//            what `original` would have emitted).
struct CGALData {
	std::unique_ptr<Arrangement_2> arr;
	Arrangement_2::Vertex_iterator current, end;

	std::vector<Point> points;
	std::size_t idx = 0;
	bool use_points = false;
	bool overflow = false;
};

namespace
{

bool intersect(Circle_2 const& circle, Iso_rectangle_2 const& rect)
{
	bool const contains0 = circle.has_on_bounded_side(rect[0]);
	bool const contains1 = circle.has_on_bounded_side(rect[1]);
	bool const contains2 = circle.has_on_bounded_side(rect[2]);
	bool const contains3 = circle.has_on_bounded_side(rect[3]);

	// if the whole box is contained, this disc doesn't contribute to the arrangement
	if (contains0 && contains1 && contains2 && contains3) {
		return false;
	}

	// if the circle doesn't intersect the box at all
	if (!contains0 && !contains1 && !contains2 && !contains3) {
		// check if bounding boxes of circle and rect intersect
		if (!CGAL::do_overlap(circle.bbox(), rect.bbox())) {
			return false;
		}

		// check if all nodes of rect are in one quadrant
		if ((rect[0].x() > circle.center().x() && rect[0].y() > circle.center().y()) ||
			(rect[1].x() < circle.center().x() && rect[1].y() > circle.center().y()) ||
			(rect[3].x() > circle.center().x() && rect[3].y() < circle.center().y()) ||
			(rect[2].x() < circle.center().x() && rect[2].y() < circle.center().y())) {

			return false;
		}
	}

	return true;
}

CGALData* build(std::vector<Curve_2> const& to_insert)
{
	CGALData* data = new CGALData();
	data->arr.reset(new Arrangement_2());

	CGAL::insert(*data->arr, to_insert.begin(), to_insert.end());

	data->current = data->arr->vertices_begin();
	data->end = data->arr->vertices_end();

	return data;
}

// ── Region emitter selection ─────────────────────────────────────────────────
//   cech   (default) the Čech maximal-region pipeline, Epeck arrangement kept
//                    as the fallback for oversized components
//   legacy           the arrangement-vertex enumeration, i.e. `original`
//                    verbatim — the control arm
enum class RegionMode { Cech, Legacy };

RegionMode region_mode() {
	static RegionMode cached = []{
		const char* e = std::getenv("MAXREGION");
		if (!e) return RegionMode::Cech;
		return std::string(e) == "legacy" ? RegionMode::Legacy : RegionMode::Cech;
	}();
	return cached;
}

double env_double(const char* name, double dflt) {
	const char* e = std::getenv(name);
	if (!e) return dflt;
	char* end = nullptr; double v = std::strtod(e, &end);
	return (end && *end == '\0' && v > 0.0) ? v : dflt;
}

std::size_t env_size(const char* name, std::size_t dflt) {
	const char* e = std::getenv(name);
	if (!e) return dflt;
	char* end = nullptr; long v = std::strtol(e, &end, 10);
	return (end && *end == '\0' && v > 0) ? (std::size_t)v : dflt;
}

maxregion::Params const& region_params() {
	static maxregion::Params cached = []{
		maxregion::Params p;
		p.band     = env_double("MAXREGION_BAND", 1e-12);
		p.dp_limit = env_size("MAXREGION_DP_LIMIT", 20);
		// MAXREGION_EXACT=1: filtered predicates with a rational fallback (band ignored)
		if (const char* e = std::getenv("MAXREGION_EXACT")) p.exact = (*e && *e != '0');
		return p;
	}();
	return cached;
}

// N6Alg builds discs of radius `distance` but asks the sub-Fréchet decider about
// `distance + epsilon_slack`, so the pair set the decider really sees is the one
// at the larger radius.  The arrangement path papers over the gap by accident
// (many boundary vertices per region); one witness per region does not, so the
// pipeline enumerates at radius + slack.  MAXREGION_SLACK overrides the caller;
// MAXREGION_SLACK=0 is the "pipeline only, no slack alignment" control arm.
double region_slack(double from_caller) {
	static double override_v = []{
		const char* e = std::getenv("MAXREGION_SLACK");
		if (!e) return -1.0;
		char* end = nullptr; double v = std::strtod(e, &end);
		return (end && *end == '\0' && v >= 0.0) ? v : -1.0;
	}();
	if (override_v >= 0.0) return override_v;
	return from_caller > 0.0 ? from_caller : 0.0;
}

// Epeck fallback: `original`'s vertex enumeration, restricted to one component.
// Components are more than 2r apart and share no point, so solving one in
// isolation is exact and mixing the two emitters across components loses
// nothing.
void append_arrangement_vertices(Discs const& discs, std::vector<Point>& out)
{
	std::vector<Curve_2> to_insert;
	to_insert.reserve(discs.size());
	for (auto const& disc: discs) {
		auto point_2 = Point_2(disc.center.x, disc.center.y);
		auto circle_2 = Circle_2(point_2, disc.radius*disc.radius);
		to_insert.push_back(Curve_2(circle_2));
	}

	Arrangement_2 arr;
	CGAL::insert(arr, to_insert.begin(), to_insert.end());
	for (auto v = arr.vertices_begin(); v != arr.vertices_end(); ++v) {
		out.push_back(Point{CGAL::to_double(v->point().x()),
		                    CGAL::to_double(v->point().y())});
	}
}

CGALData* build_candidates_cech(Discs const& discs, double caller_slack)
{
	CGALData* data = new CGALData();
	data->use_points = true;
	if (discs.empty()) return data;

	double const slack = region_slack(caller_slack);
	Discs inflated;
	if (slack > 0.0) {
		inflated = discs;
		for (auto& d: inflated) d.radius += slack;
	}
	Discs const& in = (slack > 0.0) ? inflated : discs;

	maxregion::Result res =
		maxregion::enumerate(in, region_params(), &maxregion::global_stats());

	data->points.reserve(res.regions.size());
	for (auto const& reg: res.regions) data->points.push_back(reg.witness);

	// The fallback runs on the ORIGINAL radii, never the inflated ones.  Slack
	// alignment exists for the pipeline, which emits one interior witness per
	// region; the arrangement emitter needs the opposite treatment.  Its vertices
	// sit exactly ON the circles, and at the inflated radius the sqrt error there
	// (~1.5e-8·r) exceeds the slack (~9e-9), so the decider can miss the very pair
	// a vertex was built for — the C4S pathology of README.candidate4.md §6.
	// Measured: inflating the fallback moves the answer by up to 2.7e-4 (over the
	// 1e-7 contract); on the original radii it is bit-identical to `original`.
	if (!res.overflow.empty()) {
		data->overflow = true;
		// Whole set is one oversized component: no sub-vector copy.
		if (res.regions.empty() && res.overflow.size() == 1 &&
		    res.overflow[0].size() == discs.size()) {
			append_arrangement_vertices(discs, data->points);
		}
		else {
			for (auto const& comp: res.overflow) {
				Discs sub;
				sub.reserve(comp.size());
				for (std::size_t k: comp) sub.push_back(discs[k]);
				append_arrangement_vertices(sub, data->points);
			}
		}
	}

	return data;
}

CGALData* build_candidates_block(Discs const& discs, std::vector<Point> const& block,
                                 double radius, double caller_slack)
{
	CGALData* data = new CGALData();
	data->use_points = true;
	double const slack = region_slack(caller_slack);
	double const r_in = radius + (slack > 0.0 ? slack : 0.0);
	Discs in = discs;
	for (auto& d: in) d.radius = r_in;

	maxregion::Result res =
		maxregion::enumerate_block(in, block, r_in, region_params(), &maxregion::global_stats());
	data->points.reserve(res.regions.size());
	for (auto const& reg: res.regions) data->points.push_back(reg.witness);

	// Oversized component: arrangement of (component ∪ block discs) on the
	// ORIGINAL radii.  Every face containing a box point lies inside all block
	// discs, so its vertices dominate that point's disc set.
	if (!res.overflow.empty()) {
		data->overflow = true;
		for (auto const& comp: res.overflow) {
			Discs sub;
			for (std::size_t k: comp) sub.push_back(discs[k]);
			for (auto const& b: block) sub.push_back(Disc{b, radius});
			append_arrangement_vertices(sub, data->points);
		}
	}
	return data;
}

CGALData* build_candidates_box(Discs const& discs, BoundingBox const& box, double caller_slack)
{
	CGALData* data = new CGALData();
	data->use_points = true;
	if (discs.empty()) return data;
	double const slack = region_slack(caller_slack);
	double const r_in = discs[0].radius + (slack > 0.0 ? slack : 0.0);
	// The decider accepts radius + caller_slack; whatever of that the pipeline
	// radius does not already use is the witness tolerance.
	double const tol = std::max(0.0, caller_slack - (slack > 0.0 ? slack : 0.0));
	Discs in = discs;
	for (auto& d: in) d.radius = r_in;
	maxregion::Result res =
		maxregion::enumerate_box(in, box, r_in, tol, region_params(), &maxregion::global_stats());
	data->points.reserve(res.regions.size());
	for (auto const& reg: res.regions) data->points.push_back(reg.witness);
	// Oversized component: arrangement of its circles plus the box edges (original
	// radii).  A face containing a box point is then clipped to the box, so its
	// vertices are box points and dominate that point's disc set.
	if (!res.overflow.empty()) {
		data->overflow = true;
		for (auto const& comp: res.overflow) {
			std::vector<Curve_2> to_insert;
			for (std::size_t k: comp) {
				auto const& d = discs[k];
				to_insert.push_back(Curve_2(Circle_2(Point_2(d.center.x, d.center.y), d.radius*d.radius)));
			}
			to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.min.x, box.max.y});
			to_insert.emplace_back(Point_2{box.min.x, box.max.y}, Point_2{box.max.x, box.max.y});
			to_insert.emplace_back(Point_2{box.max.x, box.max.y}, Point_2{box.max.x, box.min.y});
			to_insert.emplace_back(Point_2{box.max.x, box.min.y}, Point_2{box.min.x, box.min.y});
			Arrangement_2 arr;
			CGAL::insert(arr, to_insert.begin(), to_insert.end());
			for (auto v = arr.vertices_begin(); v != arr.vertices_end(); ++v)
				data->points.push_back(Point{CGAL::to_double(v->point().x()), CGAL::to_double(v->point().y())});
		}
	}
	return data;
}

} // end anonymous namespace

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox const& box, double predicate_slack)
{
	data = build_candidates_box(discs, box, predicate_slack);
	size = data->points.size();
}

std::vector<Point> hull_vertices(std::vector<Point> pts)
{
	std::sort(pts.begin(), pts.end(), [](Point const& a, Point const& b) {
		return a.x < b.x || (a.x == b.x && a.y < b.y);
	});
	pts.erase(std::unique(pts.begin(), pts.end(), [](Point const& a, Point const& b) {
		return a.x == b.x && a.y == b.y;
	}), pts.end());
	if (pts.size() <= 2) return pts;
	using K = CGAL::Exact_predicates_inexact_constructions_kernel;
	auto turn = [](Point const& o, Point const& a, Point const& b) {
		return CGAL::orientation(K::Point_2(o.x, o.y), K::Point_2(a.x, a.y), K::Point_2(b.x, b.y));
	};
	std::vector<Point> H(2 * pts.size());
	std::size_t k = 0;
	for (std::size_t i = 0; i < pts.size(); ++i) {
		while (k >= 2 && turn(H[k-2], H[k-1], pts[i]) != CGAL::LEFT_TURN) --k;
		H[k++] = pts[i];
	}
	for (std::size_t i = pts.size() - 1, t = k + 1; i > 0; --i) {
		while (k >= t && turn(H[k-2], H[k-1], pts[i-1]) != CGAL::LEFT_TURN) --k;
		H[k++] = pts[i-1];
	}
	H.resize(k - 1);
	return H;
}


ArrangementTraversal::ArrangementTraversal(Discs const& discs, std::vector<Point> const& block,
                                           double radius, double predicate_slack)
{
	data = build_candidates_block(discs, block, radius, predicate_slack);
	size = data->points.size();
}

bool ArrangementTraversal::hadOverflow() const { return data->overflow; }

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox box)
{
	// Check for which discs to add to the arrangement. Note that we don't add
	// the bounding box boundaries because either the disc intersecting the bounding box
	// 1) forms a face without vertices in the complete arrangement; then we can just check any point
	// 2) forms a face with vertices; then these vertices show up in this or another box
	std::vector<Curve_2> to_insert;
	for (auto const& disc: discs) {
		auto point_2 = Point_2(disc.center.x, disc.center.y);
		auto circle_2 = Circle_2(point_2, disc.radius*disc.radius);
		auto rect_2 = Iso_rectangle_2({box.min.x, box.min.y}, {box.max.x, box.max.y});

		if (intersect(circle_2, rect_2)) {
			to_insert.push_back(Curve_2(circle_2));
		}
	}
	to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.min.x, box.max.y});
	to_insert.emplace_back(Point_2{box.min.x, box.max.y}, Point_2{box.max.x, box.max.y});
	to_insert.emplace_back(Point_2{box.max.x, box.max.y}, Point_2{box.max.x, box.min.y});
	to_insert.emplace_back(Point_2{box.max.x, box.min.y}, Point_2{box.min.x, box.min.y});

	data = build(to_insert);
	size = std::distance(data->current, data->end);
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs)
{
	std::vector<Curve_2> to_insert;
	for (auto const& disc: discs) {
		auto point_2 = Point_2(disc.center.x, disc.center.y);
		auto circle_2 = Circle_2(point_2, disc.radius*disc.radius);
		to_insert.push_back(Curve_2(circle_2));
	}

	data = build(to_insert);
	size = std::distance(data->current, data->end);
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs, bool /*maximal_only*/,
                                           double predicate_slack)
{
	if (region_mode() == RegionMode::Legacy) {
		std::vector<Curve_2> to_insert;
		for (auto const& disc: discs) {
			auto point_2 = Point_2(disc.center.x, disc.center.y);
			auto circle_2 = Circle_2(point_2, disc.radius*disc.radius);
			to_insert.push_back(Curve_2(circle_2));
		}

		data = build(to_insert);
		size = std::distance(data->current, data->end);
		return;
	}

	data = build_candidates_cech(discs, predicate_slack);
	size = data->points.size();
}

ArrangementTraversal::~ArrangementTraversal()
{
	delete data;
}

bool ArrangementTraversal::hasNext()
{
	if (data->use_points) {
		return data->idx < data->points.size();
	}
	return data->current != data->end;
}

Point ArrangementTraversal::getNext()
{
	if (data->use_points) {
		return data->points[data->idx++];
	}
	auto x = CGAL::to_double(data->current->point().x());
	auto y = CGAL::to_double(data->current->point().y());
	++(data->current);
	return Point{x, y};
}

std::size_t ArrangementTraversal::getSize() const
{
	return size;
}

} // end namespace cgal_disk_arrangements
