#include "fut_n6_algorithm.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Accumulates Fréchet-decider loop time across all lessThan() calls and prints
// a single compact summary to stdout when the program exits.
namespace {
struct FrechetInstrumentation {
	long long loop_ns = 0;
	long long calls   = 0;

	~FrechetInstrumentation() {
		std::printf(
			"[frec-stats] calls=%lld loop_ms=%.1f\n",
			calls, loop_ns / 1e6);
	}
} g_frec_inst;

// C_q–ply per-query dump (NEXT §3.4): one row per decider evaluation with the
// candidate's ply (containment popcount), the query time, and whether it is an
// inclusion-maximal candidate. Active only when X3Q_DUMP is set.
// build_id lets R4 remove the N-confound with a build fixed-effects regression:
// deeper-ply candidates cluster in bigger subproblems whose queries are dearer,
// so C_q~ply^alpha must be estimated WITHIN a build, not pooled.
long long g_x3q_build = 0;
void x3q_row(long long build_id, unsigned ply, long long t_ns, int is_maximal) {
	static std::FILE* f = []() -> std::FILE* {
		const char* p = std::getenv("X3Q_DUMP");
		if (!p) return nullptr;
		std::FILE* g = std::fopen(p, "a");
		if (g) { std::fseek(g, 0, SEEK_END);
		         if (std::ftell(g) == 0) std::fprintf(g, "build_id,ply,t_query_ns,is_maximal\n"); }
		return g;
	}();
	if (f) std::fprintf(f, "%lld,%u,%lld,%d\n", build_id, ply, t_ns, is_maximal);
}
} // namespace

N6Alg::N6Alg()
	: N6Alg(1e-7)
{
}

N6Alg::N6Alg(distance_t const epsilon)
	: epsilon(epsilon), epsilon_sub(epsilon/10.), epsilon_slack(epsilon - epsilon_sub)
	, frechet(epsilon_sub)
{
}

distance_t N6Alg::calcDistance(Curve const& curve1, Curve const& curve2)
{
	assert(!curve1.empty() && !curve2.empty());

	// Use the first point-pair difference as the initial translation estimate.
	// Without a bounding box there is no geometric lower bound; the binary
	// search starts from 0.
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

	// Build direct candidates without a bounding box: all circle-circle
	// intersections and disk centers are used as candidate translations.
	//
	// The discs have radius `distance`, but every candidate below is tested with
	// `distance + epsilon_slack`, so epsilon_slack is handed to the traversal:
	// candidate4's maximal-region pipeline emits ONE witness per region and must
	// therefore enumerate the regions of the radius the decider will really use.
	// Passing it costs the arrangement path nothing — that path ignores it.
	MEASUREMENT::start(EXP::FUT_N6_ARR);
	ArrangementTraversal arr_traversal(discs, true, epsilon_slack);
	long long const x3q_build = ++g_x3q_build;   // one id per arrangement build
	std::cout << "candidate count = " << arr_traversal.getSize() << '\n';
	MEASUREMENT::stop(EXP::FUT_N6_ARR);

	if (!arr_traversal.hasNext()) {
		// No candidates: fall back to first disc center.
		auto t = curve1.front() - curve2.front();
		curve2.translate({t.x, t.y});
		MEASUREMENT::start(EXP::FUT_N6_FRECHET);
		auto less = frechet.lessThanFixedTranslation(distance+epsilon_slack, curve1, curve2);
		MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
		curve2.resetTranslation();
		if (less) min_translation = {t.x, t.y};
		return less;
	}

	MEASUREMENT::start(EXP::FUT_N6_FRECHET);
	{
		using clk = std::chrono::steady_clock;
		using ns  = std::chrono::nanoseconds;
		auto t_loop0 = clk::now();

		while (arr_traversal.hasNext()) {
			auto t = arr_traversal.getNext();
			curve2.translate({t.x, t.y});
			auto q0 = clk::now();
			auto less = frechet.lessThanFixedTranslation(distance+epsilon_slack, curve1, curve2);
			x3q_row(x3q_build, arr_traversal.lastPly(),
			        std::chrono::duration_cast<ns>(clk::now() - q0).count(),
			        arr_traversal.lastIsMaximal() ? 1 : 0);
			curve2.resetTranslation();
			if (less) {
				g_frec_inst.loop_ns += std::chrono::duration_cast<ns>(clk::now() - t_loop0).count();
				++g_frec_inst.calls;
				MEASUREMENT::stop(EXP::FUT_N6_FRECHET);
				min_translation = {t.x, t.y};
				return true;
			}
		}

		g_frec_inst.loop_ns += std::chrono::duration_cast<ns>(clk::now() - t_loop0).count();
		++g_frec_inst.calls;
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
