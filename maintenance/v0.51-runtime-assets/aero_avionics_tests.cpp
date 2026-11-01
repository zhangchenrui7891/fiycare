#include "src/autopilot.h"
#include "src/cockpit_controls.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void coefficients(){
 const auto sea=sim::atmosphere(0),high=sim::atmosphere(10000);
 check(std::abs(sea.density-1.225)<.001&&high.density<sea.density*.4,"ISA sea level and altitude density");
 const auto below=sim::atmosphere(10999.999),above=sim::atmosphere(11000.001);
 check(std::abs(below.pressure-above.pressure)<.1,"ISA tropopause continuity");
 for(int id=0;id<4;id++)for(double flap:{0.,.5,1.}){
  double oldCl=0,oldCd=0;
  for(int i=-1200;i<=1200;i++){
   const auto c=sim::aeroCoefficients(id,i*.001,.1,flap,true,500);
   check(std::isfinite(c.cl)&&std::isfinite(c.cd)&&c.cd>0,"Finite nonnegative drag throughout stall sweep");
   if(i>-1200)check(std::abs(c.cl-oldCl)<.02&&std::abs(c.cd-oldCd)<.02,"Continuous positive/negative stall transition");
   oldCl=c.cl;oldCd=c.cd;
  }
  auto peak=sim::aeroCoefficients(id,.29,0,flap,false,500),deep=sim::aeroCoefficients(id,.55,0,flap,false,500);
  check(deep.cl<peak.cl&&deep.cd>peak.cd,"Deep stall loses lift and increases drag");
 }
 auto low=sim::aeroCoefficients(3,.1,0,0,false,1),far=sim::aeroCoefficients(3,.1,0,0,false,1000);
 check(low.cd<far.cd&&low.groundFactor<far.groundFactor,"Ground effect reduces induced drag");
 auto left=sim::aeroCoefficients(0,.1,-.1,0,false,100),right=sim::aeroCoefficients(0,.1,.1,0,false,100);
 check(left.cy>0&&right.cy<0&&std::abs(left.cy+right.cy)<1e-10,"Sideslip restoring force changes sign");
 sim::Flight level;level.y=1000;level.vz=60;level.pitch=.1;level.engine=false;level.gear=false;
 auto banked=level;banked.roll=.5;banked.assist=false;
 level.step(.01,{});banked.step(.01,{});
 check(std::abs(level.aoa-.1)<1e-6&&banked.aoa<level.aoa&&banked.vx>0,"Body axes and banked lift direction");
 auto fast=level;auto slow=level;fast.vx=slow.vx=0;fast.vy=slow.vy=0;fast.vz=120;slow.vz=60;fast.step(.01,{});slow.step(.01,{});
 check(fast.liftForce/slow.liftForce>3.95&&fast.liftForce/slow.liftForce<4.05,"Aerodynamic lift scales with velocity squared");
 sim::Flight still;still.y=2000;still.vx=10;still.vz=80;still.vy=2;still.pitch=.12;
 auto shifted=still;shifted.windX=8;shifted.windZ=-6;shifted.vx+=8;shifted.vz-=6;
 still.step(.01,{});shifted.step(.01,{});
 check(std::abs(still.liftForce-shifted.liftForce)<1e-6&&std::abs(still.dragForce-shifted.dragForce)<1e-6,"Equal relative air motion produces equal aerodynamic forces in wind");
 sim::Flight a;a.y=1000;a.vz=45;auto b=a;b.trim=.3;a.step(.01,{});b.step(.01,{});
 check(b.pitch>a.pitch,"Positive trim produces nose-up control response");
}
void instruments(){
 sim::Flight f;f.y=0;f.vz=100;
 auto sea=sim::airData(f);check(std::abs(sea.ias-100)<.01&&std::abs(sea.altitude)<.01,"Ideal IAS calibration at sea level");
 f.y=5000;auto high=sim::airData(f);check(high.ias<sea.ias&&high.tas==100&&std::abs(high.altitude-5000)<1,"IAS/TAS separate with altitude");
 f.windZ=-20;auto wind=sim::airData(f);check(wind.tas==120&&wind.groundSpeed==100,"Headwind separates TAS and ground speed");
 double y=f.y;check(sim::airData(f,1033.25).altitude>high.altitude&&f.y==y,"QNH adjusts indication without teleporting aircraft");
 f.y=15000;check(std::abs(sim::airData(f).altitude-15000)<1,"Pressure altitude in lower stratosphere");
 sim::Snapshot s;auto& a=s.avionics;
 s.flight.engine=false;s.flight.rpm=0;double original=a.charge;a.step(60,s.flight);
 check(a.powered(s.flight)&&a.charge<original,"Battery supplies avionics with stopped engine");
 a.battery=false;check(!a.powered(s.flight),"Dead bus with battery disconnected and engine stopped");
 s.flight.engine=true;s.flight.rpm=1800;check(a.powered(s.flight)&&a.generating(s.flight),"Generator carries bus without battery");
 a.battery=true;double low=a.charge;a.step(10,s.flight);check(a.charge>low,"Generator recharges connected battery");
 a.master=false;check(a.busPowered(s.flight)&&!a.powered(s.flight),"Avionics master isolates displays from live bus");
 a.master=true;a.generator=false;a.charge=.0011;a.step(10,s.flight);check(!a.powered(s.flight),"Battery exhaustion removes supply");
 s.flight.y=1500;s.flight.vz=90;s.headingHold=s.altitudeHold=s.navigationHold=true;sim::Controls c;
 sim::updateAutopilot(s,c);check(!s.headingHold&&!s.altitudeHold&&!s.navigationHold,"Power failure disconnects all AP channels");
 a.charge=1;a.master=true;s.navigationHold=true;s.flight.x=0;s.flight.z=-500;s.flight.windX=8;c={};sim::updateAutopilot(s,c);
 check(s.targetHeading<0,"Navigation crabs into positive eastward crosswind");
 s.headingHold=s.altitudeHold=s.navigationHold=true;cockpit::apply(s,cockpit::Action::Avionics);
 check(!s.headingHold&&!s.altitudeHold&&!s.navigationHold,"Master switch disconnects immediately");
}
void persistence(){
 sim::Snapshot s;s.avionics={false,true,false,.42,1002.25};s.flight.trim=.15;s.flight.windX=8;s.flight.windZ=-4;s.flight.pitchRate=.1;
 std::stringstream saved;s.write(saved);sim::Snapshot loaded;
 check(loaded.read(saved)&&loaded.avionics.charge==.42&&loaded.avionics.qnh==1002.25&&!loaded.avionics.battery&&!loaded.avionics.master&&loaded.flight.trim==.15&&loaded.flight.windX==8&&loaded.flight.windZ==-4&&loaded.flight.pitchRate==.1,"v14 electrical/airdata/trim/wind state roundtrip");
 auto legacy=saved.str();legacy.replace(0,14,"FCAIR_STATE 13");legacy.erase(legacy.rfind('\n',legacy.size()-2)+1);
 std::stringstream old(legacy);check(loaded.read(old)&&loaded.avionics.powered(loaded.flight)&&loaded.flight.trim==0&&loaded.flight.windX==0,"v13 migration supplies ready-to-fly defaults");
 s.avionics.qnh=900;std::stringstream bad;s.write(bad);loaded.flight.x=123;
 check(!loaded.read(bad)&&loaded.flight.x==123,"Invalid QNH rejected atomically");
 s.avionics.qnh=1013.25;s.flight.windX=31;std::stringstream badWind;s.write(badWind);check(!loaded.read(badWind),"Unsafe wind persistence rejected");
 s.flight.windX=8;std::stringstream valid;s.write(valid);auto truncated=valid.str();truncated.resize(truncated.size()-5);std::stringstream broken(truncated);check(!loaded.read(broken),"Truncated v14 extension rejected");
}
void scenarios(){
 for(int id=0;id<4;id++){
  sim::Snapshot s;auto& f=s.flight;f.configure(id);f.y=1500;f.z=0;f.gear=false;f.vz=id?100:48;f.throttle=id?.3:.65;f.rpm=700+1900*f.throttle;
  s.targetAltitude=1500;s.altitudeHold=s.headingHold=true;s.targetHeading=0;
  double maxLateError=0;
  for(int i=0;i<30000;i++){
   sim::Controls c;sim::updateAutopilot(s,c);f.step(.01,c);s.avionics.step(.01,f);
   check(!f.crashed&&s.altitudeHold&&std::isfinite(f.y)&&f.speed()<300,"Five-minute four-aircraft AP flight remains finite and airborne");
   if(i>20000)maxLateError=std::max(maxLateError,std::abs(f.y-1500));
  }
  check(maxLateError<5,"Five-minute baseline altitude hold settles within five metres");
  std::cout<<"aircraft="<<id<<" duration_s=300 late_max_altitude_error_m="<<maxLateError<<" final_tas_mps="<<f.airspeed()<<'\n';
 }
 for(int i=0;i<int(cockpit::solids(.5).size());i++){
  auto q=cockpit::solids(.5)[i].center;check(cockpit::pick(q,.5)==i,"All physical controls are independently selectable");
 }
 // Looking up makes each overhead switch visible in the actual allowed camera range.
 const float old=cockpit::lookPitch;cockpit::lookPitch=.55f;
 for(int i=15;i<18;i++){
  auto q=cockpit::solids(.5)[i].center;const float up=q.y*std::cos(.55f)-q.z*std::sin(.55f),forward=q.y*std::sin(.55f)+q.z*std::cos(.55f);
  const float y=270*(1-up/forward/.57735027f);check(y>30&&y<440,"Overhead switches reachable within camera pitch and HUD bounds");
 }
 cockpit::lookPitch=old;
}
int main(){try{coefficients();instruments();persistence();scenarios();std::cout<<"PASS: aero/avionics regression and five-minute scenarios\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
