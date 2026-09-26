#pragma once

#include "defs.h"
#include "frechet_abstract.h"
#include "geometry_basics.h"
#include "curves.h"

class FrechetNaive final : public FrechetAbstract
{
public:
	FrechetNaive() {
		std::cout << "Initializing FrechetNaive algorithm...\n";
	}; // = default;

	bool lessThan(distance_t distance, Curve const& curve1, Curve const& curve2) override;
	bool lessThanWithFilters(distance_t distance, Curve const& curve1, Curve const& curve2) override;
	distance_t calcDistance(Curve const& curve1, Curve const& curve2) override;

	Certificate&  computeCertificate() { return cert; }

private:
	Certificate cert;
};
