#include "discrete_frechet_naive.h"

#include <vector>
#include <limits>

bool DiscreteFrechetNaive::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	distance_t dist_sqr = distance * distance;

	if (curve1[0].dist_sqr(curve2[0]) > dist_sqr || curve1.back().dist_sqr(curve2.back()) > dist_sqr) { return false; }

	std::vector<bool> last_row(curve1.size(), false);

	for (size_t i = 0; i < curve1.size(); ++i) {
		if (curve2[0].dist_sqr(curve1[i]) > dist_sqr) { break; }
		last_row[i] = true;
	}

	std::vector<bool> new_row(curve1.size(), false);
	for (size_t j = 1; j < curve2.size(); ++j) {
		bool anything_reachable = false;
		for (size_t i = 0; i < curve1.size(); ++i) {
			if (curve2[j].dist_sqr(curve1[i]) > dist_sqr) {
				new_row[i] = false;
			}
			else if (i == 0) {
				new_row[i] = last_row[0];
			}
			else {
				new_row[i] = (last_row[i-1] || last_row[i] || new_row[i-1]);
			}

			anything_reachable = (anything_reachable || new_row[i]);
		}

		if (!anything_reachable) { return false; }

		last_row.swap(new_row);
	}

	return last_row.back();
}

bool DiscreteFrechetNaive::lessThanWithFilters(distance_t distance, Curve const& curve1, Curve const& curve2)
{
	assert(!curve1.empty());
	assert(!curve2.empty());

	distance_t dist_sqr = distance * distance;
	if (curve1[0].dist_sqr(curve2[0]) > dist_sqr ||
		curve1.back().dist_sqr(curve2.back()) > dist_sqr) {
		return false;
	}
	if (curve1.size() == 1 && curve2.size() == 1) {
		return true;
	}

	// TODO: add filters for the discrete Fréchet distance

	return lessThan(distance, curve1, curve2);
}

distance_t DiscreteFrechetNaive::calcDistance(Curve const& curve1, Curve const& curve2)
{
	static constexpr distance_t epsilon = 1e-10;

	distance_t min = 0.;
	distance_t max = curve1.getUpperBoundDistance(curve2);

	while (max - min >= epsilon) {
		auto split = (max + min)/2.;
		if (lessThanWithFilters(split, curve1, curve2)) {
			max = split;
		}
		else {
			min = split;
		}
	}

	return (max + min)/2.;
}
