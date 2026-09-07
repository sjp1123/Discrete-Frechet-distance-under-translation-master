#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
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

struct CGALData;

struct BoundingBox {
	Point min, max;

	void extend(BoundingBox const& other) {
		min.x = std::min(min.x, other.min.x);
		min.y = std::min(min.y, other.min.y);
		max.x = std::max(max.x, other.max.x);
		max.y = std::max(max.y, other.max.y);
	}

	bool contains(Point const& point) const {
		static constexpr double eps = 1e-10;
		return point.x >= min.x - eps && point.x <= max.x + eps &&
		       point.y >= min.y - eps && point.y <= max.y + eps;
	}
};

class ArrangementTraversal {
public:
	ArrangementTraversal(Discs const& discs, BoundingBox box);
	ArrangementTraversal(Discs const& discs);
	ArrangementTraversal(Discs const& discs, BoundingBox box, bool maximal_only);
	ArrangementTraversal(Discs const& discs, bool maximal_only);
	~ArrangementTraversal();

	bool hasNext();
	Point getNext();
	std::size_t getSize() const;

	// C_q–ply instrumentation (NEXT §3.4): ply (containment popcount) and whether
	// the just-returned candidate is inclusion-maximal. Valid after getNext().
	std::uint32_t lastPly() const;
	bool lastIsMaximal() const;

private:
	std::unique_ptr<CGALData> data;
	std::size_t size;
};

}