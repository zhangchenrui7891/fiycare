#include "src/flight_instruments.h"
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
std::wstring legacy(double v){
 if(!std::isfinite(v))return L"---";
 if(std::abs(v)<.5)v=0;
 std::wostringstream out;out.imbue(std::locale::classic());
 out<<std::fixed<<std::setprecision(0)<<std::round(v);return out.str();
}
void check(double v){
 if(instruments::whole(v)!=legacy(v))throw std::runtime_error("Integer formatting differs");
}
struct Comma : std::numpunct<wchar_t>{
 wchar_t do_decimal_point()const override{return L',';}
 wchar_t do_thousands_sep()const override{return L'.';}
 std::string do_grouping()const override{return "\3";}
};
int main(){
 std::size_t cases=0;
 for(int n=-10000;n<=10000;++n)for(double offset:{-.5,-.499999,0.,.499999,.5}){
  check(n+offset);++cases;
 }
 std::mt19937_64 rng(55);
 for(int i=0;i<20000;++i){check(double(rng()%2000000000000001ULL)-1e15);++cases;}
 for(double v:{0.,-0.,.5,-.5,1e15,-1e15,std::nextafter(1e15,INFINITY),
   std::nextafter(-1e15,-INFINITY),1e16,-1e16,1e100,-1e100,
   std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest(),
   std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),
   std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::denorm_min()}){
  check(v);++cases;
 }
 auto saved=std::locale();
 std::locale::global(std::locale(saved,new Comma));
 for(double v:{-1234.5,0.,1234.5,1e100}){check(v);++cases;}
 std::locale::global(saved);
 if(instruments::whole(-.49)!=L"0"||instruments::whole(-.5)!=L"-1"||
    instruments::whole(.5)!=L"1")throw std::runtime_error("Rounding/zero contract");
 std::cout<<cases<<" old/new integer-format comparisons passed, including rounding, finite bounds, invalid values and non-classic global locale. No FPS claim.\n";
}
