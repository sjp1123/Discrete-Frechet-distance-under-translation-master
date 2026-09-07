#include "disc_arrangement_traversal.h"

#include <CGAL/Iso_rectangle_2.h>
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

struct CGALData {
	Arrangement_2 arr;
	Arrangement_2::Vertex_iterator current, end;
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

	CGAL::insert(data->arr, to_insert.begin(), to_insert.end());

	data->current = data->arr.vertices_begin();
	data->end = data->arr.vertices_end();

	return data;
}

} // end anonymous namespace

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

ArrangementTraversal::~ArrangementTraversal()
{
	delete data;
}

bool ArrangementTraversal::hasNext()
{
	return data->current != data->end;
}

Point ArrangementTraversal::getNext()
{
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
