#pragma once
#include "chinese_text.h"
#include "cockpit_controls.h"
#include "autopilot.h"
#include "approach.h"
#include "cockpit_detail.h"
#include "flight_instruments.h"
inline cockpit::SpatialInput cockpitInput;
inline void cockpitText(std::vector<Vertex>& out,const std::wstring& text,float x,float y,float z,float width,const XMMATRIX& transform,XMFLOAT3 color={.4f,1,.75f}){
 const size_t first=out.size();
 try{chineseFont().draw(out,text,0,0,color,900);}catch(...){out.resize(first);throw;}
 for(size_t i=first;i<out.size();i++){auto& v=out[i];XMFLOAT3 q{ x+(v.x+1)*480*width/300,y-(1-v.y)*270*width/300,z};XMStoreFloat3(&q,XMVector3TransformCoord(XMLoadFloat3(&q),transform));v.x=q.x;v.y=q.y;v.z=q.z;v.textured=2;}
}
#include "cockpit_avionics.h"
inline void cockpitMesh(std::vector<Vertex>& out,const sim::Snapshot& state,const XMMATRIX& transform){
 const size_t firstVertex=out.size();
 const auto& f=state.flight;XMFLOAT3 panel{.13f,.16f,.18f},trim{.23f,.27f,.3f};
 box(out,0,-.65f,1.03f,.95f,.33f,.13f,panel,transform);
 
 box(out,0,-1.05f,.1f,1.25f,.04f,1.4f,panel,transform);
 for(float x:{-1.1f,1.1f}){box(out,x,-.4f,.1f,.055f,.65f,1.1f,panel,transform);box(out,x,.34f,1.1f,.055f,.7f,.065f,trim,transform);}
 box(out,0,.95f,.65f,1.15f,.055f,.55f,panel,transform);
 box(out,0,.5f,1.35f,.035f,.43f,.035f,trim,transform);
 box(out,.63f,-.93f,.40f,.22f,.15f,.32f,panel,transform);
 cockpitInterior(out,transform);
 auto n=[](double v){return std::to_wstring(int(std::lround(v)));};
 const bool waypointValid=sim::validNavigationWaypoint(state.navigationWaypoint);
 const std::wstring waypointNumber=waypointValid?n(state.navigationWaypoint+1):L"--";
 const auto targetHeading=instruments::heading(state.targetHeading);
 const bool powered=state.avionics.powered(f);
 glass::draw(out,state,transform,[&](const std::wstring& value,float x,float y,float z,float width,XMFLOAT3 color){
  cockpitText(out,value,x,y,z,width,transform,color);
 });
 const wchar_t* names[]={L"发动机",L"起落架",L"襟翼",L"辅助",L"停留刹车"};
 auto s=cockpit::solids(f.throttle);
 for(int i=0;i<5;i++){bool on=i==0?f.engine:i==1?f.gear:i==2?f.flap>0:i==3?f.assist:state.parkingBrake;auto c=s[i].center;box(out,c.x,c.y,c.z,.070f,.032f,.025f,on?XMFLOAT3{.12f,.5f,.32f}:XMFLOAT3{.25f,.27f,.28f},transform);box(out,c.x,c.y+(on?.010f:-.010f),c.z-.035f,.009f,.015f,.009f,{.8f,.82f,.85f},transform);cockpitText(out,names[i],c.x-.095f,c.y+.025f,c.z-.037f,.15f,transform);}
 auto c=s[5].center;for(float dx:{-.055f,.055f}){box(out,c.x+dx,(c.y-.9f)/2,c.z,.012f,(c.y+.9f)/2,.012f,trim,transform);box(out,c.x+dx,c.y,c.z,.04f,.035f,.06f,{.75f,.78f,.8f},transform);}
 cockpitText(out,L"双发油门",.43f,-.82f,.32f,.36f,transform);
 const wchar_t* apNames[]={L"航向",L"高度",L"导航",L"航-",L"航+",L"高-",L"高+"};
 for(int i=0;i<7;i++){auto q=s[6+i].center;bool on=i==0?state.headingHold:i==1?state.altitudeHold:i==2?state.navigationHold:false;box(out,q.x,q.y,q.z,.060f,.027f,.025f,on?XMFLOAT3{.15f,.55f,.3f}:XMFLOAT3{.23f,.25f,.27f},transform);cockpitText(out,apNames[i],q.x-.045f,q.y+.017f,q.z-.027f,.12f,transform);}
 for(int i=13;i<15;i++){
  auto q=s[i].center;float a=i==13?(state.cockpitFlood?.7f:-.7f):float(-2.3+state.panelBrightness*4.6);
  // Pointer moves on the existing physical knob; hit box matches its cylinder.
  for(int j=1;j<=7;j++)cockpitFace(out,q.x+std::sin(a)*j*.0035f,q.y+std::cos(a)*j*.0035f,.850f,.0015f,.0015f,{1,.86f,.5f},transform);
  cockpitText(out,i==13?L"泛光":L"调光",q.x-.04f,-.303f,.850f,.12f,transform);
 }
 for(int i=15;i<18;i++){
  auto q=s[i].center;bool on=i==15?state.avionics.battery:i==16?state.avionics.generator:state.avionics.master;
  box(out,q.x,q.y,q.z,.065f,.028f,.055f,on?XMFLOAT3{.12f,.45f,.28f}:XMFLOAT3{.24f,.25f,.27f},transform);
  box(out,q.x,q.y-.04f,q.z+(on?.02f:-.02f),.008f,.025f,.008f,trim,transform);
  cockpitText(out,i==15?L"电池":i==16?L"发电":L"航电",q.x-.045f,q.y-.07f,q.z-.06f,.13f,transform);
 }
 const wchar_t* lower[]={L"气压-",L"气压+",L"配平-",L"配平+"};
 for(int i=18;i<22;i++){auto q=s[i].center;box(out,q.x,q.y,q.z,.065f,.025f,.027f,trim,transform);cockpitText(out,lower[i-18],q.x-.05f,q.y+.015f,q.z-.029f,.13f,transform);}
 const wchar_t* engineerNames[]={L"一发",L"二发",L"分翼面",L"力矢量",L"复位"};
 for(int i=22;i<27;i++){
  auto q=s[i].center;bool active=i==22?f.engines.units[0].enabled:i==23?f.aircraftId&&f.engines.units[1].enabled:i==24?f.surfaceModel:i==25?state.showForces:false;
  box(out,q.x,q.y,q.z,.065f,.025f,.025f,active?XMFLOAT3{.12f,.5f,.3f}:trim,transform);
  cockpitText(out,engineerNames[i-22],q.x-.05f,q.y+.015f,q.z-.027f,.13f,transform);
 }
 if(powered){
 const double crossTrack=sim::navigationCrossTrack(state);
 cockpitText(out,L"目标 "+targetHeading+L"° / "+n(state.targetAltitude)+L"m  点"+waypointNumber+L" "+n(sim::navigationDistance(state))+L"m\n航路偏差 "+(crossTrack>=0?L"右 ":L"左 ")+n(std::abs(crossTrack))+L"m",-.72f,-.19f,.69f,.66f,transform);
 const auto approach=sim::approachGuidance(state);
 if(approach.valid)cockpitText(out,std::wstring(L"训练进近 ")+approach.runway->name+(approach.direction==1?L" 正":L" 反")+L"  LOC "+(approach.localizer>=0?L"右 ":L"左 ")+n(std::abs(approach.localizer))+L"m  GS "+(approach.glideslope>=0?L"高 ":L"低 ")+n(std::abs(approach.glideslope))+L"m",-.72f,-.12f,.69f,.66f,transform,{1.f,.82f,.3f});
 }
 // Separate interior surfaces from self-lit displays. Neither receives world fog/shadows.
 for(size_t i=firstVertex;i<out.size();i++)out[i].textured=out[i].textured>1.5f?4.f:3.f;
}
// Compact overlays: instrument data stays inside the three-dimensional glass.
inline void drawCockpit(std::vector<Vertex>& out,const sim::Snapshot& state,const std::wstring& notice,bool paused){
 const auto& f=state.flight;
 std::wstring warning=f.crashed?notice:f.stallWarning?L"失速警告 · 减小迎角":!state.avionics.powered(f)?L"航电断电 · Q电池 / M发电 / Y航电":paused?L"已暂停":L"";
 if(!warning.empty()){
  screenQuad(out,300,10,360,28,{.12f,.035f,.018f});chineseFont().draw(out,warning,310,14,{1,.74f,.28f},340);
 }
}
