// _kernel_probe.cpp — X2.0 feasibility gate (experiment_design_X1_X2.md §3.0)
// Does Arr_circle_segment_traits_2 compile/run under the INEXACT-constructions
// kernel (Epick)?  If not, the Epick main path is dead and X2 falls back to F1/F2.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Arr_circle_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Arr_circle_segment_traits_2<K>                Traits;
typedef CGAL::Arrangement_2<Traits>                         Arr;
int main() { Arr a; return static_cast<int>(a.number_of_faces()); }
