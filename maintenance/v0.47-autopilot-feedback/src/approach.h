#pragma once
#include "storage.h"
#include <iterator>

namespace sim {
struct ApproachGuidance {
 const Runway* runway=nullptr;
 int runwayIndex=-1,direction=1; // +Z or -Z in the illustrative local world.
 double thresholdZ=0;
 double distance=0,localizer=0,glideslope=0,desiredAltitude=0;
 bool valid=false;
};
// Flat-runway training geometry, NOT a radio receiver, certified ILS or autoland.
// The chosen 50ft is idealized wheel clearance at the threshold, not antenna TCH.
inline constexpr double trainingThresholdClearance=15.24;
inline ApproachGuidance approachGuidance(const Flight& f,int selectedRunway=-1){
 ApproachGuidance best;
 if(selectedRunway< -1||selectedRunway>=int(std::size(runways))||f.crashed||
    !std::isfinite(f.x)||!std::isfinite(f.y)||!std::isfinite(f.z)||
    !std::isfinite(f.yaw)||!std::isfinite(f.vx)||!std::isfinite(f.vz)||f.y<0||f.y>1500)return best;
 double bestRange=0;
 for(int index=0;index<int(std::size(runways));++index){
  if(selectedRunway>=0&&index!=selectedRunway)continue;
  const auto& r=runways[index];
  for(int direction:{1,-1}){
   ApproachGuidance g;g.runway=&r;g.runwayIndex=index;g.direction=direction;
   g.thresholdZ=r.z-direction*r.halfLength;
   g.distance=direction*(g.thresholdZ-f.z);
   g.localizer=direction*(f.x-r.x); // Positive means aircraft right of inbound course.
   const double forward=direction*std::cos(f.yaw);
   if(g.distance<0||g.distance>12000||std::abs(g.localizer)>2500||forward<std::cos(pi/3))continue;
   // A strong wind or a backwards-moving aircraft must not get an inbound cue.
   if(std::hypot(f.vx,f.vz)>5&&direction*f.vz<=0)continue;
   g.desiredAltitude=f.spec().gearHeight+trainingThresholdClearance+std::tan(3*pi/180)*g.distance;
   g.glideslope=f.y-g.desiredAltitude;g.valid=true;
   const double range=std::hypot(g.distance,g.localizer);
   if(!best.valid||range<bestRange){best=g;bestRange=range;}
  }
 }
 return best;
}
inline ApproachGuidance approachGuidance(const Snapshot& state){
 if(!state.avionics.powered(state.flight)||state.flight.grounded())return {};
 // A passenger flight keeps its arrival even if another airport is closer.
 const bool arriving=state.career.mission==2&&state.career.stage==1;
 if(arriving&&(state.career.arrivalRunway<0||state.career.arrivalRunway>=int(std::size(runways))))return {};
 return approachGuidance(state.flight,arriving?state.career.arrivalRunway:-1);
}
}
