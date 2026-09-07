#include "frechet_under_translation.h"

#include "defs.h"
#include "fut_n6_algorithm.h"

#include <algorithm>
#include <cstdlib>

namespace {
// X3-b regime sweep (NEXT §3.4): arrangement_cut_limit as a runtime knob via the
// CUT_LIMIT env var (default 12, the value all X1/X2 runs used). DEPTH_LIMIT
// likewise (default 40). Read once per constructor call.
std::size_t env_size(const char* name, std::size_t dflt) {
	const char* e = std::getenv(name);
	if (!e) return dflt;
	char* end = nullptr; long v = std::strtol(e, &end, 10);
	return (end && *end == '\0' && v > 0) ? (std::size_t)v : dflt;
}
}

FrechetUnderTranslation::FrechetUnderTranslation()
	: FrechetUnderTranslation(1e-7)
{
}

// TODO: change the default arrangement parameters to something better after experiments
FrechetUnderTranslation::FrechetUnderTranslation(distance_t const epsilon)
	: FrechetUnderTranslation(epsilon, env_size("DEPTH_LIMIT", 40), env_size("CUT_LIMIT", 12))
{
}

FrechetUnderTranslation::FrechetUnderTranslation(distance_t const epsilon, std::size_t depth_limit, std::size_t arrangement_cut_limit)
	: epsilon(epsilon), epsilon_sub(epsilon/10.), frechet(epsilon_sub), kd_tree(arrangement_cut_limit+1)
{
	enableArrangement(depth_limit, arrangement_cut_limit);
}

bool FrechetUnderTranslation::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	resetSearchBoxRestriction();
	reset_search_boxes = true;
	use_kd_tree = false;

	return lessThanImpl(distance, curve1, curve2);
}

bool FrechetUnderTranslation::lessThanImpl(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	MEASUREMENT::start(EXP::FUT_PREPROCESSING1);
	this->curve_pair[0] = &curve1;
	this->curve_pair[1] = &curve2;
	this->distance = distance;
	this->dist_sqr = distance * distance;
	this->center1 = curve1.front() - curve2.front();
	this->center2 = curve1.back() - curve2.back();

	assert(curve1.size());
	assert(curve2.size());

	auto search_box = getInitialSearchBox();
	MEASUREMENT::stop(EXP::FUT_PREPROCESSING1);
	if(search_box.empty()) { return false; }

	bool less = lessThanImpl(search_box);
#ifdef VIS
	vis.writeSvg("divide_and_conquer.svg", 25);
#endif
	return less;
}

distance_t FrechetUnderTranslation::calcDistance(Curve const& curve1, Curve const& curve2)
{
	MEASUREMENT::start(EXP::FUT_PREPROCESSING1);
	resetSearchBoxRestriction();
	reset_search_boxes = true;
	use_kd_tree = true;

	// build kd-tree
	kd_tree.clear();
	for (std::size_t i = 0; i < curve1.size(); ++i) {
		for (std::size_t j = 0; j < curve2.size(); ++j) {
			auto center = curve1[i] - curve2[j];
			kd_tree.add({center.x, center.y}, center);
		}
	}
	kd_tree.build();

	distance_t min = 0.;
	distance_t max = curve1.getUpperBoundDistance(curve2);

	this->center1 = curve1.front() - curve2.front();
	this->center2 = curve1.back() - curve2.back();
	this->curve_pair[0] = &curve1;
	this->curve_pair[1] = &curve2;
	assert(!curve1.empty());
	assert(!curve2.empty());

	std::tie(min, max) = getInitialEstimates(max);
	MEASUREMENT::stop(EXP::FUT_PREPROCESSING1);

	while (max - min >= epsilon/2.) {
		auto split = (max + min)/2.;

		if (lessThanImpl(split, curve1, curve2)) {
			max = split;
			reset_search_boxes = false;
		}
		else {
			min = split;
			reset_search_boxes = true;
		}

		// Take the center of the last box, which should always be close to the
		// lowest distance that we found so far, and compute the exact distance.
		curve2.translate(last_translation);
		MEASUREMENT::start(EXP::FUT_BLACKBOX1);
		auto last_translation_dist = frechet.evaluationFixedTranslation(curve1, curve2);
		MEASUREMENT::stop(EXP::FUT_BLACKBOX1);
		if (last_translation_dist < max) {
			max = last_translation_dist;
			min_translation = last_translation;
		}
		curve2.resetTranslation();
	}

	return min;
}

distance_t FrechetUnderTranslation::calcDistance2(Curve const& curve1, Curve const& curve2)
{
#ifdef VIS
		vis.clear();
#endif

	MEASUREMENT::start(EXP::FUT_PREPROCESSING2);
	curve2.resetTranslation();
	resetSearchBoxRestriction();
	use_kd_tree = true;

	// build kd-tree
	kd_tree.clear();
	for (std::size_t i = 0; i < curve1.size(); ++i) {
		for (std::size_t j = 0; j < curve2.size(); ++j) {
			auto center = curve1[i] - curve2[j];
			kd_tree.add({center.x, center.y}, center);
		}
	}
	kd_tree.build();

	distance_t max = curve1.getUpperBoundDistance(curve2);
	
	this->center1 = curve1.front() - curve2.front();
	this->center2 = curve1.back() - curve2.back();
	this->curve_pair[0] = &curve1;
	this->curve_pair[1] = &curve2;
	assert(!curve1.empty());
	assert(!curve2.empty());
	
	distance_t lower_bound = 0.;
	std::tie(lower_bound, max) = getInitialEstimates(max);
	assert(lower_bound >= 0.);

	this->distance = max;
	this->dist_sqr = max*max;

	// Abuse the already existing function to give us the max and min of the initial
	// search box.
	auto init_search_box = getInitialSearchBox();
	assert(!init_search_box.empty());

	BBQueue search_boxes;
	search_boxes.push({init_search_box.min, init_search_box.max, lower_bound, 0});
	MEASUREMENT::stop(EXP::FUT_PREPROCESSING2);

	while (!search_boxes.empty()) {
		auto search_box = search_boxes.top();
		search_boxes.pop();
		auto const diag_dist = search_box.getDiagonalDist()/2.;
		auto& min_dist = search_box.min_dist;
		curve2.resetTranslation();

		if (min_dist > max) {
			continue;
		}

#ifdef VIS
		vis.add(search_box, search_box.layer);
#endif

		// If we reached the depth limit, we either build the arrangement or
		// just stop searching.
		if (search_box.layer >= depth_limit) {
			if (arrangement_enabled) {
				MEASUREMENT::start(EXP::FUT_DISCSELECTION2);
				computeCutCenters(search_box, min_dist, max, false);
				MEASUREMENT::stop(EXP::FUT_DISCSELECTION2);

				// The decision for this box is uniform so there cannot exist any minimum.
				if (cut_centers.size() == 0) {
					continue;
				}

				MEASUREMENT::start(EXP::FUT_ARRANGEMENT2);
				N6Alg n6_alg(epsilon_sub);
				n6_alg.setCandidateCenters(cut_centers);
				// only calculate the actual distance of this box if it improves max
				if (n6_alg.lessThan(max, curve1, curve2)) {
					max = n6_alg.calcDistance(curve1, curve2);
					min_translation = n6_alg.getTranslation();
				}
				MEASUREMENT::stop(EXP::FUT_ARRANGEMENT2);
				continue;
			}
		}

		curve2.translate(search_box.center());

		// Check if the current center is a better witness than everything we saw before.
		MEASUREMENT::start(EXP::FUT_BLACKBOX2);
		if (frechet.lessThanFixedTranslation(max, curve1, curve2)) {
			max = frechet.evaluationFixedTranslation(curve1, curve2, min_dist, max);
			min_translation = search_box.center();

			min_dist = std::max(min_dist, max-diag_dist);
		}
		else {
			// If this is true, then all the distances in the box are larger than the
			// current maximum. Thus, we can ignore it.
			if (!frechet.lessThanFixedTranslation(max + diag_dist, curve1, curve2)) {
				continue;
			}

			// TODO: Optimize the value of epsilon_coarse
			auto const epsilon_coarse = (max-min_dist)/10.;
			// We know that our distance for the box center is between min_dist and
			// max + diag_dist. The first value that would actually improve our lower
			// bound would be min_dist+diag_dist. So we compute a rough approximation
			// in the range [min_dist+diag_dist, max+diag_dist]. By subtracting
			// diag_dist, we get a lower bound for the whole box and any value in
			// this range except min_dist+diag_dist gives us an improved lower bound.
			auto const lower_bound =
				frechet.evaluationFixedTranslation(curve1, curve2,min_dist+diag_dist, max+diag_dist, epsilon_coarse)-diag_dist;
			min_dist = std::max(min_dist, lower_bound);
		}
		MEASUREMENT::stop(EXP::FUT_BLACKBOX2);

		curve2.resetTranslation();

		// If the box already contains an epsilon approximation, drop it
		if (max - min_dist <= epsilon/2.) {
		  continue;
		}

		if (arrangement_enabled) {
			MEASUREMENT::start(EXP::FUT_DISCSELECTION2);
			computeCutCenters(search_box, min_dist, max);
			MEASUREMENT::stop(EXP::FUT_DISCSELECTION2);

			// The decision for this box is uniform so there cannot exist any minimum.
			if (cut_centers.size() == 0) {
				continue;
			}

			// If the arrangement appears to be small, then we build it and
			// thus decide this box and stop the search.
			if (cut_centers.size() <= arrangement_cut_limit) {
				MEASUREMENT::start(EXP::FUT_ARRANGEMENT2);
				N6Alg n6_alg(epsilon_sub);
				n6_alg.setCandidateCenters(cut_centers);
				// only calculate the actual distance of this box if it improves max
				if (n6_alg.lessThan(max, curve1, curve2)) {
					max = n6_alg.calcDistance(curve1, curve2);
					min_translation = n6_alg.getTranslation();
				}
				MEASUREMENT::stop(EXP::FUT_ARRANGEMENT2);
				continue;
			}
		}

		// If we could not yet resolve the box, split it
		this->distance = max;
		pushChildren(search_boxes, search_box);
	}

#ifdef VIS
	vis.writeSvg("divide_and_conquer.svg", 40);
#endif

	curve2.resetTranslation();
	return max;
}

void FrechetUnderTranslation::enableArrangement(std::size_t depth_limit, std::size_t arrangement_cut_limit)
{
	this->arrangement_enabled = true;
	this->depth_limit = depth_limit;
	this->arrangement_cut_limit = arrangement_cut_limit;
}

Point FrechetUnderTranslation::getTranslation() const
{
	return min_translation;
}

distance_t FrechetUnderTranslation::getEpsilon() const
{
	return epsilon;
}

void FrechetUnderTranslation::disableArrangement()
{
	arrangement_enabled = false;
}

inline std::pair<distance_t, distance_t> FrechetUnderTranslation::getInitialEstimates(distance_t rough_ub)
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];

	//std::cout << "rough upper bound: " << std::setprecision(10) << rough_ub << std::endl;

	//TODO we use a sub_epsilon of epsilon/2. here. Is this the most reasonable choice? 

	//EXPERIMENTAL: get better initial upper and lower bounds
	curve2.translate(center1);
	assert(curve1.front().dist(curve2.front()) < epsilon/2.);
	distance_t front_approx = frechet.evaluationFixedTranslation(curve1, curve2, 0., rough_ub, epsilon / 2.); 
	//std::cout << "front_approx: " << front_approx << " (lb is: " << front_approx /2. << ")" << std::endl;
	curve2.resetTranslation();

	
	curve2.translate(center2);
	assert(curve1.back().dist(curve2.back()) < epsilon/2.);
	distance_t back_approx = frechet.evaluationFixedTranslation(curve1, curve2, front_approx/2., 2. * front_approx, epsilon / 2.); 
	//std::cout << "back_approx: " << back_approx << " (lb is: " << back_approx /2. << ")" << std::endl;
	curve2.resetTranslation();

	distance_t lower_bound, upper_bound;
       	if (front_approx < back_approx) {
		upper_bound = front_approx + epsilon /2.;
		lower_bound = back_approx/2. - epsilon/2.; 
		min_translation = center1;
	} else {
		upper_bound = back_approx + epsilon / 2.;
		lower_bound = front_approx/2. - epsilon/2.;
		min_translation = center2;
	}
	assert(upper_bound <= rough_ub + epsilon/2.);

	//TODO: does this help?
	//distance_t center_estimate = center1.dist(center2) / 2. - epsilon/2.;
	//if (center_estimate > lower_bound) {
		//std::cout << "Potential for Improvement:" << std::setprecision(20) << center_estimate/lower_bound << std::endl; 
	//	lower_bound = center_estimate;
	//}
			
	//TODO: Investigate the possible improvements below
	//Two futher upper_bound improvements -> yields a *small* improvement
	//Point center_trans = (center2 - center1)*0.5; 
	//curve2.translate(center_trans);
	//assert( std::abs(curve1.front().dist(curve2.front())-curve1.back().dist(curve2.back())) < epsilon ); 
	//distance_t new_bound = frechet.evaluationFixedTranslation(curve1, curve2, lower_bound, upper_bound, epsilon/2.);
	//curve2.resetTranslation();
	//if (new_bound < upper_bound - epsilon/2.) {
	//	upper_bound = new_bound + epsilon /2.;
	//	min_translation = center_trans;
	//}
	//auto ep1 = curve1.getExtremePoints();
	//auto ep2 = curve2.getExtremePoints();
	//Point bb_trans = Point{ (ep1.min_x +  ep1.max_x - ep2.min_x - ep2.max_x) / 2., (ep1.min_y +  ep1.max_y - ep2.min_y - ep2.max_y) / 2.};
	//curve2.translate(bb_trans);
	//auto new_ep2 = curve2.getExtremePoints();
	//assert( std::abs((new_ep2.min_x - ep1.min_x) - (ep1.max_x - new_ep2.max_x)) < epsilon ); 
	//assert( std::abs((new_ep2.min_y - ep1.min_y) - (ep1.max_y - new_ep2.max_y)) < epsilon ); 
	//distance_t new_bound_bb = frechet.evaluationFixedTranslation(curve1, curve2, lower_bound, upper_bound, epsilon/2.);
	//curve2.resetTranslation();
	//if (new_bound_bb < upper_bound - epsilon/2.) {
	//	upper_bound = new_bound_bb + epsilon/2.;
	//	min_translation = bb_trans;
	//}




	return std::make_pair(std::max(lower_bound,0.), upper_bound);
}

SearchBox FrechetUnderTranslation::getInitialSearchBox() const
{
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];

	// constrain by start and end points
	auto left = std::max(center1.x, center2.x) - distance;
	auto right = std::min(center1.x, center2.x) + distance;
	auto bottom = std::max(center1.y, center2.y) - distance;
	auto top = std::min(center1.y, center2.y) + distance;

	// constrain by extrema
	auto ep1 = curve1.getExtremePoints();
	auto ep2 = curve2.getExtremePoints();
	left = std::max(left, ep1.min_x - ep2.min_x - distance);
	right = std::min(right, ep1.max_x - ep2.max_x + distance);
	bottom = std::max(bottom, ep1.min_y - ep2.min_y - distance);
	top = std::min(top, ep1.max_y - ep2.max_y + distance);

	// constrain by previous restriction
	left = std::max(left, search_box_restriction.min.x);
	right = std::min(right, search_box_restriction.max.x);
	bottom = std::max(bottom, search_box_restriction.min.y);
	top = std::min(top, search_box_restriction.max.y);

	return SearchBox({left, bottom}, {right, top});
}

bool FrechetUnderTranslation::lessThanImpl(SearchBox const& init_search_box)
{
	MEASUREMENT::start(EXP::FUT_PREPROCESSING1);
	auto const& curve1 = *curve_pair[0];
	auto const& curve2 = *curve_pair[1];

	// check if translation discs for start and end points intersect
	auto center1 = curve1.front() - curve2.front();
	auto center2 = curve1.back() - curve2.back();
	if (center1.dist_sqr(center2) > 4*dist_sqr) {
		MEASUREMENT::stop(EXP::FUT_PREPROCESSING1);
		return false;
	}

	if (reset_search_boxes) {
		search_boxes.init(init_search_box);
#ifdef VIS
		vis.clear();
#endif
	}
	else {
#ifdef VIS
		// we will reconsider the last box, so remove it from vis
		vis.pop();
#endif
	}
	MEASUREMENT::stop(EXP::FUT_PREPROCESSING1);

	std::size_t box_count = 0;
	while (!search_boxes.empty()) {
		auto const& search_box = search_boxes.current();
		auto diag_dist = search_box.getDiagonalDist()/2.;
		++box_count;
#ifdef VIS
		vis.add(search_box, search_boxes.currentLayer());
#endif

		curve2.translate(search_box.center());
		last_translation = search_box.center();

		// negative test with distance + diagonal
		MEASUREMENT::start(EXP::FUT_BLACKBOX1);
		if (!frechet.lessThanFixedTranslation(distance + diag_dist, curve1, curve2)) {
			MEASUREMENT::stop(EXP::FUT_BLACKBOX1);
			search_boxes.step();
			continue; 
		}
		MEASUREMENT::stop(EXP::FUT_BLACKBOX1);

		// positive test
		MEASUREMENT::start(EXP::FUT_BLACKBOX1);
		if (frechet.lessThanFixedTranslation(distance, curve1, curve2)) {
			MEASUREMENT::stop(EXP::FUT_BLACKBOX1);
			curve2.resetTranslation();
			updateRestrictedSearchBox(search_boxes);
			min_translation = last_translation;
			return true;
		}
		MEASUREMENT::stop(EXP::FUT_BLACKBOX1);

		if (search_boxes.currentLayer() >= depth_limit) {
			// We reached the depth limit, so either we build the arrangement or
			// just stop searching.
			if (arrangement_enabled) {
				curve2.resetTranslation();
				MEASUREMENT::start(EXP::FUT_DISCSELECTION1);
				computeCutCenters(search_box, false);
				MEASUREMENT::stop(EXP::FUT_DISCSELECTION1);

				// The decision for this box is uniform and we already had a negative
				// representative, thus there is no valid translation in this box.
				// Note though, that this uses our non-CGAL intersection test!
				if (cut_centers.size() == 0) {
					continue;
				}

				MEASUREMENT::start(EXP::FUT_ARRANGEMENT1);
				N6Alg n6_alg(epsilon_sub);
				n6_alg.setCandidateCenters(cut_centers);
				bool less = n6_alg.lessThan(distance, curve1, curve2);
				MEASUREMENT::stop(EXP::FUT_ARRANGEMENT1);
				if (less) {
					updateRestrictedSearchBox(search_boxes);
					min_translation = n6_alg.getTranslation();
					return true;
				}
			}
		}
		else {
			if (arrangement_enabled) {
				curve2.resetTranslation();
				MEASUREMENT::start(EXP::FUT_DISCSELECTION1);
				computeCutCenters(search_box);
				MEASUREMENT::stop(EXP::FUT_DISCSELECTION1);

				// The decision for this box is uniform and we already had a negative
				// representative, thus there is no valid translation in this box.
				// Note though, that this uses our non-CGAL intersection test!
				if (cut_centers.size() == 0) {
					search_boxes.step();
					continue;
				}

				// If the arrangement appears to be small, then we build it and
				// thus decide this box and stop the search.
				if (cut_centers.size() <= arrangement_cut_limit) {
					MEASUREMENT::start(EXP::FUT_ARRANGEMENT1);
					N6Alg n6_alg(epsilon_sub);
					n6_alg.setCandidateCenters(cut_centers);
					bool less = n6_alg.lessThan(distance, curve1, curve2);
					MEASUREMENT::stop(EXP::FUT_ARRANGEMENT1);
					if (less) {
						updateRestrictedSearchBox(search_boxes);
						min_translation = n6_alg.getTranslation();
						return true;
					}
					else {
						search_boxes.step();
						continue;
					}
				}
			}

			pushChildren(search_boxes, search_box);
		}

		search_boxes.step();
	}

	curve2.resetTranslation();

	return false;
}

void FrechetUnderTranslation::updateRestrictedSearchBox(SearchBoxes const& search_boxes)
{
	distance_t left = std::numeric_limits<distance_t>::max();
	distance_t right = std::numeric_limits<distance_t>::lowest();
	distance_t bottom = std::numeric_limits<distance_t>::max();
	distance_t top = std::numeric_limits<distance_t>::lowest();

	for (auto it = search_boxes.begin_current(); it != search_boxes.end_current(); ++it) {
		left = std::min(left, it->min.x);
		right = std::max(right, it->max.x);
		bottom = std::min(bottom, it->min.y);
		top = std::max(top, it->max.y);
	}
	for (auto it = search_boxes.begin_other(); it != search_boxes.end_other(); ++it) {
		left = std::min(left, it->min.x);
		right = std::max(right, it->max.x);
		bottom = std::min(bottom, it->min.y);
		top = std::max(top, it->max.y);
	}

	search_box_restriction.min.x = left;
	search_box_restriction.max.x = right;
	search_box_restriction.min.y = bottom;
	search_box_restriction.max.y = top;
}

void FrechetUnderTranslation::resetSearchBoxRestriction()
{
	search_box_restriction.min.x = std::numeric_limits<distance_t>::lowest();
	search_box_restriction.max.x = std::numeric_limits<distance_t>::max();
	search_box_restriction.min.y = std::numeric_limits<distance_t>::lowest();
	search_box_restriction.max.y = std::numeric_limits<distance_t>::max();
}

void FrechetUnderTranslation::pushChildren(SearchBoxes& search_boxes, SearchBox const& parent) const
{
	auto center = parent.center();
	auto const& max = parent.max;
	auto const& min = parent.min;

	if (max.x - min.x >= max.y - min.y) { // vertical split
		auto left_box = SearchBox(min, Point{center.x, max.y});
		if (intersectsStartEndDiscs(left_box)) {
			search_boxes.push(left_box);
		}

		auto right_box = SearchBox(Point{center.x, min.y}, max);
		if (intersectsStartEndDiscs(right_box)) {
			search_boxes.push(right_box);
		}
	}
	else { // horizontal split
		auto bottom_box = SearchBox(min, Point{max.x, center.y});
		if (intersectsStartEndDiscs(bottom_box)) {
			search_boxes.push(bottom_box);
		}

		auto top_box = SearchBox(Point{min.x, center.y}, max);
		if (intersectsStartEndDiscs(top_box)) {
			search_boxes.push(top_box);
		}
	}
}

void FrechetUnderTranslation::pushChildren(BBQueue& search_boxes, SearchBoxWithMin const& parent) const
{
	auto center = parent.center();
	auto const& max = parent.max;
	auto const& min = parent.min;
	auto const& min_dist = parent.min_dist;
	auto const new_layer = parent.layer + 1;

	if (max.x - min.x >= max.y - min.y) { // vertical split
		auto left_box = SearchBoxWithMin(min, Point{center.x, max.y}, min_dist, new_layer);
		if (intersectsStartEndDiscs(left_box)) {
			search_boxes.push(left_box);
		}

		auto right_box = SearchBoxWithMin(Point{center.x, min.y}, max, min_dist, new_layer);
		if (intersectsStartEndDiscs(right_box)) {
			search_boxes.push(right_box);
		}
	}
	else { // horizontal split
		auto bottom_box = SearchBoxWithMin(min, Point{max.x, center.y}, min_dist, new_layer);
		if (intersectsStartEndDiscs(bottom_box)) {
			search_boxes.push(bottom_box);
		}

		auto top_box = SearchBoxWithMin(Point{min.x, center.y}, max, min_dist, new_layer);
		if (intersectsStartEndDiscs(top_box)) {
			search_boxes.push(top_box);
		}
	}
}

// Note: This function assumes that the start and end disc intersect.
bool FrechetUnderTranslation::intersectsStartEndDiscs(SearchBox const& box) const
{
	// just to add a flag with which we can turn this pruning on/off.
	if (!enable_intersection_pruning) { return true; }

	// Check if the trivial intersection point is inside the box. This also catches
	// the case where the whole intersection is inside the box and thus we later
	// only have to check for intersections that also intersect the box boundary.
	auto middle_point = center1+center2; middle_point /= 2.;
	if (box.contains(middle_point)) { return true; }

	Point start;
	Point end;
	Interval interval1;
	Interval interval2;

	{ // left
		start = box.bottomLeft();
		end = box.topLeft();
		interval1 = IntersectionAlgorithm::intersection_interval(center1, distance, start, end);
		interval2 = IntersectionAlgorithm::intersection_interval(center2, distance, start, end);

		if (interval1.intersects(interval2)) { return true; }
	}
	{ // right
		start = box.bottomRight();
		end = box.topRight();
		interval1 = IntersectionAlgorithm::intersection_interval(center1, distance, start, end);
		interval2 = IntersectionAlgorithm::intersection_interval(center2, distance, start, end);

		if (interval1.intersects(interval2)) { return true; }
	}
	{ // top
		start = box.topLeft();
		end = box.topRight();
		interval1 = IntersectionAlgorithm::intersection_interval(center1, distance, start, end);
		interval2 = IntersectionAlgorithm::intersection_interval(center2, distance, start, end);

		if (interval1.intersects(interval2)) { return true; }
	}
	{ // bottom
		start = box.bottomLeft();
		end = box.bottomRight();
		interval1 = IntersectionAlgorithm::intersection_interval(center1, distance, start, end);
		interval2 = IntersectionAlgorithm::intersection_interval(center2, distance, start, end);

		if (interval1.intersects(interval2)) { return true; }
	}

	return false;
}

void FrechetUnderTranslation::computeCutCenters(SearchBox const& box, bool threshold)
{
	cut_centers.clear();

	if (use_kd_tree) {
		auto intersection_check = [&](Point const& center) {
			return intersectAnnulus(box, center);
		};

		// Search for potential discs that are contained
		auto const min_dist_to_boundary = std::min(std::abs(box.min.x - box.center().x), std::abs(box.min.y - box.center().y));
		auto const max_dist_to_boundary = box.getDiagonalDist()/2.;
		if (min_dist_to_boundary >= distance) {
			kd_tree.search({box.center().x, box.center().y}, 0,
						   max_dist_to_boundary-distance, cut_centers, threshold, intersection_check);
			if (threshold && cut_centers.size() > arrangement_cut_limit) { return; }
		}

		// Search for potential discs that intersect
		kd_tree.search({box.center().x, box.center().y}, distance-box.getDiagonalDist()/2.,
					   distance+box.getDiagonalDist()/2., cut_centers, threshold, intersection_check);
		if (threshold && cut_centers.size() > arrangement_cut_limit) { return; }
	}
	else {
		auto const& curve1 = *curve_pair[0];
		auto const& curve2 = *curve_pair[1];

		for (std::size_t i = 0; i < curve1.size(); ++i) {
			for (std::size_t j = 0; j < curve2.size(); ++j) {
				auto center = curve1[i] - curve2[j];
				if (intersectAnnulus(box, center)) {
					cut_centers.push_back(center);
					if (cut_centers.size() > arrangement_cut_limit) {
						return;
					}
				}
			}
		}
	}
}

bool FrechetUnderTranslation::intersectAnnulus(SearchBox const& box, Point const& center) const
{
	auto const distance = this->distance;
	auto const dist_sqr = this->dist_sqr;

	// check which corners are contained
	bool contains_tl = (box.topLeft().dist_sqr(center) <= dist_sqr);
	bool contains_tr = (box.topRight().dist_sqr(center) <= dist_sqr);
	bool contains_bl = (box.bottomLeft().dist_sqr(center) <= dist_sqr);
	bool contains_br = (box.bottomRight().dist_sqr(center) <= dist_sqr);

	// if the whole box is contained, this disc doesn't contribute to the arrangement
	if (contains_tl && contains_tr && contains_bl && contains_br) {
		return false;
	}

	// if the disc does not intersect the box at all
	if (!contains_tl && !contains_tr && !contains_bl && !contains_br) {
		// check if bounding boxes of circle and box intersect
		if (center.x + distance < box.min.x ||
		    center.y + distance < box.min.y ||
		    center.x - distance > box.max.x ||
			center.y - distance > box.max.y) {

			return false;
		}

		// check if all nodes of rect are in one quadrant
		if ((box.bottomLeft().x > center.x && box.bottomLeft().y > center.y) ||
			(box.bottomRight().x < center.x && box.bottomRight().y > center.y) ||
			(box.topLeft().x > center.x && box.topLeft().y < center.y) ||
			(box.topRight().x < center.x && box.topRight().y < center.y)) {

			return false;
		}
	}

	return true;
}

void FrechetUnderTranslation::computeCutCenters(SearchBox const& box, distance_t min_radius, distance_t max_radius, bool threshold)
{
	cut_centers.clear();

	if (use_kd_tree) {
		auto intersection_check = [&](Point const& center) {
			return intersectAnnulus(box, center, min_radius, max_radius);
		};

		// Search for potential discs that are contained
		auto const min_dist_to_boundary =
			std::min(std::abs(box.min.x - box.center().x), std::abs(box.min.y - box.center().y));
		auto const max_dist_to_boundary = box.getDiagonalDist()/2.;
		if (min_dist_to_boundary >= min_radius) {
			auto const min = 0;
			auto const max = max_dist_to_boundary-min_radius;
			kd_tree.search({box.center().x, box.center().y}, min, max, cut_centers,
			               threshold, intersection_check);
			if (threshold && cut_centers.size() > arrangement_cut_limit) { return; }
		}

		// Search for potential discs that intersect
		auto const min = std::max(0., min_radius-box.getDiagonalDist()/2.);
		auto const max = max_radius+box.getDiagonalDist()/2.;
		kd_tree.search({box.center().x, box.center().y}, min, max, cut_centers,
		               threshold, intersection_check);
		if (threshold && cut_centers.size() > arrangement_cut_limit) { return; }
	}
	else {
		auto const& curve1 = *curve_pair[0];
		auto const& curve2 = *curve_pair[1];

		for (std::size_t i = 0; i < curve1.size(); ++i) {
			for (std::size_t j = 0; j < curve2.size(); ++j) {
				auto center = curve1[i] - curve2[j];
				if (intersectAnnulus(box, center, min_radius, max_radius)) {
					cut_centers.push_back(center);
					if (cut_centers.size() > arrangement_cut_limit) {
						return;
					}
				}
			}
		}
	}
}

bool FrechetUnderTranslation::intersectAnnulus(SearchBox const& box, Point const& center, distance_t min_radius, distance_t max_radius) const
{
	auto const min_radius_sqr = min_radius*min_radius;
	bool contains_tl_min = (box.topLeft().dist_sqr(center) <= min_radius_sqr);
	bool contains_tr_min = (box.topRight().dist_sqr(center) <= min_radius_sqr);
	bool contains_bl_min = (box.bottomLeft().dist_sqr(center) <= min_radius_sqr);
	bool contains_br_min = (box.bottomRight().dist_sqr(center) <= min_radius_sqr);

	// if the whole box is contained in min_radius, this disc doesn't contribute to the arrangement
	if (contains_tl_min && contains_tr_min && contains_bl_min && contains_br_min) {
		return false;
	}

	auto const max_radius_sqr = max_radius*max_radius;
	bool contains_tl_max = (box.topLeft().dist_sqr(center) <= max_radius_sqr);
	bool contains_tr_max = (box.topRight().dist_sqr(center) <= max_radius_sqr);
	bool contains_bl_max = (box.bottomLeft().dist_sqr(center) <= max_radius_sqr);
	bool contains_br_max = (box.bottomRight().dist_sqr(center) <= max_radius_sqr);

	// if the disc with max_radius does not intersect the box at all
	if (!contains_tl_max && !contains_tr_max && !contains_bl_max && !contains_br_max) {
		// check if bounding boxes of circle and box intersect
		if (center.x + max_radius < box.min.x ||
		    center.y + max_radius < box.min.y ||
		    center.x - max_radius > box.max.x ||
			center.y - max_radius > box.max.y) {

			return false;
		}

		// check if all nodes of rect are in one quadrant
		if ((box.bottomLeft().x > center.x && box.bottomLeft().y > center.y) ||
			(box.bottomRight().x < center.x && box.bottomRight().y > center.y) ||
			(box.topLeft().x > center.x && box.topLeft().y < center.y) ||
			(box.topRight().x < center.x && box.topRight().y < center.y)) {

			return false;
		}
	}

	return true;
}
