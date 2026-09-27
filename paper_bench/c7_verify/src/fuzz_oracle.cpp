// E2-oracle: independent exact oracle for the discrete Frechet distance under
// translation, and an end-to-end fuzzer of FrechetUnderTranslation::lessThan /
// calcDistance2 against it.
//
// Oracle.  c_ij = p_i - q_j (exact rationals).  For a point t let
//   F(t) = min over monotone couplings W of max_{(i,j) in W} |t - c_ij|^2
// (bottleneck DP).  delta*^2 = min_t F(t).  The optimum coupling W* has an MEC
// whose centre t* is a centre c, a midpoint of two centres or the circumcentre of
// three centres, and F(t*) <= MEC^2(W*) = delta*^2 <= F(t*), so
//   delta*^2 = min over that finite candidate set of F(t)   (exact, in mpq).
// A double pre-pass picks the candidates to evaluate exactly: F(any t) >= delta*^2,
// so min_double >= delta*^2 (1 - 1e-14); the true optimum candidate, computed in
// double, has F_double <= delta*^2 (1 + 1e-12) unless its circumcentre is
// ill-conditioned; we keep every candidate with F_double <= min_double (1 + 1e-6)
// and every ill-conditioned triple, and evaluate those exactly.
// Cross-check (mode "check"): coupling enumeration + exact MEC by brute force.
//
// usage: fuzz_oracle <gen> <seed0> <count> <depth> <cut> [verbose]
//   depth = 0 -> default constructor; else FrechetUnderTranslation(1e-7, depth, cut)
//   gen in: uniform grid dup cluster single offset scale swap check
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <gmpxx.h>
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <csignal>
#include <unistd.h>

struct D2 { double x, y; };
struct Inst { std::vector<D2> P, Q; };

static char g_cur[512] = "none";
static void on_signal(int sig) {
	char buf[700];
	int n = snprintf(buf, sizeof buf, "\nCRASH signal=%d at %s\n", sig, g_cur);
	if (n > 0) { ssize_t w = write(2, buf, n); (void)w; w = write(1, buf, n); (void)w; }
	_exit(3);
}

// ---------------- oracle ----------------
struct QP { mpq_class x, y; };

static mpq_class F_exact(std::vector<QP> const& C, int n, int m, mpq_class const& tx, mpq_class const& ty) {
	std::vector<mpq_class> B((size_t)n * m);
	for (int i = 0; i < n; ++i) for (int j = 0; j < m; ++j) {
		QP const& c = C[(size_t)i*m + j];
		mpq_class dx = tx - c.x, dy = ty - c.y;
		mpq_class d = dx*dx + dy*dy;
		if (i == 0 && j == 0) { B[0] = d; continue; }
		mpq_class const* best = nullptr;
		if (i > 0) best = &B[(size_t)(i-1)*m + j];
		if (j > 0 && (!best || B[(size_t)i*m + j-1] < *best)) best = &B[(size_t)i*m + j-1];
		if (i > 0 && j > 0 && B[(size_t)(i-1)*m + j-1] < *best) best = &B[(size_t)(i-1)*m + j-1];
		B[(size_t)i*m + j] = (d > *best) ? d : *best;
	}
	return B.back();
}
static double F_double(std::vector<D2> const& C, int n, int m, double tx, double ty, std::vector<double>& B) {
	B.resize((size_t)n*m);
	for (int i = 0; i < n; ++i) for (int j = 0; j < m; ++j) {
		D2 const& c = C[(size_t)i*m + j];
		double dx = tx - c.x, dy = ty - c.y, d = dx*dx + dy*dy;
		if (i == 0 && j == 0) { B[0] = d; continue; }
		double best = INFINITY;
		if (i > 0) best = std::min(best, B[(size_t)(i-1)*m + j]);
		if (j > 0) best = std::min(best, B[(size_t)i*m + j-1]);
		if (i > 0 && j > 0) best = std::min(best, B[(size_t)(i-1)*m + j-1]);
		B[(size_t)i*m + j] = std::max(d, best);
	}
	return B.back();
}

struct OracleStats { long exact_evals = 0, illcond = 0; };
static OracleStats OS;

// returns delta*^2 exactly
static mpq_class oracle(Inst const& I) {
	int n = I.P.size(), m = I.Q.size();
	std::vector<QP> C((size_t)n*m); std::vector<D2> Cd((size_t)n*m);
	for (int i = 0; i < n; ++i) for (int j = 0; j < m; ++j) {
		C[(size_t)i*m+j] = { mpq_class(I.P[i].x) - mpq_class(I.Q[j].x), mpq_class(I.P[i].y) - mpq_class(I.Q[j].y) };
		Cd[(size_t)i*m+j] = { I.P[i].x - I.Q[j].x, I.P[i].y - I.Q[j].y };
	}
	// distinct centres (exact)
	std::vector<int> U;
	for (int k = 0; k < n*m; ++k) {
		bool dup = false;
		for (int u : U) if (C[u].x == C[k].x && C[u].y == C[k].y) { dup = true; break; }
		if (!dup) U.push_back(k);
	}
	struct Cand { int kind, a, b, c; double tx, ty, f; bool ill; };
	std::vector<Cand> cands; std::vector<double> B;
	double mind = INFINITY;
	auto add = [&](Cand c) {
		c.f = F_double(Cd, n, m, c.tx, c.ty, B);
		if (c.f < mind) mind = c.f;
		cands.push_back(c);
	};
	int K = U.size();
	for (int a = 0; a < K; ++a) add({0, U[a], -1, -1, Cd[U[a]].x, Cd[U[a]].y, 0, false});
	for (int a = 0; a < K; ++a) for (int b = a+1; b < K; ++b) {
		D2 A = Cd[U[a]], Bp = Cd[U[b]];
		add({1, U[a], U[b], -1, (A.x+Bp.x)/2, (A.y+Bp.y)/2, 0, false});
	}
	for (int a = 0; a < K; ++a) for (int b = a+1; b < K; ++b) for (int c = b+1; c < K; ++c) {
		D2 A = Cd[U[a]], Bp = Cd[U[b]], Cc = Cd[U[c]];
		double bx = Bp.x - A.x, by = Bp.y - A.y, cx = Cc.x - A.x, cy = Cc.y - A.y;
		double den = 2*(bx*cy - by*cx);
		double lb = std::sqrt(bx*bx+by*by), lc = std::sqrt(cx*cx+cy*cy);
		bool ill = std::fabs(den) < 1e-5 * lb * lc;
		if (ill) {
			// exact collinearity test
			QP const &EA = C[U[a]], &EB = C[U[b]], &EC = C[U[c]];
			mpq_class ed = (EB.x-EA.x)*(EC.y-EA.y) - (EB.y-EA.y)*(EC.x-EA.x);
			if (ed == 0) continue;
			++OS.illcond;
			cands.push_back({2, U[a], U[b], U[c], 0, 0, INFINITY, true});
			continue;
		}
		double b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
		add({2, U[a], U[b], U[c], A.x + (cy*b2 - by*c2)/den, A.y + (bx*c2 - cx*b2)/den, 0, false});
	}
	bool have = false; mpq_class best;
	double thr = mind * (1 + 1e-6);
	for (auto const& c : cands) {
		if (!c.ill && !(c.f <= thr)) continue;
		mpq_class tx, ty;
		if (c.kind == 0) { tx = C[c.a].x; ty = C[c.a].y; }
		else if (c.kind == 1) { tx = (C[c.a].x + C[c.b].x)/2; ty = (C[c.a].y + C[c.b].y)/2; }
		else {
			QP const &EA = C[c.a], &EB = C[c.b], &EC = C[c.c];
			mpq_class bx = EB.x-EA.x, by = EB.y-EA.y, cx = EC.x-EA.x, cy = EC.y-EA.y;
			mpq_class den = 2*(bx*cy - by*cx);
			mpq_class b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
			tx = EA.x + (cy*b2 - by*c2)/den; ty = EA.y + (bx*c2 - cx*b2)/den;
		}
		++OS.exact_evals;
		mpq_class f = F_exact(C, n, m, tx, ty);
		if (!have || f < best) { best = f; have = true; }
	}
	return best;
}

// Independent cross-check: enumerate couplings, exact MEC by brute force.
static mpq_class mec2_exact(std::vector<QP> const& S) {
	// distinct
	std::vector<QP> P;
	for (auto const& s : S) { bool d = false; for (auto const& p : P) if (p.x == s.x && p.y == s.y) { d = true; break; } if (!d) P.push_back(s); }
	if (P.size() == 1) return 0;
	bool have = false; mpq_class best;
	auto tryc = [&](mpq_class const& tx, mpq_class const& ty, mpq_class const& r2) {
		if (have && r2 >= best) return;
		for (auto const& p : P) { mpq_class dx = p.x - tx, dy = p.y - ty; if (dx*dx + dy*dy > r2) return; }
		best = r2; have = true;
	};
	size_t K = P.size();
	for (size_t a = 0; a < K; ++a) for (size_t b = a+1; b < K; ++b) {
		mpq_class tx = (P[a].x+P[b].x)/2, ty = (P[a].y+P[b].y)/2; mpq_class dx = P[a].x-tx, dy = P[a].y-ty;
		tryc(tx, ty, dx*dx+dy*dy);
	}
	for (size_t a = 0; a < K; ++a) for (size_t b = a+1; b < K; ++b) for (size_t c = b+1; c < K; ++c) {
		mpq_class bx = P[b].x-P[a].x, by = P[b].y-P[a].y, cx = P[c].x-P[a].x, cy = P[c].y-P[a].y;
		mpq_class den = 2*(bx*cy - by*cx); if (den == 0) continue;
		mpq_class b2 = bx*bx+by*by, c2 = cx*cx+cy*cy;
		mpq_class ux = (cy*b2 - by*c2)/den, uy = (bx*c2 - cx*b2)/den;
		tryc(P[a].x + ux, P[a].y + uy, ux*ux+uy*uy);
	}
	return best;
}
static mpq_class oracle_enum(Inst const& I) {
	int n = I.P.size(), m = I.Q.size();
	std::vector<QP> C((size_t)n*m);
	for (int i = 0; i < n; ++i) for (int j = 0; j < m; ++j)
		C[(size_t)i*m+j] = { mpq_class(I.P[i].x) - mpq_class(I.Q[j].x), mpq_class(I.P[i].y) - mpq_class(I.Q[j].y) };
	bool have = false; mpq_class best; std::vector<QP> path;
	std::function<void(int,int)> rec = [&](int i, int j) {
		path.push_back(C[(size_t)i*m+j]);
		if (i == n-1 && j == m-1) { mpq_class r = mec2_exact(path); if (!have || r < best) { best = r; have = true; } }
		else {
			if (i+1 < n) rec(i+1, j);
			if (j+1 < m) rec(i, j+1);
			if (i+1 < n && j+1 < m) rec(i+1, j+1);
		}
		path.pop_back();
	};
	rec(0, 0);
	return best;
}

// ---------------- generators ----------------
static std::string fmt(double v) { std::ostringstream o; o << std::setprecision(17) << v; return o.str(); }
static std::string dump(Inst const& I) {
	std::ostringstream o; o << std::setprecision(17);
	o << "P(" << I.P.size() << "):"; for (auto p : I.P) o << " (" << p.x << "," << p.y << ")";
	o << "  Q(" << I.Q.size() << "):"; for (auto p : I.Q) o << " (" << p.x << "," << p.y << ")";
	return o.str();
}

using RNG = std::mt19937_64;
static double U01(RNG& g) { return std::uniform_real_distribution<double>(0, 1)(g); }
static int Ui(RNG& g, int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(g); }

static Inst gen_uniform(RNG& g) {
	Inst I; int n = Ui(g, 1, 8), m = Ui(g, 1, 8);
	if (n == 1 && m == 1) m = 2;
	double s = std::pow(10.0, Ui(g, 0, 1));
	for (int i = 0; i < n; ++i) I.P.push_back({U01(g)*s, U01(g)*s});
	for (int j = 0; j < m; ++j) I.Q.push_back({U01(g)*s, U01(g)*s});
	return I;
}
static Inst gen_grid(RNG& g) {
	Inst I; int n = Ui(g, 1, 7), m = Ui(g, 1, 7);
	if (n == 1 && m == 1) m = 2;
	int G = Ui(g, 0, 3) == 0 ? 2 : 4;
	for (int i = 0; i < n; ++i) I.P.push_back({(double)Ui(g, 0, G), (double)Ui(g, 0, G)});
	for (int j = 0; j < m; ++j) I.Q.push_back({(double)Ui(g, 0, G), (double)Ui(g, 0, G)});
	return I;
}
static std::vector<D2> dupcurve(RNG& g, int k, bool grid) {
	std::vector<D2> c;
	for (int i = 0; i < k; ++i) {
		D2 p = grid ? D2{(double)Ui(g,0,4), (double)Ui(g,0,4)} : D2{std::round(U01(g)*1e3)/1e2, std::round(U01(g)*1e3)/1e2};
		if (!c.empty() && Ui(g, 0, 4) == 0) p = c[Ui(g, 0, (int)c.size()-1)];  // revisit an earlier point
		int rep = Ui(g, 1, 3);
		for (int r = 0; r < rep; ++r) c.push_back(p);
	}
	return c;
}
static Inst gen_dup(RNG& g) {
	Inst I; bool grid = Ui(g, 0, 1);
	I.P = dupcurve(g, Ui(g, 1, 4), grid); I.Q = dupcurve(g, Ui(g, 1, 4), grid);
	if (I.P.size() == 1 && I.Q.size() == 1) I.Q.push_back({I.Q[0].x + 1, I.Q[0].y});
	while (I.P.size() > 9) I.P.pop_back();
	while (I.Q.size() > 9) I.Q.pop_back();
	return I;
}
static Inst gen_cluster(RNG& g) {
	Inst I; int K = Ui(g, 1, 3);
	std::vector<D2> cen(K);
	for (auto& c : cen) c = {U01(g)*10, U01(g)*10};
	double sp = std::pow(10.0, -Ui(g, 0, 4));
	auto mk = [&](int k) { std::vector<D2> v; int cl = Ui(g, 0, K-1);
		for (int i = 0; i < k; ++i) { if (Ui(g, 0, 2) == 0) cl = Ui(g, 0, K-1);
			int e = Ui(g, 0, 3); D2 p = cen[cl];
			if (e) { p.x += (U01(g)-0.5)*sp; p.y += (U01(g)-0.5)*sp; }
			v.push_back(p); }
		return v; };
	I.P = mk(Ui(g, 1, 8)); I.Q = mk(Ui(g, 2, 8));
	return I;
}
static Inst gen_single(RNG& g) {
	Inst I; I.P.push_back({0, 0});
	int m = Ui(g, 2, 12); bool grid = Ui(g, 0, 2) == 0;
	for (int j = 0; j < m; ++j) {
		D2 q = grid ? D2{(double)Ui(g,0,4), (double)Ui(g,0,4)} : D2{std::round(U01(g)*1e4)/1e4, std::round(U01(g)*1e4)/1e4};
		if (j && Ui(g, 0, 3) == 0) q = I.Q[Ui(g, 0, j-1)];
		I.Q.push_back(q);
	}
	if (Ui(g, 0, 1)) std::swap(I.P, I.Q);
	return I;
}
// high multiplicity: few distinct points, each repeated many times, so that
// >12 coincident cut discs keep the paper configuration (cut limit 12) splitting
// down to the depth limit 40.
static Inst gen_hdup(RNG& g) {
	Inst I;
	auto mk = [&](int k, bool grid) { std::vector<D2> c;
		for (int i = 0; i < k; ++i) {
			D2 p = grid ? D2{(double)Ui(g,0,4), (double)Ui(g,0,4)} : D2{U01(g)*10, U01(g)*10};
			if (!c.empty() && Ui(g, 0, 3) == 0) p = c[Ui(g, 0, (int)c.size()-1)];
			int rep = Ui(g, 3, 8);
			for (int r = 0; r < rep; ++r) c.push_back(p);
		}
		return c; };
	bool grid = Ui(g, 0, 1);
	int kp = Ui(g, 1, 3), kq = Ui(g, 1, 3);
	if (kp == 1 && kq == 1) kq = 2;
	I.P = mk(kp, grid); I.Q = mk(kq, grid);
	while (I.P.size() > 16) I.P.pop_back();
	while (I.Q.size() > 16) I.Q.pop_back();
	return I;
}
static Inst gen_base(RNG& g) {
	switch (Ui(g, 0, 4)) {
		case 0: case 1: return gen_uniform(g);
		case 2: return gen_grid(g);
		case 3: return gen_dup(g);
		default: return gen_cluster(g);
	}
}

// ---------------- main (v2: one forked child per instance, shared counters) ----------------
#include <sys/mman.h>
#include <sys/wait.h>
struct Shared { long ndec, nfn, nfp, nval, nvalerr, nexc; double maxvalerr; long maxseed; };
static Shared* SH;
static char g_dump[4096] = "";
static void on_alarm(int) {
	char buf[5000];
	int n = snprintf(buf, sizeof buf, "TIMEOUT at %s %s\n", g_cur, g_dump);
	if (n > 0) { ssize_t w = write(1, buf, std::min(n, (int)sizeof buf - 1)); (void)w; }
	_exit(4);
}
static void on_crash(int sig) {
	char buf[5000];
	int n = snprintf(buf, sizeof buf, "CRASH signal=%d at %s %s\n", sig, g_cur, g_dump);
	if (n > 0) { ssize_t w = write(1, buf, std::min(n, (int)sizeof buf - 1)); (void)w; }
	_exit(3);
}

int main(int argc, char** argv) {
	if (argc < 6) { std::cerr << "usage: gen seed0 count depth cut [verbose]\n"; return 2; }
	std::string gen = argv[1];
	long s0 = atol(argv[2]), cnt = atol(argv[3]);
	int depth = atoi(argv[4]), cut = atoi(argv[5]);
	bool verbose = argc > 6;
	int TD = getenv("E2_TDEC") ? atoi(getenv("E2_TDEC")) : 30, TV = getenv("E2_TVAL") ? atoi(getenv("E2_TVAL")) : 120;
	SH = (Shared*)mmap(nullptr, sizeof(Shared), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	memset(SH, 0, sizeof *SH);
	std::cout << std::setprecision(17);
	long ninst = 0, ncheck = 0, ncheckbad = 0, ntimeout = 0, ncrash = 0;
	std::hash<std::string> H;
	for (long s = s0; s < s0 + cnt; ++s) {
		RNG g(H(gen) ^ (uint64_t)(s * 0x9E3779B97F4A7C15ULL));
		Inst base, I; double scale = 1.0; std::string note;
		if (gen == "uniform" || gen == "check") base = gen_uniform(g);
		else if (gen == "grid") base = gen_grid(g);
		else if (gen == "dup") base = gen_dup(g);
		else if (gen == "cluster") base = gen_cluster(g);
		else if (gen == "single") base = gen_single(g);
		else if (gen == "hdup") base = gen_hdup(g);
		else if (gen == "revisit") { bool grid = Ui(g, 0, 1);
			auto mk = [&](int len) { int K = Ui(g, 2, 4); std::vector<D2> S; while ((int)S.size() < K) { D2 p = grid ? D2{(double)Ui(g,0,4), (double)Ui(g,0,4)} : D2{U01(g)*10, U01(g)*10}; bool d = false; for (auto& q : S) d = d || (q.x == p.x && q.y == p.y); if (!d) S.push_back(p); }
				std::vector<D2> c; while ((int)c.size() < len) { D2 p = S[Ui(g, 0, K-1)]; if (!c.empty() && c.back().x == p.x && c.back().y == p.y) continue; c.push_back(p); } return c; };
			base.P = mk(Ui(g, 2, 14)); base.Q = mk(Ui(g, 2, 14)); }
		else if (gen == "amp") { Inst b0; int w = Ui(g, 0, 2); b0 = w == 0 ? gen_dup(g) : w == 1 ? gen_grid(g) : gen_uniform(g); int r = Ui(g, 2, 3);
			for (auto p : b0.P) for (int t = 0; t < r; ++t) base.P.push_back(p);
			for (auto q : b0.Q) for (int t = 0; t < r; ++t) base.Q.push_back(q);
			while (base.P.size() > 18) base.P.pop_back(); while (base.Q.size() > 18) base.Q.pop_back(); }
		else if (gen == "file") {
			// E2_FILE: one instance per line "n x1 y1 ... xn yn m x1 y1 ... xm ym"; seed = line number (1-based)
			static std::vector<std::string> lines = []{ std::vector<std::string> L; const char* f = getenv("E2_FILE");
				FILE* fp = f ? fopen(f, "r") : nullptr; static char buf[1 << 16];
				while (fp && fgets(buf, sizeof buf, fp)) if (buf[0] != '#' && buf[0] != '\n') L.push_back(buf);
				if (fp) fclose(fp); return L; }();
			if (s < 1 || s > (long)lines.size()) break;
			std::istringstream is(lines[s-1]); int n, m; is >> n;
			for (int i = 0; i < n; ++i) { std::string a, b; is >> a >> b; base.P.push_back({strtod(a.c_str(), nullptr), strtod(b.c_str(), nullptr)}); }
			is >> m;
			for (int j = 0; j < m; ++j) { std::string a, b; is >> a >> b; base.Q.push_back({strtod(a.c_str(), nullptr), strtod(b.c_str(), nullptr)}); }
		}
		else base = gen_base(g);
		if (gen == "check") { RNG g2(H(std::string("check")) ^ (uint64_t)(s * 0x9E3779B97F4A7C15ULL)); base = gen_base(g2); }
		I = base;
		if (gen == "offset") {
			D2 oP{1.3e7 + U01(g)*1e6, 1.3e7 + U01(g)*1e6}, oQ{1.3e7 + U01(g)*1e6, 1.3e7 + U01(g)*1e6};
			if (Ui(g, 0, 2) == 0) oQ = {oP.x + (double)Ui(g, -3, 3), oP.y + (double)Ui(g, -3, 3)};  // similar offsets
			for (auto& p : I.P) { p.x += oP.x; p.y += oP.y; }
			for (auto& p : I.Q) { p.x += oQ.x; p.y += oQ.y; }
			note = "offP=(" + fmt(oP.x) + "," + fmt(oP.y) + ") offQ=(" + fmt(oQ.x) + "," + fmt(oQ.y) + ")";
		}
		else if (gen == "scale") {
			int k = Ui(g, -12, 12); scale = std::ldexp(1.0, k);
			for (auto& p : I.P) { p.x *= scale; p.y *= scale; }
			for (auto& p : I.Q) { p.x *= scale; p.y *= scale; }
			note = "scale=2^" + std::to_string(k);
		}
		else if (gen == "swap") { std::swap(I.P, I.Q); note = "swapped"; }
		// offset: the oracle runs on the shifted doubles themselves (exact), not on base
		Inst const& OI = (gen == "offset") ? I : base;
		mpq_class d2 = oracle(OI);
		if (gen == "check") {
			if (base.P.size() * base.Q.size() <= 20) {
				++ncheck;
				mpq_class e2 = oracle_enum(base);
				if (e2 != d2) { ++ncheckbad; std::cout << "ORACLE_MISMATCH seed=" << s << " cand=" << d2.get_d() << " enum=" << e2.get_d() << " " << dump(base) << "\n"; }
			}
			continue;
		}
		double dstar = std::sqrt(d2.get_d()) * ((gen == "offset") ? 1.0 : scale);
		++ninst;
		snprintf(g_dump, sizeof g_dump, "%s %s", note.c_str(), dump(I).c_str());
		std::cout.flush();
		pid_t pid = fork();
		if (pid == 0) {
			signal(SIGSEGV, on_crash); signal(SIGABRT, on_crash); signal(SIGFPE, on_crash); signal(SIGBUS, on_crash); signal(SIGALRM, on_alarm);
			Curve c1, c2;
			for (auto p : I.P) c1.push_back({p.x, p.y});
			for (auto p : I.Q) c2.push_back({p.x, p.y});
			auto make = [&]() { return std::unique_ptr<FrechetUnderTranslation>(depth ? new FrechetUnderTranslation(1e-7, depth, cut) : new FrechetUnderTranslation()); };
			std::vector<double> US{1e-6, 1e-4, 1e-2, 0.3};
			if (getenv("E2_US")) { US.clear(); std::string uss = getenv("E2_US"); for (auto& ch : uss) if (ch == ',') ch = ' '; std::istringstream us(uss); double x; while (us >> x) US.push_back(x); }
			for (double u : US) {
				if (u * dstar < (getenv("E2_MINABS") ? atof(getenv("E2_MINABS")) : 1e-6)) continue;
				for (int side = 0; side < 2; ++side) {
					if (getenv("E2_SIDES") && ((side == 0 && !strcmp(getenv("E2_SIDES"), "no")) || (side == 1 && !strcmp(getenv("E2_SIDES"), "yes")))) continue;
					double delta = side ? dstar * (1 - u) : dstar * (1 + u);
					bool truth = !side;
					snprintf(g_cur, sizeof g_cur, "gen=%s seed=%ld phase=decider delta=%.17g dstar=%.17g u=%g", gen.c_str(), s, delta, dstar, side ? -u : u);
					alarm(TD);
					bool ans;
					try { auto f = make(); ans = f->lessThan(delta, c1, c2); }
					catch (std::exception const& e) { alarm(0); __sync_fetch_and_add(&SH->nexc, 1); std::cout << "EXC decider seed=" << s << " " << e.what() << " " << g_dump << "\n"; continue; }
					alarm(0);
					__sync_fetch_and_add(&SH->ndec, 1);
					if (ans != truth) {
						__sync_fetch_and_add(truth ? &SH->nfn : &SH->nfp, 1);
						std::cout << (truth ? "FN" : "FP") << " gen=" << gen << " seed=" << s << " delta=" << delta << " dstar=" << dstar
						          << " u=" << (side ? -u : u) << " " << g_dump << "\n";
					}
					else if (verbose) std::cout << "ok dec seed=" << s << " u=" << (side ? -u : u) << " ans=" << ans << "\n";
				}
			}
			if (getenv("E2_NOVAL")) { std::cout.flush(); if (getenv("E2_STATS")) exit(0); _exit(0); }
			snprintf(g_cur, sizeof g_cur, "gen=%s seed=%ld phase=value dstar=%.17g", gen.c_str(), s, dstar);
			alarm(TV);
			double v;
			try { auto f = make(); v = f->calcDistance2(c1, c2);
				if (getenv("E2_TRANS")) { Point t = f->getTranslation(); int n = I.P.size(), m = I.Q.size(); std::vector<D2> Cd; for (auto p : I.P) for (auto q : I.Q) Cd.push_back({p.x - q.x, p.y - q.y}); std::vector<double> B;
					double ft = std::sqrt(F_double(Cd, n, m, t.x, t.y, B)); if (!(ft <= v + 1e-6 * std::max(1.0, v))) std::cout << "TRANS seed=" << s << " value=" << v << " F(getTranslation)=" << ft << " t=(" << t.x << "," << t.y << ") " << g_dump << "\n"; } }
			catch (std::exception const& e) { __sync_fetch_and_add(&SH->nexc, 1); std::cout << "EXC value seed=" << s << " " << e.what() << "\n"; v = NAN; }
			alarm(0);
			__sync_fetch_and_add(&SH->nval, 1);
			double err = v - dstar;
			double ae = std::isnan(err) ? INFINITY : std::fabs(err);
			if (ae > SH->maxvalerr) { SH->maxvalerr = ae; SH->maxseed = s; }
			if (!(ae <= 2e-7)) {
				__sync_fetch_and_add(&SH->nvalerr, 1);
				std::cout << "VAL gen=" << gen << " seed=" << s << " value=" << v << " dstar=" << dstar << " err=" << err
				          << " rel=" << err / dstar << " " << g_dump << "\n";
			}
			else if (verbose) std::cout << "ok val seed=" << s << " v=" << v << " dstar=" << dstar << "\n";
			std::cout.flush();
			if (getenv("E2_STATS")) exit(0); _exit(0);
		}
		int st = 0; waitpid(pid, &st, 0);
		if (WIFEXITED(st) && WEXITSTATUS(st) == 4) ++ntimeout;
		else if (!(WIFEXITED(st) && WEXITSTATUS(st) == 0)) { ++ncrash; std::cout << "CHILD_BAD status=" << st << " seed=" << s << " " << g_dump << "\n"; }
	}
	std::cout << "DONE gen=" << gen << " inst=" << ninst << " dec=" << SH->ndec << " FN=" << SH->nfn << " FP=" << SH->nfp
	          << " val=" << SH->nval << " valerr=" << SH->nvalerr << " maxvalerr=" << SH->maxvalerr << "@" << SH->maxseed
	          << " exc=" << SH->nexc << " timeout=" << ntimeout << " crash=" << ncrash
	          << " check=" << ncheck << " checkbad=" << ncheckbad << " oracle_exact_evals=" << OS.exact_evals << " illcond=" << OS.illcond << "\n";
}
