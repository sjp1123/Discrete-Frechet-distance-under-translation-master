// Box-family fallback on degenerate boxes (report item E).  24 overlapping discs
// form one component above MAXREGION_DP_LIMIT = 20, so the box family hands them
// to the box-arrangement fallback.  candidate6 inserted zero-length edges for a
// zero-width box and CGAL crashed (SIGSEGV, audit item E).  Expected: every case
// prints a candidate count, the point box exactly 1 (itself), and exit 0.
#include "disc_arrangement_traversal.h"
#include <cstdio>
using namespace cgal_disk_arrangements;
int main() {
	Discs d;
	for (int i = 0; i < 24; ++i) d.push_back({{0.01 * i, 0.003 * (i % 5)}, 1.0});
	BoundingBox const boxes[] = {
		{{0.1, 0.0}, {0.1 + 1e-3, 0.05}},   // ordinary
		{{0.1, 0.0}, {0.1, 0.05}},          // zero width
		{{0.1, 0.02}, {0.2, 0.02}},         // zero height
		{{0.1, 0.02}, {0.1, 0.02}},         // a point
	};
	char const* name[] = {"ordinary", "zero width", "zero height", "point"};
	for (int b = 0; b < 4; ++b) {
		ArrangementTraversal t(d, boxes[b], 9e-9);
		std::printf("%-12s candidates=%zu overflow=%d\n", name[b], t.getSize(), (int)t.hadOverflow());
	}
	std::printf("DONE\n");
}
