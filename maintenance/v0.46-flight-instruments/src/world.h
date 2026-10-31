#pragma once
#include <algorithm>
#include <cmath>
#include <atomic>
namespace sim {
inline std::atomic<int> worldMode{1}; // 0: fictional mountains, 1: Pudong imagery
inline double terrain(double x,double z,int mode){
 if((std::abs(x)<500&&std::abs(z)<2400)||(std::abs(x-3500)<450&&std::abs(z-2500)<1000))return 0;
 double airportBlend=std::clamp(std::max((std::abs(x)-500)/700.0,(std::abs(z)-2400)/800.0),0.0,1.0);
 if(mode==1)return airportBlend*(.7+.6*std::sin(x*.0006)*std::cos(z*.0007)); // illustrative relief, no DEM
 double ridge=680*std::exp(-std::pow((x-2600)/1600,2))*(.65+.35*std::sin(z*.00055));
 double hills=55+45*std::sin(x*.0012)*std::cos(z*.001);
 double river=std::exp(-std::pow((x+1500-200*std::sin(z*.0007))/170,2));
 return airportBlend*std::max(0.0,(ridge+hills)*(1-river));
}
inline double terrain(double x,double z){return terrain(x,z,worldMode.load());}
struct Runway {double x,z,halfWidth,halfLength;const wchar_t* name;};
inline constexpr Runway runways[2]={{0,0,35,1800,L"浦东训练跑道"},{3500,2500,24,700,L"东海训练机场"}};
inline bool onRunway(double x,double z){
 for(const auto& r:runways)if(std::abs(x-r.x)<=r.halfWidth&&std::abs(z-r.z)<=r.halfLength)return true;
 return false;
}
inline const Runway& nearestRunway(double x,double z){
 const auto distance=[&](const Runway& r){return std::hypot(x-r.x,z-r.z);};
 return distance(runways[1])<distance(runways[0])?runways[1]:runways[0];
}
inline int nearestRunwayIndex(double x,double z){return int(&nearestRunway(x,z)-runways);}
struct MapUV{float u=0,v=0;bool covered=false;};
inline MapUV satelliteUV(double x,double z){
 // X right, Z runway heading 034 degrees; origin near Pudong airport.
 constexpr double r=6378137,pi=3.14159265358979323846,lat=31.145,lon=121.805,h=34*pi/180;
 double east=x*std::cos(h)+z*std::sin(h),north=-x*std::sin(h)+z*std::cos(h);
 double longitude=lon+east/(r*std::cos(lat*pi/180))*180/pi,latitude=lat+north/r*180/pi;
 auto merc=[&](double l){return r*std::log(std::tan(pi/4+l*pi/360));};
 double u=(longitude-121.66)/.29,v=(merc(31.29)-merc(latitude))/(merc(31.29)-merc(31.0));
 return {float((1+u*1022)/2048),float((1+v*1022)/2048),u>=0&&u<=1&&v>=0&&v<=1};
}
}
