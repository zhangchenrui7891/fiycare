#pragma once
#include "simulation.h"
namespace sim {
struct AirData {double ias,tas,groundSpeed,mach,altitude,verticalSpeed,aoaDegrees,betaDegrees;};
inline AirData airData(const Flight& f,double qnh=1013.25){
 const auto a=atmosphere(f.y);const double tas=f.airspeed();
 // Ideal subsonic pitot impact pressure -> sea-level calibrated airspeed.
 // Instrument/position errors are omitted; the cockpit calls this ideal IAS.
 const double mach=tas/a.soundSpeed;
 const double impact=a.pressure*(std::pow(1+.2*mach*mach,3.5)-1);
 const double ias=340.294*std::sqrt(5*(std::pow(1+impact/101325.,2./7)-1));
 const double normalizedPressure=a.pressure*101325/(std::clamp(qnh,950.,1050.)*100);
 const double p11=atmosphere(11000).pressure;
 const double altitude=normalizedPressure>=p11?44330.77*(1-std::pow(normalizedPressure/101325.,1/5.2558797)):
  11000-287.05287*216.65/9.80665*std::log(normalizedPressure/p11);
 return {ias,tas,std::hypot(f.vx,f.vz),tas/a.soundSpeed,altitude,f.vy,f.aoa*180/pi,f.beta*180/pi};
}
struct Avionics {
 bool battery=true,generator=true,master=true;double charge=1,qnh=1013.25;
 bool generating(const Flight& f)const{return generator&&(f.engines.initialized?f.engines.anyGenerator(f.aircraftId,f.engine,f.fuel):(f.engine&&f.fuel>0&&f.rpm>1000));}
 bool busPowered(const Flight& f)const{return generating(f)||(battery&&charge>.001);}
 bool powered(const Flight& f)const{return master&&busPowered(f);}
 void step(double dt,const Flight& f){
  if(!std::isfinite(dt)||dt<=0)return;
  if(generating(f)&&battery)charge=std::min(1.,charge+dt/900.);
  else if(battery&&charge>0)charge=std::max(0.,charge-dt*(master?1.:.3)/2700.);
 }
};
}
