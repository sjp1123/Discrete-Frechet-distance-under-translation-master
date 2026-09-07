#include "curve_parameters.h"

namespace
{

//
// Angle
//

// FIXME:
bool isValid(Angle angle) {
	// return angle >= 0. && angle <= 2*M_PI;
	return true;
}

Angle ccw_dist(Angle a1, Angle a2) {
	auto dist = std::fmod(a2-a1, 2*M_PI);
	return dist < 0. ? dist + 2*M_PI : dist;
}

//
// Cone
//

struct Cone
{
	// the angle goes counter clockwise
	Angle start, end;

	Cone() : start(-1.), end(-1.) {}
	Cone(Angle angle) : Cone(angle, angle) {}
	Cone(Angle start, Angle end) : start(start), end(end) {
		if(!isValid(start) || !isValid(end)) { ERROR("invalid angle"); }
	}

	bool invalid() { return start == -1. && end == -1.; }
	void extend(Angle angle) {
		if(!isValid(angle)) { ERROR("invalid angle: " << angle); }
		if (invalid()) {
			start = end = angle;
		}
		else if (!this->contains(angle)) {
			if (ccw_dist(angle, start) <= ccw_dist(end, angle)) {
				start = angle;
			}
			else {
				end = angle;
			}
		}
	}

	Angle getAngle() { return ccw_dist(start, end); }
	bool contains(Angle angle) {
		if(!isValid(angle)) { ERROR("invalid angle: " << angle); }
		return (angle >= start && angle <= end) ||
			(this->containsZero() && (angle >= start || angle <= end));
	}
	bool containsZero() { return start > end; }
};

//
// helpers
//

Angle getAngle(Point const& p1, Point const& p2) {
	auto direction = p2 - p1;
	auto angle = std::atan2(direction.x, direction.y) + M_PI;
	return std::max(0., std::min(2*M_PI, angle));
}

} // end anonymous

int calcConeChanges(Curve const& curve, Angle cone_angle)
{
	int cone_changes = 0;

	Cone current_cone;
	for (PointID i = 0; i < curve.size()-1; ++i) {
		auto new_angle = getAngle(curve[i], curve[i+1]);
		current_cone.extend(new_angle);
		// std::cout << new_angle << " " << current_cone.start << " " << current_cone.end << " " << current_cone.getAngle() << std::endl;
		if (current_cone.getAngle() > cone_angle) {
			++cone_changes;
			current_cone = Cone(new_angle);
		}
	}

	// std::cout << cone_changes << std::endl;
	return cone_changes;
}
