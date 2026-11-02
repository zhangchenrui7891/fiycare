#include "src/cockpit_controls.h"
#include "src/autopilot.h"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(){try{
 int cases=0;
 for(int id=0;id<sim::aircraftCount;++id)for(bool surface:{false,true}){
  sim::Snapshot s;s.flight.configure(id);s.flight.setSurfaceModel(surface);
  s.flight.y=1000;s.flight.x=100;s.flight.z=0;s.flight.vz=120;s.flight.yaw=.35;
  s.altitudeHold=true;s.targetAltitude=1000;
  cockpit::apply(s,cockpit::Action::NavigationHold);
  sim::Controls c;sim::updateAutopilot(s,c);
  check(s.navigationHold&&s.headingHold,"NAV initially controls lateral guidance");
  cockpit::apply(s,cockpit::Action::HeadingHold);
  check(!s.navigationHold&&s.headingHold,"HDG must leave NAV and engage heading hold");
  check(s.targetHeading==s.flight.yaw&&s.altitudeHold,"Capture current heading, preserve ALT");
  cockpit::apply(s,cockpit::Action::HeadingPlus);double selected=s.targetHeading;
  for(int i=0;i<100;++i){c={};sim::updateAutopilot(s,c);}
  check(s.headingHold&&!s.navigationHold&&s.targetHeading==selected,"NAV must not overwrite selected heading next tick");
  cockpit::apply(s,cockpit::Action::HeadingHold);c={};sim::updateAutopilot(s,c);
  check(!s.headingHold&&!s.navigationHold&&s.altitudeHold&&c.roll==0,"Second HDG press releases lateral hold only");
  cockpit::apply(s,cockpit::Action::HeadingHold);
  check(s.headingHold&&s.targetHeading==s.flight.yaw,"HDG from manual captures current heading");
  cockpit::apply(s,cockpit::Action::NavigationHold);c={};sim::updateAutopilot(s,c);
  check(s.navigationHold&&s.headingHold,"NAV can be reselected");
  cockpit::apply(s,cockpit::Action::NavigationHold);
  check(!s.navigationHold&&s.headingHold,"Existing NAV-off heading hold fallback preserved");
  s.navigationHold=true;c={};c.roll=.5;sim::updateAutopilot(s,c);
  check(!s.navigationHold&&!s.headingHold&&c.roll==.5,"Manual roll still disconnects both lateral modes");
  s.navigationHold=true;s.headingHold=false;
  cockpit::apply(s,cockpit::Action::HeadingHold);
  check(s.headingHold&&!s.navigationHold,"Inconsistent restored NAV state recovers to HDG");
  s.avionics.master=false;cockpit::apply(s,cockpit::Action::HeadingHold);
  check(!s.headingHold&&!s.navigationHold&&!s.altitudeHold,"Unpowered avionics cannot keep AP modes");
  ++cases;
 }
 std::cout<<"PASS: "<<cases<<" aircraft/model mode sequences; 2000 subsequent guidance updates. CPU state tests, not a flight or hardware test.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
