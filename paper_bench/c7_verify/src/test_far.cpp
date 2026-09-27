// E1-tests: depth-limit truncation (H2) in the PAPER configuration.
// pi = [P, F1..F4], sigma = [S0, S_1..S_K (with repeats), G1..G4], G_i = F_i - tau*.
// Centres c_j = P - S_j lie on a circle of radius R around tau* (S0 at tau* itself), the far
// pairs (F_i, G_i) sit exactly at tau* and make the extreme-point constraints of
// getInitialSearchBox() loose, so the initial box is ~2R x 2R and a depth-40 box is ~2e-6 R wide.
// The brute-force computeCutCenters (decider, use_kd_tree=false) stops after 13 cut entries,
// which by construction are the "early" discs only.  delta* = R exactly (MEC of the centres).
//   test_far <seed0> <count> <K> <mode> <scale> [v]
//   mode 1: first K-3 centres on an arc of 2.9 rad, last 3 on the opposite side
//   mode 2: K-1 centres within +-0.3 rad of angle rot, the last one antipodal (rot+pi)
// For u in {1e-7,3e-7,1e-6,3e-6,1e-5,1e-4}: decider at R(1+u) (truth YES) and R(1-u) (truth NO)
// with (depth,cut) = (40,12) [paper default ctor], (40,1000) [no truncation].
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <vector>
struct P{double x,y;};
static double dfd_at(std::vector<P> const& a, std::vector<P> const& b, double tx, double ty) {
	std::size_t n = a.size(), m = b.size(); std::vector<double> D(n * m);
	for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < m; ++j) {
		double dx = a[i].x - b[j].x - tx, dy = a[i].y - b[j].y - ty, d = dx*dx + dy*dy, best;
		if (i == 0 && j == 0) best = 0; else if (i == 0) best = D[j-1]; else if (j == 0) best = D[(i-1)*m];
		else best = std::min(D[(i-1)*m + j], std::min(D[(i-1)*m + j-1], D[i*m + j-1]));
		D[i*m + j] = std::max(best, d);
	}
	return std::sqrt(D[n*m - 1]);
}
int main(int argc,char**argv){
  long s0=atol(argv[1]),cnt=atol(argv[2]); int K=atoi(argv[3]); int mode=atoi(argv[4]); double sc=atof(argv[5]); bool verbose=argc>6;
  std::cout<<std::setprecision(17);
  long fy[2]={0,0}, fn[2]={0,0}, lerr=0, q=0, witbad=0;
  const double us[]={1e-7,3e-7,1e-6,3e-6,1e-5,1e-4};
  for(long s=s0;s<s0+cnt;++s){
    std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0,1);
    double R=sc*(0.3+0.7*U(g)), rot=U(g)*6.283185307179586, tx=sc*(U(g)-0.5), ty=sc*(U(g)-0.5);
    double L=20*sc;
    std::vector<P> A, B; // pi, sigma
    A.push_back({0,0});
    double a0=U(g)*6.283185307179586, a4=U(g)*6.283185307179586; const double OFF=getenv("OFF")?atof(getenv("OFF")):0.4;
    B.push_back({-(tx+OFF*R*std::cos(a0)),-(ty+OFF*R*std::sin(a0))}); // S0: centre inside, 0.4R from tau* (breaks box symmetry)
    for(int k=0;k<K;k++){
      double th = mode==1 ? (k<K-3 ? rot+2.9*k/(K-4) : rot+3.6+0.9*(k-(K-3)))
                          : (k<K-1 ? rot-0.3+0.6*k/std::max(1,K-2) : rot+3.141592653589793);
      double cx=tx+R*std::cos(th), cy=ty+R*std::sin(th);
      int m=1+(int)(U(g)*3); for(int t=0;t<m;t++) B.push_back({-cx,-cy});
    }
    const P F[4]={{-L,0},{0,L},{L,0},{0,-L}};
    for(auto&f:F){ A.push_back(f); }
    for(int i=0;i<4;i++){ double ox= i==3 ? OFF*R*std::cos(a4) : 0.0, oy= i==3 ? OFF*R*std::sin(a4) : 0.0; B.push_back({F[i].x-tx-ox,F[i].y-ty-oy}); }
    // exact delta*: R up to rounding of the constructed centres; take the DFD at tau* as the reference
    double r=dfd_at(A,B,tx,ty);
    Curve c1,c2; for(auto&p:A) c1.push_back({p.x,p.y}); for(auto&p:B) c2.push_back({p.x,p.y});
    { FrechetUnderTranslation f; double v=f.calcDistance2(c1,c2);
      if(std::fabs(v-r)>1e-7){++lerr; std::cout<<"LMFERR seed="<<s<<" r="<<r<<" v="<<v<<"\n";} }
    for(double u:us) for(int a=0;a<2;a++){
      ++q;
      { FrechetUnderTranslation f = a==0 ? FrechetUnderTranslation() : FrechetUnderTranslation(1e-7,40,1000);
        bool ans=f.lessThan(r*(1+u),c1,c2);
        if(!ans){++fy[a]; std::cout<<"FAILY seed="<<s<<" arm="<<a<<" u="<<u<<" r="<<r<<" abs_gap="<<r*u<<"\n";} }
      { FrechetUnderTranslation f = a==0 ? FrechetUnderTranslation() : FrechetUnderTranslation(1e-7,40,1000);
        bool ans=f.lessThan(r*(1-u),c1,c2);
        if(ans){++fn[a]; std::cout<<"FAILN seed="<<s<<" arm="<<a<<" u="<<u<<" r="<<r<<"\n";} }
    }
    if(verbose){ std::cout<<"  inst seed="<<s<<" tau*=("<<tx<<","<<ty<<") R="<<R<<" dfd(tau*)="<<r<<"\n  pi:"; for(auto&p:A) std::cout<<" ("<<p.x<<","<<p.y<<")"; std::cout<<"\n  sigma:"; for(auto&p:B) std::cout<<" ("<<p.x<<","<<p.y<<")"; std::cout<<"\n"; }
  }
  std::cout<<"DONE K="<<K<<" mode="<<mode<<" scale="<<sc<<" count="<<cnt<<" lmferr="<<lerr
           <<" failY(paper,nocut)="<<fy[0]<<","<<fy[1]<<" failN="<<fn[0]<<","<<fn[1]<<" q="<<q<<"\n";
}
