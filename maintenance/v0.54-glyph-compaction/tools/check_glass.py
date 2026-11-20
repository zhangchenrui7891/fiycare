"""Compile the production glass displays with CPU math/text adapters.

Checks real geometry, labels, power gating and data bindings. Optional --preview
exports an orthographic layout preview, not a screenshot of the Windows game.
"""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
code = r'''
#include "assets.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
struct XMFLOAT3{float x,y,z;};
using XMMATRIX=int;
XMFLOAT3 XMLoadFloat3(const XMFLOAT3* p){return *p;}
XMFLOAT3 XMVector3TransformCoord(XMFLOAT3 p,int){return p;}
void XMStoreFloat3(XMFLOAT3* p,XMFLOAT3 v){*p=v;}
void tri(std::vector<Vertex>& out,XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 color){
 for(auto p:{a,b,c})out.push_back({p.x,p.y,p.z,color.x,color.y,color.z});
}
// Emulate legacy SDK macros to catch the v0.50 class of Windows build failure.
#define near
#define far
#include "cockpit_avionics.h"
#undef near
#undef far
struct Label{std::wstring text;float x,y,z,w;XMFLOAT3 color;};
std::vector<Label> labels;
void label(const std::wstring& s,float x,float y,float z,float w,XMFLOAT3 c){labels.push_back({s,x,y,z,w,c});}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool contains(const std::wstring& token){for(const auto& l:labels)if(l.text.find(token)!=std::wstring::npos)return true;return false;}
void validate(const std::vector<Vertex>& out){
 require(out.size()%3==0&&out.size()<16000,"Display triangle budget");
 for(const auto& v:out){
  require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"Finite vertices");
  require(v.x>=-.775f&&v.x<=.775f&&v.y>=-.883f&&v.y<=-.587f,"Display geometry within bezels");
  require(v.textured==2&&v.z>.870f&&v.z<.875f,"Self-lit glass layers");
 }
 for(const auto& l:labels){
  require(l.y<=-.588f&&l.y>=-.883f&&l.z<.872f,"Text on glass face");
  float center=l.x<-.326f?-.55f:l.x>.326f?.55f:0.f;
  require(l.x>=center-.2241f&&l.x+l.text.size()*9*l.w/300<=center+.2241f,"Conservative text width within bezel");
 }
}
int main(int argc,char** argv){
 std::vector<Vertex> out;int cases=0;
 for(int id=0;id<sim::aircraftCount;id++)for(double pitch:{-.8,0.,.8})for(double roll:{-2.8,-.6,0.,.6,2.8})for(double yaw:{-sim::pi,0.,6.28})for(bool on:{false,true}){
  sim::Snapshot state;state.flight.configure(id);state.flight.y=2400;state.flight.vz=90;state.flight.vy=-.3;state.flight.pitch=pitch;state.flight.roll=roll;state.flight.yaw=yaw;state.flight.rpm=2100;state.avionics.master=on;
  out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
  require(on?!labels.empty():labels.empty(),"Powered state gates all flight text");
  if(!on)require(out.size()==18,"Power loss leaves only three dark rectangles");
  if(on){require(contains(L"km/h")&&contains(L"ALT m")&&contains(L"VS -0.3 m/s"),"Metric units and precise vertical speed");require(contains(L"RPM")&&!contains(L"N1"),"Actual RPM, no invented jet N1");}
  ++cases;
 }
 // At the origin both endpoints of the closing leg are off screen.
 {sim::Snapshot origin;origin.flight.configure(3);origin.flight.x=0;origin.flight.z=0;origin.flight.yaw=0;
  out.clear();labels.clear();glass::draw(out,origin,0,label);
  int routeVertices=0;
  for(const auto& v:out)if(v.x>-.224f&&v.x<.224f&&v.r==glass::magenta.x&&v.g==glass::magenta.y&&v.b==glass::magenta.z)++routeVertices;
  require(routeVertices==6,"Closing route leg crossing viewport must remain visible with both endpoints outside");
 }
 // Isolate the NAV's green ground-track cue and verify its endpoint in
 // the real heading-up display coordinates, including reciprocal motion.
 for(double yaw:{-sim::pi,-.7,0.,.8,sim::pi})
  for(double vx:{-40.,0.,40.})for(double vz:{-30.,0.,30.}){
   sim::Snapshot flight;flight.flight.configure(3);
   flight.flight.yaw=yaw;flight.flight.vx=vx;flight.flight.vz=vz;
   out.clear();labels.clear();glass::draw(out,flight,0,label);validate(out);
   std::vector<Vertex> cue;
   for(const auto& v:out)
    if(v.x>-.2241f&&v.x<.2241f&&v.r==glass::green.x&&v.g==glass::green.y&&v.b==glass::green.z)
     cue.push_back(v);
   const double speed=std::hypot(vx,vz);
   require(cue.size()==(speed>=5?30u:0u),"Ground-track cue has bounded geometry and hides when stopped");
   if(speed>=5){
    const float x=150+float((vx*std::cos(yaw)-vz*std::sin(yaw))/speed)*44;
    const float y=111-float((vx*std::sin(yaw)+vz*std::cos(yaw))/speed)*44;
    // The four diamond strokes are centrally symmetric around the endpoint.
    double centerX=0,centerY=0;
    for(size_t i=6;i<cue.size();++i){
     centerX+=150+cue[i].x*300/.448;
     centerY+=100-(cue[i].y+.735)*200/.294;
    }
    require(std::abs(centerX/24-x)<.001&&std::abs(centerY/24-y)<.001,"Ground-track cue follows velocity relative to heading");
    for(const auto& v:cue){
     const float logicalX=150+v.x*300/.448f,logicalY=100-(v.y+.735f)*200/.294f;
     require(logicalX>=102&&logicalX<=198&&logicalY>=63&&logicalY<=159,
             "Cue stays within the NAV map and above the footer");
    }
   }
  }
 // Runway silhouettes are clipped even if their centres are off-screen.
 // Amber NAV geometry belongs only to the chosen destination.
 auto airportVertices=[&](bool destination){std::vector<Vertex> result;
  const XMFLOAT3 color=destination?glass::amber:XMFLOAT3{.55f,.68f,.76f};
  for(const auto& v:out)if(v.x>-.2241f&&v.x<.2241f&&v.r==color.x&&v.g==color.y&&v.b==color.z)result.push_back(v);
  return result;
 };
 int airportCases=0;
 for(int airport:{0,1})for(double yaw:{-sim::pi,-.7,0.,.8,sim::pi})
  for(double offset:{-500.,0.,500.}){
   sim::Snapshot map;const auto& r=sim::runways[airport];
   map.flight.x=r.x+offset;map.flight.z=r.z;map.flight.y=500;map.flight.yaw=yaw;
   map.flight.vx=30;map.flight.vz=40;
   map.career.mission=2;map.career.stage=1;map.career.arrivalRunway=airport;
   out.clear();labels.clear();glass::draw(out,map,0,label);validate(out);
   auto outline=airportVertices(true);
   require(!outline.empty()&&outline.size()<=24,"Selected airport silhouette is visible within four-line budget");
   const double lineX=(outline[2].x-outline[0].x)*300/.448;
   const double lineY=-(outline[2].y-outline[0].y)*200/.294;
   const double length=std::hypot(lineX,lineY);
   require(length>.001&&(-lineX*std::sin(yaw)-lineY*std::cos(yaw))/length>.9999,"Runway long axis rotates opposite heading in heading-up map");
   const double startX=150+(outline[0].x+outline[1].x)*.5*300/.448;
   const double startY=100-((outline[0].y+outline[1].y)*.5+.735)*200/.294;
   const double centerX=150-offset*std::cos(yaw)/2000*96;
   const double centerY=111+offset*std::sin(yaw)/2000*96;
   require(std::abs((startX-centerX)*std::cos(yaw)-(startY-centerY)*std::sin(yaw)+3)<.01,"Narrow runway outline retains three-pixel half-width under rotation/clipping");
   for(const auto& v:outline){
    const float x=150+v.x*300/.448f,y=100-(v.y+.735f)*200/.294f;
    require(x>=18&&x<=282&&y>=27&&y<=163,"Runway strokes stay inside map and above footer");
   }
   require(contains(L"ARR "+instruments::whole(airport+1)+L"   "+instruments::whole(std::abs(offset))+L" m"),"ARR footer identifies selected airport and horizontal centre distance");
   const auto bearing=offset==0?L"---":offset>0?L"270":L"090";
   require(contains(L"BRG "+std::wstring(bearing)+L"   GS 180 km/h")&&contains(L"MAP / ARRIVAL"),"BRG uses world bearing; coincident airport hides unstable bearing");
   map.navigationHold=true;out.clear();labels.clear();glass::draw(out,map,0,label);
   require(contains(L"NAV / CIRCUIT")&&contains(L"ARR "),"Active training AP remains explicitly a circuit while ARR is informational");
   map.career.arrivalRunway=-1;out.clear();labels.clear();glass::draw(out,map,0,label);
   require(airportVertices(true).empty()&&contains(L"ARR INVALID")&&contains(L"BRG ---"),"Invalid arrival does not highlight nearest airport or retain stale bearing");
   map.career.mission=0;out.clear();labels.clear();glass::draw(out,map,0,label);
   require(airportVertices(true).empty()&&!airportVertices(false).empty()&&contains(L"WPT ")&&!contains(L"ARR "),"Other missions retain training footer and ordinary runway silhouettes");
   map.career.mission=2;map.career.stage=3;out.clear();labels.clear();glass::draw(out,map,0,label);
   require(!contains(L"ARR "),"Completed passenger flight exits arrival display");
   map.career.stage=1;map.career.arrivalRunway=airport;map.avionics.master=false;
   out.clear();labels.clear();glass::draw(out,map,0,label);
   require(out.size()==18&&labels.empty(),"Airports and arrival data extinguish with avionics power");
   ++airportCases;
  }
 {sim::Snapshot clipped;clipped.career.mission=2;clipped.career.stage=1;clipped.career.arrivalRunway=0;
  clipped.flight.x=0;clipped.flight.z=-2000;clipped.flight.y=200;
  out.clear();labels.clear();glass::draw(out,clipped,0,label);validate(out);
  require(!airportVertices(true).empty(),"Runway edges crossing map remain visible with airport centre off-screen");
 }
 std::cout<<airportCases<<" actual airport display position/heading/destination cases, clipped off-screen centre, lifecycle, circuit label and power gates passed. CPU only.\n";
 // Extract only the PFD magenta diamonds to check actual display direction,
 // magnitude and clipping for both airports and reciprocal runway headings.
 auto approachDiamonds=[&](){std::vector<Vertex> result;
  for(const auto& v:out)if(v.x<-.326f&&v.r==glass::magenta.x&&v.g==glass::magenta.y&&v.b==glass::magenta.z)result.push_back(v);
  return result;
 };
 int approachCases=0;
 for(int id=0;id<sim::aircraftCount;++id)for(int airport:{0,1})for(int direction:{1,-1})
  for(double distance:{1000.,5000.})for(double lateral:{-120.,0.,120.})for(double vertical:{-20.,0.,20.}){
   sim::Snapshot inbound;auto& f=inbound.flight;f.configure(id);
   const auto& runway=sim::runways[airport];
   f.x=runway.x+direction*lateral;f.z=runway.z-direction*(runway.halfLength+distance);
   f.yaw=direction==1?0:sim::pi;f.vz=direction*90;
   f.y=f.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*distance+vertical;
   inbound.career.mission=2;inbound.career.stage=1;inbound.career.arrivalRunway=airport;
   out.clear();labels.clear();glass::draw(out,inbound,0,label);validate(out);
   const auto diamonds=approachDiamonds();
   require(diamonds.size()==12&&contains(L"TRAIN")&&contains(L"LOC")&&contains(L"G/S"),"Eligible destination approach displays two diamonds and both labels");
   double x[2]{},y[2]{};
   for(int cue=0;cue<2;++cue)for(int k=0;k<6;++k){
    const auto& v=diamonds[size_t(cue*6+k)];
    x[cue]+=(150+(v.x+.55)*300/.448)/6;
    y[cue]+=(100-(v.y+.735)*200/.294)/6;
   }
   const double loc=std::clamp(-lateral/std::max(30.,distance*std::tan(2.5*sim::pi/180)),-1.,1.);
   const double gs=std::clamp(vertical/std::max(10.,distance*std::tan(.7*sim::pi/180)),-1.,1.);
   require(std::abs(x[0]-(150+loc*32))<.001&&std::abs(y[0]-145)<.001,"LOC diamond points toward course, with bounded range-scaled displacement");
   require(std::abs(x[1]-230)<.001&&std::abs(y[1]-(77+gs*32))<.001,"G/S diamond points toward training path, with bounded range-scaled displacement");
   f.stallWarning=true;out.clear();labels.clear();glass::draw(out,inbound,0,label);validate(out);
   require(approachDiamonds().empty()&&!contains(L"LOC")&&!contains(L"G/S")&&contains(L"STALL"),"Stall has priority over approach display");
   f.stallWarning=false;f.vz=-f.vz;out.clear();labels.clear();glass::draw(out,inbound,0,label);
   require(approachDiamonds().empty()&&!contains(L"LOC"),"Outbound track hides approach display");
   f.vz=-f.vz;inbound.career.arrivalRunway=-1;out.clear();labels.clear();glass::draw(out,inbound,0,label);
   require(approachDiamonds().empty(),"Invalid mission destination cannot display nearest-airport cues");
   inbound.career.arrivalRunway=airport;inbound.avionics.master=false;
   out.clear();labels.clear();glass::draw(out,inbound,0,label);
   require(approachDiamonds().empty()&&labels.empty()&&out.size()==18,"Power loss extinguishes approach cues");
   ++approachCases;
  }
 std::cout<<approachCases<<" actual PFD approach aircraft/airport/direction/range/deviation cases plus stall, outbound, invalid destination and power gates passed. CPU only.\n";
 // Actual display binding across both terrain modes and all aircraft.
 // QNH affects ALT but never the geometric AGL readout.
 int aglCases=0;
 for(int id=0;id<sim::aircraftCount;++id)for(int terrainMode:{0,1})
  for(double height:{-1.,0.,1.6,123.4,123.6,749.6,750.,751.}){
   sim::worldMode.store(terrainMode);
   sim::Snapshot low;low.flight.configure(id);low.flight.x=2600;low.flight.z=5000;
   const double elevation=sim::terrain(low.flight.x,low.flight.z);
   low.flight.y=elevation+height;
   const auto expected=height<0?L"---":height>750?L"":instruments::whole(height);
   double firstAltitude=0;bool first=true;
   for(double qnh:{950.,1013.25,1050.}){
    low.avionics.qnh=qnh;
    out.clear();labels.clear();glass::draw(out,low,0,label);validate(out);
    const Label* agl=nullptr;for(const auto& l:labels)if(l.text.rfind(L"AGL ",0)==0)agl=&l;
    require(std::wstring(expected).empty()?agl==nullptr:agl&&agl->text==L"AGL "+std::wstring(expected)+L" m","PFD AGL uses geometric terrain clearance and unrounded visibility boundary");
    if(agl)require(agl->color.x==glass::white.x&&agl->w==.448f*.70f,"AGL uses its reserved readable label style");
    const auto altitude=sim::airData(low.flight,qnh).altitude;
    if(first){firstAltitude=altitude;first=false;}else require(std::abs(altitude-firstAltitude)>1,"QNH changes barometric ALT while AGL stays fixed");
    ++aglCases;
   }
   low.flight.stallWarning=true;
   out.clear();labels.clear();glass::draw(out,low,0,label);validate(out);
   require(contains(L"STALL"),"AGL cannot suppress the stall warning");
   if(height>=0&&height<=750)require(contains(L"AGL "),"Low-altitude and stall labels coexist in separate rows");
   low.avionics.master=false;out.clear();labels.clear();glass::draw(out,low,0,label);
   require(labels.empty()&&out.size()==18,"AGL extinguishes with all displays after avionics power loss");
  }
 sim::worldMode.store(1);
 std::cout<<aglCases<<" AGL aircraft/terrain/QNH display bindings plus range, invalid-height, stall and power checks passed. CPU only.\n";
 sim::Snapshot state;state.flight.configure(3);state.flight.rpm=2250;state.flight.vz=90;state.flight.y=1234;state.flight.vy=1.2;state.flight.pitch=.035;state.flight.roll=-.12;state.flight.throttle=.84;state.flight.flap=.5;state.flight.fuel=87;state.headingHold=true;state.navigationHold=true;state.flight.z=1000;
 out.clear();labels.clear();glass::draw(out,state,0,label);
 require(contains(L"2250")&&contains(L"FUEL 87%")&&contains(L"FLAP 50%")&&contains(L"THR 84%"),"Flight state feeds engine/system glass");
 state.flight.vx=30;state.flight.vz=40;state.flight.yaw=0;
 out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
 require(contains(L"TRK 037")&&contains(L"GS 180 km/h")&&contains(L"HDG 000"),"Track follows ground velocity while heading follows aircraft yaw");
 state.flight.vx=state.flight.vz=0;
 out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
 require(contains(L"TRK ---")&&contains(L"GS 0 km/h"),"Stationary heading is not presented as ground track");
 state.avionics.master=false;
 out.clear();labels.clear();glass::draw(out,state,0,label);
 require(!contains(L"TRK")&&!contains(L"GS "),"Ground motion text obeys avionics power");
 state.avionics.master=true;
 state.navigationWaypoint=99;out.clear();labels.clear();glass::draw(out,state,0,label);require(contains(L"WPT INVALID"),"Invalid waypoint never dereferenced");
 state.navigationWaypoint=0;state.flight.stallWarning=true;out.clear();labels.clear();glass::draw(out,state,0,label);require(contains(L"STALL"),"Stall warning retained");
 state.flight.stallWarning=false;
 state.flight.x=3540;state.flight.z=1300;state.flight.y=state.flight.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*500+8;
 state.flight.vx=0;state.flight.vz=90;state.navigationHold=state.headingHold=false;
 state.career.mission=2;state.career.stage=1;state.career.arrivalRunway=1;
 out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
 if(argc>1){std::ofstream file(argv[1]);file<<"{\"vertices\":[";bool first=true;for(const auto& v:out){if(!first)file<<',';first=false;file<<'['<<v.x<<','<<v.y<<','<<v.z<<','<<v.r<<','<<v.g<<','<<v.b<<']';}
  file<<"],\"labels\":[";first=true;for(const auto& l:labels){if(!first)file<<',';first=false;file<<"{\"text\":\"";for(wchar_t c:l.text)file<<char(c);file<<"\",\"x\":"<<l.x<<",\"y\":"<<l.y<<",\"w\":"<<l.w<<",\"color\":["<<l.color.x<<','<<l.color.y<<','<<l.color.z<<"]}";}file<<"]}";
 }
 std::cout<<"PASS production glass: "<<cases<<" aircraft/pitch/bank/heading/power cases; units, RPM, fuel/flap/throttle bindings, bounds, vertex budget, invalid waypoint and stall warning. CPU adapters only.\n";
}
'''
# Also compile the complete production cockpit integration, including controls,
# mechanical geometry, text binding and display material conversion.
scene = (root/'src/scene.h').read_text()
box = scene[scene.index('inline void box('):scene.index('inline float terrain(')]
detail = (root/'src/cockpit_detail.h').read_text().replace('#pragma once', '').replace('#include "scene.h"', '')
body = (root/'src/cockpit.h').read_text()
body = '\n'.join(line for line in body.splitlines() if not line.startswith(('#include', '#pragma')))
font = (root/'src/chinese_text.h').read_text()
quad = font[font.index('inline void screenQuad'):font.index('\nstruct ChineseFont')]
integration = r'''
#include "cockpit_controls.h"
constexpr float XM_2PI=6.283185307f;
using XMVECTOR=XMFLOAT3;
int XMMatrixIdentity(){return 0;}
bool XMMatrixIsIdentity(int matrix){return matrix==0;}
int XMMatrixRotationZ(float){return 0;}
int XMMatrixTranslation(float,float,float){return 0;}
XMFLOAT3 XMVectorSet(float x,float y,float z,float){return {x,y,z};}
struct TestFont{void draw(std::vector<Vertex>&,const std::wstring&,float,float,XMFLOAT3={1,1,1},float=900){}};
TestFont& chineseFont(){static TestFont f;return f;}
''' + quad + box + detail + body + (root/'tests/reference/cockpit_v094.h').read_text() + r'''
bool windowFrameBlocks(const std::vector<Vertex>& frame,cockpit::Point ray,float maxDistance){
 auto subtract=[](cockpit::Point a,cockpit::Point b){return cockpit::Point{a.x-b.x,a.y-b.y,a.z-b.z};};
 auto cross=[](cockpit::Point a,cockpit::Point b){return cockpit::Point{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
 auto dot=[](cockpit::Point a,cockpit::Point b){return a.x*b.x+a.y*b.y+a.z*b.z;};
 auto point=[](const Vertex& v){return cockpit::Point{v.x,v.y,v.z};};
 for(size_t i=0;i<frame.size();i+=3){
  const auto a=point(frame[i]),ab=subtract(point(frame[i+1]),a),ac=subtract(point(frame[i+2]),a);
  const auto perpendicular=cross(ray,ac);
  const float determinant=dot(ab,perpendicular);
  if(std::abs(determinant)<1e-6f)continue;
  const cockpit::Point toEye{-a.x,-a.y,-a.z};
  const auto q=cross(toEye,ab);
  const float u=dot(toEye,perpendicular)/determinant,v=dot(ray,q)/determinant;
  const float distance=dot(ac,q)/determinant;
  if(u>=0&&v>=0&&u+v<=1&&distance>0&&distance<maxDistance)return true;
 }
 return false;
}
void check777WindowFrame(){
 std::vector<Vertex> frame;
 cockpit777WindowFrame(frame,0);
 require(frame.size()==7*36,"777 frame uses the shared frame's seven-solid budget");
 for(const auto& vertex:frame)require(std::isfinite(vertex.x)&&std::isfinite(vertex.y)&&std::isfinite(vertex.z),"Finite window geometry");
 require(windowFrameBlocks(frame,{.24f,0,1},3),"Right-of-centre mullion is opaque");
 for(float x:{-.32f,0.f,.48f})
  require(!windowFrameBlocks(frame,{x,0,1},3),"Forward window apertures remain open");
 for(double throttle:{0.,.5,1.})
  for(const auto& control:cockpit::solids(throttle))
   require(!windowFrameBlocks(frame,control.center,1),"777 frame must not cover a control centre");
 for(float x:{-.55f,0.f,.55f})
  require(!windowFrameBlocks(frame,{x,-.735f,.873f},1),"777 frame must not cover a display centre");
 std::cout<<"PASS 777 window: solid budget, three clear forward rays, visible mullion, display and control center clearance. CPU geometry only.\n";
}
void checkInteriorCache(){
 for(bool b777:{false,true}){
  std::vector<Vertex> reference,cached;
  buildCockpitInterior(reference,0,b777);
  const auto& geometry=cockpitInteriorGeometry(b777);
  require(reference.size()==geometry.size(),"Cached interior vertex count matches original");
  const auto same=[&](const std::vector<Vertex>& values){
   require(values.size()==reference.size(),"Cached interior append count");
   require(std::memcmp(values.data(),reference.data(),reference.size()*sizeof(Vertex))==0,
           "Cached interior bytes match the original construction");
  };
  same(geometry);
  const auto* storage=geometry.data();
  for(int frame=0;frame<20;++frame){
   cached.clear();cockpitInterior(cached,0,b777);same(cached);
   require(cockpitInteriorGeometry(b777).data()==storage,"Static interior storage survives frame clears");
  }
  cached.assign(1,Vertex{});cockpitInterior(cached,0,b777);
  require(cached.size()==reference.size()+1,"Interior append preserves the caller's prefix");
  require(std::memcmp(cached.data()+1,reference.data(),reference.size()*sizeof(Vertex))==0,"Appended interior order matches");
  cached.clear();reference.clear();
  buildCockpitInterior(reference,1,b777);cockpitInterior(cached,1,b777);same(cached);
  std::cout<<"Interior variant "<<b777<<": "<<geometry.size()<<" vertices, "
           <<geometry.capacity()*sizeof(Vertex)<<" retained bytes. CPU adapters only.\n";
 }
}
void checkFullCockpitEquivalence(){
 for(int id=0;id<sim::aircraftCount;++id)for(bool power:{false,true})
  for(double throttle:{0.,.5,1.}){
   sim::Snapshot state;state.flight.configure(id);state.avionics.master=power;
   state.flight.throttle=throttle;
   std::vector<Vertex> before(1),after(1);
   legacyCockpitMesh(before,state,0);cockpitMesh(after,state,0);
   require(before.size()==after.size()&&std::memcmp(before.data(),after.data(),before.size()*sizeof(Vertex))==0,
           "Complete cockpit geometry and final materials must match v0.94");
  }
 for(bool b777:{false,true}){
  const auto& geometry=cockpitFixedGeometry(b777);
  require(!geometry.empty(),"Fixed shell cache populated");
  for(const auto& v:geometry)require(v.textured==3||v.textured==4,"Cache stores final cabin materials");
  std::cout<<"Fixed shell "<<b777<<": "<<geometry.size()<<" vertices bypass per-frame material rewrite.\n";
 }
}
void checkCockpitIntegration(){
 for(int id=0;id<sim::aircraftCount;id++)for(bool on:{false,true}){
  sim::Snapshot state;state.flight.configure(id);state.avionics.master=on;std::vector<Vertex> geometry;
  cockpitMesh(geometry,state,0);
  require(!geometry.empty(),"Full cockpit draws geometry");
  const auto trimVertices=std::count_if(geometry.begin(),geometry.end(),[](const Vertex& v){return v.r==.205f&&v.g==.232f&&v.b==.255f&&v.textured==3;});
  require(trimVertices==(id==10?60:0),"777 opaque window trim is emitted only for the 777");
  for(const auto& v:geometry)require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&(v.textured==3||v.textured==4),"Cockpit finite and material integration");
 }
}
'''
code = code.replace('int main(int argc,char** argv){', integration + '\nint main(int argc,char** argv){\n checkCockpitIntegration();\n checkFullCockpitEquivalence();\n checkInteriorCache();\n check777WindowFrame();')
with tempfile.TemporaryDirectory(prefix='fcair-glass-') as folder:
    source = Path(folder) / 'glass.cpp'
    exe = Path(folder) / 'glass'
    source.write_text(code)
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O2']
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic', *flags,
                    '-I', str(root / 'src'), str(source), '-o', str(exe)], check=True)
    preview = root / 'validation_v100/glass_layout.json'
    if '--preview' in sys.argv:
        preview.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(exe), *([str(preview)] if '--preview' in sys.argv else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})

if '--preview' in sys.argv:
    from PIL import Image, ImageDraw, ImageFont
    data = json.loads(preview.read_text())
    image = Image.new('RGB', (1580, 490), '#111a23')
    draw = ImageDraw.Draw(image)
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    draw.text((32, 18), 'FLIGHTCAREER AIR  /  GLASS COCKPIT', font=ImageFont.truetype(font_path, 22), fill='#dae7f0')
    draw.text((32, 53), 'CPU layout preview - production geometry, substitute font; not an in-game screenshot', font=ImageFont.truetype(font_path, 14), fill='#8fa7b9')
    scale = 940
    def point(x, y): return (790+x*scale, 108+(-.588-y)*scale)
    def color(c): return tuple(max(0,min(255,round(v*255))) for v in c)
    vertices = data['vertices']
    for center in [-.55, 0, .55]:
        a=point(center-.237,-.58); b=point(center+.237,-.89)
        draw.rounded_rectangle((*a,*b), radius=9, fill='#26313c', outline='#4a5865', width=2)
    for i in range(0,len(vertices),3):
        v=vertices[i:i+3]
        draw.polygon([point(p[0],p[1]) for p in v],fill=color(v[0][3:]))
    for l in data['labels']:
        font=ImageFont.truetype(font_path,max(8,round(16*l['w']/300*scale)))
        draw.text(point(l['x'],l['y']),l['text'],font=font,fill=color(l['color']))
    for x,title in [(77,'PRIMARY FLIGHT DISPLAY'),(595,'NAVIGATION DISPLAY'),(1110,'ENGINE / SYSTEM DISPLAY')]:
        draw.text((x,419),title,font=ImageFont.truetype(font_path,16),fill='#9fbdcd')
    image.save(root/'validation_v100/glass_preview.png')
    print('Saved CPU glass layout preview.')
