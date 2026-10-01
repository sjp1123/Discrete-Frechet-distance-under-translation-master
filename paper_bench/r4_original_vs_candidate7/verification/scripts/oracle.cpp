// Independent exact oracle for the discrete Frechet distance under translation (DFDuT).
// Written from the definition only (no code from the repository):
//   d(P,Q) = min_t DFD(P, Q + t),  DFD(P, Q+t) = min_coupling max_(i,j) |p_i - q_j - t|.
// For a fixed coupling C the inner minimum is the radius of the minimum enclosing circle (MEC) of
// {p_i - q_j : (i,j) in C}; the MEC centre is a point d_k, the midpoint of two points, or the
// circumcentre of three.  Hence  d(P,Q) = min over those candidate centres c of DFD(P, Q + c):
// every candidate gives an upper bound and the optimal coupling's MEC centre is a candidate.
// Screening in long double, then exact evaluation (GMP rationals) of every candidate within a
// generous tolerance of the screened minimum.  Output: exact squared distance as a rational.
//
// usage: oracle <list>   list lines: <id> <curveA> <curveB>
// output: <id> <n> <m> <ncand> <nexact> <opt (double)> <opt^2 numerator> <opt^2 denominator>
#include <gmpxx.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

typedef long double LD;
struct PtD { double x, y; };
struct PtL { LD x, y; };
struct PtQ { mpq_class x, y; };

static std::vector<PtD> read_curve(const std::string& path) {
	std::ifstream in(path);
	std::vector<PtD> c;
	double x, y;
	std::string line;
	while (std::getline(in, line)) {
		std::istringstream ls(line);
		if (ls >> x >> y) c.push_back({x, y});
	}
	return c;
}

// squared DFD of D (n x m matrix of points d_ij) shifted by c, long double
static LD dfd2_ld(const std::vector<PtL>& D, int n, int m, LD cx, LD cy, std::vector<LD>& buf) {
	buf.assign((size_t)n * m, 0);
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < m; ++j) {
			LD dx = D[i * m + j].x - cx, dy = D[i * m + j].y - cy;
			LD c = dx * dx + dy * dy, prev;
			if (i == 0 && j == 0) prev = 0;
			else if (i == 0) prev = buf[j - 1];
			else if (j == 0) prev = buf[(i - 1) * m];
			else prev = std::min(std::min(buf[(i - 1) * m + j], buf[i * m + j - 1]), buf[(i - 1) * m + j - 1]);
			buf[i * m + j] = std::max(c, prev);
		}
	return buf[(size_t)n * m - 1];
}

static mpq_class dfd2_q(const std::vector<PtQ>& D, int n, int m, const mpq_class& cx, const mpq_class& cy) {
	std::vector<mpq_class> buf((size_t)n * m);
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < m; ++j) {
			mpq_class dx = D[i * m + j].x - cx, dy = D[i * m + j].y - cy;
			mpq_class c = dx * dx + dy * dy;
			if (i == 0 && j == 0) { buf[0] = c; continue; }
			const mpq_class* prev;
			if (i == 0) prev = &buf[j - 1];
			else if (j == 0) prev = &buf[(i - 1) * m];
			else {
				prev = &buf[(i - 1) * m + j];
				if (buf[i * m + j - 1] < *prev) prev = &buf[i * m + j - 1];
				if (buf[(i - 1) * m + j - 1] < *prev) prev = &buf[(i - 1) * m + j - 1];
			}
			buf[i * m + j] = (c > *prev) ? c : *prev;
		}
	return buf[(size_t)n * m - 1];
}

struct Cand { LD v; int k, l, r; };   // l = r = -1: point; r = -1: midpoint; else circumcentre

static bool centre_ld(const std::vector<PtL>& P, const Cand& c, LD& x, LD& y) {
	if (c.l < 0) { x = P[c.k].x; y = P[c.k].y; return true; }
	if (c.r < 0) { x = (P[c.k].x + P[c.l].x) / 2; y = (P[c.k].y + P[c.l].y) / 2; return true; }
	const PtL &a = P[c.k], &b = P[c.l], &d = P[c.r];
	LD den = 2 * (a.x * (b.y - d.y) + b.x * (d.y - a.y) + d.x * (a.y - b.y));
	if (den == 0) return false;
	LD a2 = a.x * a.x + a.y * a.y, b2 = b.x * b.x + b.y * b.y, d2 = d.x * d.x + d.y * d.y;
	x = (a2 * (b.y - d.y) + b2 * (d.y - a.y) + d2 * (a.y - b.y)) / den;
	y = (a2 * (d.x - b.x) + b2 * (a.x - d.x) + d2 * (b.x - a.x)) / den;
	return std::isfinite((double)x) && std::isfinite((double)y);
}

static bool centre_q(const std::vector<PtQ>& P, const Cand& c, mpq_class& x, mpq_class& y) {
	if (c.l < 0) { x = P[c.k].x; y = P[c.k].y; return true; }
	if (c.r < 0) { x = (P[c.k].x + P[c.l].x) / 2; y = (P[c.k].y + P[c.l].y) / 2; return true; }
	const PtQ &a = P[c.k], &b = P[c.l], &d = P[c.r];
	mpq_class den = 2 * (a.x * (b.y - d.y) + b.x * (d.y - a.y) + d.x * (a.y - b.y));
	if (den == 0) return false;
	mpq_class a2 = a.x * a.x + a.y * a.y, b2 = b.x * b.x + b.y * b.y, d2 = d.x * d.x + d.y * d.y;
	x = (a2 * (b.y - d.y) + b2 * (d.y - a.y) + d2 * (a.y - b.y)) / den;
	y = (a2 * (d.x - b.x) + b2 * (a.x - d.x) + d2 * (b.x - a.x)) / den;
	return true;
}

int main(int argc, char** argv) {
	if (argc != 2) { std::cerr << "usage: oracle <list>\n"; return 1; }
	std::ifstream list(argv[1]);
	std::string id, fa, fb;
	std::vector<LD> buf;
	while (list >> id >> fa >> fb) {
		auto A = read_curve(fa), B = read_curve(fb);
		int n = A.size(), m = B.size();
		if (n == 0 || m == 0) { std::cout << id << " EMPTY\n"; continue; }
		int N = n * m;
		std::vector<PtL> D(N);
		std::vector<PtQ> DQ(N);
		LD scale = 0;
		for (int i = 0; i < n; ++i)
			for (int j = 0; j < m; ++j) {
				DQ[i * m + j].x = mpq_class(A[i].x) - mpq_class(B[j].x);
				DQ[i * m + j].y = mpq_class(A[i].y) - mpq_class(B[j].y);
				D[i * m + j].x = (LD)A[i].x - (LD)B[j].x;
				D[i * m + j].y = (LD)A[i].y - (LD)B[j].y;
				scale = std::max(scale, std::max(std::fabs(D[i * m + j].x), std::fabs(D[i * m + j].y)));
			}
		// centre the differences at d_00 (exact): DFD(P, Q + t) is invariant under t -> t + d_00, so the
		// optimum is unchanged, and the long double screening works on the spread, not on the offset
		{
			PtQ o = DQ[0];
			scale = 0;
			for (int k = 0; k < N; ++k) {
				DQ[k].x -= o.x; DQ[k].y -= o.y;
				D[k].x = (LD)DQ[k].x.get_d(); D[k].y = (LD)DQ[k].y.get_d();
				scale = std::max(scale, std::max(std::fabs(D[k].x), std::fabs(D[k].y)));
			}
		}
		// distinct difference points (duplicates add nothing to the candidate set)
		std::vector<int> U;
		for (int k = 0; k < N; ++k) {
			bool dup = false;
			for (int u : U) if (DQ[u].x == DQ[k].x && DQ[u].y == DQ[k].y) { dup = true; break; }
			if (!dup) U.push_back(k);
		}
		std::vector<PtL> UP; std::vector<PtQ> UQ;
		for (int u : U) { UP.push_back(D[u]); UQ.push_back(DQ[u]); }
		int K = UP.size();
		std::vector<Cand> C;
		LD best = INFINITY, x, y;
		auto add = [&](int k, int l, int r) {
			Cand c{0, k, l, r};
			if (!centre_ld(UP, c, x, y)) return;
			c.v = dfd2_ld(D, n, m, x, y, buf);
			best = std::min(best, c.v);
			C.push_back(c);
		};
		for (int k = 0; k < K; ++k) add(k, -1, -1);
		for (int k = 0; k < K; ++k) for (int l = k + 1; l < K; ++l) add(k, l, -1);
		for (int k = 0; k < K; ++k) for (int l = k + 1; l < K; ++l) for (int r = l + 1; r < K; ++r) add(k, l, r);
		// exact pass over every candidate near the screened minimum
		LD tol = best * 1e-6L + scale * scale * 1e-12L + 1e-300L;   // scale = spread of the centred differences
		mpq_class opt2; bool have = false; int nexact = 0;
		for (auto& c : C) {
			if (c.v > best + tol) continue;
			mpq_class qx, qy;
			if (!centre_q(UQ, c, qx, qy)) continue;
			mpq_class v = dfd2_q(DQ, n, m, qx, qy);
			++nexact;
			if (!have || v < opt2) { opt2 = v; have = true; }
		}
		opt2.canonicalize();
		double opt = (double)std::sqrt((LD)opt2.get_d());
		// refine the double: closest double to sqrt(opt2) by exact comparison of neighbours
		{
			mpq_class o(opt);
			for (int it = 0; it < 4; ++it) {
				double up = std::nextafter(opt, INFINITY), dn = std::nextafter(opt, -INFINITY);
				mpq_class qu(up), qd(dn);
				mpq_class eo = o * o - opt2, eu = qu * qu - opt2, ed = qd * qd - opt2;
				if (abs(eu) < abs(eo)) { opt = up; o = qu; continue; }
				if (abs(ed) < abs(eo)) { opt = dn; o = qd; continue; }
				break;
			}
		}
		char s[64]; snprintf(s, sizeof s, "%.17g", opt);
		std::cout << id << " " << n << " " << m << " " << C.size() << " " << nexact << " " << s << " "
		          << opt2.get_num().get_str() << " " << opt2.get_den().get_str() << "\n";
		std::cout.flush();
	}
	return 0;
}
