#pragma once

#include <cstddef>
#include <vector>

namespace cgal_disk_arrangements
{

struct Point {
	double x, y;
};

struct Disc {
	Point center;
	double radius;
};
using Discs = std::vector<Disc>;

struct BoundingBox {
	Point min, max;

	void extend(BoundingBox const& other) {
		min.x = std::min(min.x, other.min.x);
		min.y = std::min(min.y, other.min.y);
		max.x = std::max(max.x, other.max.x);
		max.y = std::max(max.y, other.max.y);
	}

	bool contains(Point const& point) const
	{
		// FIXME: this shouldn't be a fixed eps
		static constexpr double eps = 1e-10;
		return point.x >= min.x - eps && point.x <= max.x + eps && point.y >= min.y - eps && point.y <= max.y + eps;
	}
};

class ArrangementTraversal
{
public:
	ArrangementTraversal(Discs const& discs, BoundingBox box);
	ArrangementTraversal(Discs const& discs);
	// `predicate_slack` is the extra radius the CALLER's decider will allow
	// beyond disc.radius (N6Alg::epsilon_slack).  The Cech pipeline emits ONE
	// witness per region and must therefore enumerate the regions of the
	// radius the decider will really use; the arrangement path ignores it.
	ArrangementTraversal(Discs const& discs, bool maximal_only, double predicate_slack);
	~ArrangementTraversal();

	bool hasNext();
	Point getNext();
	std::size_t getSize() const;

private:
	struct CGALData* data;
	std::size_t size;
};

} // end namespace cgal_disk_arrangements
