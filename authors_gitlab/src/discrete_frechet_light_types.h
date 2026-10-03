#pragma once

#include "geometry_basics.h"
#include "id.h"
#include "curves.h"

#include <vector>

//
// DiscreteBox
//

struct DiscreteBox {
	PointID min1;
	PointID max1;
	PointID min2;
	PointID max2;

	DiscreteBox(PointID min1, PointID max1, PointID min2, PointID max2)
		: min1(min1), max1(max1), min2(min2), max2(max2) {}

	bool isCell() const {
		return max1 - min1 == 1 && max2 - min2 == 1;
	}
};
using DiscreteBoxes = std::vector<DiscreteBox>;

//
// DiscreteInputs
//

struct DiscreteInputs {
	PIntervals::iterator begin1;
	PIntervals::iterator end1;
	PIntervals::iterator begin2;
	PIntervals::iterator end2;

	bool haveDownInput() const { return begin1 != end1; }
	bool haveLeftInput() const { return begin2 != end2; }

	bool downContains(PointID point_id) const {
		for (auto it = begin1; it != end1; ++it) {
			if (it->begin <= point_id && it->end >= point_id) {
				return true;
			}
		}
		return false;
	}
	bool leftContains(PointID point_id) const {
		for (auto it = begin2; it != end2; ++it) {
			if (it->begin <= point_id && it->end >= point_id) {
				return true;
			}
		}
		return false;
	}
};

//
// DiscreteOutputs
//

struct DiscreteOutputs {
	PIntervalsID id1;
	PIntervalsID id2;
};

//
// DiscreteQSimpleInterval
//

struct DiscreteQSimpleInterval
{
	DiscreteQSimpleInterval() : valid(false) {}
	DiscreteQSimpleInterval(PointID const& begin, PointID const& end)
		: valid(true), free(begin, end) {}

	void setFreeInterval(PointID begin, PointID end) {
		free = {begin, end};
		//valid = true;
	}

	PInterval const& getFreeInterval() {
		return free;
	}

	void setLastValidPoint(PointID point) {
		last_valid_point = point;
		//valid = false;
	}
	PointID getLastValidPoint() const {
		return last_valid_point;
	}
	bool hasPartialInformation() const {
		return last_valid_point.valid();
	}

	void validate() { valid = true; }
	void invalidate() { valid = false; }
	void clamp(PointID min, PointID max) {
		free.clamp(min, max);
	}

	bool is_empty() const { return free.is_empty(); }
	bool is_valid() const { return valid; }

private:
	bool valid;
	PInterval free;
	PointID last_valid_point;
};

using DiscreteQSimpleIntervals = std::vector<DiscreteQSimpleInterval>;
using DiscreteQSimpleID = ID<DiscreteQSimpleInterval>;

//
// DiscreteQSimpleOutputs
//

struct DiscreteQSimpleOutputs
{
	DiscreteQSimpleID id1;
	DiscreteQSimpleID id2;
};

//
// DiscreteBoxData
//

struct DiscreteBoxData {
	DiscreteBox box;
	DiscreteInputs inputs;
	DiscreteOutputs outputs;
	DiscreteQSimpleOutputs qsimple_outputs;
};
