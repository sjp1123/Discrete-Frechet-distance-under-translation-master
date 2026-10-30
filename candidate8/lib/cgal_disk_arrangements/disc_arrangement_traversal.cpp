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
#include <cstdio>
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
		std::string const v(e);
		if (v == "legacy") return RegionMode::Legacy;
		// FIX (candidate7): say so instead of silently mapping an unknown value to cech.
		if (v != "cech") std::fprintf(stderr, "MAXREGION=%s not understood; using cech\n", e);
		return RegionMode::Cech;
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
		// FIX (candidate7): exact is the default (the paper configuration); the lenient double
		// predicates with a zero witness tolerance gave wrong NO answers.  MAXREGION_EXACT=0
		// restores them.
		p.exact = true;
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
// FIX (candidate8): MAXREGION_SLACK = budget (default) | caller | <number>.
//   budget : enumerate at radius + max(0, slack − decider_err − slack/9) wherever the caller passes a
//            bound on its decider's rounding (the box family and union's first stage); 0 elsewhere,
//            i.e. candidate7.  See enumeration_slack().
//   caller : enumerate at radius + slack (the candidate5/6 default).
//   <v>    : enumerate at radius + min(v, slack).  0 is candidate7.  candidate7 did not clamp, and a
//            value above the caller's slack let enumerate_box accept a witness the decider rejects.
// Encoded as  ≥ 0: a number,  −1: caller,  −2: budget.
double slack_setting() {
	static double v = []{
		const char* e = std::getenv("MAXREGION_SLACK");
		if (!e) return -2.0;
		std::string const s(e);
		if (s == "budget") return -2.0;
		if (s == "caller") return -1.0;
		char* end = nullptr; double x = std::strtod(e, &end);
		if (end && *end == '\0' && x >= 0.0) return x;
		std::fprintf(stderr, "MAXREGION_SLACK=%s not understood; using budget\n", e);
		return -2.0;
	}();
	return v;
}

// Enumeration slack (extra radius) for a caller whose decider accepts radius + `from_caller`.
// `decider_err` ≥ 0 is the caller's bound on its decider's rounding; < 0 means not supplied.
//
// FIX (candidate8): why the budget default.  The decider accepts a translation when every pair it
// needs lies within radius + slack (9e-9 at the default ε) in its own floating-point evaluation.
// Enumerating at the bare radius (candidate7) makes the base case answer YES only from the box's
// true minimum upward, so the LMF bisection settles 0.2–8e-9 above where the authors' arrangement,
// whose vertices are tested with the same slack, settles.  On pairs whose start/end lower bound sits
// within ε/2 of the optimum this decides the stopping test max − min_dist ≤ ε/2 of every box, and
// candidate7 kept splitting boxes the authors drop (r5: 5× the boxes on those pairs).
// The budget spends the slack the decider itself grants: the decider's rounding is kept out, and a
// margin slack/9 (= N6Alg's epsilon_sub at the default ε) is left for rounding the exact witness, so
// enumerate_box rarely needs its fallback.  Soundness does not depend on this choice: for every
// value in [0, slack] enumerate_box accepts a witness only within radius + slack − decider_err and
// sends any other region to the box-arrangement fallback on the original radii.
double enumeration_slack(double from_caller, double decider_err)
{
	double const caller = from_caller > 0.0 ? from_caller : 0.0;
	double const set = slack_setting();
	if (set >= 0.0) return std::min(set, caller);
	if (set == -1.0) return caller;
	if (decider_err < 0.0) return 0.0;
	return std::max(0.0, caller - decider_err - caller / 9.0);
}

double region_slack(double from_caller) { return enumeration_slack(from_caller, -1.0); }

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

// FIX (candidate8): `decider_err` < 0 (every caller but union's first stage) keeps candidate7's
// enumeration radius.  Union's first stage only looks for an early YES, which the decider itself
// confirms; the box family after it carries completeness, so its witnesses need no certificate.
CGALData* build_candidates_cech(Discs const& discs, double caller_slack, double decider_err = -1.0)
{
	CGALData* data = new CGALData();
	data->use_points = true;
	if (discs.empty()) return data;

	double const slack = enumeration_slack(caller_slack, decider_err);
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

// Arrangement of the discs' circles clipped to the box (original radii), vertices appended to `out`.
// A face containing a box point is then clipped to the box, so its vertices are box points and
// dominate that point's disc set.
// FIX (candidate7): a box of zero width or height (the midpoint split rounds to an end once a side
// is 1 ulp) gave CGAL zero-length and overlapping segments (SIGSEGV).  Insert only the
// non-degenerate sides; a segment box is one segment whose arrangement edges end at box points.
// FIX (candidate7 follow-up): a point box is its own and only witness; the circles' arrangement
// (which the first version still built, about 600 unused vertices on a 24-disc component) is skipped.
void append_box_arrangement_vertices(Discs const& discs, BoundingBox const& box, std::vector<Point>& out)
{
	bool const wide = box.min.x < box.max.x, tall = box.min.y < box.max.y;
	if (!wide && !tall) { out.push_back(Point{box.min.x, box.min.y}); return; }
	std::vector<Curve_2> to_insert;
	for (auto const& d: discs)
		to_insert.push_back(Curve_2(Circle_2(Point_2(d.center.x, d.center.y), d.radius*d.radius)));
	if (wide && tall) {
		to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.min.x, box.max.y});
		to_insert.emplace_back(Point_2{box.min.x, box.max.y}, Point_2{box.max.x, box.max.y});
		to_insert.emplace_back(Point_2{box.max.x, box.max.y}, Point_2{box.max.x, box.min.y});
		to_insert.emplace_back(Point_2{box.max.x, box.min.y}, Point_2{box.min.x, box.min.y});
	}
	else if (wide) {
		to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.max.x, box.min.y});
	}
	else {
		to_insert.emplace_back(Point_2{box.min.x, box.min.y}, Point_2{box.min.x, box.max.y});
	}
	Arrangement_2 arr;
	CGAL::insert(arr, to_insert.begin(), to_insert.end());
	for (auto v = arr.vertices_begin(); v != arr.vertices_end(); ++v)
		out.push_back(Point{CGAL::to_double(v->point().x()), CGAL::to_double(v->point().y())});
}

CGALData* build_candidates_box(Discs const& discs, BoundingBox const& box, double caller_slack, double decider_err)
{
	CGALData* data = new CGALData();
	data->use_points = true;
	if (discs.empty()) return data;
	// FIX (candidate8): budgeted enumeration slack (enumeration_slack); ≤ caller_slack always.
	double const slack = enumeration_slack(caller_slack, decider_err);
	double const derr = decider_err > 0.0 ? decider_err : 0.0;
	double const r_in = discs[0].radius + (slack > 0.0 ? slack : 0.0);
	// The decider accepts radius + caller_slack; whatever of that the pipeline radius does not
	// already use, minus a bound on the decider's own rounding at a box point (FIX candidate7
	// follow-up; 0 when the caller does not supply one), is the witness tolerance.  A witness that
	// does not clear it stays, and its region's discs also go to the fallback below.
	// With slack ≤ caller_slack, r_in + tol = radius + caller_slack − derr whatever the slack is:
	// every accepted witness lies where the decider, rounding included, still accepts it.
	double const tol = std::max(0.0, caller_slack - (slack > 0.0 ? slack : 0.0)) - derr;
	Discs in = discs;
	for (auto& d: in) d.radius = r_in;
	maxregion::Result res =
		maxregion::enumerate_box(in, box, r_in, tol, region_params(), &maxregion::global_stats());
	data->points.reserve(res.regions.size());
	for (auto const& reg: res.regions) data->points.push_back(reg.witness);
	// Oversized components, and regions whose witness could not be certified.
	if (!res.overflow.empty()) {
		data->overflow = true;
		for (auto const& comp: res.overflow) {
			Discs sub;
			for (std::size_t k: comp) sub.push_back(discs[k]);
			append_box_arrangement_vertices(sub, box, data->points);
		}
	}
	return data;
}

} // end anonymous namespace

ArrangementTraversal::ArrangementTraversal(Discs const& discs, BoundingBox const& box, double predicate_slack,
                                           double decider_err)
{
	data = build_candidates_box(discs, box, predicate_slack, decider_err);
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

ArrangementTraversal::ArrangementTraversal(Discs const& discs, bool maximal_only,
                                           double predicate_slack)
	: ArrangementTraversal(discs, maximal_only, predicate_slack, -1.0)
{
}

ArrangementTraversal::ArrangementTraversal(Discs const& discs, bool /*maximal_only*/,
                                           double predicate_slack, double decider_err)
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

	data = build_candidates_cech(discs, predicate_slack, decider_err);
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
