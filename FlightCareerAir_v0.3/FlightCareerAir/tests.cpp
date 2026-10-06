#include "src/storage.h"
#include "src/tutorial.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
void require(bool value,const char* description){if(!value)throw std::runtime_error(description);}
void takeoff(){
 sim::Flight f;sim::Progress career;double maxAltitude=0;
 for(int i=0;i<7500;i++){
  sim::Controls c;c.throttle=f.throttle<1?1:0;
  double target=f.speed()<29?0:.12;
  c.pitch=std::clamp((target-f.pitch)*6,-1.0,1.0);
  f.step(.01,c);career.update(f,.01);maxAltitude=std::max(maxAltitude,f.y);
  require(!f.crashed,"Runway takeoff crashed");
 }
 require(maxAltitude>200&&career.stage==2,"Takeoff and 200m license checkpoint");
 require(std::isfinite(f.x)&&f.speed()>30,"Takeoff numerical stability");
}
void landing(){
 sim::Flight f;f.y=24;f.z=-1200;f.vz=35;f.vy=-1;f.pitch=-.04;f.flap=1;f.throttle=.25;f.rpm=1175;
 bool stopped=false;
 for(int i=0;i<10000;i++){
  sim::Controls c;double targetHeight=1.6+std::max(0.0,(-620-f.z)*.035);
  double vertical=std::clamp((targetHeight-f.y)*.6-1.0,-3.0,1.0);
  double v=std::max(20.0,f.airspeed()),q=.5*1.225*v*v;
  double trim=(1050*9.81/(q*16.2)-.32-.6*f.flap)/4.6;
  c.pitch=std::clamp((std::atan2(vertical,v)+trim-f.pitch)*6,-1.0,1.0);
  c.brake=f.grounded();if(c.brake)c.throttle=-1;
  f.step(.01,c);require(!f.crashed,"Approach and flare crashed");
  if(f.grounded()&&f.speed()<1){stopped=true;break;}
 }
 require(stopped&&sim::onRunway(f.x,f.z),"Runway landing and full stop");
}
void systems(){
 sim::Flight f;for(int i=0;i<1000;i++)f.step(.01,{});
 require(!f.crashed&&f.y==1.6&&f.speed()<.01,"Idle ground state");
 double fuel=f.fuel;f.engine=false;for(int i=0;i<1000;i++)f.step(.01,{});
 require(f.fuel==fuel&&f.rpm<1,"Engine off must not consume fuel");
 sim::Flight impact;impact.y=2;impact.vy=-10;impact.step(.02,{});impact.step(.02,{});require(impact.crashed&&impact.crashReason=="HARD LANDING","Hard landing detection");
 sim::Flight gear;gear.y=2;gear.vy=-2;gear.vz=20;gear.gear=false;for(int i=0;i<100;i++)gear.step(.01,{});require(gear.crashed,"Gear up landing detection");
 sim::Flight bank;bank.y=1000;bank.roll=.4;bank.vz=40;
 for(int i=0;i<1000;i++)bank.step(.01,{});
 require(!bank.crashed&&std::abs(bank.roll)<.01&&std::isfinite(bank.wingFlex)&&std::abs(bank.wingFlex)<.5,"Assisted bank leveling and flex stability");
}
void missions(){
 sim::Progress p;sim::Flight m;
 require(!p.select(2,m)&&!p.buy(m),"Unlicensed mission and fleet lock");
 m.y=40;p.update(m,.01);m.y=210;p.update(m,.01);m.y=1.6;p.update(m,.01);p.update(m,.01);
 require(p.credits==1500&&p.licensed&&p.completed==1,"License reward once");
 for(int type=1;type<=4;type++){
  require(p.select(type,m),"Parked mission selection");m.y=100;p.update(m,.01);
  if(type==4)require(!p.photograph(m),"Photo out of range must fail");
  for(auto wp:p.route){m.x=wp.x;m.z=wp.z;if(type==4)require(p.photograph(m),"Photo checkpoint");else p.update(m,.01);}
  require(p.stage==2,"Mission gates complete");m.x=0;m.z=-500;m.y=1.6;p.update(m,.01);
  int credited=p.credits;p.update(m,.01);require(p.stage==3&&p.credits==credited,"Mission reward guard");
 }
 require(p.completed==5&&p.photos==4&&p.best>0,"Five mission types and race record");
 int old=p.credits;require(p.buy(m)&&p.fleet==2&&p.credits==old-6000,"Fleet purchase accounting");
 m.fuel=5;require(p.service(m)&&m.fuel==100,"Service and refuel");
 m.y=100;require(!p.buy(m)&&!p.select(1,m)&&!p.service(m),"Airborne operations denied");
 std::stringstream legacy;p.save(legacy);sim::Progress loaded;loaded.load(legacy);
 require(loaded.credits==p.credits&&loaded.fleet==2&&loaded.photos==4,"Legacy career migration");
}
void saveState(){
 sim::Snapshot s;s.career.licensed=true;s.career.mission=4;s.career.stage=1;s.career.gate=2;s.career.elapsed=42.5;
 s.flight.x=500;s.flight.y=200;s.flight.z=300;s.flight.vz=35;s.flight.fuel=67.8;s.flight.gear=false;s.season=2;s.clock=18;
 std::stringstream written;s.write(written);sim::Snapshot read;
 require(read.read(written),"Full flight save parse");
 require(read.flight.x==500&&read.flight.fuel==67.8&&read.flight.gear==false&&read.career.gate==2&&read.career.mission==4&&read.season==2,"Full flight save roundtrip");
 std::stringstream corrupt("FCAIR_STATE 3 broken");read.flight.x=100;
 require(!read.read(corrupt)&&read.flight.x==100,"Corrupt save must not partially apply");
 s.career.gate=4;std::stringstream invalid;s.write(invalid);require(!read.read(invalid),"Invalid active gate rejection");
 s.career.gate=2;s.flight.fuel=-10;std::stringstream bounds;s.write(bounds);require(!read.read(bounds),"Invalid fuel rejection");
}
int main(){
 try{systems();takeoff();landing();missions();saveState();std::cout<<"PASS: ground, engine/fuel, impacts, assisted controls, flex, takeoff, 200m climb, approach, touchdown, braking, five missions, reward guard, fleet, photo, legacy migration, full save and corrupt-save rejection\n";return 0;}
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
