#pragma once
#include "world.h"
#include <algorithm>
#include <cmath>
#include <string>
namespace sim {
constexpr double pi=3.141592653589793;
inline double wrapAngle(double a){return std::remainder(a,2*pi);}
struct V3 {
 double x=0,y=0,z=0;
 V3 operator+(V3 a)const{return {x+a.x,y+a.y,z+a.z};}
 V3 operator-(V3 a)const{return {x-a.x,y-a.y,z-a.z};}
 V3 operator*(double s)const{return {x*s,y*s,z*s};}
 double dot(V3 a)const{return x*a.x+y*a.y+z*a.z;}
 double length()const{return std::sqrt(dot(*this));}
 V3 cross(V3 a)const{return {y*a.z-z*a.y,z*a.x-x*a.z,x*a.y-y*a.x};}
 V3 unit()const{const double n=length();return n>.0001?*this*(1/n):V3{0,0,1};}
};
struct Controls {double pitch=0,roll=0,yaw=0,throttle=0;bool brake=false;};
struct Flight {
 double x=0,y=1.6,z=-500,vx=0,vy=0,vz=0,pitch=0,roll=0,yaw=0,throttle=0;
 double fuel=100,rpm=0,wingFlex=0,flexRate=0,payload=0,condition=1;
 double pitchRate=0,rollRate=0,aoa=0,load=0;
 double windX=0,windZ=0,flap=0,time=0;
 bool engine=true,gear=true,crashed=false,assist=true,stalled=false;
 std::string crashReason;
 double speed()const{return std::sqrt(vx*vx+vy*vy+vz*vz);}
 double airspeed()const{return std::sqrt((vx-windX)*(vx-windX)+vy*vy+(vz-windZ)*(vz-windZ));}
 bool grounded()const{return y<=terrain(x,z)+1.61;}
 void crash(const char* why){crashed=true;crashReason=why;}
 void step(double dt,const Controls& c){
  if(crashed||dt<=0)return;
  dt=std::min(dt,.02);time+=dt;
  throttle=std::clamp(throttle+c.throttle*dt*.5,0.0,1.0);
  const double v=airspeed(),authority=std::clamp(v/35.0,.12,1.2);
  double rollCommand=c.roll;
  if(assist&&std::abs(c.roll)<.001)rollCommand=std::clamp(-roll*1.8,-1.0,1.0);
  pitchRate+=(c.pitch*.5*authority-pitchRate)*std::min(1.0,dt*5);
  rollRate+=(rollCommand*.8*authority-rollRate)*std::min(1.0,dt*6);
  pitch=std::clamp(pitch+pitchRate*dt,-.7,.7);
  roll=std::clamp(roll+rollRate*dt,-1.2,1.2);
  if(grounded()&&v<25){pitch*=std::exp(-2*dt);roll*=std::exp(-4*dt);}
  yaw=wrapAngle(yaw+(c.yaw*(grounded()?.45:.15)+std::tan(roll)*9.81/std::max(v,20.0))*dt);
  const V3 forward{std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
  const V3 right{std::cos(yaw)*std::cos(roll),-std::sin(roll),-std::sin(yaw)*std::cos(roll)};
  const V3 velocity{vx-windX,vy,vz-windZ},direction=velocity.unit();
  const double path=std::atan2(vy,std::max(.1,std::hypot(vx-windX,vz-windZ)));
  aoa=pitch-path;stalled=std::abs(aoa)>.30&&v>12;
  double cl=std::clamp(.32+4.6*aoa+.60*flap,-1.0,1.6);
  if(stalled)cl*=std::max(.18,1-(std::abs(aoa)-.30)*2.5);
  const double density=1.225*std::exp(-std::max(0.0,y)/8500),q=.5*density*v*v;
  const double lift=q*16.2*cl,drag=q*16.2*(.027+.05*cl*cl+.03*flap+(gear?.015:0));
  if(engine&&fuel>0)fuel=std::max(0.0,fuel-(.001+throttle*.008)*dt);
  const double targetRpm=engine&&fuel>0?700+throttle*1900:0;
  rpm+=(targetRpm-rpm)*std::min(1.0,dt*2);
  const double thrust=std::max(0.0,(rpm-700)/1900)*2800*std::clamp(condition,.3,1.0);
  const double mass=1050+std::max(0.0,payload);
  const V3 liftDirection=direction.cross(right).unit();
  V3 acceleration=(forward*thrust+liftDirection*lift-direction*drag)*(1/mass)+V3{0,-9.81,0};
  vx+=acceleration.x*dt;vy+=acceleration.y*dt;vz+=acceleration.z*dt;
  x+=vx*dt;y+=vy*dt;z+=vz*dt;
  const double floor=terrain(x,z)+1.6;
  load=lift/(mass*9.81);
  const double flexTarget=std::clamp(load*.09,-.15,.3);
  flexRate+=(35*(flexTarget-wingFlex)-9*flexRate)*dt;wingFlex+=flexRate*dt;
  if(y<=floor){
   if(vy<-5)crash("HARD LANDING");
   else if(!gear&&speed()>3)crash("GEAR UP LANDING");
   else if(std::abs(roll)>.3&&speed()>5)crash("WING STRIKE");
   else if(!onRunway(x,z)&&speed()>8)crash("OFF RUNWAY");
   y=floor;vy=0;
   const double resistance=c.brake?1.4:.025;
   vx*=std::exp(-resistance*dt);vz*=std::exp(-resistance*dt);
   // Nose wheel suppresses lateral sliding at low speed.
   const double side=vx*std::cos(yaw)-vz*std::sin(yaw);
   const double damp=1-std::exp(-dt*2);
   vx-=side*std::cos(yaw)*damp;vz+=side*std::sin(yaw)*damp;
  }
 }
};
}
