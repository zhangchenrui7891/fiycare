#include "src/approach.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void close(double a,double b,const char* message){require(std::abs(a-b)<1e-8,message);}
std::string saved(const sim::Snapshot& state){std::ostringstream out;state.write(out);return out.str();}

// Exact v0.33 geometry excerpt for deterministic regression, not Windows evidence.
sim::ApproachGuidance legacy(const sim::Flight& f){
 const auto& r=sim::nearestRunway(f.x,f.z);sim::ApproachGuidance g;g.runway=&r;
 g.distance=std::abs(f.z-r.z);g.localizer=f.x-r.x;
 g.desiredAltitude=std::tan(3*sim::pi/180)*std::max(0.0,g.distance-80)+f.spec().gearHeight;
 g.glideslope=f.y-g.desiredAltitude;
 g.valid=g.distance<=12000&&std::abs(g.localizer)<=2500&&f.y<=1500;
 return g;
}
sim::Flight inbound(int airport,int direction,double distance){
 sim::Flight f;f.configure(3);const auto& r=sim::runways[airport];
 f.x=r.x;f.z=r.z-direction*r.halfLength-direction*distance;
 f.yaw=direction==1?0:sim::pi;f.vz=direction*90.;
 f.y=f.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*distance;
 return f;
}
void legacyDefects(){
 auto f=inbound(0,1,0);const auto old=legacy(f),now=sim::approachGuidance(f,0);
 require(old.distance==1800&&now.distance==0,"Legacy measures from centre, new measures from threshold");
 require(old.desiredAltitude-now.desiredAltitude>74,"Legacy path at threshold differs by over 74m from chosen training clearance");
 std::cout<<"Legacy PVG threshold: centre-distance="<<old.distance<<"m, path delta="
          <<old.desiredAltitude-now.desiredAltitude<<"m relative to chosen training clearance.\n";
 f=inbound(0,-1,1000);f.x=120;
 require(legacy(f).localizer>0&&sim::approachGuidance(f,0).localizer<0,"Reciprocal approach fixes aircraft-right sign");
 f=inbound(0,1,1000);f.yaw=sim::pi;f.vz=-90;
 require(legacy(f).valid&&!sim::approachGuidance(f,0).valid,"Outbound false approach suppressed");
 f.z=0;require(legacy(f).valid&&!sim::approachGuidance(f,0).valid,"In-runway false glide cue suppressed");
}
void bothAirportsAndEnds(){
 int samples=0;
 for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(int airport=0;airport<2;++airport)
  for(int direction:{1,-1})for(double distance:{0.,10.,100.,1000.,5000.,12000.}){
   auto f=inbound(airport,direction,distance);f.aircraftId=aircraft;
   f.y=f.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*distance;
   auto g=sim::approachGuidance(f,airport);
   require(g.valid&&g.runwayIndex==airport&&g.direction==direction,"Each aircraft and reciprocal runway end selectable");
   close(g.distance,distance,"Threshold distance exact");close(g.glideslope,0,"Chosen training glide path centred");
   close(g.desiredAltitude-f.spec().gearHeight-std::tan(3*sim::pi/180)*distance,
         sim::trainingThresholdClearance,"Threshold clearance independent of runway length and aircraft gear height");
   f.x+=direction*120;f.y+=40;g=sim::approachGuidance(f,airport);
   close(g.localizer,120,"Aircraft right of inbound course is positive in both directions");
   close(g.glideslope,40,"High is positive in both directions");
   f.x-=direction*240;f.y-=80;g=sim::approachGuidance(f,airport);
   // The selected path may extend below the flat ground at very short distance.
   if(f.y>=0){require(g.valid,"Low nonnegative-altitude sample valid");close(g.localizer,-120,"Aircraft left is negative");close(g.glideslope,-40,"Low is negative");}
   ++samples;
  }
 std::cout<<"7 aircraft x2 airports x2 directions x6 distances: "<<samples<<" threshold, slope and signed-deviation samples passed.\n";
}
void selectionPowerAndPersistence(){
 sim::Snapshot state;state.flight=inbound(1,1,5000);
 require(sim::nearestRunwayIndex(state.flight.x,state.flight.z)==0,"Test geometry is nearer departure centre");
 state.career.mission=2;state.career.stage=1;state.career.arrivalRunway=1;state.career.licensed=true;
 const auto before=saved(state);auto g=sim::approachGuidance(state);
 require(g.valid&&g.runwayIndex==1,"Passenger approach stays on destination despite closer departure centre");
 for(int frame=0;frame<1000;++frame)require(sim::approachGuidance(state).runwayIndex==1,"Destination selection does not drift");
 require(saved(state)==before,"Guidance has no Snapshot mutation");
 state.avionics.master=false;require(!sim::approachGuidance(state).valid,"Unpowered avionics hide guidance");
 state.avionics.master=true;state.avionics.generator=false;state.avionics.battery=false;
 require(!sim::approachGuidance(state).valid,"No electric bus hides guidance");
 state.avionics.battery=true;state.avionics.charge=1;
 require(sim::approachGuidance(state).valid,"Battery restores guidance");
 const auto airborne=state.flight;state.flight.y=state.flight.spec().gearHeight;
 require(!sim::approachGuidance(state).valid,"Grounded cockpit does not show approach cues");state.flight=airborne;
 state.career.arrivalRunway=-1;require(!sim::approachGuidance(state).valid,"Invalid mission arrival does not silently select nearby runway");
 state.career.arrivalRunway=1;
 std::stringstream stream;state.write(stream);sim::Snapshot loaded;
 require(loaded.read(stream)&&sim::approachGuidance(loaded).runwayIndex==1,"Existing v16 state restores same arrival without new fields");
 state.career.mission=0;state.flight=inbound(0,1,1000);
 require(sim::approachGuidance(state).runwayIndex==0,"Non-passenger training auto-selects eligible runway end");
}
void validityAndContinuity(){
 auto f=inbound(0,1,1000);
 f.yaw=sim::pi/2;require(!sim::approachGuidance(f,0).valid,"Crossing heading is not inbound");
 f.yaw=0;f.vz=-10;require(!sim::approachGuidance(f,0).valid,"Wrong-way ground track is not inbound");
 f.vz=90;f.x=2500.01;require(!sim::approachGuidance(f,0).valid,"Outside lateral envelope hidden");
 f.x=0;f.y=1500.01;require(!sim::approachGuidance(f,0).valid,"Outside altitude envelope hidden");
 f=inbound(0,1,12000.01);require(!sim::approachGuidance(f,0).valid,"Beyond range hidden");
 f=inbound(0,1,-.01);require(!sim::approachGuidance(f,0).valid,"Past threshold hidden, no flare/autoland cue");
 f=inbound(0,1,0);require(sim::approachGuidance(f,0).valid,"Exactly at threshold remains valid");
 f=inbound(0,1,1000);require(!sim::approachGuidance(f,-2).valid&&!sim::approachGuidance(f,2).valid,"Selected airport bounds guarded");
 f.crashed=true;require(!sim::approachGuidance(f,0).valid,"Crashed flight has no guidance");
 f=inbound(0,1,1000);f.x=std::numeric_limits<double>::quiet_NaN();require(!sim::approachGuidance(f).valid,"NaN position rejected");
 f=inbound(0,1,1000);f.yaw=std::numeric_limits<double>::infinity();require(!sim::approachGuidance(f).valid,"Infinite heading rejected");
 double last=0;int count=0;
 for(int step=0;step<=12000;++step){
  f=inbound(1,-1,double(step));const auto g=sim::approachGuidance(f,1);
  require(g.valid,"One-metre reciprocal-path sweep remains valid");
  if(step)close(g.desiredAltitude-last,std::tan(3*sim::pi/180),"No runway-centre cusp or path discontinuity");
  last=g.desiredAltitude;++count;
 }
 std::cout<<"12,001 one-metre path samples: continuous slope; selection/power/save and invalid envelopes passed.\n";
}
int main(){
 try{legacyDefects();bothAirportsAndEnds();selectionPowerAndPersistence();validityAndContinuity();
  std::cout<<"Training approach CPU geometry passed; not Windows, radio ILS, certified procedure or autoland validation.\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
