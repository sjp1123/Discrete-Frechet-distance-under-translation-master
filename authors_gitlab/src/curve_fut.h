#pragma once

#include "defs.h"
#include "geometry_basics.h"
#include "id.h"

// Represents a trajectory. Additionally to the points given in the input file,
// we also store the length of any prefix of the trajectory.
class Curve
{
public:
    Curve() = default;
    Curve(const Points& points);

    std::size_t size() const { return points.size(); }
	bool empty() const { return points.empty(); }
    Point const& operator[](PointID i) const
	{
		if (!is_translated) {
			return points[i];
		}

		translated_points[i] = points[i] + tau;
		return translated_points[i];
	}
	Point interpolate_at(CPoint const& pt) const
	{
		assert(pt.getFraction() >= 0. && pt.getFraction() <= 1.);
		assert((pt.getPoint() < points.size()-1 || (pt.getPoint() == points.size()-1 && pt.getFraction() == 0.)));
		return (pt.getFraction() == 0. ? points[pt.getPoint()] : points[pt.getPoint()]*(1.-pt.getFraction()) + points[pt.getPoint()+1]*pt.getFraction())+tau;
	}
    distance_t curve_length(PointID i, PointID j) const
		{ return prefix_length[j] - prefix_length[i]; }

    Point const& front() const
	{
		if (!is_translated) {
			return points.front();
		}

		translated_points.front() = points.front() + tau;
		return translated_points.front();
	}
    Point const& back() const {
		if (!is_translated) {
			return points.back();
		}

		translated_points.back() = points.back() + tau;
		return translated_points.back();
	}

    void push_back(Point const& point);

	// FIXME: remove?
	// Points::const_iterator begin() { return points.begin(); }
	// Points::const_iterator end() { return points.end(); }
	// Points::const_iterator begin() const { return points.cbegin(); }
	// Points::const_iterator end() const { return points.cend(); }
	
	std::string filename;

	struct ExtremePoints { distance_t min_x, min_y, max_x, max_y; };
	ExtremePoints getExtremePoints() const;
	distance_t getUpperBoundDistance(Curve const& other) const;

	void translate(Point const& translation) const { is_translated = true; this->tau = translation; }
	void resetTranslation() const { is_translated = false; this->tau = {0,0}; }

private:
    Points points;
    std::vector<distance_t> prefix_length;
	ExtremePoints extreme_points = {
		std::numeric_limits<distance_t>::max(), std::numeric_limits<distance_t>::max(),
		std::numeric_limits<distance_t>::lowest(), std::numeric_limits<distance_t>::lowest()
	};

	mutable bool is_translated = false;
	mutable Point tau{0,0};
    mutable Points translated_points;
};
using Curves = std::vector<Curve>;

std::ostream& operator<<(std::ostream& out, const Curve& curve);
