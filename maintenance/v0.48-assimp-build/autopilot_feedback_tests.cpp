#include "src/autopilot.h"
#include "src/autopilot_feedback.h"
#include "src/cockpit_controls.h"
#include <iostream>
#include <stdexcept>

void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
sim::AutopilotModes modes(int mask){return {bool(mask&1),bool(mask&2),bool(mask&4)};}
int mask(sim::AutopilotModes value){return int(value.heading)+2*int(value.altitude)+4*int(value.navigation);}
sim::Snapshot sample(int aircraft,bool surface,int modeMask){
 sim::Snapshot s;auto& f=s.flight;f.configure(aircraft);f.setSurfaceModel(surface);
 f.x=f.z=0;f.y=s.targetAltitude=1500;f.vx=0;f.vz=90;f.gear=false;
 const auto m=modes(modeMask);s.headingHold=m.heading;s.altitudeHold=m.altitude;s.navigationHold=m.navigation;
 s.navigationWaypoint=0;return s;
}
int main(){
 try{
  int cases=0;
  for(int before=0;before<8;++before)for(int after=0;after<8;++after){
   const auto lost=sim::disconnectedAutopilotModes(modes(before),modes(after));
   require(mask(lost)==(before&~after),"Only true-to-false channels produce an event");
   const auto text=sim::autopilotDisconnectMessage(lost);
   require(text.empty()==!lost.any(),"No message for gains/unchanged/off");
   require(text.size()<=46,"Message fits existing cockpit notice character limit");
   require((text.find(L"航向")!=std::wstring::npos)==lost.heading,"Heading name is exact");
   require((text.find(L"高度")!=std::wstring::npos)==lost.altitude,"Altitude name is exact");
   require((text.find(L"导航")!=std::wstring::npos)==lost.navigation,"Navigation name is exact");
   ++cases;
  }
  for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true})
   for(int before=0;before<8;++before){
    for(int manual=0;manual<4;++manual){
     auto s=sample(aircraft,surface,before);const auto previous=sim::autopilotModes(s);
     sim::Controls c;c.roll=(manual&1)?.3:0;c.pitch=(manual&2)?-.3:0;
     sim::updateAutopilot(s,c);
     const int expected=before&(((manual&1)?5:0)|((manual&2)?2:0));
     require(mask(sim::disconnectedAutopilotModes(previous,sim::autopilotModes(s)))==expected,"Actual controller manual disconnect reports correct channels");
     const auto stable=sim::autopilotModes(s);sim::Controls again=c;
     // Use the same manual inputs rather than the controller's output.
     again.roll=(manual&1)?.3:0;again.pitch=(manual&2)?-.3:0;again.yaw=0;
     sim::updateAutopilot(s,again);
     require(!sim::disconnectedAutopilotModes(stable,sim::autopilotModes(s)).any(),"Next control step does not repeat the event");
     ++cases;
    }
    for(int condition=0;condition<6;++condition){
     auto s=sample(aircraft,surface,before);
     if(condition==0)s.avionics.master=false;
     if(condition==1){s.avionics.battery=false;s.avionics.generator=false;}
     if(condition==2)s.flight.crashed=true;
     if(condition==3)s.flight.y=sim::terrain(s.flight.x,s.flight.z)+s.flight.spec().gearHeight;
     if(condition==4)s.flight.stallWarning=true;
     if(condition==5)s.flight.vz=0;
     const auto previous=sim::autopilotModes(s);sim::Controls c;sim::updateAutopilot(s,c);
     require(mask(sim::disconnectedAutopilotModes(previous,sim::autopilotModes(s)))==before,"Actual protection reports all previously active channels");
     const auto stable=sim::autopilotModes(s);sim::updateAutopilot(s,c);
     require(!sim::disconnectedAutopilotModes(stable,sim::autopilotModes(s)).any(),"Protection does not repeat while channels remain off");
     ++cases;
    }
    auto s=sample(aircraft,surface,before);s.navigationWaypoint=9;
    const auto previous=sim::autopilotModes(s);sim::Controls c;sim::updateAutopilot(s,c);
    require(mask(sim::disconnectedAutopilotModes(previous,sim::autopilotModes(s)))==((before&4)?before&5:0),"Invalid NAV reports lateral loss, preserves altitude channel");
    ++cases;
   }
  for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true})
   for(int before=0;before<8;++before)for(int action=0;action<3;++action){
    auto s=sample(aircraft,surface,before);s.avionics.generator=false;
    auto control=cockpit::Action::Avionics;
    if(action==1)control=cockpit::Action::Battery;
    if(action==2){control=cockpit::Action::Generator;s.avionics.battery=false;s.avionics.generator=true;s.flight.engine=true;s.flight.fuel=100;s.flight.rpm=1600;}
    require(s.avionics.powered(s.flight),"Power-switch sample starts with live avionics");
    const auto previous=sim::autopilotModes(s);cockpit::apply(s,control);
    require(!s.avionics.powered(s.flight),"Actual cockpit power action loses avionics");
    require(mask(sim::disconnectedAutopilotModes(previous,sim::autopilotModes(s)))==before,"Pre-physics power action reports channels already cleared by cockpit::apply");
    ++cases;
   }
  for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true}){
   auto s=sample(aircraft,surface,7);s.avionics.generator=false;s.avionics.charge=.001001;
   const auto previous=sim::autopilotModes(s);sim::Controls c;sim::updateAutopilot(s,c);
   require(sim::autopilotModes(s).any(),"Battery is initially just above power threshold");
   s.avionics.step(.01,s.flight);
   if(!s.avionics.powered(s.flight))s.headingHold=s.altitudeHold=s.navigationHold=false;
   require(mask(sim::disconnectedAutopilotModes(previous,sim::autopilotModes(s)))==7,"Post-step battery loss is captured");
   ++cases;
  }
  std::cout<<"PASS "<<cases<<" transition/controller/manual/protection/NAV/post-battery cases across seven aircraft and both physics modes. No Windows input or GPU test.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
