#include "src/autopilot.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void close(double a,double b,const char* why){require(std::abs(a-b)<1e-8,why);}
sim::Snapshot sample(int active,double remaining,double right,double speed=90,int aircraft=3,bool surface=false){
 sim::Snapshot s;s.navigationWaypoint=active;s.navigationHold=s.headingHold=s.altitudeHold=true;
 auto& f=s.flight;f.configure(aircraft);f.setSurfaceModel(surface);f.y=s.targetAltitude=1500;f.gear=false;
 const auto& a=sim::navigationRoute[(active+3)%4];const auto& b=sim::navigationRoute[active];
 const double length=std::hypot(b.x-a.x,b.z-a.z),ux=(b.x-a.x)/length,uz=(b.z-a.z)/length;
 f.x=b.x-remaining*ux+right*uz;f.z=b.z-remaining*uz-right*ux;f.vx=ux*speed;f.vz=uz*speed;
 f.yaw=std::atan2(ux,uz);f.throttle=aircraft?.3:.65;f.rpm=700+1900*f.throttle;return s;
}
double legacyBearing(const sim::Snapshot& s){
 const auto& p=sim::navigationRoute[s.navigationWaypoint];return std::atan2(p.x-s.flight.x,p.z-s.flight.z);
}
std::string saved(const sim::Snapshot& s){std::ostringstream out;s.write(out);return out.str();}
void geometry(){
 int cases=0;
 for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true})for(int active=0;active<4;++active)
  for(double fraction:{.2,.4,.6,.8})for(double right:{-3000.,-600.,-100.,0.,100.,600.,3000.})for(double speed:{60.,100.,200.}){
   auto s=sample(active,1000,right,speed,aircraft,surface);const double length=sim::navigationLeg(s).length;
   s=sample(active,length*fraction,right,speed,aircraft,surface);const auto before=saved(s);
   const auto g=sim::navigationGuidance(s);const auto& a=sim::navigationRoute[(active+3)%4];const auto& b=sim::navigationRoute[active];
   const double course=std::atan2(b.x-a.x,b.z-a.z);
   require(g.valid&&g.tracking,"Interior straight leg is tracked");
   close(g.lookAhead,std::min(length*fraction,std::clamp(speed*12.,600.,1800.)),"Lookahead scales with ground speed and stops at endpoint");
   require(std::abs(g.interceptAngle)<=sim::pi/4,"Intercept angle bounded to 45 degrees");
   close(sim::wrapAngle(g.trackBearing-course),g.interceptAngle,"Track is course plus bounded intercept across heading wrap");
   require(right==0?g.interceptAngle==0:right*g.interceptAngle<0,"Cross-track correction points toward the line");
   require(saved(s)==before,"Guidance query is read only");
   sim::Controls c;sim::updateAutopilot(s,c);
   require(s.navigationHold&&s.headingHold&&s.navigationWaypoint==active,"Guidance does not change distant waypoint or AP availability");
   close(sim::wrapAngle(s.targetHeading-g.trackBearing),0,"Actual AP uses ground-track guidance in still air");
   require(std::isfinite(c.roll)&&std::abs(c.roll)<=1,"Roll controller remains bounded");++cases;
  }
 std::cout<<cases<<" profile/model/course/offset/speed guidance/AP cases passed.\n";
 auto s=sample(0,4000,600);const auto g=sim::navigationGuidance(s);
 require(std::abs(g.trackBearing)>std::abs(legacyBearing(s))*2,"Far-end direct-to undershoot reproduced; new command captures line earlier");
 std::cout<<"X=600 Z=-1500 speed=90: legacy bearing="<<legacyBearing(s)*180/sim::pi
          <<" deg; new="<<g.trackBearing*180/sim::pi<<" deg; lookahead="<<g.lookAhead<<"m (CPU geometry only).\n";
}
void boundariesAndWind(){
 for(int active=0;active<4;++active){
  auto s=sample(active,1000,600);const double length=sim::navigationLeg(s).length;
  for(double remaining:{length+1.,0.,-1000.}){
   s=sample(active,remaining,1000);const auto g=sim::navigationGuidance(s);
   require(g.valid&&!g.tracking,"Before/at/after leg retains direct-to recovery");close(sim::wrapAngle(g.trackBearing-legacyBearing(s)),0,"Recovery bearing unchanged");
  }
  s=sample(active,1e-6,600);auto g=sim::navigationGuidance(s);
  require(g.valid&&g.tracking&&g.lookAhead>0&&std::abs(g.interceptAngle)<=sim::pi/4,"Tiny positive endpoint distance stays finite and bounded");
  for(double speed:{0.,1.,40.,50.,90.,150.,10000.}){
   s=sample(active,length*.8,600,speed);g=sim::navigationGuidance(s);
   require(g.valid&&g.lookAhead>=600&&g.lookAhead<=1800,"Slow/fast lookahead clamps, independent of AP availability");
  }
  for(double wind:{-8.,8.}){
   s=sample(active,length*.8,600);s.flight.windX=wind;s.flight.windZ=-wind*.5;g=sim::navigationGuidance(s);
   const auto d=sim::airData(s.flight,s.avionics.qnh);const double windRight=s.flight.windX*std::cos(g.trackBearing)-s.flight.windZ*std::sin(g.trackBearing);
   const double expected=sim::wrapAngle(g.trackBearing-std::asin(std::clamp(windRight/std::max(1.,d.tas),-.7,.7)));
   sim::Controls c;sim::updateAutopilot(s,c);close(sim::wrapAngle(s.targetHeading-expected),0,"Original wind-crab conversion applies to new desired ground track");
  }
 }
 for(int index:{-1,4,std::numeric_limits<int>::max()}){auto s=sample(0,4000,600);s.navigationWaypoint=index;require(!sim::navigationGuidance(s).valid,"Invalid index has no guidance or array access");}
 auto s=sample(0,4000,600);s.flight.x=NAN;require(!sim::navigationGuidance(s).valid,"NaN position rejected");
 s=sample(0,4000,600);s.flight.vx=INFINITY;require(!sim::navigationGuidance(s).valid,"Infinite velocity rejected");
 s=sample(0,4000,600);sim::Controls c;c.yaw=.2;sim::updateAutopilot(s,c);require(!s.navigationHold&&!s.headingHold&&s.altitudeHold,"Manual lateral override still wins");
 s=sample(0,4000,600);s.avionics.master=false;c={};sim::updateAutopilot(s,c);require(!s.navigationHold&&!s.headingHold&&!s.altitudeHold,"Power-off still disconnects all AP channels");
}
void idealTrackComparison(){
 int cases=0;
 for(int active=0;active<4;++active)for(double side:{-600.,600.})for(double speed:{48.,90.,120.}){
  auto now=sample(active,1000,side,speed);const double length=sim::navigationLeg(now).length;
  now=sample(active,length*.8,side,speed);auto old=now;
  // Ideal instantaneous track response, constant ground speed, no wind or bank
  // dynamics: isolates path geometry. NOT Flight.step or an aircraft trial.
  for(int step=0;step<2000;++step){
   const auto g=sim::navigationGuidance(now);require(g.valid,"Ideal sequence guidance remains valid");
   const double legacy=legacyBearing(old);
   now.flight.vx=std::sin(g.trackBearing)*speed;now.flight.vz=std::cos(g.trackBearing)*speed;
   now.flight.x+=now.flight.vx*.01;now.flight.z+=now.flight.vz*.01;
   old.flight.x+=std::sin(legacy)*speed*.01;old.flight.z+=std::cos(legacy)*speed*.01;
  }
  const double newError=std::abs(sim::navigationCrossTrack(now)),oldError=std::abs(sim::navigationCrossTrack(old));
  require(newError<oldError*.9,"New ideal straight-leg geometry reduces offset sooner than old direct-to");
  std::cout<<"IDEAL_ONLY course="<<active<<" side="<<side<<" speed="<<speed<<" 20s old_XTK="<<oldError<<" new_XTK="<<newError<<'\n';++cases;
 }
 std::cout<<cases<<" ideal-track comparisons passed; NOT aircraft dynamics or full route.\n";
}
void physicalSmoke(){
 for(int aircraft:{0,3})for(bool surface:{false,true})for(double wind:{-8.,8.}){
  auto s=sample(0,4000,600,aircraft?100:48,aircraft,surface);s.flight.windX=wind;
  for(int step=0;step<2000;++step){sim::Controls c;sim::updateAutopilot(s,c);s.flight.step(.01,c);s.avionics.step(.01,s.flight);
   require(!s.flight.crashed&&std::isfinite(s.flight.x)&&std::isfinite(s.flight.y)&&std::isfinite(s.flight.z),"Actual 20s physics remains finite and noncrashed");
  }
  require(s.navigationHold&&s.headingHold,"Actual short physical loop retains lateral modes");
  require(s.navigationWaypoint==0&&std::abs(sim::navigationCrossTrack(s))<300,"Eight specified physical cases halve initial offset on the same straight leg");
  std::cout<<"PHYSICS_SMOKE aircraft="<<aircraft<<" surface="<<surface<<" wind="<<wind<<" 20s XTK="<<sim::navigationCrossTrack(s)<<'\n';
 }
}
int main(){try{geometry();boundariesAndWind();idealTrackComparison();physicalSmoke();
 std::cout<<"Bounded training leg-guidance CPU checks passed. No FMS/turn anticipation/Windows/GPU/full-route acceptance.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
