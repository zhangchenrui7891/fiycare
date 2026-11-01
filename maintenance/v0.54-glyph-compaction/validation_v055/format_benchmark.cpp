#include "../src/flight_instruments.h"
#include <iostream>
std::wstring legacy(double v){
 if(!std::isfinite(v))return L"---";
 if(std::abs(v)<.5)v=0;
 std::wostringstream out;out.imbue(std::locale::classic());
 out<<std::fixed<<std::setprecision(0)<<std::round(v);return out.str();
}
#include <chrono>
#include <algorithm>
volatile std::size_t sink=0;
template<class F> double measure(F f){
 auto start=std::chrono::steady_clock::now();
 std::size_t result=0;
 for(int i=0;i<200000;++i){auto s=f(double(i%60000)-1000.25);result+=s.size()+std::size_t(s[0]);}
 sink=result;
 return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
int main(){
 double old[5],current[5];
 for(int i=0;i<5;++i){
  if(i%2){current[i]=measure(instruments::whole);old[i]=measure(legacy);}
  else{old[i]=measure(legacy);current[i]=measure(instruments::whole);}
 }
 std::sort(old,old+5);std::sort(current,current+5);
 std::cout<<"Linux host microbenchmark only; GCC O2, median of 5, 200000 labels: legacy_ms="<<old[2]<<", current_ms="<<current[2]<<". Includes returned strings. Not game/Windows/FPS/HD6000.\n";
}
