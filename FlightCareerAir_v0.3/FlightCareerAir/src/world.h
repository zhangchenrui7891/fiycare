#pragma once
#include <algorithm>
#include <cmath>
namespace sim {
inline double terrain(double x,double z){
 if(std::abs(x)<120&&std::abs(z)<1000)return 0;
 const double edge=std::clamp((std::abs(x)-120)/450.0,0.0,1.0);
 return edge*(45+35*std::sin(x*.003)*std::cos(z*.002));
}
inline bool onRunway(double x,double z){return std::abs(x)<=22&&std::abs(z)<=700;}
}
