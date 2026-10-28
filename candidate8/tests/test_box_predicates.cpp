// Build: see tests/CMakeLists.txt (target test_box_predicates), or
//   g++ -O2 -std=c++14 -include cstdint -I../lib/cgal_disk_arrangements test_box_predicates.cpp
// Expected: mismatches=0.
//
// Q2 (lens meets box) and Q1 (disc meets box) against an independent exact
// reference: the rational minimax over the box (minimax_box_exact candidates,
// evaluated here directly in cpp_rational).  Near-tangent radii are drawn at the
// exact optimum rounded to double, ± a few ulps.
#include "maximal_regions.cpp"
#include <random>
#include <cstdio>
using namespace cgal_disk_arrangements;
using namespace cgal_disk_arrangements::maxregion;
static Q exact_minimax2(std::vector<std::pair<double,double>> const& c, BoundingBox const& B) {
	double wx, wy; // reuse the rational candidate enumeration but read back the exact value
	std::size_t const m = c.size();
	std::vector<Q> X(m), Y(m); for (std::size_t i=0;i<m;++i){X[i]=Q(c[i].first);Y[i]=Q(c[i].second);}
	Q bx0=Q(B.min.x),bx1=Q(B.max.x),by0=Q(B.min.y),by1=Q(B.max.y); bool have=false; Q best;
	auto consider=[&](Q const& px, Q const& py){ if(px<bx0||px>bx1||py<by0||py>by1) return; Q v=0; for(std::size_t k=0;k<m;++k){Q dx=px-X[k],dy=py-Y[k];Q d=dx*dx+dy*dy; if(d>v)v=d;} if(!have||v<best){have=true;best=v;} };
	auto cl=[](Q const& v,Q const& lo,Q const& hi){return v<lo?lo:(v>hi?hi:v);};
	for(std::size_t i=0;i<m;++i) consider(X[i],Y[i]);
	for(std::size_t i=0;i<m;++i)for(std::size_t j=i+1;j<m;++j) consider((X[i]+X[j])/2,(Y[i]+Y[j])/2);
	for(Q const& y0:{by0,by1}){consider(bx0,y0);consider(bx1,y0);for(std::size_t i=0;i<m;++i){consider(cl(X[i],bx0,bx1),y0);for(std::size_t j=i+1;j<m;++j){if(X[i]==X[j])continue;Q di=y0-Y[i],dj=y0-Y[j];consider(cl(((X[j]*X[j]-X[i]*X[i])+(dj*dj-di*di))/(2*(X[j]-X[i])),bx0,bx1),y0);}}}
	for(Q const& x0:{bx0,bx1}){for(std::size_t i=0;i<m;++i){consider(x0,cl(Y[i],by0,by1));for(std::size_t j=i+1;j<m;++j){if(Y[i]==Y[j])continue;Q di=x0-X[i],dj=x0-X[j];consider(x0,cl(((Y[j]*Y[j]-Y[i]*Y[i])+(dj*dj-di*di))/(2*(Y[j]-Y[i])),by0,by1));}}}
	(void)wx;(void)wy; return best;
}
int main(){
	std::mt19937_64 g(7); std::uniform_real_distribution<double> U01(0,1);
	long n=0,bad=0,near=0,q2ex=0; Stats S;
	for(int it=0; it<300000; ++it){
		double sc = std::pow(10.0, -2 + 7*U01(g));           // coordinate scale 1e-2 .. 1e5
		double off = (it%3==0) ? 1.36e7 : 0.0;               // Sigspatial-like offset on some cases
		BoundingBox B; double bx=off+sc*U01(g), by=sc*U01(g), w=sc*0.3*U01(g)+1e-12*sc, h=sc*0.3*U01(g)+1e-12*sc;
		B.min={bx,by}; B.max={bx+w,by+h};
		double xi=off+sc*U01(g), yi=sc*U01(g), xj=off+sc*U01(g), yj=sc*U01(g);
		if(it%5==1){ xj = xi + sc*1e-9*(U01(g)-0.5); }       // near-vertical pair (ill-conditioned crossing)
		if(it%5==2){ yj = yi; }
		std::vector<std::pair<double,double>> c={{xi,yi},{xj,yj}};
		Q v = exact_minimax2(c,B);                            // exact min over box of max dist²
		double r = std::sqrt(v.convert_to<double>());
		int k = (int)(U01(g)*7) - 3; for(int t=0;t<std::abs(k);++t) r = std::nextafter(r, k>0? 1e300:-1e300);
		if(it%4==3) r *= (0.5+U01(g));                        // generic radius
		bool truth = v <= Q(r)*Q(r);
		bool got = lens_meets_box(xi,yi,xj,yj,B,r,4.0*r*r,true,0.0,S);
		++n; if(got!=truth){ ++bad; if(bad<5) std::printf("MISMATCH it=%d truth=%d got=%d\n",it,truth,got);} if(k!=0||it%4!=3) ++near;
		// Q1 check for disc i
		std::vector<std::pair<double,double>> c1={{xi,yi}};
		Q v1=exact_minimax2(c1,B); double r1=std::sqrt(v1.convert_to<double>()); r1=std::nextafter(r1,(it&1)?1e300:-1e300);
		bool t1 = v1 <= Q(r1)*Q(r1);
		double qx=clampd(xi,B.min.x,B.max.x), qy=clampd(yi,B.min.y,B.max.y);
		bool g1 = in_disc(qx,qy,xi,yi,r1,true,0.0,S);
		if(g1!=t1){ ++bad; if(bad<5) std::printf("Q1 MISMATCH it=%d\n",it);}
	}
	std::printf("cases=%ld near_tangent=%ld mismatches=%ld q2_calls=%lld q2_rational=%lld p2_rational=%lld\n",n,near,bad,S.q2_calls,S.q2_exact,S.p2_exact);
}
