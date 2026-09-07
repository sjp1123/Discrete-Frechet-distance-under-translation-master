#pragma once

#include "divide_and_conquer_vis.h"
#include "discrete_frechet_queries.h"
#include "frechet_under_translation_types.h"
#include "geometry_basics.h"
#include "curves.h"
#include "kdtree2.h"

#include <array>
#include <limits>
#include <queue>
#include <vector>
#include <string>
#include <unordered_set>

namespace unit_tests {
	void testFrechetUnderTranslation();
	void testFrechetUnderTranslationQuery();
}

using ArrKdTree = KdTree2<distance_t, Point>;

class FrechetUnderTranslation
{
	using CurvePair = std::array<Curve const*, 2>;

public:
	FrechetUnderTranslation();
	FrechetUnderTranslation(distance_t const epsilon);
	FrechetUnderTranslation(distance_t const epsilon, std::size_t depth_limit, std::size_t arrangement_cut_limit);

	bool lessThan(distance_t distance, Curve const& curve1, Curve const& curve2);
	distance_t calcDistance(Curve const& curve1, Curve const& curve2);
	distance_t calcDistance2(Curve const& curve1, Curve const& curve2);

	// only returns a valid value if the last call to lessThan returned true
	Point getTranslation() const;
	distance_t getEpsilon() const;

	void disableArrangement();

private:
	distance_t const epsilon;
	distance_t const epsilon_sub;

	CurvePair curve_pair;
	Point center1, center2;
	distance_t distance;
	distance_t dist_sqr;

	DiscreteFrechetQueries frechet;

	Point last_translation;
	Point min_translation;

	SearchBoxes search_boxes;
	bool reset_search_boxes = true;
	SearchBox search_box_restriction {
		{std::numeric_limits<distance_t>::lowest(), std::numeric_limits<distance_t>::lowest()},
		{std::numeric_limits<distance_t>::max(), std::numeric_limits<distance_t>::max()}
	};

	using BBQueue = std::priority_queue<
		SearchBoxWithMin, std::vector<SearchBoxWithMin>, std::greater<SearchBoxWithMin>>;

	bool enable_intersection_pruning = true;
	bool enable_arrangement_pruning = false;
	bool arrangement_enabled = false;
	std::size_t depth_limit = std::numeric_limits<std::size_t>::max();
	std::size_t arrangement_cut_limit = 10;
	void enableArrangement(std::size_t depth_limit, std::size_t arrangement_cut_limit);

	
	bool lessThanImpl(distance_t distance, Curve const& curve1, Curve const& curve2);
	bool lessThanImpl(SearchBox const& init_search_box);
	SearchBox getInitialSearchBox() const;
	inline std::pair<distance_t,distance_t> getInitialEstimates(distance_t rough_upper_bound);
	void pushChildren(SearchBoxes& search_box_queue, SearchBox const& parent) const;
	void pushChildren(BBQueue& search_box_queue, SearchBoxWithMin const& parent) const;

	bool intersectsStartEndDiscs(SearchBox const& box) const;
	void computeCutCenters(SearchBox const& box, bool threshold = true);
	void computeCutCenters(SearchBox const& box, distance_t min_radius, distance_t max_radius, bool threshold = true);
	Points cut_centers;

	bool intersectAnnulus(SearchBox const& box, Point const& center) const;
	bool intersectAnnulus(SearchBox const& box, Point const& center, distance_t min_radius, distance_t max_radius) const;
	void updateRestrictedSearchBox(SearchBoxes const& search_boxes);
	void resetSearchBoxRestriction();

	std::vector<int> yes_boxes;
	std::vector<int> no_boxes;

#ifdef VIS
	DivideAndConquerVis vis;
#endif

	ArrKdTree kd_tree;
	bool use_kd_tree;

	friend void unit_tests::testFrechetUnderTranslation();
	friend void unit_tests::testFrechetUnderTranslationQuery();
};
