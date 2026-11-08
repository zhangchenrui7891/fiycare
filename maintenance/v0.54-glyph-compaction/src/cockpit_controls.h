#pragma once
#include "storage.h"
#include <vector>
#include <array>
namespace cockpit {
enum class Action{Engine,Gear,Flaps,Assist,ParkingBrake,Throttle,HeadingHold,AltitudeHold,HeadingMinus,HeadingPlus,AltitudeMinus,AltitudePlus,NavigationHold,PanelBrightness,CockpitFlood,Battery,Generator,Avionics,QnhMinus,QnhPlus,TrimMinus,TrimPlus,Wind,EngineLeft,EngineRight,SurfaceModel,ForceDisplay,ResetFaults};
inline void apply(sim::Snapshot& state,Action action,double value=0){
 auto& f=state.flight;
 switch(action){
  case Action::EngineLeft:f.engines.units[0].enabled=!f.engines.units[0].enabled;break;
  case Action::EngineRight:if(f.aircraftId)f.engines.units[1].enabled=!f.engines.units[1].enabled;break;
  case Action::SurfaceModel:f.setSurfaceModel(!f.surfaceModel);break;
  case Action::ForceDisplay:state.showForces=!state.showForces;break;
  case Action::ResetFaults:f.engines.resetFaults();break;
  case Action::Battery:state.avionics.battery=!state.avionics.battery;break;
  case Action::Generator:state.avionics.generator=!state.avionics.generator;break;
  case Action::Avionics:state.avionics.master=!state.avionics.master;break;
  case Action::QnhMinus:state.avionics.qnh=std::max(950.,state.avionics.qnh-1);break;
  case Action::QnhPlus:state.avionics.qnh=std::min(1050.,state.avionics.qnh+1);break;
  case Action::TrimMinus:f.trim=std::max(-.5,f.trim-.025);break;
  case Action::TrimPlus:f.trim=std::min(.5,f.trim+.025);break;
  case Action::Wind:f.windX=f.windX==0?8:0;f.windZ=0;break;
  case Action::PanelBrightness:state.panelBrightness=state.panelBrightness>=.999?0:std::min(1.,state.panelBrightness+.25);break;
  case Action::CockpitFlood:state.cockpitFlood=!state.cockpitFlood;break;
  case Action::Engine:f.engine=!f.engine;break;
  case Action::Gear:f.gear=!f.gear;break;
  case Action::Flaps:f.flap=f.aircraftId>0?(f.flap<.25?.25:f.flap<.5?.5:f.flap<1?1:0):(f.flap==0?1:0);break;
  case Action::Assist:f.assist=!f.assist;break;
  case Action::ParkingBrake:state.parkingBrake=!state.parkingBrake;break;
  case Action::Throttle:if(std::isfinite(value))f.throttle=std::clamp(value,0.0,1.0);break;
  case Action::HeadingHold:
   // NAV uses the heading controller internally. Selecting HDG from NAV
   // transfers lateral control instead of toggling that internal flag off.
   state.headingHold=state.navigationHold||!state.headingHold;
   state.navigationHold=false;
   if(state.headingHold)state.targetHeading=f.yaw;
   break;
  case Action::AltitudeHold:state.altitudeHold=!state.altitudeHold;if(state.altitudeHold)state.targetAltitude=std::clamp(sim::airData(f,state.avionics.qnh).altitude,0.,12000.);break;
  case Action::HeadingMinus:state.targetHeading=sim::wrapAngle(state.targetHeading-10*sim::pi/180);break;
  case Action::HeadingPlus:state.targetHeading=sim::wrapAngle(state.targetHeading+10*sim::pi/180);break;
  case Action::AltitudeMinus:state.targetAltitude=std::max(0.,state.targetAltitude-100);break;
  case Action::AltitudePlus:state.targetAltitude=std::min(12000.,state.targetAltitude+100);break;
  case Action::NavigationHold:state.navigationHold=!state.navigationHold;state.headingHold=state.navigationHold||state.headingHold;break;
 }
 if(!state.avionics.powered(f))state.headingHold=state.altitudeHold=state.navigationHold=false;
}
struct Event{Action action;double value=0;};
struct Input {
 bool enabled=false,dragging=false,looking=false;std::vector<Event> pending;
 void enqueue(Action action,double value=0){
  // Absolute positions may replace only the immediately preceding finite
  // throttle position. Never cross a switch, invalid value or other action.
  if(action==Action::Throttle&&std::isfinite(value)&&!pending.empty()&&
     pending.back().action==Action::Throttle&&std::isfinite(pending.back().value)){
   pending.back().value=value;return;
  }
  pending.push_back({action,value});
 }
 // The first held drag owns the pointer until its button is released.
 bool beginLook(){if(!enabled||dragging)return false;looking=true;return true;}
 void endLook(){looking=false;}
 void release(){dragging=false;}
 void loseCapture(){dragging=false;looking=false;}
 void cancel(){enabled=false;loseCapture();pending.clear();}
};
}

namespace cockpit {
struct Point {float x,y,z;};
struct Solid {Action action;Point center,half;};
inline std::array<Solid,27> solids(double throttle){
 std::array<Solid,27> s{};size_t count=0;
 auto add=[&](Solid solid){s[count++]=solid;};
 for(int i=0;i<5;i++)add({static_cast<Action>(i),{-.64f+i*.27f,-.48f,.88f},{.070f,.032f,.025f}});
 add({Action::Throttle,{.63f,-.75f+float(throttle)*.3f,.55f},{.15f,.07f,.07f}});
 const Action extra[]={Action::HeadingHold,Action::AltitudeHold,Action::NavigationHold,Action::HeadingMinus,Action::HeadingPlus,Action::AltitudeMinus,Action::AltitudePlus};
 for(int i=0;i<7;i++)add({extra[i],{-.72f+i*.22f,-.35f,.88f},{.060f,.027f,.025f}});
 add({Action::CockpitFlood,{-.94f,-.37f,.88f},{.035f,.035f,.028f}});
 add({Action::PanelBrightness,{.94f,-.37f,.88f},{.035f,.035f,.028f}});
 for(int i=0;i<3;i++)add({static_cast<Action>(int(Action::Battery)+i),{-.24f+i*.24f,.755f,.65f},{.065f,.028f,.055f}});
 const Action lower[]={Action::QnhMinus,Action::QnhPlus,Action::TrimMinus,Action::TrimPlus};
 for(int i=0;i<4;i++)add({lower[i],{-.67f+i*.23f,-.55f,.84f},{.065f,.025f,.027f}});
 const Action engineer[]={Action::EngineLeft,Action::EngineRight,Action::SurfaceModel,Action::ForceDisplay,Action::ResetFaults};
 for(int i=0;i<5;i++)add({engineer[i],{-.64f+i*.27f,-.26f,.88f},{.065f,.025f,.025f}});
 return s;
}
inline float lookYaw=0,lookPitch=-.40f;
inline float viewportAspect=960.f/540;
inline Point ray(float x,float y){
 const float a=(x/480-1)*.57735027f*viewportAspect,b=(1-y/270)*.57735027f;
 const float sy=std::sin(lookYaw),cy=std::cos(lookYaw),sp=std::sin(lookPitch),cp=std::cos(lookPitch);
 return {a*cy-b*sy*sp+sy*cp,b*cp+sp,-a*sy-b*cy*sp+cy*cp};
}
inline int pick(Point d,double throttle){
 float nearest=1e9f;int hit=-1;auto s=solids(throttle);
 for(size_t i=0;i<s.size();i++){
  float lo=0,hi=1e9f;const float ds[]={d.x,d.y,d.z},cs[]={s[i].center.x,s[i].center.y,s[i].center.z},hs[]={s[i].half.x,s[i].half.y,s[i].half.z};
  for(int k=0;k<3;k++){if(std::abs(ds[k])<1e-7f){if(std::abs(cs[k])>hs[k])hi=-1;}else{float a=(cs[k]-hs[k])/ds[k],b=(cs[k]+hs[k])/ds[k];if(a>b)std::swap(a,b);lo=std::max(lo,a);hi=std::min(hi,b);}}
  if(hi>=lo&&lo<nearest){nearest=lo;hit=int(i);}
 }return hit;
}
struct SpatialInput:Input {
 double throttle=0;float lastY=0;
 bool press(float x,float y){if(!enabled||looking)return false;int hit=pick(ray(x,y),throttle);if(hit<0)return false;auto action=solids(throttle)[hit].action;dragging=action==Action::Throttle;lastY=y;enqueue(action,throttle);return true;}
 void move(float y){if(enabled&&dragging){throttle=std::clamp(throttle+(lastY-y)/160.,0.,1.);lastY=y;enqueue(Action::Throttle,throttle);}}
};
}
