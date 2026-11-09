#include "src/ui_state.h"
#include "src/approach.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void park(sim::Flight& flight,int airport){
 flight.x=sim::runways[airport].x;flight.z=sim::runways[airport].z;
 flight.y=flight.spec().gearHeight;flight.vx=flight.vy=flight.vz=0;
}
sim::Snapshot pending(int id,int airport,int mission){
 sim::Snapshot s;s.flight.configure(id);park(s.flight,airport);
 s.career.licensed=true;s.career.credits=12345;s.career.completed=7;
 s.career.fleet=3;s.career.photos=12;s.career.best=96;s.career.maintenance=.9;
 require(s.career.select(mission,s.flight),"Initial mission selected");
 s.headingHold=s.altitudeHold=s.navigationHold=true;s.navigationWaypoint=3;s.targetHeading=1.7;s.targetAltitude=9000;
 s.avionics.qnh=1023;s.panelBrightness=.5;
 s.career.stage=2;s.career.gate=4;s.career.elapsed=123;s.career.raceLap=90;
 s.career.lastFlightScore=87;s.career.arrivalLandingScore=95;
 return s;
}
void clean(const sim::Snapshot& s,int mission,int departure){
 const auto& c=s.career;
 require(c.departureRunway==departure&&c.arrivalRunway==1-departure,
         "New flight airports must match actual spawn, not previous departure");
 require(c.stage==0&&c.gate==0&&c.elapsed==0&&c.raceLap==0,"Active mission progress cleared");
 require(c.lastFlightScore==0&&c.arrivalLandingScore==-1,"Previous flight scores cleared");
 require(c.mission==mission&&c.licensed&&c.completed==7&&c.fleet==3&&c.photos==12&&c.best==96,
         "Career history and selected mission retained");
 std::stringstream saved;s.write(saved);sim::Snapshot restored;
 require(restored.read(saved),"Reset state remains compatible with save format 16");
 require(restored.career.departureRunway==departure&&restored.career.arrivalRunway==1-departure&&
         restored.career.arrivalLandingScore==-1&&restored.career.lastFlightScore==0,
         "Save/load preserves clean flight airports and evidence");
}
void cleanGuidance(const sim::Snapshot& s){
 require(!s.headingHold&&!s.altitudeHold&&!s.navigationHold,"New flight must disengage all autopilot modes");
 require(s.navigationWaypoint==0&&s.targetHeading==s.flight.yaw&&s.targetAltitude==1000,"New flight must discard old navigation leg and targets");
 require(s.avionics.qnh==1023&&s.panelBrightness==.5,"Restart retains avionics preferences");
 std::stringstream saved;s.write(saved);sim::Snapshot restored;
 require(restored.read(saved)&&!restored.headingHold&&!restored.altitudeHold&&!restored.navigationHold&&restored.navigationWaypoint==0&&restored.targetHeading==s.flight.yaw&&restored.targetAltitude==1000,"Save immediately after restart contains clean guidance");
}
void lifecycle(sim::Snapshot s){
 const int credits=s.career.credits,completed=s.career.completed;
 s.flight.y=100;s.career.update(s.flight,.01);
 require(s.career.stage==1,"New flight starts after climb");
 park(s.flight,s.career.departureRunway);s.career.update(s.flight,.01);
 require(s.career.stage==1&&s.career.credits==credits,"Return to departure must not settle passenger flight");
 // Construct arrival states to test mission transitions, not a flown trajectory.
 s.flight.x=sim::runways[s.career.arrivalRunway].x;
 s.flight.z=sim::runways[s.career.arrivalRunway].z-1500;
 s.flight.y=100;s.flight.yaw=0;s.flight.vz=60;
 const auto guidance=sim::approachGuidance(s);
 require(guidance.valid&&guidance.runwayIndex==s.career.arrivalRunway,"Approach targets new destination");
 park(s.flight,s.career.arrivalRunway);
 s.career.recordTouchdown(s.flight,sim::assessLanding(s.flight,-1,0,0));
 require(!ui::setPassengers(s,s.flight.spec().seats),"Cannot inflate passenger count immediately before arrival settlement");
 s.career.update(s.flight,.01);
 require(s.career.stage==3&&s.career.completed==completed+1&&s.career.credits>credits,
         "New destination touchdown and stop settle flight");
 require(s.career.arrivalLandingScore==100,"New touchdown supplies new evidence");
 const int settled=s.career.credits;s.career.update(s.flight,.01);
 require(s.career.credits==settled&&s.career.completed==completed+1,"Settlement happens once");
}
}
int main(){
 try{
  int boardingCases=0;
  for(int id=0;id<sim::aircraftCount;++id)for(int airport=0;airport<2;++airport)for(int stage=0;stage<4;++stage){
   auto s=pending(id,airport,2);s.career.stage=stage;s.career.gate=0;
   std::stringstream saved;s.write(saved);sim::Snapshot restored;
   require(restored.read(saved),"Passenger flight state roundtrip");
   for(auto* state:{&s,&restored}){
    std::stringstream before;state->write(before);
    const bool allowed=id>0&&(stage==0||stage==3);
    require(ui::setPassengers(*state,state->flight.spec().seats)==allowed,"Only preflight/completed passenger flights may adjust payload");
    if(!allowed){std::stringstream after;state->write(after);require(before.str()==after.str(),"Rejected boarding must leave entire snapshot unchanged");}
    else require(state->flight.passengers==state->flight.spec().seats,"Allowed boarding still clamps to seats");
   }
   s.career.mission=1;require(ui::setPassengers(s,0)==(id>0),"Other mission boarding rules preserved");
   ++boardingCases;
  }
  std::cout<<"Passenger boarding: "<<boardingCases<<" aircraft/airport/stage cases plus save roundtrips passed.\n";
  int checks=0;
  for(int id=0;id<sim::aircraftCount;++id)for(int airport=0;airport<2;++airport)for(int mission=0;mission<5;++mission){
   auto restart=pending(id,airport,mission);restart.flight.fuel=45;
   const int pax=restart.flight.passengers;restart.flight.y=300;
   ui::restart(restart);cleanGuidance(restart);clean(restart,mission,0);
   require(restart.flight.aircraftId==id&&restart.flight.passengers==pax&&restart.flight.fuel==45&&
           restart.career.credits==12345&&restart.career.maintenance==.9,"Normal restart preserves payload, fuel and money");
   if(mission==2)lifecycle(restart);
   auto crashed=pending(id,airport,mission);crashed.flight.crashed=true;ui::restart(crashed);cleanGuidance(crashed);clean(crashed,mission,0);
   require(crashed.career.credits==11845&&std::abs(crashed.career.maintenance-.8)<1e-12,"Crash penalties unchanged");
   auto switched=pending(id,airport,mission);
   const int next=(id+1)%sim::aircraftCount;
   require(ui::switchAircraft(switched,next),"Parked aircraft switch allowed");cleanGuidance(switched);clean(switched,mission,0);
   require(switched.flight.aircraftId==next&&switched.career.credits==12345,"Switch preserves career balance");
   if(mission==2)lifecycle(switched);
   auto selected=pending(id,airport,mission);
   require(selected.career.select(mission,selected.flight),"Reselect mission allowed");clean(selected,mission,airport);
   require(selected.navigationWaypoint==3&&selected.headingHold&&selected.altitudeHold&&selected.navigationHold,"Mission selection alone preserves guidance");
   checks+=4;
  }
  auto rejected=pending(2,1,2);rejected.flight.y=300;
  std::stringstream before;rejected.write(before);
  require(!ui::switchAircraft(rejected,3)&&!rejected.career.select(1,rejected.flight),"Airborne changes rejected");
  std::stringstream after;rejected.write(after);require(before.str()==after.str(),"Rejected actions do not mutate state");
  park(rejected.flight,1);before.str("");before.clear();rejected.write(before);
  require(!ui::switchAircraft(rejected,-1)&&!ui::switchAircraft(rejected,sim::aircraftCount)&&
          !rejected.career.select(5,rejected.flight),"Invalid requests rejected");
  after.str("");after.clear();rejected.write(after);require(before.str()==after.str(),"Invalid requests do not reset progress");
  std::cout<<"PASS: "<<checks<<" reset/select cases, 40 passenger lifecycles, save roundtrips and rejected-action immutability\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
