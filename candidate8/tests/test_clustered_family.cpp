// Adversarial family B: pi has 2-4 random points, sigma is made of K clusters of
// exactly repeated points (as GPS repeats produce).  Prints the LMF value v and
// the decider answer at delta = v(1+u) + 1e-7 for u in {1e-5 .. 0.1}; every
// answer must be 1.  Compare the value column across arms.
//
//   test_clustered_family <seed0> <count>
//
// Seeds 1..20000: candidate5 prints 1,215 zeros on 893 pairs; original's value
// differs from original+N6_RANGE on 63 pairs (up to 2x); candidate6: no zeros,
// values equal to original+N6_RANGE.
#include "defs.h"
#include "curves.h"
#include "frechet_under_translation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cmath>
int main(int argc,char**argv){
  long s0=atol(argv[1]),cnt=atol(argv[2]);
  std::cout<<std::setprecision(17);
  for(long s=s0;s<s0+cnt;++s){
    std::mt19937_64 g(s); std::uniform_real_distribution<double> U(0,1);
    int n1=2+(int)(U(g)*3); Curve c1,c2;
    double spread=0.05+0.4*U(g);
    for(int i=0;i<n1;i++) c1.push_back({std::round(U(g)*spread*1e4)/1e4, std::round(U(g)*spread*1e4)/1e4});
    int K=3+(int)(U(g)*4);
    for(int k=0;k<K;k++){ double x=std::round(U(g)*1e4)/1e4, y=std::round(U(g)*1e4)/1e4; int m=1+(int)(U(g)*6); for(int t=0;t<m;t++) c2.push_back({-x,-y}); }
    FrechetUnderTranslation f; double val=f.calcDistance2(c1,c2);
    std::cout<<"S "<<s<<" "<<val;
    const double us[]={1e-5,1e-4,3e-4,1e-3,3e-3,1e-2,3e-2,0.1};
    for(double u:us){ FrechetUnderTranslation d; std::cout<<" "<<d.lessThan(val*(1+u)+1e-7,c1,c2);} 
    std::cout<<"\n";
  }
}
