#pragma once
#include "world.h"
#include "aircraft.h"
#include "aerodynamics.h"
#include "math3d.h"
#include "engines.h"
#include "surface_aero.h"
#include "rigid_body.h"
#include <algorithm>
#include <cmath>
#include <string>
namespace sim {
struct Controls {double pitch=0,roll=0,yaw=0,throttle=0;bool brake=false,parkingBrake=false;};
struct Flight {
 int aircraftId=0,passengers=0;
 double x=0,y=1.6,z=-500,vx=0,vy=0,vz=0,pitch=0,roll=0,yaw=0,throttle=0;
 double fuel=100,rpm=0,wingFlex=0,flexRate=0,payload=0,condition=1;
 double pitchRate=0,rollRate=0,aoa=0,beta=0,load=0,trim=0;
 double liftForce=0,dragForce=0,sideForce=0,separation=0;
 bool stallWarning=false;
 bool surfaceModel=false;
 double yawRate=0;
 EngineSystem engines;
 RigidRotation rotation;
 SurfaceResult surfaceResult;
 ForceMoment bodyForceMoment;
 void setSurfaceModel(bool enabled){surfaceModel=enabled;rotation.initialized=false;}

 double windX=0,windZ=0,flap=0,time=0;
 bool engine=true,gear=true,crashed=false,assist=true,stalled=false;
 std::string crashReason;
 double speed()const{return std::sqrt(vx*vx+vy*vy+vz*vz);}
 double airspeed()const{return std::sqrt((vx-windX)*(vx-windX)+vy*vy+(vz-windZ)*(vz-windZ));}
 const AircraftSpec& spec()const{return aircraft(aircraftId);}
 void configure(int id){aircraftId=std::clamp(id,0,aircraftCount-1);y=spec().gearHeight;z=spec().spawnZ;passengers=aircraftId==2?180:aircraftId==3?220:aircraftId==1?50:aircraftId>=4?int(spec().seats*2/3):0;}
 bool grounded()const{return y<=terrain(x,z)+spec().gearHeight+.01;}
 void crash(const char* why){crashed=true;crashReason=why;}
 void step(double dt,const Controls& c){
  if(crashed||!std::isfinite(dt)||dt<=0)return;
  dt=std::min(dt,.02);time+=dt;const auto& model=spec();
  throttle=std::clamp(throttle+c.throttle*dt*.5,0.0,1.0);
  const double v=airspeed(),authority=std::clamp(v/(aircraftId>0?60.0:35.0),.12,1.2);
  double rollCommand=c.roll;
  if(assist&&std::abs(c.roll)<.001)rollCommand=std::clamp(-roll*1.8,-1.0,1.0);
  if(!surfaceModel){
  pitchRate+=((c.pitch+trim)*.5*authority*model.controlScale-pitchRate)*std::min(1.0,dt*5);
  rollRate+=(rollCommand*.8*authority*model.controlScale-rollRate)*std::min(1.0,dt*6);
  pitch=std::clamp(pitch+pitchRate*dt,-.7,.7);
  roll=std::clamp(roll+rollRate*dt,-1.2,1.2);
  if(grounded()&&v<model.rotationSpeed*.85){pitch*=std::exp(-2*dt);roll*=std::exp(-4*dt);}
  yaw=wrapAngle(yaw+(c.yaw*(grounded()?.45:.15)+(grounded()?0:beta*.4*authority)+std::tan(roll)*9.81/std::max(v,20.0))*dt);
  }
  const V3 forward{std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
  // Orthonormal body frame matching the renderer: positive roll = right wing down.
  const V3 levelRight{std::cos(yaw),0,-std::sin(yaw)};
  const V3 pitchUp{-std::sin(yaw)*std::sin(pitch),std::cos(pitch),-std::cos(yaw)*std::sin(pitch)};
  const V3 right=levelRight*std::cos(roll)-pitchUp*std::sin(roll);
  const V3 up=forward.cross(right);
  const V3 velocity{vx-windX,vy,vz-windZ},direction=velocity.unit();
  aoa=v>.5?std::atan2(-velocity.dot(up),velocity.dot(forward)):0;
  beta=v>.5?std::asin(std::clamp(velocity.dot(right)/v,-1.,1.)):0;
  const auto coeff=aeroCoefficients(aircraftId,aoa,beta,flap,gear,y-terrain(x,z)+.1*std::sqrt(model.wingArea));
  separation=coeff.separation;stalled=separation>.05&&v>12;stallWarning=coeff.stallWarning&&v>12&&!grounded();
  const double q=.5*atmosphere(y).density*v*v;
  const double lift=q*model.wingArea*coeff.cl,drag=q*model.wingArea*coeff.cd;
  liftForce=lift;dragForce=drag;sideForce=q*model.wingArea*coeff.cy;
  engines.update(aircraftId,dt,engine,throttle,fuel,condition,rpm);
  rpm=engines.averageRpm;
  const double thrust=engines.totalThrust;
  const double mass=model.emptyMass+std::max(0.0,payload)+passengers*85.0;
  const V3 liftDirection=direction.cross(right).unit();
  const V3 sideDirection=liftDirection.cross(direction).unit();
  V3 totalForce=forward*thrust+liftDirection*lift+sideDirection*sideForce-direction*drag;
  if(surfaceModel){
   if(!rotation.initialized)rotation.capture(pitch,roll,yaw,pitchRate,rollRate,yawRate);
   const auto orientation=rotation.orientation;
   SurfaceInput input;
   input.aircraftId=aircraftId;input.bodyAirVelocity=orientation.inverseRotate(velocity);
   input.angularVelocity=rotation.angularVelocity;input.density=atmosphere(y).density;
   input.groundHeight=y-terrain(x,z);input.flap=flap;input.gear=gear;
   input.pitchControl=std::clamp(c.pitch+trim,-1.,1.);input.rollControl=rollCommand;input.yawControl=c.yaw;
   surfaceResult=solveSurfaces(input);bodyForceMoment=surfaceResult.total;
   bodyForceMoment.add(engines.bodyForces);
   // Mechanical friction is not a substitute for aerodynamic damping; local-flow
   // tail/wing samples generate that damping. These small terms stabilize zero flow.
   const auto inertia=approximateInertia(aircraftId,mass);
   const auto omega=rotation.angularVelocity;
   bodyForceMoment.moment+=V3{-omega.x*inertia.x*.08,-omega.y*inertia.y*.08,-omega.z*inertia.z*.08};
   if(grounded()){
    // Runway reaction restrains bank/yaw and keeps the nose supported until rotation.
    bodyForceMoment.moment.z+=roll*inertia.z*8-omega.z*inertia.z*2;
    bodyForceMoment.moment.y+=c.yaw*inertia.y*.3-omega.y*inertia.y*3;
    if(v<model.rotationSpeed*.85)bodyForceMoment.moment.x+=pitch*inertia.x*8-omega.x*inertia.x*2;
   }
   if(!surfaceResult.finite()||!rotation.integrate(bodyForceMoment.moment,inertia,dt)){
    crash("分翼面模型数值超出包线");return;
   }
   totalForce=orientation.rotate(bodyForceMoment.force);
   const auto attitude=rotation.orientation.euler();pitch=attitude.x;roll=attitude.y;yaw=attitude.z;
   pitchRate=-rotation.angularVelocity.x;rollRate=-rotation.angularVelocity.z;yawRate=rotation.angularVelocity.y;
   liftForce=surfaceResult.totalLift;dragForce=surfaceResult.totalDrag;sideForce=bodyForceMoment.force.x;
   separation=surfaceResult.meanSeparation;stalled=separation>.05&&v>12;
  }else{
   // Independent-engine yaw is also observable in the established flight model.
   const double yawAcceleration=engines.bodyForces.moment.y/approximateInertia(aircraftId,mass).y;
   yawRate+=(yawAcceleration-yawRate*.8)*dt;
   yaw=wrapAngle(yaw+yawRate*dt);
  }
  V3 acceleration=totalForce*(1/mass)+V3{0,-9.81,0};
  if(c.parkingBrake&&grounded()&&speed()<.5){vx=vz=0;acceleration.x=acceleration.z=0;}
  vx+=acceleration.x*dt;vy+=acceleration.y*dt;vz+=acceleration.z*dt;
  x+=vx*dt;y+=vy*dt;z+=vz*dt;
  const double floor=terrain(x,z)+model.gearHeight;
  load=liftForce/(mass*9.81);
  const double flexTarget=std::clamp(load*.09,-.15,.3);
  flexRate+=(35*(flexTarget-wingFlex)-9*flexRate)*dt;wingFlex+=flexRate*dt;
  if(y<=floor){
   if(vy<-5)crash("接地下降率过大");
   else if(!gear&&speed()>3)crash("未放起落架着陆");
   else if(std::abs(roll)>.3&&speed()>5)crash("坡度过大，机翼触地");
   else if(!onRunway(x,z)&&speed()>8)crash("高速偏出跑道");
   y=floor;vy=0;
   const double resistance=c.brake?model.brakeRate:.025;
   vx*=std::exp(-resistance*dt);vz*=std::exp(-resistance*dt);
   // Nose wheel suppresses lateral sliding at low speed.
   const double side=vx*std::cos(yaw)-vz*std::sin(yaw);
   const double damp=1-std::exp(-dt*2);
   vx-=side*std::cos(yaw)*damp;vz+=side*std::sin(yaw)*damp;
  }
 }
};
}
