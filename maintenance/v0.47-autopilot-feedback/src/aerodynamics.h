#pragma once
#include "aircraft.h"
#include <algorithm>
#include <cmath>
namespace sim {
// ISA troposphere and isothermal lower stratosphere, SI throughout.
struct Atmosphere {double temperature,pressure,density,soundSpeed;};
inline Atmosphere atmosphere(double altitude){
 const double h=std::clamp(altitude,0.,30000.);
 const double t=288.15-.0065*std::min(h,11000.);
 double p=101325*std::pow(t/288.15,5.2558797);
 if(h>11000)p*=std::exp(-9.80665*(h-11000)/(287.05287*t));
 return {t,p,p/(287.05287*t),std::sqrt(1.4*287.05287*t)};
}
// Prototype tuning values, NOT certified aircraft/airfoil data.
struct AeroTuning {double aspectRatio,efficiency,stallAngle;};
inline AeroTuning aeroTuning(int id){
 static constexpr AeroTuning values[]={{7.3,.78,.30},{10.,.80,.29},{9.3,.82,.28},{9.5,.85,.28},{9.48,.82,.28},{9.45,.80,.29},{9.6,.86,.28}};
 return values[std::clamp(id,0,aircraftCount-1)];
}
struct AeroCoefficients {double cl,cd,cy,separation,groundFactor;bool stallWarning;};
inline AeroCoefficients aeroCoefficients(int id,double alpha,double beta,double flap,bool gear,double wingHeight){
 constexpr double p=3.141592653589793;
 const auto& m=aircraft(id);const auto t=aeroTuning(id);
 flap=std::clamp(flap,0.,1.);alpha=std::remainder(alpha,2*p);beta=std::clamp(beta,-p/2,p/2);
 const double critical=t.stallAngle-.035*flap;
 const double u=std::clamp((std::abs(alpha)-critical)/.16,0.,1.);
 const double separated=u*u*(3-2*u);
 const double attached=.32+4.6*std::clamp(alpha,-critical,critical)+m.flapLift*flap;
 const double cl=((1-separated)*attached+separated*.85*std::sin(2*alpha))*std::pow(std::cos(beta),2);
 const double span=std::sqrt(t.aspectRatio*m.wingArea);
 // Approximate reduction in induced drag near the ground, based on wing height/span.
 const double ratio=16*std::max(.01,wingHeight)/span;
 const double ge=std::clamp(ratio*ratio/(1+ratio*ratio),.08,1.);
 const double induced=cl*cl/(p*t.aspectRatio*t.efficiency)*ge;
 const double cd=m.baseDrag+induced+.03*flap+(gear?.015:0)+.9*separated*std::pow(std::sin(alpha),2)+.25*std::pow(std::sin(beta),2);
 return {cl,cd,-.65*std::sin(beta),separated,ge,std::abs(alpha)>critical*.85};
}
}
