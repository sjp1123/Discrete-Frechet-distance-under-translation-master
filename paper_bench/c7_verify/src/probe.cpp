// G3 probe: runs calcDistance2 (LMF, default constructor) on generated instances, one forked child
// per instance with a timeout; the instrumented build prints a "G3 ..." line per call when G3LOG is set.
//   probe hang  <S>                                   F04's 2x3 instance scaled by S
//   probe rand  <seed0> <count> <scale> <nmax>        uniform coords in [0,scale)
//   probe grid  <seed0> <count> <scale> <G>           integer grid [0,G]^2 times scale
//   probe sym   <seed0> <count> <scale> <nmax>        Q = mirror image of P (x -> -x), point order reversed
//   probe inst  "<n x y ... m x y ...>"               one explicit instance
// env G3_T = timeout seconds (default 20)
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

struct D2 { double x, y; };
static std::string dump(std::vector<D2> const& P, std::vector<D2> const& Q) {
	std::ostringstream o; o << std::setprecision(17) << P.size();
	for (auto p : P) o << " " << p.x << " " << p.y;
	o << " " << Q.size();
	for (auto q : Q) o << " " << q.x << " " << q.y;
	return o.str();
}
static long nhang = 0, ntmo = 0, nok = 0, nbad = 0;
static void runone(std::vector<D2> const& P, std::vector<D2> const& Q, std::string const& tag) {
	int T = getenv("G3_T") ? atoi(getenv("G3_T")) : 20;
	std::string d = dump(P, Q);
	std::cout.flush(); std::cerr.flush();
	pid_t pid = fork();
	if (pid == 0) {
		alarm(T);
		Curve c1, c2;
		for (auto p : P) c1.push_back({p.x, p.y});
		for (auto q : Q) c2.push_back({q.x, q.y});
		FrechetUnderTranslation f;
		double v = f.calcDistance2(c1, c2);
		printf("VAL %s value=%.17g\n", tag.c_str(), v);
		fflush(stdout); fflush(stderr);
		_exit(0);
	}
	int st = 0; waitpid(pid, &st, 0);
	if (WIFEXITED(st) && WEXITSTATUS(st) == 0) { ++nok; return; }
	if (WIFEXITED(st) && WEXITSTATUS(st) == 7) { ++nhang; printf("HANG(itercap) %s inst: %s\n", tag.c_str(), d.c_str()); }
	else if (WIFSIGNALED(st) && WTERMSIG(st) == SIGALRM) { ++ntmo; printf("TIMEOUT(%ds) %s inst: %s\n", T, tag.c_str(), d.c_str()); }
	else { ++nbad; printf("CHILD_BAD status=%d %s inst: %s\n", st, tag.c_str(), d.c_str()); }
	fflush(stdout);
}

int main(int argc, char** argv) {
	std::string mode = argv[1];
	if (mode == "hang") {
		double S = atof(argv[2]);
		std::vector<D2> P{{0, 0}, {S, 0.3*S}, {0.2*S, 0.9*S}}, Q{{0, 0}, {0.1*S, 0.5*S}};
		runone(P, Q, "hang S=" + std::string(argv[2]));
	}
	else if (mode == "circ") {
		// P = {(0,0)}, Q = {-c_j}: d(t) = max_j |c_j - t| (1-centre), c_j on the circle of radius D around 0:
		// angles k*360/K (k != the one at 315 deg), c_1 at 0 deg (start), c_m at 180 deg (end),
		// plus one tuned vertex at 315 deg + phi (radians).
		double D = atof(argv[2]), phi = atof(argv[3]); int K = argc > 4 ? atoi(argv[4]) : 16;
		const double PI = std::acos(-1.0);
		std::vector<D2> C;
		C.push_back({D, 0});
		for (int k = 1; k < K; ++k) {
			double deg = 360.0 * k / K;
			if (k * 2 == K) continue;                       // 180 goes last
			if (std::fabs(deg - 315.0) < 1e-9) continue;    // replaced by the tuned vertex
			C.push_back({D * std::cos(deg * PI / 180), D * std::sin(deg * PI / 180)});
		}
		double th = 315.0 * PI / 180 + phi;
		C.push_back({D * std::cos(th), D * std::sin(th)});
		C.push_back({-D, 0});
		std::vector<D2> P{{0, 0}}, Q;
		for (auto c : C) Q.push_back({-c.x, -c.y});
		std::ostringstream t; t << std::setprecision(17) << "circ D=" << D << " phi=" << phi << " K=" << K;
		runone(P, Q, t.str());
	}
	else if (mode == "circi") {
		// integer variant: 16-gon vertices truncated toward 0, tuned vertex (X, Y) given
		double D = atof(argv[2]), X = atof(argv[3]), Y = atof(argv[4]);
		const double PI = std::acos(-1.0);
		std::vector<D2> C;
		C.push_back({D, 0});
		for (int k = 1; k < 16; ++k) {
			if (k == 8 || k == 14) continue;
			double deg = 22.5 * k;
			C.push_back({std::trunc(D * std::cos(deg * PI / 180)), std::trunc(D * std::sin(deg * PI / 180))});
		}
		C.push_back({X, Y});
		C.push_back({-D, 0});
		std::vector<D2> P{{0, 0}}, Q;
		for (auto c : C) Q.push_back({c.x == 0 ? 0.0 : -c.x, c.y == 0 ? 0.0 : -c.y});
		std::ostringstream t; t << std::setprecision(17) << "circi D=" << D << " X=" << X << " Y=" << Y;
		runone(P, Q, t.str());
	}
	else if (mode == "inst") {
		std::istringstream is(argv[2]); int n, m; std::vector<D2> P, Q; is >> n;
		for (int i = 0; i < n; ++i) { double x, y; is >> x >> y; P.push_back({x, y}); }
		is >> m;
		for (int j = 0; j < m; ++j) { double x, y; is >> x >> y; Q.push_back({x, y}); }
		runone(P, Q, "inst");
	}
	else {
		long s0 = atol(argv[2]), cnt = atol(argv[3]); double scale = atof(argv[4]); int k = atoi(argv[5]);
		for (long s = s0; s < s0 + cnt; ++s) {
			std::mt19937_64 g(s * 7919 + 17);
			std::uniform_real_distribution<double> U(0, 1);
			auto Ui = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(g); };
			std::vector<D2> P, Q;
			if (mode == "rand") {
				int n = Ui(1, k), m = Ui(1, k); if (n == 1 && m == 1) m = 2;
				for (int i = 0; i < n; ++i) P.push_back({U(g)*scale, U(g)*scale});
				for (int j = 0; j < m; ++j) Q.push_back({U(g)*scale, U(g)*scale});
			} else if (mode == "grid") {
				int n = Ui(1, 6), m = Ui(1, 6); if (n == 1 && m == 1) m = 2;
				for (int i = 0; i < n; ++i) P.push_back({Ui(0, k)*scale, Ui(0, k)*scale});
				for (int j = 0; j < m; ++j) Q.push_back({Ui(0, k)*scale, Ui(0, k)*scale});
			} else if (mode == "sym") {
				int n = Ui(2, k);
				for (int i = 0; i < n; ++i) P.push_back({std::round(U(g)*8)*scale, std::round(U(g)*8)*scale});
				for (int i = n-1; i >= 0; --i) Q.push_back({-P[i].x, P[i].y});
			} else { std::cerr << "bad mode\n"; return 2; }
			std::ostringstream t; t << mode << " seed=" << s << " scale=" << scale;
			runone(P, Q, t.str());
		}
	}
	printf("DONE ok=%ld hang=%ld timeout=%ld bad=%ld\n", nok, nhang, ntmo, nbad);
}
