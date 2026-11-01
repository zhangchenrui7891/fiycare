#pragma once
#include "storage.h"
#include "autopilot.h"
#include "approach.h"
#include "flight_instruments.h"
#include <array>

// Three 300 x 200 logical displays, drawn as self-lit planar geometry.
// Coordinates stay within the existing bezels; no change to control hit boxes.
namespace glass {
inline constexpr XMFLOAT3 white{.90f,.95f,1.f},green{.30f,1.f,.58f},cyan{.22f,.83f,1.f},
 amber{1.f,.70f,.20f},magenta{1.f,.28f,.78f},muted{.34f,.43f,.49f},black{.009f,.016f,.025f};
template<class Text> struct Display {
 std::vector<Vertex>& out;const XMMATRIX& matrix;float center;Text& text;
 XMFLOAT3 point(float x,float y,float layer)const{
  XMFLOAT3 p{center+(x-150)*.448f/300,-.735f+(100-y)*.294f/200,.874f-layer*.00015f};
  XMStoreFloat3(&p,XMVector3TransformCoord(XMLoadFloat3(&p),matrix));return p;
 }
 void triangle(float ax,float ay,float bx,float by,float cx,float cy,XMFLOAT3 color,float layer=1){
  const auto first=out.size();tri(out,point(ax,ay,layer),point(bx,by,layer),point(cx,cy,layer),color);
  for(size_t i=first;i<out.size();++i)out[i].textured=2;
 }
 void rect(float x,float y,float w,float h,XMFLOAT3 color,float layer=1){
  triangle(x,y,x,y+h,x+w,y,color,layer);triangle(x+w,y,x,y+h,x+w,y+h,color,layer);
 }
 void line(float x,float y,float xx,float yy,XMFLOAT3 color,float width=1,float layer=3){
  const float length=std::hypot(xx-x,yy-y);if(length<.001f)return;
  const float dx=-(yy-y)/length*width*.5f,dy=(xx-x)/length*width*.5f;
  triangle(x+dx,y+dy,x-dx,y-dy,xx+dx,yy+dy,color,layer);
  triangle(xx+dx,yy+dy,x-dx,y-dy,xx-dx,yy-dy,color,layer);
 }
 void label(const std::wstring& value,float x,float y,XMFLOAT3 color=white,float scale=.72f){
  // cockpitText uses a 300 logical pixel width; scale the font accordingly.
  text(value,center+(x-150)*.448f/300,-.735f+(100-y)*.294f/200,.871f,.448f*scale,color);
 }
 void arc(float x,float y,float radius,float start,float end,XMFLOAT3 color,int steps=48){
  for(int i=0;i<steps;i++){float a=start+(end-start)*i/steps,b=start+(end-start)*(i+1)/steps;
   line(x+std::sin(a)*radius,y-std::cos(a)*radius,x+std::sin(b)*radius,y-std::cos(b)*radius,color,.8f);
  }
 }
 void aircraft(float x,float y,XMFLOAT3 color){
  line(x,y-8,x,y+8,color,1.8f,7);line(x-10,y+1,x+10,y+1,color,1.8f,7);line(x-4,y+7,x+4,y+7,color,1.8f,7);
 }
};

template<class Text> inline void draw(std::vector<Vertex>& out,const sim::Snapshot& state,const XMMATRIX& matrix,Text text){
 const auto& f=state.flight;const auto air=sim::airData(f,state.avionics.qnh);
 const bool powered=state.avionics.powered(f);
 Display<Text> p{out,matrix,-.55f,text},nav{out,matrix,0,text},engine{out,matrix,.55f,text};
 for(auto* screen:{&p,&nav,&engine})screen->rect(0,0,300,200,black,0);
 if(!powered)return; // Dark glass: never display stale flight data after power loss.
 const auto whole=[](double v){return instruments::whole(v);};
 // PFD: mode annunciations, clipped attitude, tapes and digital values.
 p.rect(0,0,300,22,{.018f,.036f,.042f});
 p.label(state.headingHold?L"HDG":L"MAN",8,4,green);
 p.label(state.altitudeHold?L"ALT":L"PITCH",101,4,green);
 p.label(state.navigationHold?L"NAV":L"FD OFF",212,4,state.navigationHold?green:muted);
 constexpr float left=62,right=238,top=28,bottom=156,cx=150,cy=92;
 p.rect(left,top,right-left,bottom-top,{.075f,.31f,.53f});
 const float roll=float(f.roll),s=std::sin(roll),c=std::cos(roll);
 const float offset=float(std::clamp(f.pitch,-1.4,1.4))*104;
 struct P{float x,y;};std::array<P,8> polygon{},clipped{};
 polygon[0]={left,top};polygon[1]={right,top};polygon[2]={right,bottom};polygon[3]={left,bottom};
 int count=0;auto distance=[&](P q){return (q.x-cx)*s+(q.y-cy)*c-offset;};
 for(int i=0;i<4;i++){const auto a=polygon[size_t(i)],b=polygon[size_t((i+1)%4)];const float da=distance(a),db=distance(b);
  if(da>=0)clipped[size_t(count++)]=a;
  if((da>=0)!=(db>=0)){float t=da/(da-db);clipped[size_t(count++)]={a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};}
 }
 for(int i=1;i+1<count;i++)p.triangle(clipped[0].x,clipped[0].y,clipped[size_t(i)].x,clipped[size_t(i)].y,clipped[size_t(i+1)].x,clipped[size_t(i+1)].y,{.32f,.19f,.10f},2);
 // Clip all rotating pitch ladder segments to the attitude rectangle.
 auto ladder=[&](float pitch,float width,XMFLOAT3 color){
  float yy=offset-pitch*104;float ax=cx-width*c+yy*s,ay=cy+width*s+yy*c;
  float bx=cx+width*c+yy*s,by=cy-width*s+yy*c;float lo=0,hi=1;
  auto clip=[&](float d,float v){if(std::abs(d)<1e-6f)return v>=0;float t=v/d;if(d<0)lo=std::max(lo,t);else hi=std::min(hi,t);return lo<=hi;};
  if(clip(-(bx-ax),ax-left)&&clip(bx-ax,right-ax)&&clip(-(by-ay),ay-top)&&clip(by-ay,bottom-ay))
   p.line(ax+(bx-ax)*lo,ay+(by-ay)*lo,ax+(bx-ax)*hi,ay+(by-ay)*hi,color,1.1f,4);
 };
 ladder(0,200,white);for(int i=-6;i<=6;i++)if(i)ladder(float(i*sim::pi/18),i%2?12.f:22.f,white);
 for(int angle:{-60,-45,-30,-20,-10,0,10,20,30,45,60}){
  float a=float(angle*sim::pi/180);p.line(cx+std::sin(a)*48,cy-std::cos(a)*48,cx+std::sin(a)*54,cy-std::cos(a)*54,white,1,5);
 }
 p.triangle(cx,cy-48,cx-4,cy-42,cx+4,cy-42,amber,6);
 p.line(118,cy,140,cy,amber,2.3f,7);p.line(160,cy,182,cy,amber,2.3f,7);p.rect(148,cy-2,4,4,amber,7);
 // Independent speed/altitude tapes; metric units match the simulator physics.
 const double speed=air.ias*3.6,alt=air.altitude;
 for(int side=0;side<2;side++){
  auto& screen=p;float x=side?242.f:4.f;const double value=side?alt:speed,step=side?100.:20.;
  screen.rect(x,28,54,128,{.045f,.062f,.080f},3);
  const double base=std::floor(value/step)*step;
  for(int k=-4;k<=4;k++){const double v=base+k*step;float y=92-float((v-value)/step)*25;
   if(y<36||y>144||(y>74&&y<114)||(!side&&v<0))continue;
   screen.line(side?x:x+46,y,side?x+7:x+53,y,white,1,5);screen.label(whole(v),x+9,y-6,white,.58f);
  }
  screen.rect(x,83,54,21,black,6);screen.line(x,83,x+54,83,white,1,7);screen.line(x,104,x+54,104,white,1,7);
  screen.label(whole(value),x+5,85,green,.88f);
 }
 p.label(L"km/h",9,160,cyan,.65f);p.label(L"ALT m",243,160,cyan,.65f);
 p.label(L"HDG "+instruments::heading(f.yaw),79,161,white,.8f);
 p.label(L"VS "+instruments::verticalSpeed(air.verticalSpeed)+L" m/s",8,182,cyan,.67f);
 p.label(L"QNH "+whole(state.avionics.qnh),184,182,cyan,.67f);
 if(f.stallWarning){p.rect(103,134,93,19,{.35f,.025f,.018f},7);p.label(L"STALL",119,136,amber,.85f);}
 // NAV: heading-up map, real training route positions and active leg.
 nav.label(L"NAV / TRAINING",8,5,cyan,.75f);nav.label(L"HDG "+instruments::heading(f.yaw),197,5,white,.7f);
 for(float radius:{34.f,65.f,96.f})nav.arc(150,111,radius,float(-sim::pi*.5),float(sim::pi*.5),muted);
 for(int a=-90;a<=90;a+=30){float r=float(a*sim::pi/180);nav.line(150+std::sin(r)*92,111-std::cos(r)*92,150+std::sin(r)*98,111-std::cos(r)*98,white);}
 nav.label(L"2 km",226,138,muted,.65f);nav.aircraft(150,111,white);
 constexpr double range=2000;const double sn=std::sin(f.yaw),cs=std::cos(f.yaw);
 bool previous=false;float px=0,py=0;
 for(size_t i=0;i<4;i++){
  const auto& w=sim::navigationRoute[i];const double dx=w.x-f.x,dz=w.z-f.z;
  const float x=150+float((dx*cs-dz*sn)/range)*96,y=111-float((dx*sn+dz*cs)/range)*96;
  const bool inside=x>=18&&x<=282&&y>=27&&y<=163;
  if(inside&&previous)nav.line(px,py,x,y,magenta,1.3f);
  if(inside){bool active=int(i)==state.navigationWaypoint;XMFLOAT3 col=active?magenta:cyan;
   nav.line(x,y-4,x+4,y,col);nav.line(x+4,y,x,y+4,col);nav.line(x,y+4,x-4,y,col);nav.line(x-4,y,x,y-4,col);
   nav.label(whole(i+1),x+6,y-6,col,.56f);
  }
  previous=inside;px=x;py=y;
 }
 const bool valid=sim::validNavigationWaypoint(state.navigationWaypoint);
 nav.rect(0,165,300,35,black,6);
 nav.label(valid?L"WPT "+whole(state.navigationWaypoint+1)+L"   "+whole(sim::navigationDistance(state))+L" m":L"WPT INVALID",8,167,valid?magenta:amber,.75f);
 nav.label(L"XTK "+whole(sim::navigationCrossTrack(state))+L" m   QNH "+whole(state.avionics.qnh),8,184,cyan,.63f);
 // Engine/system display. RPM is the model's actual RPM, not a fictitious jet N1.
 engine.label(L"ENGINE / SYSTEM",8,5,cyan,.8f);
 const int engines=f.engines.count(f.aircraftId);
 for(int i=0;i<2;i++){
  float x=78.f+i*144;const bool available=i<engines;
  const double rpm=available?(f.engines.initialized?f.engines.units[size_t(i)].rpm:f.rpm):0;
  engine.label(L"ENG "+whole(i+1),x-26,28,available?white:muted,.7f);
  engine.arc(x,91,38,-2.3f,2.3f,muted);
  if(available){const float a=float(-2.3+std::clamp(rpm/2600.,0.,1.)*4.6);
   engine.arc(x,91,38,-2.3f,a,green);engine.line(x,91,x+std::sin(a)*31,91-std::cos(a)*31,white,1.4f);
  }
  engine.label(available?whole(rpm):L"---",x-23,99,available?green:muted,.95f);engine.label(L"RPM",x-15,118,cyan,.65f);
 }
 engine.line(8,139,292,139,muted);
 engine.label(L"FUEL "+whole(f.fuel)+L"%",8,145,f.fuel<15?amber:green,.8f);
 engine.label(L"FLAP "+whole(f.flap*100)+L"%",165,145,cyan,.8f);
 engine.label(f.gear?L"GEAR DOWN":L"GEAR UP",8,165,f.gear?green:white,.75f);
 engine.label(state.parkingBrake?L"PARK BRK":L"BRK OFF",165,165,state.parkingBrake?amber:muted,.75f);
 engine.label(state.avionics.generating(f)?L"GEN ON":L"BAT "+whole(state.avionics.charge*100)+L"%",8,184,green,.65f);
 engine.label(L"THR "+whole(f.throttle*100)+L"%",165,184,cyan,.65f);
}
} // namespace glass
