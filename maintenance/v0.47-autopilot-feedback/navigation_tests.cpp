#include "src/autopilot.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void close(double a,double b,const char* why){require(std::abs(a-b)<1e-8,why);}
std::string saved(const sim::Snapshot& s){std::ostringstream out;s.write(out);return out.str();}

sim::Snapshot sample(int active,double remaining,double right,double speed=90,int aircraft=3,bool surface=false){
 sim::Snapshot s;s.navigationWaypoint=active;s.navigationHold=s.headingHold=s.altitudeHold=true;
 auto& f=s.flight;f.configure(aircraft);f.setSurfaceModel(surface);f.gear=false;f.y=s.targetAltitude=1500;
 const auto& a=sim::navigationRoute[(active+3)%4];const auto& b=sim::navigationRoute[active];
 const double length=std::hypot(b.x-a.x,b.z-a.z),ux=(b.x-a.x)/length,uz=(b.z-a.z)/length;
 f.x=b.x-remaining*ux+right*uz;f.z=b.z-remaining*uz-right*ux;
 f.vx=ux*speed;f.vz=uz*speed;f.yaw=std::atan2(ux,uz);
 f.throttle=aircraft?.3:.65;f.rpm=700+1900*f.throttle;
 return s;
}
void legacyMiss(){
 auto s=sample(0,-200,600);const auto& p=sim::navigationRoute[0];
 const double distance=std::hypot(p.x-s.flight.x,p.z-s.flight.z);
 int oldWaypoint=s.navigationWaypoint;
 if(distance<450)oldWaypoint=(oldWaypoint+1)%4; // Exact v0.34 sequencing condition.
 const double oldBearing=std::atan2(p.x-s.flight.x,p.z-s.flight.z);
 require(oldWaypoint==0&&std::cos(oldBearing)<0,"Legacy missed-radius target remains behind the aircraft");
 sim::Controls c;sim::updateAutopilot(s,c);
 require(s.navigationWaypoint==1&&s.navigationHold&&std::sin(s.targetHeading)>0,"New terminal capture selects the next eastbound point");
 std::cout<<"Legacy reproduced at X=600 Z=2700: distance="<<distance
          <<"m, old point=1 behind; new point="<<s.navigationWaypoint+1<<".\n";
}
void geometryAndCapture(){
 int count=0;
 for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true})
  for(int active=0;active<4;++active)for(double right:{-600.,600.})for(double overshoot:{0.,200.,900.}){
   auto s=sample(active,-overshoot,right,90,aircraft,surface);const auto leg=sim::navigationLeg(s);
   require(leg.valid&&leg.length>0,"Incoming leg is defined");
   close(leg.remaining,-overshoot,"Signed remaining distance from endpoint");
   close(leg.crossTrack,right,"Aircraft-right cross track on all four courses");
   close(leg.distance,std::hypot(right,overshoot),"Point distance matches geometry");
   close(leg.alongSpeed,90,"Along-track velocity is projected onto course");
   require(leg.distance>=450&&sim::navigationCaptured(leg),"Missed radial capture recovered in terminal window");
   sim::Controls c;sim::updateAutopilot(s,c);
   require(s.navigationWaypoint==(active+1)%4&&s.navigationHold&&s.headingHold,"Actual AP advances one point for all profiles/models/courses");
   require(std::isfinite(c.roll)&&std::isfinite(c.yaw)&&std::isfinite(c.pitch)&&std::abs(c.roll)<=1&&std::abs(c.yaw)<=1&&std::abs(c.pitch)<=1,
           "AP demands remain bounded");
   c={};sim::updateAutopilot(s,c);require(s.navigationWaypoint==(active+1)%4,"Stationary repeated query cannot cascade to another point");
   ++count;
  }
 std::cout<<"7 profiles x2 models x4 courses x2 sides x3 overshoots: "<<count<<" AP capture cases passed.\n";
 for(int active=0;active<4;++active){
  for(const auto& s:{sample(active,100,600),sample(active,-900.01,600),sample(active,-200,900.01),
                     sample(active,-200,600,-90),sample(active,-200,600,0),sample(active,-200,600,5)}){
   require(!sim::navigationCaptured(sim::navigationLeg(s)),"Early/far/wide/backward/stopped/slow passage must not advance");
  }
  auto radial=sample(active,100,100,-90);
  require(sim::navigationCaptured(sim::navigationLeg(radial)),"Original proximity capture remains direction independent");
  sim::Controls c;sim::updateAutopilot(radial,c);require(radial.navigationWaypoint==(active+1)%4,"Original radial AP transition retained");
 }
}
void availabilityAndPersistence(){
 auto s=sample(0,-200,600);const auto before=saved(s);
 for(int i=0;i<1000;++i){sim::navigationLeg(s);sim::navigationDistance(s);sim::navigationCrossTrack(s);}
 require(saved(s)==before,"Read-only geometry queries do not mutate flight/save state");
 sim::Controls c;c.roll=1;sim::updateAutopilot(s,c);
 require(s.navigationWaypoint==0&&!s.navigationHold&&!s.headingHold&&s.altitudeHold,"Manual lateral input wins over capture");
 s=sample(0,-200,600);s.avionics.master=false;c={};sim::updateAutopilot(s,c);
 require(s.navigationWaypoint==0&&!s.navigationHold&&!s.headingHold&&!s.altitudeHold,"Power loss prevents capture and disconnects AP");
 s=sample(0,-200,600);s.flight.y=s.flight.spec().gearHeight;c={};sim::updateAutopilot(s,c);
 require(s.navigationWaypoint==0&&!s.navigationHold,"Ground state cannot sequence AP navigation");
 for(int active:{-1,4,std::numeric_limits<int>::max()}){
  s=sample(0,-200,600);s.navigationWaypoint=active;
  require(!sim::navigationLeg(s).valid&&sim::navigationDistance(s)==0&&sim::navigationCrossTrack(s)==0,"Invalid route slot has neutral geometry, no array access");
  c={};sim::updateAutopilot(s,c);
  require(!s.navigationHold&&!s.headingHold&&s.altitudeHold,"Invalid navigation disables lateral modes without disabling valid altitude hold");
 }
 s=sample(0,-200,600);s.flight.x=std::numeric_limits<double>::quiet_NaN();
 require(!sim::navigationLeg(s).valid,"Nonfinite position invalidates geometry");
 s=sample(0,-200,600);s.flight.vz=std::numeric_limits<double>::infinity();
 require(!sim::navigationLeg(s).valid,"Nonfinite velocity invalidates geometry");
 s=sample(3,-200,600);c={};sim::updateAutopilot(s,c);require(s.navigationWaypoint==0,"Last point wraps to first");
 std::stringstream stream;s.write(stream);sim::Snapshot loaded;
 require(loaded.read(stream)&&loaded.navigationHold&&loaded.navigationWaypoint==0,"Existing v16 save preserves advanced active point");
 c={};sim::updateAutopilot(loaded,c);require(loaded.navigationWaypoint==0,"Reloaded point does not sequence again without moving");
}
void shortPhysicalScenarios(){
 int count=0;
 for(int aircraft:{0,3})for(bool surface:{false,true})for(double wind:{-8.,8.}){
  auto s=sample(0,-200,600,aircraft?100:48,aircraft,surface);s.flight.windX=wind;
  double maxRoll=0;int firstPoint=-1;
  for(int step=0;step<1000;++step){
   sim::Controls c;sim::updateAutopilot(s,c);if(step==0)firstPoint=s.navigationWaypoint;
   s.flight.step(.01,c);s.avionics.step(.01,s.flight);
   require(!s.flight.crashed&&std::isfinite(s.flight.x)&&std::isfinite(s.flight.y)&&std::isfinite(s.flight.z),"10s actual flight integrator scenario remains finite and noncrashed");
   require(sim::validNavigationWaypoint(s.navigationWaypoint),"Actual physical loop preserves valid active point");
   maxRoll=std::max(maxRoll,std::abs(s.flight.roll));
  }
  require(firstPoint==1,"Actual physical scenario starts with corrected next point");
  require(s.navigationHold&&s.headingHold,"Lateral navigation remains engaged through the short physical scenario");
  std::cout<<"CPU flight aircraft="<<aircraft<<" surface="<<surface<<" wind="<<wind
           <<" seconds=10 first_point="<<firstPoint+1<<" final_point="<<s.navigationWaypoint+1<<" max_roll_deg="<<maxRoll*180/sim::pi<<'\n';
  ++count;
 }
 std::cout<<count<<" short actual Flight.step scenarios passed; not complete-route, Windows or calibrated-aircraft validation.\n";
}
int main(){
 try{legacyMiss();geometryAndCapture();availabilityAndPersistence();shortPhysicalScenarios();
  std::cout<<"Training navigation CPU checks passed; no certified FMS, turn anticipation or real input/GPU evidence.\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
