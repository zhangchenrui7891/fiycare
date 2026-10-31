#include "src/cockpit_controls.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>
// Frozen v0.39 control geometry oracle; not another production control table.
namespace legacy {
using cockpit::Solid;using cockpit::Action;
inline std::vector<Solid> solids(double throttle){
 std::vector<Solid> s;
 for(int i=0;i<5;i++)s.push_back({static_cast<Action>(i),{-.64f+i*.27f,-.48f,.88f},{.070f,.032f,.025f}});
 s.push_back({Action::Throttle,{.63f,-.75f+float(throttle)*.3f,.55f},{.15f,.07f,.07f}});
 const Action extra[]={Action::HeadingHold,Action::AltitudeHold,Action::NavigationHold,Action::HeadingMinus,Action::HeadingPlus,Action::AltitudeMinus,Action::AltitudePlus};
 for(int i=0;i<7;i++)s.push_back({extra[i],{-.72f+i*.22f,-.35f,.88f},{.060f,.027f,.025f}});
 s.push_back({Action::CockpitFlood,{-.94f,-.37f,.88f},{.035f,.035f,.028f}});
 s.push_back({Action::PanelBrightness,{.94f,-.37f,.88f},{.035f,.035f,.028f}});
 for(int i=0;i<3;i++)s.push_back({static_cast<Action>(int(Action::Battery)+i),{-.24f+i*.24f,.755f,.65f},{.065f,.028f,.055f}});
 const Action lower[]={Action::QnhMinus,Action::QnhPlus,Action::TrimMinus,Action::TrimPlus};
 for(int i=0;i<4;i++)s.push_back({lower[i],{-.67f+i*.23f,-.55f,.84f},{.065f,.025f,.027f}});
 const Action engineer[]={Action::EngineLeft,Action::EngineRight,Action::SurfaceModel,Action::ForceDisplay,Action::ResetFaults};
 for(int i=0;i<5;i++)s.push_back({engineer[i],{-.64f+i*.27f,-.26f,.88f},{.065f,.025f,.025f}});
 return s;
}

int pick(cockpit::Point d,double throttle){
 float nearest=1e9f;int hit=-1;const auto s=solids(throttle);
 for(size_t i=0;i<s.size();i++){
  float lo=0,hi=1e9f;const float ds[]={d.x,d.y,d.z},cs[]={s[i].center.x,s[i].center.y,s[i].center.z},hs[]={s[i].half.x,s[i].half.y,s[i].half.z};
  for(int k=0;k<3;k++){if(std::abs(ds[k])<1e-7f){if(std::abs(cs[k])>hs[k])hi=-1;}else{float a=(cs[k]-hs[k])/ds[k],b=(cs[k]+hs[k])/ds[k];if(a>b)std::swap(a,b);lo=std::max(lo,a);hi=std::min(hi,b);}}
  if(hi>=lo&&lo<nearest){nearest=lo;hit=int(i);}
 }return hit;
}
}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool same(cockpit::Point a,cockpit::Point b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
int main(){
 static_assert(std::is_same<decltype(cockpit::solids(0)),std::array<cockpit::Solid,27>>::value,"Fixed 27-element value, no vector storage");
 size_t cases=0,rays=0;
 for(int tick=0;tick<=1000;tick++){
  const double throttle=tick/1000.;
  const auto current=cockpit::solids(throttle);const auto previous=legacy::solids(throttle);
  require(current.size()==previous.size(),"Control count");
  for(size_t i=0;i<current.size();i++){
   require(current[i].action==previous[i].action&&same(current[i].center,previous[i].center)&&same(current[i].half,previous[i].half),"Ordered control geometry is exactly preserved");++cases;
   const auto d=current[i].center;
   require(cockpit::pick(d,throttle)==legacy::pick(d,throttle),"Center ray nearest hit");++rays;
  }
 }
 for(float aspect:{1.f,16.f/9,21.f/9})for(float yaw:{-.85f,0.f,.85f})for(float pitch:{-.8f,-.4f,.55f}){
  cockpit::viewportAspect=aspect;cockpit::lookYaw=yaw;cockpit::lookPitch=pitch;
  for(int y=-30;y<=570;y+=30)for(int x=-60;x<=1020;x+=30)for(double throttle:{0.,.5,1.}){
   auto ray=cockpit::ray(float(x),float(y));
   require(cockpit::pick(ray,throttle)==legacy::pick(ray,throttle),"Viewport ray nearest hit");++rays;
  }
 }
 // Range checking is not added: preserve finite out-of-normal-range inputs too.
 for(double value:{-1.,2.}){
  const auto a=cockpit::solids(value);const auto b=legacy::solids(value);
  require(same(a[5].center,b[5].center),"Throttle extrapolation unchanged");
 }
 std::cout<<"PASS: "<<cases<<" exact ordered solids, "<<rays<<" equivalent hit rays; fixed array type. Not GPU/Windows/FPS evidence.\n";
}

