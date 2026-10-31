#include "src/frame_timing.h"
#include "src/performance.h"
#include "src/benchmark.h"
#include "src/cockpit_controls.h"
#include "src/autopilot.h"
#include "src/flight_review.h"
#include "src/approach.h"
#include "src/lighting.h"
#include "src/assets.h"
#include "src/texture_quality.h"
#include "src/storage.h"
#include "src/tutorial.h"
#include "src/ui_state.h"
#include <vector>
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
 sim::Flight impact;impact.y=2;impact.vy=-10;impact.step(.02,{});impact.step(.02,{});require(impact.crashed&&!impact.crashReason.empty(),"Hard landing detection");
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
  if(type==2){m.x=sim::runways[p.arrivalRunway].x;m.z=sim::runways[p.arrivalRunway].z;m.y=m.spec().gearHeight;m.vx=m.vy=m.vz=0;p.update(m,.01);require(p.stage==3&&p.lastFlightScore>0,"Passenger flight settles only at destination");}
  else{for(auto wp:p.route){m.x=wp.x;m.z=wp.z;if(type==4)require(p.photograph(m),"Photo checkpoint");else p.update(m,.01);}require(p.stage==2,"Mission gates complete");m.x=0;m.z=-500;m.y=1.6;p.update(m,.01);}
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
 sim::Flight arrival;arrival.x=sim::runways[1].x;arrival.z=sim::runways[1].z;arrival.y=arrival.spec().gearHeight;
 sim::Progress good;good.licensed=true;good.mission=2;good.stage=1;
 auto rough=good,missing=good;
 good.recordTouchdown(arrival,{true,100,0,0,0,0});rough.recordTouchdown(arrival,{true,20,0,0,0,0});
 good.update(arrival,.01);rough.update(arrival,.01);missing.update(arrival,.01);
 require(good.lastFlightScore-rough.lastFlightScore==40&&missing.lastFlightScore<=50,"Landing quality weights half of flight score");
 std::ostringstream report;good.writeFlightReport(report,arrival,good.credits);
 require(report.str().find("landing_points_of_50=50")!=std::string::npos&&report.str().find("touchdown_recorded=yes")!=std::string::npos,"Report itemizes touchdown evidence and points");
 std::ostringstream absent;missing.writeFlightReport(absent,arrival,missing.credits);
 require(absent.str().find("touchdown_recorded=no")!=std::string::npos&&absent.str().find("landing_points_of_50=0")!=std::string::npos,"Missing evidence is explicit in report");
 require(good.lastFlightScore==int(std::lround(good.timePoints()+good.fuelPoints(arrival)+good.landingPoints())),"Report and settlement share scoring formula");
 auto pending=good;pending.stage=1;std::ostringstream empty;pending.writeFlightReport(empty,arrival,0);require(empty.str().empty(),"No report before settlement");
 auto wrong=sim::Progress{};wrong.mission=2;wrong.stage=1;wrong.recordTouchdown(sim::Flight{},{true,100,0,0,0,0});require(wrong.arrivalLandingScore==-1,"Departure touchdown cannot contaminate arrival score");
 rough.arrivalRunway=5;rough.stage=1;rough.update(arrival,.01);require(rough.stage==1,"Invalid arrival index safely rejected");
 sim::Snapshot s;s.career.licensed=true;s.career.mission=4;s.career.stage=1;s.career.gate=2;s.career.elapsed=42.5;
 s.career.arrivalLandingScore=83;
 s.flight.x=500;s.flight.y=200;s.flight.z=300;s.flight.vz=35;s.flight.fuel=67.8;s.flight.gear=false;s.season=2;s.clock=18;
 std::stringstream written;s.write(written);sim::Snapshot read;
 require(read.read(written),"Full flight save parse");
 require(read.flight.x==500&&read.flight.fuel==67.8&&read.flight.gear==false&&read.career.gate==2&&read.career.mission==4&&read.season==2,"Full flight save roundtrip");
 require(read.career.departureRunway==0&&read.career.arrivalRunway==1,"v11 airport pair save roundtrip");
 require(read.career.arrivalLandingScore==83,"v12 touchdown score persistence");
 std::string oldSave=written.str();oldSave.replace(0,14,"FCAIR_STATE 11");oldSave=oldSave.substr(0,oldSave.find("83\n"));
 std::stringstream oldStream(oldSave);sim::Snapshot oldLoaded;require(oldLoaded.read(oldStream)&&oldLoaded.career.arrivalLandingScore==-1,"v11 missing touchdown migration");
 std::stringstream corrupt("FCAIR_STATE 3 broken");read.flight.x=100;
 require(!read.read(corrupt)&&read.flight.x==100,"Corrupt save must not partially apply");
 s.career.gate=4;std::stringstream invalid;s.write(invalid);require(!read.read(invalid),"Invalid active gate rejection");
 s.career.gate=2;s.flight.fuel=-10;std::stringstream bounds;s.write(bounds);require(!read.read(bounds),"Invalid fuel rejection");
}
void airliner(){
 sim::Flight f;f.configure(1);f.flap=.5;sim::Progress career;
 for(int i=0;i<5500;i++){
  sim::Controls c;c.throttle=f.throttle<1?1:0;double target=f.speed()<f.spec().rotationSpeed?0:.15;
  c.pitch=std::clamp((target-f.pitch)*6,-1.0,1.0);f.step(.01,c);career.update(f,.01);
  require(!f.crashed,"Airliner runway takeoff");
 }
 require(f.y>200&&career.stage==2,"Airliner climb and license checkpoint");
 sim::Flight landing;landing.configure(1);landing.y=48;landing.z=-2500;landing.vz=62;landing.vy=-2.4;landing.pitch=.02;landing.flap=1;landing.throttle=.4;landing.rpm=1460;
 bool stopped=false;
 for(int i=0;i<10000;i++){
  sim::Controls c;double targetH=landing.spec().gearHeight+std::max(0.0,(-1400-landing.z)*.04);
  double vertical=std::clamp((targetH-landing.y)*.5-1.5,-3.5,1.0),v=std::max(50.0,landing.airspeed()),q=.5*1.225*v*v;
  double mass=landing.spec().emptyMass+landing.passengers*85;
  double trim=(mass*9.81/(q*landing.spec().wingArea)-.32-landing.spec().flapLift*landing.flap)/4.6;
  c.pitch=std::clamp((std::atan2(vertical,v)+trim-landing.pitch)*6,-1.0,1.0);c.brake=landing.grounded();if(c.brake)c.throttle=-1;
  landing.step(.01,c);require(!landing.crashed,"Airliner approach impact");
  if(landing.grounded()&&landing.speed()<1){stopped=true;break;}
 }
 require(stopped&&sim::onRunway(landing.x,landing.z),"Airliner landing and braking");
 sim::Flight empty,full;empty.configure(1);full.configure(1);empty.passengers=0;full.passengers=70;
 sim::Controls power;power.throttle=1;
 for(int i=0;i<1500;i++){empty.step(.01,power);full.step(.01,power);}
 require(empty.speed()>full.speed()+1,"Passenger payload must affect acceleration");
 sim::Snapshot snapshot;snapshot.flight=f;std::stringstream stored;snapshot.write(stored);sim::Snapshot restored;
 require(restored.read(stored)&&restored.flight.aircraftId==1&&restored.flight.passengers==50,"Airliner and passengers persisted");
}
void menuActions(){
 sim::Snapshot s;s.career.credits=9000;s.career.licensed=true;
 require(ui::switchAircraft(s,1)&&s.flight.aircraftId==1&&s.flight.y==3.8&&s.career.credits==9000,"Parked aircraft selection preserves career");
 require(ui::setPassengers(s,80)&&s.flight.passengers==70,"Passenger upper bound");
 require(ui::setPassengers(s,-10)&&s.flight.passengers==0,"Passenger lower bound");
 s.flight.y=200;require(!ui::switchAircraft(s,0)&&!ui::setPassengers(s,50),"No switching aircraft or boarding in flight");
 s.flight.fuel=45;s.flight.passengers=20;ui::restart(s);
 require(s.flight.aircraftId==1&&s.flight.passengers==20&&s.flight.fuel==45&&s.career.credits==9000,"Restart preserves aircraft and payload");
 sim::Snapshot old;std::stringstream current;old.write(current);std::vector<std::string> lines;std::string line;
 while(std::getline(current,line))lines.push_back(line);
 lines[0]="FCAIR_STATE 3";std::istringstream flightLine(lines[4]);std::ostringstream truncated;std::string token;
 for(int i=0;i<17;i++){flightLine>>token;truncated<<token<<' ';}lines[4]=truncated.str();
 std::stringstream v3;for(auto& l:lines)v3<<l<<'\n';sim::Snapshot migrated;
 require(migrated.read(v3)&&migrated.flight.aircraftId==0,"v3 full-save compatibility");
 s.career.mission=2;s.career.stage=1;s.career.departureRunway=0;s.career.arrivalRunway=1;s.flight.x=sim::runways[1].x;s.flight.z=sim::runways[1].z;s.flight.y=s.flight.spec().gearHeight;s.flight.vx=s.flight.vy=s.flight.vz=0;s.flight.passengers=50;s.career.update(s.flight,.01);
 require(s.career.stage==3&&s.career.credits>9000+1800+1500&&s.career.lastFlightScore>0,"Passenger destination and quality settlement");
}
void widebodyAndMap(){
 for(int id:{2,3}){
  sim::Snapshot s;require(ui::switchAircraft(s,id),"Widebody selection");require(ui::setPassengers(s,999)&&s.flight.passengers==s.flight.spec().seats,"Widebody capacity clamp");s.flight.flap=.5;
  for(int i=0;i<7500;i++){sim::Controls c;c.throttle=s.flight.throttle<1?1:0;double target=s.flight.speed()<s.flight.spec().rotationSpeed?0:.15;c.pitch=std::clamp(6*(target-s.flight.pitch),-1.0,1.0);s.flight.step(.01,c);require(!s.flight.crashed,"Widebody full-load takeoff");}
  require(s.flight.y>200,"Widebody climb checkpoint");s.terrainMode=0;std::stringstream save;s.write(save);sim::Snapshot loaded;require(loaded.read(save)&&loaded.flight.aircraftId==id&&loaded.flight.passengers==s.flight.spec().seats&&loaded.terrainMode==0,"Widebody/map save roundtrip");
 }
 auto center=sim::satelliteUV(0,0),east=sim::satelliteUV(1000,0),north=sim::satelliteUV(0,1000);require(center.covered&&center.u>.24f&&center.u<.26f&&center.v>.24f&&center.v<.26f,"Satellite origin projection");
 require(east.u>center.u&&north.u>center.u&&north.v<center.v,"Satellite runway heading and north-up mapping");require(!sim::satelliteUV(100000,100000).covered,"Satellite coverage guard");
 require(sim::terrain(0,-1600,0)==0&&sim::terrain(0,-1600,1)==0,"Both modes preserve runway surface");require(sim::terrain(2600,1000,0)>400&&sim::terrain(2600,1000,1)<2,"Mountain mode separate from Pudong");
 std::stringstream v4;v4<<"FCAIR_STATE 4\nFCAIR 2 0 0 0 1 0 0 1\n0 0 0 0 0\n0 1.6 -500 0 0 0 0 0 0 0 100 700 0 1 1 0 1 0 0\n12 0 0 1\n";sim::Snapshot migrated;require(migrated.read(v4)&&migrated.terrainMode==1,"v4 save migration");
}
void assetsCheck(const std::filesystem::path& folder){
 auto a=loadAtlas(folder/"atlas.fctex");require(a.width==2048&&a.height==2048,"Atlas dimensions");
 const auto materials=buildMaterialTextures(a);require(materials.mipCount==11&&materials.subresources.size()==55,"Runtime atlas produces five complete 1024-to-1 mip chains");
 for(const auto& image:materials.subresources)require(image.rgba.size()==size_t(image.width)*image.height*4,"All runtime subresource payloads match dimensions");
 for(const auto& name:{"a330.fcmesh","a350.fcmesh"}){auto m=loadModel(folder/name);require(m.vertices.size()>30000,"Imported model geometry");float low=1000,high=-1000;for(auto v:m.vertices){low=std::min(low,v.z);high=std::max(high,v.z);}require(high-low>63&&high-low<68,"Aircraft metre scale");float maxY=-1000,tailZ=0;for(auto v:m.vertices)if(v.y>maxY){maxY=v.y;tailZ=v.z;}require(tailZ<0,"Imported aircraft nose points forward, tail aft");}
 auto a350=loadModel(folder/"a350.fcmesh");size_t gears=0;for(auto f:a350.gear)gears+=f;require(gears>0&&gears<a350.gear.size(),"A350 independent gear tags");
 auto tmp=std::filesystem::temp_directory_path()/"fcair_bad_asset.fctex";{std::ofstream out(tmp,std::ios::binary);out<<"FCTEX005";uint32_t size=1;out.write(reinterpret_cast<const char*>(&size),4);out.write(reinterpret_cast<const char*>(&size),4);}
 bool rejected=false;try{loadAtlas(tmp);}catch(const std::runtime_error&){rejected=true;}std::filesystem::remove(tmp);require(rejected,"Truncated texture rejected");
}
void lightingChecks(){
 static_assert(sizeof(LightingFrame)==176,"GPU lighting register layout");
 for(double hour:{0.,6.,12.,17.4,18.,23.9}){auto f=makeLighting(hour,0,.00016,true);double length=f.sun[0]*f.sun[0]+f.sun[1]*f.sun[1]+f.sun[2]*f.sun[2];require(std::abs(length-1)<.0001,"Unit sun direction");for(auto v:f.sunColor)require(std::isfinite(v)&&v>=0,"Finite nonnegative sunlight");}
 require(makeLighting(12,0,.00016,true).sunColor[0]>makeLighting(0,0,.00016,true).sunColor[0],"Day brighter than night");require(makeLighting(12,1,.00016,false).options[0]==2&&makeLighting(12,0,.00016,false).options[3]==0,"Exposure and shadow switch");
 sim::Snapshot s;s.exposureEV=.75;s.haze=.00032;s.shadows=false;std::stringstream save;s.write(save);sim::Snapshot loaded;require(loaded.read(save)&&loaded.exposureEV==.75&&loaded.haze==.00032&&!loaded.shadows,"Lighting settings persisted");
 s.exposureEV=3;std::stringstream bad;s.write(bad);require(!loaded.read(bad),"Invalid exposure rejected");
 std::stringstream legacy;legacy<<"FCAIR_STATE 5\nFCAIR 2 0 0 0 1 0 0 1\n0 0 0 0 0\n0 1.6 -500 0 0 0 0 0 0 0 100 700 0 1 1 0 1 0 0\n12 0 0 1\n";require(loaded.read(legacy)&&loaded.exposureEV==0&&loaded.shadows,"v5 save migration defaults");
}
void cabinLightingChecks(){
 sim::Snapshot s;double original=s.panelBrightness;
 cockpit::apply(s,cockpit::Action::CockpitFlood);require(!s.cockpitFlood,"Flood knob switches off");
 cockpit::apply(s,cockpit::Action::PanelBrightness);require(s.panelBrightness==1,"Dimmer reaches maximum");
 cockpit::apply(s,cockpit::Action::PanelBrightness);require(s.panelBrightness==0,"Dimmer wraps to minimum");
 for(int i=0;i<3;i++)cockpit::apply(s,cockpit::Action::PanelBrightness);
 require(s.panelBrightness==original,"Dimmer cycle restores setting");
 auto night=makeLighting(0,0,.00016,true,.25,false),lit=makeLighting(0,0,.00016,true,1,true);
 require(night.sun[3]==.25f&&night.horizon[3]==0&&lit.sun[3]==1&&lit.horizon[3]==1,"GPU frame carries independent dimmer and flood");
 for(int i=0;i<3;i++)require(night.sunColor[i]==lit.sunColor[i]&&night.horizon[i]==lit.horizon[i],"Cabin switches preserve outdoor lighting");
 require(makeLighting(0,0,.00016,true,-5).sun[3]==0&&makeLighting(0,0,.00016,true,8).sun[3]==1,"Dimmer uniform clamps input");
 std::stringstream stored;s.write(stored);sim::Snapshot loaded;
 require(loaded.read(stored)&&loaded.panelBrightness==.75&&!loaded.cockpitFlood,"v13 cabin settings roundtrip");
 std::string legacy=stored.str();legacy.replace(0,14,"FCAIR_STATE 12");legacy.erase(legacy.rfind("0.75 0\n"));
 std::stringstream v12(legacy);require(loaded.read(v12)&&loaded.panelBrightness==.75&&loaded.cockpitFlood,"v12 migration has readable cabin defaults");
 for(double bad:{-1.,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
  s.panelBrightness=bad;std::stringstream invalid;s.write(invalid);loaded.panelBrightness=.5;
  require(!loaded.read(invalid)&&loaded.panelBrightness==.5,"Invalid dimmer cannot partially mutate saved state");
 }
 s.panelBrightness=.25;std::stringstream valid;s.write(valid);std::string broken=valid.str();broken.erase(broken.rfind("0.25 0\n"));
 std::stringstream truncated(broken);require(!loaded.read(truncated),"Truncated v13 lighting tail rejected");
}
void cockpitChecks(){
 sim::Snapshot s;
 cockpit::apply(s,cockpit::Action::Engine);require(!s.flight.engine,"Clickable engine switch");s.flight.configure(3);cockpit::apply(s,cockpit::Action::Flaps);require(s.flight.flap==.25,"Jet flap first detent");for(int i=0;i<3;i++)cockpit::apply(s,cockpit::Action::Flaps);require(s.flight.flap==0,"Jet flap detents wrap");cockpit::apply(s,cockpit::Action::Gear);require(!s.flight.gear,"Gear switch changes simulation");
 cockpit::apply(s,cockpit::Action::ParkingBrake);s.flight.gear=true;s.flight.engine=true;s.flight.throttle=1;sim::Controls c;c.brake=c.parkingBrake=s.parkingBrake;double z=s.flight.z;for(int i=0;i<2000;i++)s.flight.step(.01,c);require(!s.flight.crashed&&s.flight.z==z&&s.flight.speed()<.01,"Parking brake holds stationary airplane at full power");
 std::stringstream written;s.write(written);sim::Snapshot loaded;require(loaded.read(written)&&loaded.parkingBrake&&loaded.flight.throttle==1&&loaded.flight.gear,"Cockpit state save roundtrip");
 ui::restart(loaded);require(!loaded.parkingBrake,"Restart releases parking brake");
 std::stringstream v6;v6<<"FCAIR_STATE 6\nFCAIR 2 0 0 0 1 0 0 1\n0 0 0 0 0\n0 1.6 -500 0 0 0 0 0 0 0 100 700 0 1 1 0 1 0 0\n12 0 0 1 0 .00016 1\n";require(loaded.read(v6)&&!loaded.parkingBrake,"v6 migration defaults parking brake off");

}
void spatialChecks(){
 for(size_t i=0;i<cockpit::solids(.5).size();i++){auto c=cockpit::solids(.5)[i].center;require(cockpit::pick(c,.5)==int(i),"3D control ray selects its physical switch");}
 require(cockpit::pick({0,1,1},.5)==-1&&cockpit::pick({0,0,-1},.5)==-1,"Sky and backward rays miss cockpit controls");
 const float oldYaw=cockpit::lookYaw,oldPitch=cockpit::lookPitch;cockpit::lookYaw=.3f;cockpit::lookPitch=-.5f;
 auto target=cockpit::solids(.5)[2].center;float sy=std::sin(cockpit::lookYaw),cy=std::cos(cockpit::lookYaw),sp=std::sin(cockpit::lookPitch),cp=std::cos(cockpit::lookPitch);
 float right=target.x*cy-target.z*sy,up=-target.x*sy*sp+target.y*cp-target.z*cy*sp,forward=target.x*sy*cp+target.y*sp+target.z*cy*cp;
 require(cockpit::pick(cockpit::ray(480*(1+right/forward/(.57735027f*960/540)),270*(1-up/forward/.57735027f)),.5)==2,"Picking follows rotated cockpit view");for(float aspect:{4.f/3,16.f/9,21.f/9}){cockpit::viewportAspect=aspect;require(cockpit::pick(cockpit::ray(480*(1+right/forward/(.57735027f*aspect)),270*(1-up/forward/.57735027f)),.5)==2,"Control picking at resized viewport aspect");}
 for(float aspect:{4.f/3,16.f/9,21.f/9})for(size_t i=0;i<cockpit::solids(.5).size();i++){
  auto q=cockpit::solids(.5)[i].center;
  float r=q.x*cy-q.z*sy,u=-q.x*sy*sp+q.y*cp-q.z*cy*sp,f=q.x*sy*cp+q.y*sp+q.z*cy*cp;
  cockpit::viewportAspect=aspect;
  require(cockpit::pick(cockpit::ray(480*(1+r/f/(.57735027f*aspect)),270*(1-u/f/.57735027f)),.5)==int(i),"All 15 controls project and pick across viewport aspects");
 }
 cockpit::viewportAspect=960.f/540;cockpit::lookYaw=oldYaw;cockpit::lookPitch=oldPitch;
 cockpit::SpatialInput input;input.enabled=true;input.throttle=.5;input.dragging=true;input.move(-1000);require(input.throttle==1,"Spatial throttle clamps to full");input.move(1000);require(input.throttle==0,"Spatial throttle clamps to idle");input.cancel();require(input.pending.empty()&&!input.dragging,"Spatial input cancel clears capture");
 sim::Snapshot s;s.antiAlias=false;std::stringstream out;s.write(out);sim::Snapshot loaded;require(loaded.read(out)&&!loaded.antiAlias,"v8 AA setting persists");
 std::stringstream old;old<<"FCAIR_STATE 7\nFCAIR 2 0 0 0 1 0 0 1\n0 0 0 0 0\n0 1.6 -500 0 0 0 0 0 0 0 100 700 0 1 1 0 1 0 0\n12 0 0 1 0 .00016 1 1\n";require(loaded.read(old)&&loaded.antiAlias&&loaded.parkingBrake,"v7 migration defaults FXAA on");
}
void autopilotCheck(){
 sim::Snapshot s;s.flight.configure(3);s.flight.y=1500;s.flight.vz=90;s.flight.yaw=.4;
 cockpit::apply(s,cockpit::Action::HeadingHold);cockpit::apply(s,cockpit::Action::AltitudeHold);
 require(s.headingHold&&s.altitudeHold&&std::abs(s.targetHeading-.4)<1e-9&&s.targetAltitude==1500,"Autopilot captures present heading and altitude");
 cockpit::apply(s,cockpit::Action::HeadingPlus);cockpit::apply(s,cockpit::Action::AltitudePlus);
 require(std::abs(s.targetHeading-(.4+10*sim::pi/180))<1e-9&&s.targetAltitude==1600,"Autopilot selectors change targets");
 sim::Controls c;sim::updateAutopilot(s,c);require(c.roll>0&&c.pitch>0,"Autopilot commands bank and climb toward selected targets");
 c={};c.roll=-1;sim::updateAutopilot(s,c);require(!s.headingHold&&s.altitudeHold,"Manual roll disconnects heading hold only");
 c={};c.pitch=1;sim::updateAutopilot(s,c);require(!s.altitudeHold,"Manual pitch disconnects altitude hold");
 s.headingHold=s.altitudeHold=true;s.flight.y=s.flight.spec().gearHeight;s.flight.vz=0;c={};sim::updateAutopilot(s,c);require(!s.headingHold&&!s.altitudeHold,"Autopilot disconnects on ground or low airspeed");
 s.flight.y=1500;s.flight.vz=90;s.targetHeading=.2;s.targetAltitude=2200;s.headingHold=s.altitudeHold=true;std::stringstream out;s.write(out);sim::Snapshot loaded;require(loaded.read(out)&&loaded.headingHold&&loaded.altitudeHold&&loaded.targetHeading==.2&&loaded.targetAltitude==2200,"v9 autopilot save roundtrip");
 s.flight.x=0;s.flight.z=200;s.flight.yaw=1;s.navigationWaypoint=0;cockpit::apply(s,cockpit::Action::NavigationHold);c={};sim::updateAutopilot(s,c);require(s.navigationHold&&s.headingHold&&std::abs(s.targetHeading)<1e-9&&c.roll<0,"Navigation steers toward active waypoint");
 require(std::abs(sim::navigationCrossTrack(s))<1e-9,"Aircraft on first leg has zero cross-track error");
 s.flight.x=120;require(sim::navigationCrossTrack(s)>119&&sim::navigationCrossTrack(s)<121,"Cross-track sign and metres on northbound leg");s.flight.x=0;
 s.flight.z=2200;c={};sim::updateAutopilot(s,c);require(s.navigationWaypoint==1&&s.targetHeading>0,"Navigation sequences waypoint inside capture radius");
 std::stringstream navSave;s.write(navSave);require(loaded.read(navSave)&&loaded.navigationHold&&loaded.navigationWaypoint==1,"v10 navigation state roundtrip");
 std::stringstream old;old<<"FCAIR_STATE 8\nFCAIR 2 0 0 0 1 0 0 1\n0 0 0 0 0\n0 1.6 -500 0 0 0 0 0 0 0 100 700 0 1 1 0 1 0 0\n12 0 0 1 0 .00016 1 0 1\n";require(loaded.read(old)&&!loaded.headingHold&&!loaded.altitudeHold&&loaded.targetAltitude==1000,"v8 migration defaults autopilot off");
}
void flightReviewCheck(){
 sim::Flight f;f.x=2;f.z=0;f.y=f.spec().gearHeight;
 auto smooth=sim::assessLanding(f,-1.4,.02,.03);require(smooth.valid&&smooth.score>=90&&std::string("优秀").size()>0,"Centred gentle landing scores excellent");
 auto rough=sim::assessLanding(f,-4.5,.2,.25);require(rough.valid&&rough.score<smooth.score&&rough.sinkRate==4.5,"Hard misaligned landing loses points");
 f.x=100;require(!sim::assessLanding(f,-1,0,0).valid,"Off-runway touchdown is not scored");
 f.x=0;f.crashed=true;require(!sim::assessLanding(f,-6,0,0).valid,"Crashed touchdown is not scored");
 f.crashed=false;f.x=3504;f.z=2500;auto east=sim::assessLanding(f,-1.5,.01,.02);require(east.valid&&east.centerlineError==4,"Second runway touchdown uses its own centreline");
}
void approachCheck(){
 sim::Flight f;f.configure(3);f.x=0;f.z=-5000;auto initial=sim::approachGuidance(f);f.y=initial.desiredAltitude;
 auto centred=sim::approachGuidance(f);require(centred.valid&&std::abs(centred.localizer)<1e-9&&std::abs(centred.glideslope)<1e-9,"PVG localizer and three-degree glide path centre");
 f.x=120;f.y+=40;auto highRight=sim::approachGuidance(f);require(highRight.localizer>119&&highRight.glideslope>39,"Approach deviations preserve right/high signs");
 f.x=3500;f.z=500;f.y=100;auto east=sim::approachGuidance(f);require(east.runway==&sim::runways[1]&&east.valid,"Nearest-runway guidance selects East Sea training airport");
 f.y=2000;require(!sim::approachGuidance(f).valid,"Guidance hides outside approach envelope");
}
void timingCheck(){
 timing::FixedSteps t;int steps=0;
 for(int i=0;i<60;++i)steps+=t.advance(1.0/60,true);
 require(steps==100,"60 FPS produces 100 physics steps per second");
 t.reset();require(t.advance(5,true)==8,"Long stall has bounded catch-up");
 require(t.remainder<.01,"Long stall leaves no simulation backlog");
 require(t.advance(.005,true)==0,"Fractional step retained");
 require(t.advance(3,false)==0&&t.remainder==0,"Pause discards pending time");
 require(t.advance(.005,true)==0,"Resume does not inherit old remainder");
 require(t.advance(std::numeric_limits<double>::quiet_NaN(),true)==0,"Invalid time rejected");
}
void performanceCheck(){
 timing::FrameWindow w;require(w.summary().samples==0,"Empty telemetry");
 for(int i=0;i<100;++i)w.add(i<95?.02:.1);
 auto s=w.summary();require(std::abs(s.fps-100/2.4)<1e-9,"FPS uses total wall time");
 require(s.p95Ms==20&&s.maxMs==100,"Nearest rank P95 and spike preserved");
 w.add(5);require(w.summary().maxMs==5000,"Long frames not clamped");
 for(int i=0;i<300;++i)w.add(.05);
 s=w.summary();require(s.samples==300&&std::abs(s.fps-20)<1e-9&&s.maxMs==50,"Bounded ring expires old spikes");
 w.add(0);w.add(-1);w.add(std::numeric_limits<double>::quiet_NaN());w.add(std::numeric_limits<double>::infinity());
 require(w.summary().samples==300,"Invalid telemetry rejected");
 w.reset();require(w.summary().samples==0,"Focus transition resets window");
}
void benchmarkCheck(){
 timing::BenchmarkSamples run;timing::FrameWindow rolling;
 for(int i=0;i<600;++i){run.add(.1);rolling.add(.1);}
 for(int i=0;i<300;++i){run.add(.02);rolling.add(.02);}
 require(run.summary().samples==900&&run.summary().p95Ms==100&&rolling.summary().p95Ms==20,"Full benchmark retains early stutter beyond HUD window");
 require(!timing::assessBenchmark(run.elapsed(),run.summary()).target20,"Early stutter cannot be hidden by smooth ending");
 auto count=run.summary().samples;run.add(0);run.add(-1);run.add(NAN);run.add(INFINITY);require(run.storedSamples()==count&&!run.valid()&&run.summary().samples==0,"Benchmark invalid intervals invalidate the report without adding stored samples");
 timing::BenchmarkSamples bounded;for(int i=0;i<65537;++i)bounded.add(.001);require(!bounded.valid()&&bounded.storedSamples()==65536&&bounded.summary().samples==0,"Benchmark sample cap exposes no partial-run evidence");
 timing::FrameStats good30{31,32,40,300};auto a=timing::assessBenchmark(60,good30);require(a.complete&&a.target20&&a.target30,"30 FPS benchmark passes");
 timing::FrameStats stutter{40,55,70,300};auto b=timing::assessBenchmark(60,stutter);require(!b.target20&&!b.target30,"P95 stutter fails target despite average");
 timing::FrameStats good20{22,48,60,300};auto c=timing::assessBenchmark(60,good20);require(c.target20&&!c.target30,"20 FPS tier classified");
 require(!timing::assessBenchmark(59,good30).complete,"Short benchmark rejected");
}
void textureQualityChecks(){
 AtlasImage checker{2,2,{0,0,0,255,255,255,255,255,255,255,255,255,0,0,0,255}};
 auto chain=textureMips(checker);require(chain.size()==2&&chain[1].width==1&&chain[1].height==1,"Mips reach 1x1");
 require(chain[1].rgba[0]>=185&&chain[1].rgba[0]<=187&&chain[1].rgba[3]==255,"Gamma-aware checker mip retains brightness");
 AtlasImage odd{3,1,{0,0,0,255,0,0,0,255,255,255,255,255}};
 auto oddChain=textureMips(odd);require(oddChain.back().rgba[0]>=154&&oddChain.back().rgba[0]<=156,"Odd dimension mip retains final column");
 AtlasImage a;a.width=a.height=8;a.rgba.resize(8*8*4,255);
 const unsigned char colors[5][4]={{240,10,20,255},{30,230,40,255},{50,60,220,255},{190,180,70,255},{80,170,160,255}};
 for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){int layer=y<4?(x<4?0:1):x<4?2:x<6?3:4;std::copy_n(colors[layer],4,a.rgba.data()+(y*8+x)*4);}
 auto materials=buildMaterialTextures(a);require(materials.mipCount==3&&materials.subresources.size()==15,"D3D slice-major mip layout");
 for(size_t slice=0;slice<5;slice++)for(size_t mip=0;mip<materials.mipCount;mip++){
  const auto& image=materials.subresources[slice*materials.mipCount+mip];require(image.width==(4u>>mip)&&image.height==image.width,"Array slices have equal mip dimensions");
  for(size_t i=0;i<image.rgba.size();i++)require(image.rgba[i]==colors[slice][i%4],"Every material mip remains isolated without atlas color bleed");
 }
 bool rejected=false;try{buildMaterialTextures(odd);}catch(const std::runtime_error&){rejected=true;}require(rejected,"Malformed material atlas rejected");
 rejected=false;try{textureMips({2,2,{0}});}catch(const std::runtime_error&){rejected=true;}require(rejected,"Truncated texture rejected");
}
int main(int argc,char** argv){
 performanceCheck();benchmarkCheck();timingCheck();textureQualityChecks();
 try{systems();takeoff();landing();missions();saveState();airliner();menuActions();widebodyAndMap();lightingChecks();cabinLightingChecks();cockpitChecks();spatialChecks();autopilotCheck();flightReviewCheck();approachCheck();if(argc>1)assetsCheck(argv[1]);std::cout<<"PASS: ground, engine/fuel, impacts, assisted controls, flex, takeoff, landing, missions, saves, fleet, assets, lighting, cockpit, FXAA persistence, fixed timing, performance benchmark, heading, altitude, waypoint navigation, cross-track, dual runway, ILS-style guidance, landing review, isolated material mipmaps and gamma-aware filtering\n";return 0;}
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
