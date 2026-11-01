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
 const auto heading=instruments::heading(f.yaw);
 const auto targetHeading=instruments::heading(state.targetHeading);
 const bool powered=state.avionics.powered(f);const auto data=sim::airData(f,state.avionics.qnh);
 if(powered){
 cockpitAttitude(out,-.55f,-.735f,.876f,f.pitch,f.roll,transform);
 const auto primary=instruments::primary(data.ias,data.altitude,data.verticalSpeed,f.yaw);
 cockpitText(out,primary.speedAltitude,-.76f,-.60f,.870f,.42f,transform);
 cockpitText(out,primary.headingVertical,-.76f,-.85f,.870f,.42f,transform);
 // Navigation display range rings and heading marker.
 for(float radius:{.045f,.085f,.12f})for(int i=0;i<48;i++){
  float a=i*XM_2PI/48;
  cockpitFace(out,std::sin(a)*radius,-.75f+std::cos(a)*radius,.873f,.0015f,.0015f,{.16f,.48f,.37f},transform);
 }
 cockpitFace(out,0,-.75f,.870f,.003f,.013f,{1,1,1},transform);cockpitFace(out,0,-.755f,.870f,.015f,.002f,{1,1,1},transform);
 if(waypointValid){
  const auto& waypoint=sim::navigationRoute[state.navigationWaypoint];double bearing=std::atan2(waypoint.x-f.x,waypoint.z-f.z);float a=float(bearing-f.yaw);
  cockpitFace(out,std::sin(a)*.09f,-.75f+std::cos(a)*.09f,.870f,.005f,.005f,{1,.2f,.8f},transform);
 }
 cockpitText(out,L"导航  "+heading+L"°",-.19f,-.60f,.870f,.40f,transform);
 cockpitText(out,waypointValid?L"航点 "+waypointNumber+L"  "+n(sim::navigationDistance(state))+L"m":L"航点无效",-.19f,-.85f,.870f,.40f,transform);
 cockpitText(out,L"双发监控",.35f,-.60f,.870f,.40f,transform);
 for(int i=0;i<2;i++){
  float x=.45f+i*.20f;cockpitCylinder(out,x,-.735f,.873f,.063f,.001f,{.08f,.12f,.15f},transform);
  const double actualRpm=f.engines.initialized?(i<f.engines.count(f.aircraftId)?f.engines.units[size_t(i)].rpm:0):f.rpm;
  float a=float(-2.3+std::clamp((actualRpm-700)/1900.,0.,1.)*4.6);
  for(int j=0;j<18;j++){float t=-2.3f+j*4.6f/17;cockpitFace(out,x+std::sin(t)*.059f,-.735f+std::cos(t)*.059f,.869f,.002f,.002f,{.3f,.9f,.6f},transform);}
  for(int j=0;j<12;j++)cockpitFace(out,x+std::sin(a)*j*.004f,-.735f+std::cos(a)*j*.004f,.866f,.0015f,.0015f,{1,1,1},transform);
 }
 cockpitText(out,L"转速 "+n(f.rpm/26)+L"% 燃油 "+n(f.fuel)+L"%",.35f,-.85f,.865f,.40f,transform);
 cockpitText(out,L"TAS "+n(data.tas*3.6)+L" GS "+n(data.groundSpeed*3.6),-.76f,-.92f,.867f,.40f,transform);
 cockpitText(out,L"QNH "+n(state.avionics.qnh),-.19f,-.92f,.867f,.40f,transform);
 cockpitText(out,L"AOA "+n(data.aoaDegrees)+L"  配平 "+n(f.trim*100),.35f,-.92f,.867f,.40f,transform);
 }
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
inline void drawCockpit(std::vector<Vertex>& out,const sim::Snapshot& state,const std::wstring& notice,bool paused){
 screenQuad(out,0,0,960,30,{.025f,.04f,.06f});chineseFont().draw(out,L"三维驾驶舱 · 右键观察 · 左键开关/拖油门 · C切换 · F11全屏 · Tab菜单",12,5,{.88f,.94f,1},940);
 const auto& f=state.flight;const auto data=sim::airData(f,state.avionics.qnh);
 const bool powered=state.avionics.powered(f);
 chineseFont().draw(out,powered?L"航电在线 · "+std::wstring(state.avionics.generating(f)?L"发电机供电":L"电池供电")+L" · 电量 "+std::to_wstring(int(state.avionics.charge*100))+L"%":L"航电断电 · 检查电池 Q / 发电机 M / 航电 Y",12,35,powered?XMFLOAT3{.5f,1,.7f}:XMFLOAT3{1,.5f,.2f},930);
 if(powered){
  std::wostringstream instruments;instruments<<std::fixed<<std::setprecision(1)<<L"IAS "<<data.ias*3.6<<L" / TAS "<<data.tas*3.6<<L" / GS "<<data.groundSpeed*3.6<<L" km/h · M "<<std::setprecision(2)<<data.mach<<L" · QNH "<<std::setprecision(1)<<state.avionics.qnh;
  chineseFont().draw(out,instruments.str(),12,60,{.8f,.95f,1},930);
  chineseFont().draw(out,f.stallWarning?L"失速警告 · 减小迎角":L"迎角 "+std::to_wstring(int(data.aoaDegrees))+L"° · 侧滑 "+std::to_wstring(int(data.betaDegrees))+L"° · 配平 "+std::to_wstring(int(f.trim*100))+L"% · 横风 "+std::to_wstring(int(f.windX))+L"m/s",12,440,f.stallWarning?XMFLOAT3{1,.25f,.1f}:XMFLOAT3{.8f,.95f,1},930);
 }
 chineseFont().draw(out,L"J 仪表亮度 "+std::to_wstring(int(std::lround(state.panelBrightness*100)))+L"% · O 泛光灯 "+(state.cockpitFlood?L"开":L"关"),12,470,{.88f,.94f,1},650);
 chineseFont().draw(out,paused?L"已暂停 · 操作锁定":notice.substr(0,46),12,495,{.88f,.94f,1},930);
}
