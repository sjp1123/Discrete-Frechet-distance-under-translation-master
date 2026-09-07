#pragma once

#include "defs.h"
#include "discrete_frechet_light_types.h"
#include "filter.h"
#include "frechet_abstract.h"
#include "geometry_basics.h"
#include "id.h"
#include "curves.h"

#include <array>
#include <vector>

namespace unit_tests { void testDiscreteFrechetLight(); }

class DiscreteFrechetLight final : public FrechetAbstract
{
	using CurvePair = std::array<Curve const*, 2>;

public:
	distance_t const epsilon;
	
	DiscreteFrechetLight() : DiscreteFrechetLight(1e-8) {}
	DiscreteFrechetLight(distance_t const epsilon) : epsilon(epsilon) {}

	bool lessThan(distance_t distance, Curve const& curve1, Curve const& curve2) override;
	bool lessThanWithFilters(distance_t distance, Curve const& curve1, Curve const& curve2) override;
	distance_t calcDistance(Curve const& curve1, Curve const& curve2) override;
	distance_t calcDistance(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max);
	distance_t calcDistance(Curve const& curve1, Curve const& curve2, distance_t min, distance_t max, distance_t const epsilon);
	void clear();

	CurvePair getCurvePair() const;
	Certificate& computeCertificate();

	void setPruningLevel(int pruning_level);
	void setRules(std::array<bool,5> const& enable) override;

	std::size_t getNumberOfBoxes() const;

	std::size_t non_filtered = 0;

private:
	CurvePair curve_pair;

	distance_t distance;
	distance_t dist_sqr;

	std::vector<PIntervals> reachable_intervals_vec;
	DiscreteQSimpleIntervals qsimple_intervals;
	std::size_t num_boxes = 0;

	// 0 = no pruning ... 6 = full pruning
	int pruning_level = 6;
	// ... and additionally bools to enable/disable rules
	bool enable_box_shrinking = true;
	bool enable_empty_outputs = true;
	bool enable_propagation1 = true;
	bool enable_propagation2 = true;
	bool enable_boundary_rule = true;

	Certificate cert;

	PInterval getInterval(Point const& point, Curve const& curve, PointID i) const;
	void merge(PIntervals& v, PInterval const& i) const;

	DiscreteOutputs createFinalOutputs();
	DiscreteInputs computeInitialInputs();

	bool isClose(Point const& point, Curve const& curve) const;
	PointID getLastReachablePoint(Point const& point, Curve const& curve) const;
	bool isTopRightReachable(DiscreteOutputs const& outputs) const;
	void computeOutputs(DiscreteBox const& initial_box, DiscreteInputs const& initial_inputs, DiscreteOutputs& final_outputs);

	void getReachableIntervals(DiscreteBoxData& data);

	// subfunctions of getReachableIntervals
	bool emptyInputsRule(DiscreteBoxData& data);
	void boxShrinkingRule(DiscreteBoxData& data);
	void handleCellCase(DiscreteBoxData& data);
	void getQSimpleIntervals(DiscreteBoxData& data);
	void calculateQSimple1(DiscreteBoxData& data);
	void calculateQSimple2(DiscreteBoxData& data);
	bool boundaryPruningRule(DiscreteBoxData& data);
	void splitAndRecurse(DiscreteBoxData& data);

	// intervals used in getReachableIntervals and subfunctions
	PInterval const empty;
	PInterval const* firstinterval1;
	PInterval const* firstinterval2;
	DiscreteQSimpleInterval qsimple1, qsimple2;
	PInterval out1, out2;
	// TODO: can those be made members of out1, out2?
	bool out1_valid = false, out2_valid = false;

	PointID first_reachable1, first_reachable2;

	// qsimple interval calculation functions
	DiscreteQSimpleInterval getFreshQSimpleInterval(const Point& fixed_point, PointID min1, PointID max1, const Curve& curve) const;
	bool updateQSimpleInterval(DiscreteQSimpleInterval& qsimple, const Point& fixed_point, PointID min1, PointID max1, const Curve& curve) const;
	void continueQSimpleSearch(DiscreteQSimpleInterval& qsimple, const Point& fixed_point, PointID min1, PointID max1, const Curve& curve) const;
};
