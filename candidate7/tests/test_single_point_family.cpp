// Adversarial family A: pi = one point, sigma = K clusters of exactly repeated
// points.  Every traversal pairs pi_1 with every sigma_j, so the decider is YES at
// tau iff tau lies in every disc D(pi_1 - sigma_j, delta): the exact answer is
// MEC-radius(centres) <= delta, computed here by brute force.
//
//   test_single_point_family <seed0> <count> <Kmax> <max_multiplicity> [v]
//     decider: delta = r*(1+u), u log-uniform in [1e-4, 1]; truth is YES; prints FAIL on NO
//   LMF=1 test_single_point_family ...
//     value computation: prints LMFERR when |value - r*| > 2e-7
//
// candidate5 (MAXREGION_FIX=none N6_RANGE=0): 78 decider FAILs over seeds 1..20000 (6 6);
// original (MAXREGION=legacy ...): 0 decider FAILs, 75 LMFERRs (up to 2x); candidate6: 0 / 0.
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
static bool inAll(std::vector<P>const&c,double cx,double cy,double r2){for(auto&p:c){double dx=p.x-cx,dy=p.y-cy;if(dx*dx+dy*dy>r2*(1+1e-12))return false;}return true;}
double mec(std::vector<P> const& c){
  double best=1e300; int n=c.size();
  if(n==1) return 0;
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++){double cx=(c[i].x+c[j].x)/2,cy=(c[i].y+c[j].y)/2;double r2=((c[i].x-cx)*(c[i].x-cx)+(c[i].y-cy)*(c[i].y-cy));if(r2<best*best&&inAll(c,cx,cy,r2))best=std::sqrt(r2);}
  for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)for(int k=j+1;k<n;k++){
    double ax=c[i].x,ay=c[i].y,bx=c[j].x,by=c[j].y,qx=c[k].x,qy=c[k].y;
    double d=2*(ax*(by-qy)+bx*(qy-ay)+qx*(ay-by)); if(std::fabs(d)<1e-15) continue;
    double ux=((ax*ax+ay*ay)*(by-qy)+(bx*bx+by*by)*(qy-ay)+(qx*qx+qy*qy)*(ay-by))/d;
    double uy=((ax*ax+ay*ay)*(qx-bx)+(bx*bx+by*by)*(ax-qx)+(qx*qx+qy*qy)*(bx-ax))/d;
    double r2=(ax-ux)*(ax-ux)+(ay-uy)*(ay-uy); if(r2<best*best&&inAll(c,ux,uy,r2))best=std::sqrt(r2);}
  return best;
}
int main(int argc,char**argv){
  long s0=atol(argv[1]),cnt=atol(argv[2]); int kmax=atoi(argv[3]); int mult=atoi(argv[4]); int verbose=argc>5;
  std::cout<<std::setprecision(17);
  long fails=0;
  for(long s=s0;s<s0+cnt;++s){
    std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0,1);
    int K=3+ (int)(U(g)*(kmax-2)); std::vector<P> cen(K);
    for(auto&p:cen){p.x=std::round(U(g)*1e4)/1e4;p.y=std::round(U(g)*1e4)/1e4;}
    Curve c1,c2; c1.push_back({0,0});
    std::vector<int> order;
    for(int k=0;k<K;k++){int m=1+(int)(U(g)*mult); for(int t=0;t<m;t++) order.push_back(k);}
    std::shuffle(order.begin(),order.end(),g);
    for(int k:order) c2.push_back({-cen[k].x,-cen[k].y});
    double r=mec(cen);
    double u=std::pow(10.,-4+4*U(g));
    double delta=r*(1+u);
    if(getenv("LMF")){ FrechetUnderTranslation f; double v=f.calcDistance2(c1,c2); double err=(v-r)/r;
      if(std::fabs(v-r)>2e-7){++fails; std::cout<<"LMFERR seed="<<s<<" K="<<K<<" m="<<c2.size()<<" r*="<<r<<" lmf="<<v<<" relerr="<<err<<"\n"; if(verbose){for(int k:order)std::cout<<"  c "<<cen[k].x<<" "<<cen[k].y<<"\n";}} continue; }
    FrechetUnderTranslation f; bool ans=f.lessThan(delta,c1,c2);
    if(!ans){++fails; std::cout<<"FAIL seed="<<s<<" K="<<K<<" m="<<c2.size()<<" r*="<<r<<" delta="<<delta<<" u="<<u<<"\n";
      if(verbose){for(int k:order)std::cout<<"  c "<<cen[k].x<<" "<<cen[k].y<<"\n";}}
  }
  std::cout<<"DONE fails="<<fails<<" of "<<cnt<<"\n";
}
