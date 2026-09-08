#include "fut_n6_algorithm.h"

N6Alg::N6Alg()
	: N6Alg(1e-7)
{
}

N6Alg::N6Alg(distance_t const epsilon)
	// note that we need higher precision in the Fréchet call here
	: epsilon(epsilon), epsilon_sub(epsilon/10.), epsilon_slack(epsilon - epsilon_sub)
	, frechet(epsilon_sub)
{
}

distance_t N6Alg::calcDistance(Curve const& curve1, Curve const& curve2)
{
	assert(!curve1.empty() && !curve2.empty());

	auto center = curve1.front() - curve2.front();
	curve2.translate(center);
	MEASUREMENT::start(EXP::FUT_N6_FRECHET);
	distance_t max = frechet.evaluationFixedTranslation(curve1, curve2);
	MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
	distance_t min = 0.;
	min_translation = center;
	curve2.resetTranslation();

	while (max - min >= epsilon/2.) {
		auto split = (max + min)/2.;
		if (lessThan(split, curve1, curve2)) {
			max = split;
		}
		else {
			min = split;
		}
	}

	return min;
}

distance_t N6Alg::calcDistance(Curve const& curve1, Curve const& curve2, BoundingBox box)
{
	assert(!curve1.empty() && !curve2.empty());

	Point const center = {(box.min.x + box.max.x)/2., (box.min.y + box.max.y)/2.};
	// TODO: Ugly manual back-conversion. Works, but ewwwwww....
	distance_t const diag_dist = Point{box.min.x, box.min.y}.dist(Point{box.max.x, box.max.y})/2.;

	curve2.translate(center);
	MEASUREMENT::start(EXP::FUT_N6_FRECHET);
	distance_t max = frechet.evaluationFixedTranslation(curve1, curve2);
	MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
	distance_t min = std::max(0., max - diag_dist);
	min_translation = center;
	curve2.resetTranslation();

	while (max - min >= epsilon/2.) {
		auto split = (max + min)/2.;
		if (lessThan(split, curve1, curve2, box)) {
			max = split;
		}
		else {
			min = split;
		}
	}

	return min;
}

bool N6Alg::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	assert(!curve1.empty() && !curve2.empty());

	if (curve1.size() == 1 && curve2.size() == 1) {
		min_translation = curve1.front() - curve2.front();
		return true;
	}

	ArrDiscs discs;
	if (candidate_centers.empty()) {
		for (std::size_t i = 0; i < curve1.size(); ++i) {
			for (std::size_t j = 0; j < curve2.size(); ++j) {
				auto center = curve1[i] - curve2[j];
				discs.push_back({{center.x, center.y}, distance});
			}
		}
	}
	else {
		for (auto const& candidate_center: candidate_centers) {
			discs.push_back({{candidate_center.x, candidate_center.y}, distance});
		}
	}

	MEASUREMENT::start(EXP::FUT_N6_ARR);
	ArrangementTraversal arr_traversal(discs, true, epsilon_slack);
	MEASUREMENT::stop(EXP::FUT_N6_ARR);

	if (!arr_traversal.hasNext()) {
		return false;
	}

	MEASUREMENT::start(EXP::FUT_N6_FRECHET);
	while (arr_traversal.hasNext()) {
		auto t = arr_traversal.getNext();
		curve2.translate({t.x, t.y});
		auto less = frechet.lessThanFixedTranslation(distance+epsilon_slack, curve1, curve2);
		curve2.resetTranslation();
		if (less) {
			MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
			min_translation = {t.x, t.y};
			return true;
		}
	}

	MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
	return false;
}

bool N6Alg::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2, BoundingBox box)
{
	assert(!curve1.empty() && !curve2.empty());

	if (curve1.size() == 1 && curve2.size() == 1) {
		min_translation = curve1.front() - curve2.front();
		return true;
	}

	// build arrangement
	ArrDiscs discs;
	if (candidate_centers.empty()) { // no candidates provided
		for (std::size_t i = 0; i < curve1.size(); ++i) {
			for (std::size_t j = 0; j < curve2.size(); ++j) {
				auto center = curve1[i] - curve2[j];
				discs.push_back({{center.x, center.y}, distance});
			}
		}
	}
	else {
		for (auto const& candidate_center: candidate_centers) {
			discs.push_back({{candidate_center.x, candidate_center.y}, distance});
		}
	}

	MEASUREMENT::start(EXP::FUT_N6_ARR);
	ArrangementTraversal arr_traversal(discs, box);
	MEASUREMENT::stop(EXP::FUT_N6_ARR);

	// if the arrangement is empty for this box, just check any point inside it
	if (!arr_traversal.hasNext()) {
		curve2.translate({(box.min.x + box.max.x)/2., (box.min.y + box.max.y)/2.});
		MEASUREMENT::start(EXP::FUT_N6_FRECHET);
		auto less = frechet.lessThanFixedTranslation(distance+epsilon_slack, curve1, curve2);
		MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
		curve2.resetTranslation();
		if (less) {
			min_translation = {(box.min.x + box.max.x)/2., (box.min.y + box.max.y)/2.};
		}
		return less;
	}

	MEASUREMENT::start(EXP::FUT_N6_FRECHET);
	// traverse arrangement and call Fréchet decision algorithm
	// XXX: Note that we don't check for intersection in the box here, because building the
	// arrangement is the expensive part and if we consider a wider box here, this is also
	// fine and makes us find better upper bounds faster.
	while (arr_traversal.hasNext()) {
		auto t = arr_traversal.getNext();
		curve2.translate({t.x, t.y});
		auto less = frechet.lessThanFixedTranslation(distance+epsilon_slack, curve1, curve2);
		curve2.resetTranslation();
		if (less) {
			MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
			min_translation = {t.x, t.y};
			return true;
		}
	}

	MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
	return false;
}

void N6Alg::setCandidateCenters(Points const& points)
{
	candidate_centers = std::move(points);
}	

void N6Alg::resetCandidateCenters()
{
	candidate_centers.clear();
}

auto N6Alg::toBoundingBox(SearchBox const& search_box) const -> BoundingBox
{
	return BoundingBox({
		{search_box.min.x, search_box.min.y},
		{search_box.max.x, search_box.max.y}
	});
}

Point const& N6Alg::getTranslation() const
{
	return min_translation;
}

auto N6Alg::getBoundingBox(distance_t distance, Curve const& curve1, Curve const& curve2)
	-> BoundingBox
{
	auto center1 = curve1.front() - curve2.front();
	auto center2 = curve1.back() - curve2.back();

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

	return N6Alg::BoundingBox({{left, bottom}, {right, top}});
}
