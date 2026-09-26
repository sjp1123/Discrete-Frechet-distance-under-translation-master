#include "discrete_frechet_light.h"

#include "defs.h"
#include "discrete_frechet_light_types.h"
#include "filter.h"
#include "times.h"

#include <algorithm>

inline PInterval DiscreteFrechetLight::getInterval(Point const& point, Curve const& curve, PointID i) const
{
	PInterval result;

	bool first_close = (point.dist_sqr(curve[i]) <= dist_sqr);
	bool second_close = (point.dist_sqr(curve[i+1]) <= dist_sqr);
	if (first_close && second_close) { result.begin = i; result.end = i+1; }
	else if (first_close) { result.begin = i; result.end = i; }
	else if (second_close) { result.begin = i+1; result.end = i+1; }

	return result;
}

inline void DiscreteFrechetLight::merge(PIntervals& intervals, PInterval const& new_interval) const
{
	if (new_interval.is_empty()) { return; }

	if (!intervals.empty() && new_interval.begin == intervals.back().end) {
		intervals.back().end = new_interval.end;
	}
	else {
		intervals.emplace_back(new_interval);
	}
}

inline DiscreteQSimpleInterval DiscreteFrechetLight::getFreshQSimpleInterval(Point const& fixed_point, PointID min, PointID max, const Curve& curve) const
{
	DiscreteQSimpleInterval qsimple;
	updateQSimpleInterval(qsimple, fixed_point, min, max, curve);
	return qsimple;
}


inline bool DiscreteFrechetLight::updateQSimpleInterval(DiscreteQSimpleInterval& qsimple, Point const& fixed_point, PointID min, PointID max, const Curve& curve) const
{
	if (qsimple.is_valid() or (qsimple.hasPartialInformation() and qsimple.getLastValidPoint() >= max)) {
		qsimple.validate();
		qsimple.clamp(min, max);
		return false; //parent information already valid
	} 


	if (qsimple.hasPartialInformation()) {
	       if (qsimple.getLastValidPoint() < min) {
		       qsimple = DiscreteQSimpleInterval(); //we know nothing about this part
	       } else  {
		       qsimple.clamp(min, max);
		       if (!qsimple.getFreeInterval().is_empty()) {
			       return false; //even restricted to current interval, parent information still gives invalidity
		       }
	       }
	}
	
	//from here on, need to compute new information
	global::times.startCountingFreeTests();

	if (!qsimple.hasPartialInformation()) { //fresh computation, start with heuristics
		// heuristic check:
		auto mid = (min + max)/2;
		auto maxdist = std::max(curve.curve_length(min, mid), curve.curve_length(mid, max));
		auto mid_dist_sqr = fixed_point.dist_sqr(curve[mid]);

		//heuristic tests avoiding sqrts
		auto comp_dist1 = distance - maxdist;
		auto comp_dist2 = distance + maxdist;
		if ((comp_dist1 > 0 && mid_dist_sqr <= std::pow(comp_dist1, 2))) {
			qsimple.setFreeInterval(min, max); //full
			qsimple.validate();
			global::times.incrementFreeTests(max - min); 
			global::times.stopCountingFreeTests();
			return true;
		} else if (mid_dist_sqr > std::pow(comp_dist2, 2)) {
			qsimple.setFreeInterval(max, min); //empty
			qsimple.validate();
			global::times.incrementFreeTests(max - min); 
			global::times.stopCountingFreeTests();
			return true;
		}
	}

	continueQSimpleSearch(qsimple, fixed_point, min, max, curve);

	return true;
}

inline void DiscreteFrechetLight::continueQSimpleSearch(DiscreteQSimpleInterval& qsimple, Point const& fixed_point, PointID min, PointID max, const Curve& curve) const
{
	assert(!qsimple.hasPartialInformation() or (qsimple.getLastValidPoint() >= min and qsimple.getLastValidPoint() <= max));

	PointID start = (qsimple.hasPartialInformation() ? qsimple.getLastValidPoint() : min);

	//Logical assert: there should be no free point in [min, start):
	// if start > min, then the free interval must be empty

	bool current_free = (fixed_point.dist_sqr(curve[start]) <= dist_sqr);
	// Work directly on the simple_interval of the boundary
	assert(!current_free || start == min);
	PointID first_free = (current_free ? min : PointID(max+1)); //qsimple.free.begin;

	assert(first_free > max || fixed_point.dist_sqr(curve[first_free]) <= dist_sqr);
	
	std::size_t stepsize = static_cast<std::size_t>(max - start)/2;
	if (stepsize < 1 || qsimple.hasPartialInformation()) { stepsize = 1; }

	PointID cur = start;
	while (cur < max) {
		stepsize = std::min<std::size_t>(stepsize, max - cur);
		auto mid = cur + (stepsize+1)/2; 
		auto maxdist = std::max(curve.curve_length(cur, mid), curve.curve_length(mid, cur + stepsize));
		auto mid_dist_sqr = fixed_point.dist_sqr(curve[mid]);

		// heuristic steps:
		auto comp_dist1 = distance - maxdist;
		if (current_free && comp_dist1 > 0 && mid_dist_sqr <= std::pow(comp_dist1, 2)) {
			cur += stepsize;
			global::times.incrementFreeTests(stepsize);
			stepsize *= 2;
			continue;
		}
		auto comp_dist2 = distance + maxdist;
		if (!current_free && mid_dist_sqr > std::pow(comp_dist2, 2)) {
			cur += stepsize;
			global::times.incrementFreeTests(stepsize);
			stepsize *= 2;
			continue;
		}
		// if heuristic steps don't work, then reduce stepsize:
		if (stepsize > 1) {
			stepsize /= 2;
			global::times.incrementFreeTests(0);
			continue;
		}
		
		// from here on stepsize == 1:
		// mid == end holds in this case
		auto const end_dist_sqr = mid_dist_sqr;
		// if last and next point are both free:
		if (current_free) {
			if (end_dist_sqr <= dist_sqr) { // free - free
				stepsize *= 2;
			}
			else { // free - non-free
				qsimple.setFreeInterval(first_free, cur);
				qsimple.validate();
				current_free = false;
			}
		}
		else {
			if (end_dist_sqr <= dist_sqr) { // non-free - free
				if (qsimple.is_valid()) { // we encountered a second free interval -> not qsimple
					qsimple.invalidate();
					// still store information in qsimple interval for future use
					qsimple.setLastValidPoint(cur);
					global::times.incrementFreeTests(0);
					global::times.stopCountingFreeTests();
					return;
				}
				else {
					first_free = cur+1;
					current_free = true;
				}
			}
			else { // non-free - non-free
				stepsize *= 2;
			}
		}

		++cur;
		global::times.incrementFreeTests(1);
	}
	
	global::times.stopCountingFreeTests();
	
	// If it is still invalid but we didn't return yet, then the free interval ends at max
	// Note: if no free points were found, first_free defaults to an invalid value (>> max), making the interval empty   
	if (!qsimple.is_valid()) {
		qsimple.setFreeInterval(first_free, max);
		qsimple.validate();
	}
	
	return;
}

PIntervals::iterator getIntervalContainingNumber(const PIntervals::iterator& begin, const PIntervals::iterator& end, PointID x) {
	auto it = std::upper_bound(begin, end, PInterval{x, std::numeric_limits<PointID::IDType>::max()});

	if (it != begin) {
		--it;
		if (it->begin <= x && it->end >= x) {
			return it;
		}
	}
	return end;
}

void DiscreteFrechetLight::getReachableIntervals(DiscreteBoxData& data)
{
	++num_boxes;

	auto const& box = data.box;
	auto const& inputs = data.inputs;

	global::times.newSplit();

	assert(box.max1 > box.min1 && box.max2 > box.min2);
	assert(data.outputs.id1.valid() || data.outputs.id2.valid());

	firstinterval1 = (inputs.begin1 != inputs.end1) ? &*inputs.begin1 : &empty;
	firstinterval2 = (inputs.begin2 != inputs.end2) ? &*inputs.begin2 : &empty;

	if (emptyInputsRule(data)) { return; }
	boxShrinkingRule(data);

	if (box.isCell()) {
		handleCellCase(data);
	}
	else {
		getQSimpleIntervals(data);
		calculateQSimple1(data);
		calculateQSimple2(data);

		if (out1_valid && out2_valid) { return; }
		if (boundaryPruningRule(data)) { return; }

		assert(box.max1 >= box.min1 + 2 || box.max2 >= box.min2 + 2);
		assert(box.max1 >= box.min1 && box.max2 >= box.min2);

		splitAndRecurse(data);
	}
}

inline bool DiscreteFrechetLight::emptyInputsRule(DiscreteBoxData& data)
{
	auto const& box = data.box;

	// empty input intervals -> empty output intervals
	// Note: if we are currently handling a cell then even if we are not pruning,
	// we have to return with empty outputs for the subsequent code to work.
	if (pruning_level > 0 || (box.min1 == box.max1 - 1 && box.min2 == box.max2 - 1)) {
		if (firstinterval2->is_empty() && firstinterval1->is_empty()) {
			return true;
		}
	}

	return false;
}

inline void DiscreteFrechetLight::boxShrinkingRule(DiscreteBoxData& data)
{
	auto& box = data.box;

	first_reachable1 = firstinterval2->is_empty() ? firstinterval1->begin : box.min1;
	first_reachable2 = firstinterval1->is_empty() ? firstinterval2->begin : box.min2;

	// "movement of input boundary" if one of the inputs is empty
	if (pruning_level > 1 && enable_box_shrinking) {
		if (firstinterval2->is_empty() && firstinterval1->begin > box.min1 + 1) {
			box.min1 = firstinterval1->begin - 1;
			first_reachable1 = firstinterval1->begin;
			assert(box.min1 <= box.max1);
			if (box.min1 == box.max1) {
				box.min1 = box.max1 - 1;
			}
		}
		else if (firstinterval1->is_empty() && firstinterval2->begin > box.min2 + 1) {
			box.min2 = firstinterval2->begin - 1;
			first_reachable2 = firstinterval2->begin;
			assert(box.min2 <= box.max2);
			if (box.min2 == box.max2) {
				box.min2 = box.max2 - 1;
			}
		}
	}
}

inline void DiscreteFrechetLight::handleCellCase(DiscreteBoxData& data)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& box = data.box;

	if (data.outputs.id1.valid()) {
		PInterval output1 = getInterval(curve2[box.max2], curve1, box.min1);
		if (firstinterval2->is_empty()) {
			output1.begin = std::max(output1.begin, firstinterval1->begin);
		}
		merge(reachable_intervals_vec[data.outputs.id1], output1);
	}

	if (data.outputs.id2.valid()) {
		PInterval output2 = getInterval(curve1[box.max1], curve2, box.min2);
		if (firstinterval1->is_empty()) {
			output2.begin = std::max(output2.begin, firstinterval2->begin);
		}
		merge(reachable_intervals_vec[data.outputs.id2], output2);
	}
}

inline void DiscreteFrechetLight::getQSimpleIntervals(DiscreteBoxData& data)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& box = data.box;

	qsimple1.invalidate();
	qsimple2.invalidate();

	// Get qsimple intervals. Different cases depending on what has been calculated yet.
	if (data.outputs.id1.valid()) {
		if (data.qsimple_outputs.id1.valid()) {
			qsimple1 = qsimple_intervals[data.qsimple_outputs.id1];
		} else {
			qsimple1 = DiscreteQSimpleInterval();
		}
		bool changed = updateQSimpleInterval(qsimple1, curve2[box.max2], box.min1, box.max1, curve1);
		if (changed) {
			qsimple_intervals.push_back(qsimple1);
			data.qsimple_outputs.id1 = qsimple_intervals.size() - 1;
		}
	}
	if (data.outputs.id2.valid()) {
		if (data.qsimple_outputs.id2.valid()) {
			qsimple2 = qsimple_intervals[data.qsimple_outputs.id2];
		} else {
			qsimple2 = DiscreteQSimpleInterval();
		}
		bool changed = updateQSimpleInterval(qsimple2, curve1[box.max1], box.min2, box.max2, curve2);
		if (changed) {
			qsimple_intervals.push_back(qsimple2);
			data.qsimple_outputs.id2 = qsimple_intervals.size()-1;
		}
	}
}

inline void DiscreteFrechetLight::calculateQSimple1(DiscreteBoxData& data)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& box = data.box;

	out1_valid = false;
	out1.make_empty();

	// pruning rules depending on qsimple1
	if (qsimple1.is_valid()) {
		// output boundary is empty
		if (qsimple1.is_empty()) {
			// out1 is already empty due to initialization, so leave it.
			if (pruning_level > 2 && enable_empty_outputs) {
				out1_valid = true;
			}
		}
		else {
			PointID x = std::max(qsimple1.getFreeInterval().begin, first_reachable1);
			// check if beginning is reachable
			if (x == box.min1 && pruning_level > 3 && enable_propagation1) {
				auto it = getIntervalContainingNumber(data.inputs.begin2, data.inputs.end2, box.max2);
				if (it != data.inputs.end2) { //(box.min1, box.max2) is reachable from interval *it 
					out1 = qsimple1.getFreeInterval();
					out1_valid = true;
				}
			}
			// check if nothing can be reachable
			if (x != box.min1 && x > qsimple1.getFreeInterval().end && pruning_level > 4 && enable_propagation2) {
				// out1 is already empty due to initialization, so leave it.
				out1_valid = true;
			}
			// check if we can propagate reachability through the box to the beginning
			// of the free interval
			else if (x > box.min1 && pruning_level > 4 && enable_propagation2) {
				auto it = getIntervalContainingNumber(data.inputs.begin1, data.inputs.end1, x);
				if (it != data.inputs.end1) { //(x, box.min2) is reachable from interval *it 
					auto interval = getFreshQSimpleInterval(curve1[x], box.min2, box.max2, curve2);
					if (interval.is_valid()) {
						out1 = qsimple1.getFreeInterval();
						out1.begin = x;
						out1_valid = true;
					}
				}
			}
		}
	}
	if (out1_valid) {
		merge(reachable_intervals_vec[data.outputs.id1], out1);
		data.outputs.id1.invalidate();
	}
	out1_valid = !data.outputs.id1.valid();
}

inline void DiscreteFrechetLight::calculateQSimple2(DiscreteBoxData& data)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& box = data.box;

	out2_valid = false;
	out2.make_empty();

	// pruning rules depending on qsimple2
	if (qsimple2.is_valid()) {
		// output boundary is empty
		if (qsimple2.is_empty()) {
			// out2 is already empty due to initialization, so leave it.
			if (pruning_level > 2 && enable_empty_outputs) {
				out2_valid = true;
			}
		}
		else {
			PointID x = std::max(qsimple2.getFreeInterval().begin, first_reachable2);
			// check if beginning is reachable
			if (x == box.min2 && pruning_level > 3 && enable_propagation1) {
				auto it = getIntervalContainingNumber(data.inputs.begin1, data.inputs.end1, box.max1);
				if (it != data.inputs.end1) {
					out2 = qsimple2.getFreeInterval();
					out2_valid = true;
				}
			}
			// check if nothing can be reachable
			if (x != box.min2 && x > qsimple2.getFreeInterval().end && pruning_level > 4 && enable_propagation2) {
				// out2 is already empty due to initialization, so leave it.
				out2_valid = true;
			}
			// check if we can propagate reachability through the box to the beginning
			// of the free interval
			else if (x > box.min2 && pruning_level > 4 && enable_propagation2) {
				auto it = getIntervalContainingNumber(data.inputs.begin2, data.inputs.end2, x);
				if (it != data.inputs.end2) {
					auto interval = getFreshQSimpleInterval(curve2[x], box.min1, box.max1, curve1);
					if (interval.is_valid()) {
						out2 = qsimple2.getFreeInterval();
						out2.begin = x;
						out2_valid = true;
					}
				}
			}
		}
	}
	if (out2_valid) {
		merge(reachable_intervals_vec[data.outputs.id2], out2);
		data.outputs.id2.invalidate();
	}
	out2_valid = !data.outputs.id2.valid();
}

inline bool DiscreteFrechetLight::boundaryPruningRule(DiscreteBoxData& data)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& box = data.box;

	// special cases for boxes which are at the boundary of the freespace diagram
	if (pruning_level > 5 && enable_boundary_rule) {
		if (box.max1 == curve1.size()-1 && out1_valid) {
			return true;
		}
		if (box.max2 == curve2.size()-1 && out2_valid) {
			return true;
		}
	}

	return false;
}

inline void DiscreteFrechetLight::splitAndRecurse(DiscreteBoxData& data)
{
	auto const& box = data.box;

	if (box.max2 - box.min2 > box.max1 - box.min1) { // horizontal split
		reachable_intervals_vec.emplace_back();
		PIntervalsID inputs1_middleID = reachable_intervals_vec.size()-1;

		PointID split_position = (box.max2 + box.min2) / 2;
		assert(split_position > box.min2 && split_position < box.max2);

		auto bound = PInterval{split_position, std::numeric_limits<PointID::IDType>::max()};
		auto it = std::upper_bound(data.inputs.begin2, data.inputs.end2, bound);

		DiscreteBoxData data_bottom{
			{box.min1, box.max1, box.min2, split_position},
			{data.inputs.begin1, data.inputs.end1, data.inputs.begin2, it},
			{inputs1_middleID, data.outputs.id2},
			{DiscreteQSimpleID(), data.qsimple_outputs.id2}
		};
		getReachableIntervals(data_bottom);

		if (it != data.inputs.begin2 && (it-1)->end >= split_position) { --it; }
		PIntervals& inputs1_middle = reachable_intervals_vec[inputs1_middleID];

		DiscreteBoxData data_top{
			{box.min1, box.max1, split_position, box.max2},
			{inputs1_middle.begin(), inputs1_middle.end(), it, data.inputs.end2},
			{data.outputs.id1, data.outputs.id2},
			{data.qsimple_outputs.id1, data.qsimple_outputs.id2}
		};
		getReachableIntervals(data_top);
	} else { // vertical split
		reachable_intervals_vec.emplace_back();
		PIntervalsID inputs2_middleID = reachable_intervals_vec.size()-1;

		PointID split_position = (box.max1 + box.min1) / 2;
		assert(split_position > box.min1 && split_position < box.max1);

		auto bound = PInterval{split_position, std::numeric_limits<PointID::IDType>::max()};
		auto it = std::upper_bound(data.inputs.begin1, data.inputs.end1, bound);

		DiscreteBoxData data_left{
			{box.min1, split_position, box.min2, box.max2},
			{data.inputs.begin1, it, data.inputs.begin2, data.inputs.end2},
			{data.outputs.id1, inputs2_middleID},
			{data.qsimple_outputs.id1, DiscreteQSimpleID()}
		};
		getReachableIntervals(data_left);

		if (it != data.inputs.begin1 && (it-1)->end >= split_position) { --it; }
		PIntervals& inputs2_middle = reachable_intervals_vec[inputs2_middleID];

		DiscreteBoxData data_right{
			{split_position, box.max1, box.min2, box.max2},
			{it, data.inputs.end1, inputs2_middle.begin(), inputs2_middle.end()},
			{data.outputs.id1, data.outputs.id2},
			{data.qsimple_outputs.id1, data.qsimple_outputs.id2}
		};
		getReachableIntervals(data_right);
	}
}

PointID DiscreteFrechetLight::getLastReachablePoint(Point const& point, Curve const& curve) const
{
	PointID max = curve.size()-1;
	std::size_t stepsize = 1;
	for (PointID cur = 0; cur < max; ) {
		// heuristic steps:
		stepsize = std::min<std::size_t>(stepsize, max - cur);

		auto mid = cur + (stepsize+1)/2; 
		auto first_part = curve.curve_length(cur+1, mid);
		auto second_part = curve.curve_length(mid, cur + stepsize);
		auto maxdist = std::max(first_part, second_part);
		auto mid_dist_sqr = point.dist_sqr(curve[mid]);

		auto comp_dist1 = distance - maxdist;
		if (comp_dist1 > 0 && mid_dist_sqr <= std::pow(comp_dist1, 2)) {
			cur += stepsize;
			stepsize *= 2;
		}
		// if heuristic steps don't work, then reduce stepsize:
		else if (stepsize > 1) {
			stepsize /= 2;
		}
		else {
			// stepsize = 1 and heuristic step didn't work
			return cur;
		}
	}
	return max;
}

bool DiscreteFrechetLight::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	this->curve_pair[0] = &curve1;
	this->curve_pair[1] = &curve2;
	this->distance = distance;
	this->dist_sqr = distance * distance;

	// curves empty or start or end are already far
	if (curve1.empty() || curve2.empty()) { return false; }
	if (curve1.front().dist_sqr(curve2.front()) > dist_sqr) { return false; }
	if (curve1.back().dist_sqr(curve2.back()) > dist_sqr) { return false; }

	// cases where at least one curve has length 1
	if (curve1.size() == 1 && curve2.size() == 1) { return true; }
	if (curve1.size() == 1) { return isClose(curve1.front(), curve2); }
	if (curve2.size() == 1) { return isClose(curve2.front(), curve1); }

	clear();

	DiscreteBox initial_box(0, curve1.size()-1, 0, curve2.size()-1);
	DiscreteInputs initial_inputs = computeInitialInputs();
	DiscreteOutputs final_outputs = createFinalOutputs();

	// this is the main computation of the decision problem
	computeOutputs(initial_box, initial_inputs, final_outputs);

	return isTopRightReachable(final_outputs);
}

bool DiscreteFrechetLight::lessThanWithFilters(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	this->curve_pair[0] = &curve1;
	this->curve_pair[1] = &curve2;
	this->distance = distance;
	this->dist_sqr = distance * distance;

	assert(curve1.size());
	assert(curve2.size());

	if (curve1[0].dist_sqr(curve2[0]) > dist_sqr ||
		curve1.back().dist_sqr(curve2.back()) > dist_sqr) {
		return false;
	}
	if (curve1.size() == 1 && curve2.size() == 1) {
		return true;
	}

	// FIXME: check if filters really hold for discrete Fréchet distance

	Filter filter(curve1, curve2, distance);
	
	if (filter.bichromaticFarthestDistance()) {
		return true;
	}
	
	PointID pos1;
	PointID pos2;
	if (filter.adaptiveGreedy(pos1, pos2)) {
		return true;
	}
	if (filter.negative(pos1, pos2)) {
		return false;
	}
	if (filter.adaptiveSimultaneousGreedy()) {
		return true;
	}
	
	++non_filtered;

	return lessThan(distance, curve1, curve2);
}

inline void DiscreteFrechetLight::computeOutputs(
	DiscreteBox const& initial_box, DiscreteInputs const& initial_inputs, DiscreteOutputs& final_outputs)
{
	num_boxes = 0;

	DiscreteBoxData box_data{initial_box, initial_inputs, final_outputs, DiscreteQSimpleOutputs()};
	getReachableIntervals(box_data);
}

inline bool DiscreteFrechetLight::isClose(Point const& point, Curve const& curve) const
{
	return getLastReachablePoint(point, curve) == PointID(curve.size()-1);
}

inline bool DiscreteFrechetLight::isTopRightReachable(DiscreteOutputs const& outputs) const
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];
	auto const& outputs1 = reachable_intervals_vec[outputs.id1];
	auto const& outputs2 = reachable_intervals_vec[outputs.id2];

	return (!outputs1.empty() && (outputs1.back().end == curve1.size()-1))
		|| (!outputs2.empty() && (outputs2.back().end == curve2.size()-1));
}

inline auto DiscreteFrechetLight::createFinalOutputs() -> DiscreteOutputs
{
	DiscreteOutputs outputs;

	reachable_intervals_vec.emplace_back();
	outputs.id1 = reachable_intervals_vec.size()-1;
	reachable_intervals_vec.emplace_back();
	outputs.id2 = reachable_intervals_vec.size()-1;

	return outputs;
}

// this function assumes that the start points of the two curves are close
inline auto DiscreteFrechetLight::computeInitialInputs() -> DiscreteInputs
{
	DiscreteInputs inputs;

	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];

	auto const first = PointID(0);

	auto last1 = getLastReachablePoint(curve2.front(), curve1);
	reachable_intervals_vec.emplace_back();
	reachable_intervals_vec.back().emplace_back(first, last1);
	inputs.begin1 = reachable_intervals_vec.back().begin();
	inputs.end1 = reachable_intervals_vec.back().end();

	auto last2 = getLastReachablePoint(curve1.front(), curve2);
	reachable_intervals_vec.emplace_back();
	reachable_intervals_vec.back().emplace_back(first, last2);
	inputs.begin2 = reachable_intervals_vec.back().begin();
	inputs.end2 = reachable_intervals_vec.back().end();

	return inputs;
}

distance_t DiscreteFrechetLight::calcDistance(Curve const& curve1, Curve const& curve2)
{
	distance_t min = 0.;
	distance_t max = curve1.getUpperBoundDistance(curve2);
	return calcDistance(curve1, curve2, min, max);
}

distance_t DiscreteFrechetLight::calcDistance(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max)
{
	return calcDistance(curve1, curve2, min, max, this->epsilon);
}

distance_t DiscreteFrechetLight::calcDistance(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max, distance_t const epsilon)
{
	if (lessThanWithFilters(min, curve1, curve2)) {
		return min;
	}
	if (!lessThanWithFilters(max, curve1, curve2)) {
		return max;
	}

	while (max - min >= epsilon/2.) {
		auto split = (max + min)/2.;
		if (lessThanWithFilters(split, curve1, curve2)) {
			max = split;
		}
		else {
			min = split;
		}
	}

	return min;
}

// This doesn't have to be called but is handy to make time measurements more consistent
// such that the clears in the lessThan call doen't have to do anything.
void DiscreteFrechetLight::clear()
{
	reachable_intervals_vec.clear();
	qsimple_intervals.clear();
}

auto DiscreteFrechetLight::getCurvePair() const -> CurvePair
{
	return curve_pair;
}

Certificate& DiscreteFrechetLight::computeCertificate()
{
	return cert;
}

void DiscreteFrechetLight::setPruningLevel(int pruning_level)
{
	this->pruning_level = pruning_level;
}

void DiscreteFrechetLight::setRules(std::array<bool,5> const& enable)
{
	enable_box_shrinking = enable[0];
	enable_empty_outputs = enable[1];
	enable_propagation1 = enable[2];
	enable_propagation2 = enable[3];
	enable_boundary_rule = enable[4];
}

std::size_t DiscreteFrechetLight::getNumberOfBoxes() const
{
	return num_boxes;
}
