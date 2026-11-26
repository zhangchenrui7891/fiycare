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
// Repeatable starting point for the playable Pudong final-approach exercise.
inline Snapshot pudongA330Final(){
 Snapshot state;
 auto& flight=state.flight;
 flight.configure(2);
 constexpr double range=6000.0;
 const double slope=std::tan(3*pi/180);
 const auto& runway=runways[0];
 flight.x=runway.x;
 flight.z=runway.z-runway.halfLength-range;
 flight.y=flight.spec().gearHeight+trainingThresholdClearance+slope*range;
 flight.vz=80;
 flight.vy=-flight.vz*slope;
 // Trim for this game's assisted flight model at the exercise weight.
 flight.pitch=-.6*pi/180;
 flight.flap=.5;
 flight.payload=150;
 flight.throttle=.09;
 flight.rpm=700+1900*flight.throttle;
 flight.engines.update(flight.aircraftId,0,flight.engine,flight.throttle,flight.fuel,flight.condition,flight.rpm);
 state.career.mission=2;
 state.career.stage=1;
 state.career.departureRunway=1;
 state.career.arrivalRunway=0;
 return state;
}
inline Snapshot pudongA330Rollout(){
 Snapshot state;
 auto& f=state.flight;
 f.configure(2);
 const auto& runway=runways[0];
 f.x=runway.x;f.z=runway.z-runway.halfLength+400;
 f.y=f.spec().gearHeight;f.vz=65;f.flap=.5;f.payload=150;
 f.rpm=700;
 f.engines.update(f.aircraftId,0,f.engine,f.throttle,f.fuel,f.condition,f.rpm);
 // A ground start has no earned touchdown score or passenger settlement.
 state.career.mission=2;state.career.stage=3;
 return state;
}
inline constexpr double trainingInboundMinCosine=.5; // 60-degree acquisition cone.
inline constexpr double trainingMinTrackSpeed=5.0; // m/s
inline ApproachGuidance approachGuidance(const Flight& f,int selectedRunway=-1){
 ApproachGuidance best;
 if(selectedRunway< -1||selectedRunway>=int(std::size(runways))||f.crashed||
    !std::isfinite(f.x)||!std::isfinite(f.y)||!std::isfinite(f.z)||
    !std::isfinite(f.yaw)||!std::isfinite(f.vx)||!std::isfinite(f.vz)||f.y<0||f.y>1500)return best;
 const double groundSpeed=std::hypot(f.vx,f.vz);
 if(!std::isfinite(groundSpeed))return best;
 const double headingZ=std::cos(f.yaw);
 double bestRange=0;
 for(int index=0;index<int(std::size(runways));++index){
  if(selectedRunway>=0&&index!=selectedRunway)continue;
  const auto& r=runways[index];
  for(int direction:{1,-1}){
   ApproachGuidance g;g.runway=&r;g.runwayIndex=index;g.direction=direction;
   g.thresholdZ=r.z-direction*r.halfLength;
   g.distance=direction*(g.thresholdZ-f.z);
   g.localizer=direction*(f.x-r.x); // Positive means aircraft right of inbound course.
   const double forward=direction*headingZ;
   if(g.distance<0||g.distance>12000||std::abs(g.localizer)>2500||forward<trainingInboundMinCosine)continue;
   // Above taxi speed, require an inbound ground track as well as heading.
   // A small positive along-runway component is insufficient when crossing it.
   if(groundSpeed>trainingMinTrackSpeed&&direction*f.vz<groundSpeed*trainingInboundMinCosine)continue;
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
// Along-runway distance to the end in the direction of ground travel.
inline std::optional<double> remainingRunway(const Flight& f){
 if(f.crashed||!f.grounded()||!std::isfinite(f.vx)||!std::isfinite(f.vz))return {};
 const double speed=std::hypot(f.vx,f.vz);
 if(speed<trainingMinTrackSpeed||std::abs(f.vz)<speed*trainingInboundMinCosine)return {};
 for(const auto& runway:runways){
  if(std::abs(f.x-runway.x)<=runway.halfWidth&&std::abs(f.z-runway.z)<=runway.halfLength)
   return runway.halfLength-(f.vz>0?1.0:-1.0)*(f.z-runway.z);
 }
 return {};
}
struct ApproachDeflection {double lateral=0,vertical=0;bool valid=false;};
// Bounded game-display sensitivity, not a radio receiver or ILS calibration.
// Signs tell the pilot where to fly: right of course -> left diamond;
// above the path -> lower diamond (screen Y increases downward).
inline ApproachDeflection approachDeflection(const ApproachGuidance& g){
 if(!g.valid||!std::isfinite(g.distance)||g.distance<0||
    !std::isfinite(g.localizer)||!std::isfinite(g.glideslope))return {};
 const double lateralScale=std::max(30.,g.distance*std::tan(2.5*pi/180));
 const double verticalScale=std::max(10.,g.distance*std::tan(.7*pi/180));
 return {std::clamp(-g.localizer/lateralScale,-1.,1.),
         std::clamp(g.glideslope/verticalScale,-1.,1.),true};
}

}
