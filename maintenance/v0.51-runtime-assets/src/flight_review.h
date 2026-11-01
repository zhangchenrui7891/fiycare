#pragma once
#include "simulation.h"

namespace sim {
struct LandingResult {
 bool valid=false;
 int score=0;
 double sinkRate=0,centerlineError=0,bankDegrees=0,headingErrorDegrees=0;
};

inline LandingResult assessLanding(const Flight& flight,double impactVy,double impactRoll,double impactYaw){
 LandingResult r;
 if(!onRunway(flight.x,flight.z)||flight.crashed)return r;
 r.valid=true;
 r.sinkRate=std::max(0.0,-impactVy);
 r.centerlineError=std::abs(flight.x-nearestRunway(flight.x,flight.z).x);
 r.bankDegrees=std::abs(impactRoll)*180/pi;
 const double north=std::abs(wrapAngle(impactYaw));
 const double south=std::abs(wrapAngle(impactYaw-pi));
 r.headingErrorDegrees=std::min(north,south)*180/pi;
 const double penalty=std::max(0.0,r.sinkRate-1.0)*12.0+
                      r.centerlineError*.8+r.bankDegrees*1.5+r.headingErrorDegrees*.7;
 r.score=std::clamp(int(std::lround(100-penalty)),0,100);
 return r;
}

inline const wchar_t* landingGrade(int score){
 return score>=90?L"优秀":score>=75?L"良好":score>=60?L"合格":L"需练习";
}
}
