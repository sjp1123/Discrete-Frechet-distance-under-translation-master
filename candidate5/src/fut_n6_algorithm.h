#pragma once

#include "curves.h"
#include "discrete_frechet_queries.h"
#include "frechet_under_translation.h"
#include "geometry_basics.h"

#include <disc_arrangement_traversal.h>

namespace unit_tests { void testN6Algorithm(); }

class N6Alg
{
public:
	using BoundingBox = cgal_disk_arrangements::BoundingBox;

	N6Alg();
	N6Alg(distance_t const epsilon);

	distance_t calcDistance(Curve const& curve1, Curve const& curve2);
	distance_t calcDistance(Curve const& curve1, Curve const& curve2, BoundingBox box);
	bool lessThan(distance_t distance, Curve const& curve1, Curve const& curve2);
	bool lessThan(distance_t distance, Curve const& curve1, Curve const& curve2, BoundingBox box);

	void setCandidateCenters(Points const& points);
	void resetCandidateCenters();

	BoundingBox toBoundingBox(SearchBox const& search_box) const;
	Point const& getTranslation() const;

private:
	using ArrDiscs = cgal_disk_arrangements::Discs;
	using ArrangementTraversal = cgal_disk_arrangements::ArrangementTraversal;

	distance_t const epsilon;
	distance_t const epsilon_sub;
	distance_t const epsilon_slack;

	Points candidate_centers;

	Point min_translation;
	DiscreteFrechetQueries frechet;

	BoundingBox getBoundingBox(distance_t distance, Curve const& curve1, Curve const& curve2);

	friend void unit_tests::testN6Algorithm();
};
