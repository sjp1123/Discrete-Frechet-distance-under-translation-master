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
	// `predicate_slack` is the extra radius the CALLER's decider will allow
	// beyond disc.radius.  N6Alg builds discs of radius `distance` but then asks
	// the sub-Fréchet decider about `distance + epsilon_slack`, so the pair set
	// the decider can actually see is the one at the larger radius.  The
	// maximal-region pipeline enumerates at radius + slack, which is what makes
	// one witness per region cover exactly what the decider will test.  The
	// arrangement path ignores it (it probes many boundary vertices per region
	// and papers over the gap by accident).
	ArrangementTraversal(Discs const& discs, bool maximal_only, double predicate_slack);
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