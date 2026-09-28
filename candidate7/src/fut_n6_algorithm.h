#pragma once

#include "curves.h"
#include "discrete_frechet_queries.h"
#include "frechet_under_translation.h"
#include "geometry_basics.h"

#include <disc_arrangement_traversal.h>
#include <cctype>
#include <cstdio>
#include <functional>
#include <cstdlib>
#include <string>

// FIX knobs (env, read once):
//   MAXREGION_FIX = union (default) | box | none | full | aware | lazy
//       union : candidate5's witnesses first (early YES), then the box family
//       box   : maximal cut-disc sets whose intersection meets the box, witness in the box
//       none  : candidate5 as shipped (unsound per box)
//       full  : cut discs + box-containing discs through the unchanged emitter (slow)
//       aware : Helly with the box-containing discs as a common block (slow)
//       lazy  : shipped emitter, then 'aware' when a NO cannot be certified (slow)
//   N6_RANGE      = 1 (default) | 0
//       LMF base case: binary search on [l_B, max] only and never raise max.
//       0 restores the authors' search on [0, f(tau_start)], which can return up to 2x the optimum.
// FIX (candidate7): an unknown value used to select 0 = none (the unsound candidate5 path)
// silently; it now warns and keeps the default.  N6_RANGE accepts 0/1, off/on, false/true, no/yes.
inline int maxregion_fix_mode() {
	static int m = []{ const char* e = std::getenv("MAXREGION_FIX"); if (!e) return 5; std::string v(e);
		if (v == "none") return 0;
		int const r = v == "full" ? 1 : v == "aware" ? 2 : v == "lazy" ? 3 : v == "box" ? 4 : v == "union" ? 5 : -1;
		if (r < 0) { std::fprintf(stderr, "MAXREGION_FIX=%s not understood; using union\n", e); return 5; }
		return r; }();
	return m;
}
inline bool n6_range_fix() {
	static bool b = []{ const char* e = std::getenv("N6_RANGE"); if (!e) return true; std::string v(e);
		for (auto& ch: v) ch = (char)std::tolower((unsigned char)ch);
		if (v == "0" || v == "off" || v == "false" || v == "no") return false;
		if (v == "1" || v == "on" || v == "true" || v == "yes") return true;
		std::fprintf(stderr, "N6_RANGE=%s not understood; using 1\n", e); return true; }();
	return b;
}

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
	// FIX: the search box of this base case and a lazy source of the centres of
	// every disc that contains it (at the smallest radius the caller will query).
	void setBox(BoundingBox const& b) { box_ = b; has_box = true; }
	void setContainProvider(std::function<void(Points&)> f) { contain_provider = std::move(f); }
	distance_t calcDistanceRange(Curve const& curve1, Curve const& curve2, distance_t lo, distance_t hi);
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

	bool has_box = false;
	BoundingBox box_;
	std::function<void(Points&)> contain_provider;
	bool block_ready = false;
	std::vector<cgal_disk_arrangements::Point> block_hull;
	void ensureBlock();
	bool witnessInBlock(Point const& w, distance_t r) const;
	double deciderRoundingBound(Curve const& curve1, distance_t radius) const;

	Point min_translation;
	DiscreteFrechetQueries frechet;

	BoundingBox getBoundingBox(distance_t distance, Curve const& curve1, Curve const& curve2);

	friend void unit_tests::testN6Algorithm();
};
