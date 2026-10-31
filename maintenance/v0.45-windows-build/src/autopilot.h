#pragma once
#include "storage.h"

namespace sim {
struct NavigationTarget {double x,z;};
inline constexpr NavigationTarget navigationRoute[4]={{0,2500},{3500,2500},{3500,-2500},{0,-2500}};
inline bool validNavigationWaypoint(int active){return active>=0&&active<4;}
struct NavigationLeg {
 double length=0,alongTrack=0,remaining=0,crossTrack=0,distance=0,alongSpeed=0;
 bool valid=false;
};
inline NavigationLeg navigationLeg(const Snapshot& state){
 NavigationLeg leg;const auto& f=state.flight;const int active=state.navigationWaypoint;
 if(!validNavigationWaypoint(active)||!std::isfinite(f.x)||!std::isfinite(f.z)||
    !std::isfinite(f.vx)||!std::isfinite(f.vz))return leg;
 const auto& a=navigationRoute[(active+3)%4];const auto& b=navigationRoute[active];
 const double dx=b.x-a.x,dz=b.z-a.z;leg.length=std::hypot(dx,dz);
 if(leg.length<=.001)return {};
 const double ux=dx/leg.length,uz=dz/leg.length;
 leg.alongTrack=(f.x-a.x)*ux+(f.z-a.z)*uz;
 leg.remaining=leg.length-leg.alongTrack;
 leg.crossTrack=(f.x-a.x)*uz-(f.z-a.z)*ux;
 leg.distance=std::hypot(b.x-f.x,b.z-f.z);
 leg.alongSpeed=f.vx*ux+f.vz*uz;
 leg.valid=std::isfinite(leg.alongTrack)&&std::isfinite(leg.remaining)&&
  std::isfinite(leg.crossTrack)&&std::isfinite(leg.distance)&&std::isfinite(leg.alongSpeed);
 return leg.valid?leg:NavigationLeg{};
}
inline constexpr double navigationCaptureRadius=450,navigationPassWindow=900;
inline bool navigationCaptured(const NavigationLeg& leg){
 if(!leg.valid)return false;
 // Preserve the original radius capture; the extra game-only terminal window
 // recovers a nearby missed point, never an unbounded extension or turn lead.
 return leg.distance<navigationCaptureRadius||
  (leg.remaining<=0&&leg.remaining>=-navigationPassWindow&&
   std::abs(leg.crossTrack)<=navigationPassWindow&&leg.alongSpeed>5);
}
inline double navigationDistance(const Snapshot& state){return navigationLeg(state).distance;}
inline double navigationCrossTrack(const Snapshot& state){return navigationLeg(state).crossTrack;}
struct NavigationGuidance {
 double trackBearing=0,lookAhead=0,interceptAngle=0;
 bool valid=false,tracking=false;
};
inline NavigationGuidance navigationGuidance(const Snapshot& state){
 const auto leg=navigationLeg(state);if(!leg.valid)return {};
 const auto& f=state.flight;const auto& p=navigationRoute[state.navigationWaypoint];
 NavigationGuidance g;g.trackBearing=std::atan2(p.x-f.x,p.z-f.z);
 // Game-only straight-leg capture. Before the leg and after its endpoint,
 // preserve direct-to recovery. Sequencing remains navigationCaptured's job.
 if(leg.alongTrack>=0&&leg.remaining>0){
  const auto& a=navigationRoute[(state.navigationWaypoint+3)%4];
  const double course=std::atan2(p.x-a.x,p.z-a.z);
  g.lookAhead=std::min(leg.remaining,std::clamp(std::hypot(f.vx,f.vz)*12.,600.,1800.));
  g.interceptAngle=std::clamp(std::atan2(-leg.crossTrack,g.lookAhead),-pi/4,pi/4);
  g.trackBearing=wrapAngle(course+g.interceptAngle);g.tracking=true;
 }
 g.valid=std::isfinite(g.trackBearing);return g.valid?g:NavigationGuidance{};
}
inline void updateAutopilot(Snapshot& state,Controls& controls){
 auto& f=state.flight;
 const bool manualRoll=std::abs(controls.roll)>.05||std::abs(controls.yaw)>.05;
 const bool manualPitch=std::abs(controls.pitch)>.05;
 if(manualRoll){state.headingHold=false;state.navigationHold=false;}
 if(manualPitch)state.altitudeHold=false;
 const auto data=airData(f,state.avionics.qnh);
 const bool available=state.avionics.powered(f)&&!f.crashed&&!f.grounded()&&!f.stallWarning&&data.ias>(f.aircraftId?45.0:25.0);
 if(!available){state.headingHold=false;state.altitudeHold=false;state.navigationHold=false;return;}
 if(state.navigationHold){
  const auto leg=navigationLeg(state);
  if(!leg.valid){state.navigationHold=false;state.headingHold=false;}
  else{
  if(navigationCaptured(leg))state.navigationWaypoint=(state.navigationWaypoint+1)%4;
  const auto guidance=navigationGuidance(state);
  if(!guidance.valid){state.navigationHold=false;state.headingHold=false;}
  else{
  const double bearing=guidance.trackBearing;
  const double windRight=f.windX*std::cos(bearing)-f.windZ*std::sin(bearing);
  state.targetHeading=wrapAngle(bearing-std::asin(std::clamp(windRight/std::max(1.,data.tas),-.7,.7)));state.headingHold=true;
  }
  }
 }
 if(state.headingHold){
  const double error=wrapAngle(state.targetHeading-f.yaw);
  const double targetRoll=std::clamp(error*.7,-.42,.42);
  controls.roll=std::clamp((targetRoll-f.roll)*2.8-f.rollRate*.7,-1.0,1.0);
  if(f.surfaceModel){
   const auto d=airframeDimensions(f.aircraftId);
   const double q=.5*atmosphere(f.y).density*data.tas*data.tas;
   const double rudderMomentPerUnit=q*f.spec().wingArea*.10*d.tailArm*.65;
   const double engineCompensation=-f.engines.bodyForces.moment.y/std::max(1.,rudderMomentPerUnit);
   const double coordinatedRate=std::tan(f.roll)*9.81/std::max(20.,data.tas);
   controls.yaw=std::clamp(engineCompensation-(f.yawRate-coordinatedRate)*.7,-1.,1.);
  }
 }
 if(state.altitudeHold){
  const double error=state.targetAltitude-data.altitude;
  const double q=.5*atmosphere(f.y).density*data.tas*data.tas;
  const double mass=f.spec().emptyMass+std::max(0.,f.payload)+f.passengers*85.;
  const double alpha=std::clamp((mass*9.81/std::max(1.,q*f.spec().wingArea)-.32-f.spec().flapLift*f.flap)/4.6,-.1,.18);
  const double targetVy=std::clamp(error*.04,-8.,8.);
  const double targetPitch=std::clamp(alpha+(targetVy-f.vy)*.012,-.20,.25);
  double demand=(targetPitch-f.pitch)*3.2-f.pitchRate*.55-f.trim;
  if(f.surfaceModel){
   const auto dimensions=airframeDimensions(f.aircraftId);
   const double tailArea=f.spec().wingArea*.18;
   const double requiredTailCl=dimensions.wingHeight*f.dragForce/std::max(1.,dimensions.tailArm*q*tailArea);
   const double elevatorTrim=(alpha-.035-requiredTailCl/3.6)/.22;
   demand=elevatorTrim+(targetPitch-f.pitch)*4.5-f.pitchRate*1.0-f.trim;
  }
  controls.pitch=std::clamp(demand,-1.0,1.0);
 }
}
}
