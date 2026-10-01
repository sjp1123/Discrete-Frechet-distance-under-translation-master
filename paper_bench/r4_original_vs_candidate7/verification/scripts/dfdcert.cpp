// independent certificate: discrete Frechet distance of P and Q + t at the translation the
// implementation reports, in long double (DP over squared distances).
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
typedef long double LD;
std::map<std::string, std::vector<std::pair<LD,LD>>> cache;
const std::vector<std::pair<LD,LD>>& curve(const std::string& p) {
	auto it = cache.find(p); if (it != cache.end()) return it->second;
	std::ifstream in(p); std::string line; std::vector<std::pair<LD,LD>> c; double x, y;
	while (std::getline(in, line)) { std::istringstream ls(line); if (ls >> x >> y) c.push_back({x, y}); }
	return cache[p] = c;
}
int main(int argc, char** argv) {
	// argv: csv dir ; csv columns file1,file2,n1,n2,value,...,tx,ty (tx, ty last two)
	std::ifstream in(argv[1]); std::string dir = argv[2]; if (dir.back() != '/') dir += '/';
	std::string line; std::getline(in, line);
	while (std::getline(in, line)) {
		std::vector<std::string> f; std::stringstream ss(line); std::string t;
		while (std::getline(ss, t, ',')) f.push_back(t);
		auto& P = curve(dir + f[0]); auto& Q = curve(dir + f[1]);
		LD v = std::stold(f[4]), tx = std::stold(f[f.size()-2]), ty = std::stold(f[f.size()-1]);
		size_t n = P.size(), m = Q.size();
		std::vector<LD> D(n * m);
		for (size_t i = 0; i < n; ++i) for (size_t j = 0; j < m; ++j) {
			LD dx = P[i].first - (Q[j].first + tx), dy = P[i].second - (Q[j].second + ty);
			LD c = dx*dx + dy*dy, prev;
			if (!i && !j) prev = 0; else if (!i) prev = D[j-1]; else if (!j) prev = D[(i-1)*m];
			else prev = std::min({D[(i-1)*m+j], D[i*m+j-1], D[(i-1)*m+j-1]});
			D[i*m+j] = std::max(c, prev);
		}
		LD d = std::sqrt(D[n*m-1]);
		printf("%s %s %.17Lg %.17Lg %.6Le\n", f[0].c_str(), f[1].c_str(), v, d, d - v);
	}
}
