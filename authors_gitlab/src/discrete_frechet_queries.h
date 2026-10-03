#pragma once

#include "discrete_frechet_light.h"

class DiscreteFrechetQueries {

public:
	DiscreteFrechetQueries(distance_t epsilon) : epsilon(epsilon) { 	}

	bool lessThanFixedTranslation(distance_t distance, Curve const& curve1, Curve const& curve2) {
		MEASUREMENT::inc(EXP::BBCALLS_COUNTER);
		return frechet.lessThan(distance, curve1, curve2);
	}

	distance_t evaluationFixedTranslation(Curve const& curve1, Curve const& curve2)
	{
		distance_t min = 0.;
		distance_t max = curve1.getUpperBoundDistance(curve2);
		return evaluationFixedTranslation(curve1, curve2, min, max);
	}

	distance_t evaluationFixedTranslation(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max)
	{
		return evaluationFixedTranslation(curve1, curve2, min, max, epsilon);
	}

	distance_t evaluationFixedTranslation(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max, distance_t const epsilon)
	{
		assert(!curve1.empty() && !curve2.empty());

		MEASUREMENT::inc(EXP::BBCALLS_COUNTER);
		if (frechet.lessThan(min, curve1, curve2)) {
			return min;
		}
		MEASUREMENT::inc(EXP::BBCALLS_COUNTER);
		if (!frechet.lessThan(max, curve1, curve2)) {
			return max;
		}

		while (max - min >= epsilon/2.) {
			auto split = (max + min)/2.;
			MEASUREMENT::inc(EXP::BBCALLS_COUNTER);
			if (frechet.lessThan(split, curve1, curve2)) {
				max = split;
			}
			else {
				min = split;
			}
		}

		return min;
	}

private:
	distance_t const epsilon;

	DiscreteFrechetLight frechet;

};
