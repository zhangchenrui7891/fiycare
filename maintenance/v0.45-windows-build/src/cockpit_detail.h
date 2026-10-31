#pragma once
#include "scene.h"
// Cylindrical instrument bezels, knobs and fasteners; axis points along Z.
inline void cockpitCylinder(std::vector<Vertex>& out,float x,float y,float z,float radius,float depth,XMFLOAT3 color,const XMMATRIX& transform,int segments=32){
 for(int i=0;i<segments;i++){
  float a=i*XM_2PI/segments,b=(i+1)*XM_2PI/segments;
  XMFLOAT3 p{x+radius*std::cos(a),y+radius*std::sin(a),z-depth},q{x+radius*std::cos(b),y+radius*std::sin(b),z-depth};
  XMFLOAT3 r{p.x,p.y,z+depth},s{q.x,q.y,z+depth},c{x,y,z-depth};
  auto t=[&](XMFLOAT3 v){XMFLOAT3 w;XMStoreFloat3(&w,XMVector3TransformCoord(XMLoadFloat3(&v),transform));return w;};
  tri(out,t(c),t(p),t(q),color);tri(out,t(p),t(r),t(q),{color.x*.65f,color.y*.65f,color.z*.65f});tri(out,t(q),t(r),t(s),color);
 }
}
inline void cockpitFace(std::vector<Vertex>& out,float x,float y,float z,float w,float h,XMFLOAT3 color,const XMMATRIX& transform){
 size_t start=out.size();box(out,x,y,z,w,h,.0003f,color,transform);for(size_t i=start;i<out.size();i++)out[i].textured=2;
}
inline void cockpitAttitude(std::vector<Vertex>& out,float x,float y,float z,double pitch,double roll,const XMMATRIX& transform){
 // Clip the moving ground half-plane against the display rectangle.
 const float w=.155f,h=.096f,s=float(std::sin(roll)),c=float(std::cos(roll)),offset=float(std::clamp(pitch,-.6,.6))*.15f;
 cockpitFace(out,x,y,z,w,h,{.12f,.39f,.64f},transform);
 std::vector<XMFLOAT3> polygon{{-w,-h,0},{w,-h,0},{w,h,0},{-w,h,0}},clipped;
 auto distance=[&](XMFLOAT3 p){return p.x*s+p.y*c+offset;};
 for(size_t i=0;i<polygon.size();i++){auto a=polygon[i],b=polygon[(i+1)%polygon.size()];float da=distance(a),db=distance(b);if(da<=0)clipped.push_back(a);if((da<=0)!=(db<=0)){float t=da/(da-db);clipped.push_back({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,0});}}
 auto point=[&](XMFLOAT3 p){p.x+=x;p.y+=y;p.z=z-.001f;XMFLOAT3 v;XMStoreFloat3(&v,XMVector3TransformCoord(XMLoadFloat3(&p),transform));return v;};
 size_t first=out.size();for(size_t i=1;i+1<clipped.size();i++)tri(out,point(clipped[0]),point(clipped[i]),point(clipped[i+1]),{.40f,.23f,.12f});for(size_t i=first;i<out.size();i++)out[i].textured=2;
 for(int i=-3;i<=3;i++){float yy=y+i*.022f;if(i!=0)cockpitFace(out,x,yy,z-.003f,i%2?.018f:.032f,.001f,{.9f,.93f,.95f},transform);}
 cockpitFace(out,x-.055f,y,z-.005f,.035f,.002f,{1,.85f,.13f},transform);cockpitFace(out,x+.055f,y,z-.005f,.035f,.002f,{1,.85f,.13f},transform);cockpitFace(out,x,y-.008f,z-.005f,.002f,.01f,{1,.85f,.13f},transform);
}
inline void cockpitInterior(std::vector<Vertex>& out,const XMMATRIX& transform){
 const XMFLOAT3 shell{.20f,.23f,.25f},edge{.075f,.085f,.10f},metal{.40f,.43f,.45f};
 box(out,0,-1.06f,1.0f,1.10f,.15f,.09f,shell,transform);
 // Angled glare shield, side consoles and fabric seats.
 box(out,0,-.22f,1.01f,1.06f,.035f,.12f,edge,transform);
 for(float side:{-1.f,1.f}){
  box(out,side*1.13f,-.58f,.1f,.09f,.48f,1.25f,shell,transform);
  box(out,side*.97f,-.76f,-.10f,.16f,.065f,.51f,edge,transform);
  box(out,side*.53f,-.98f,-.64f,.28f,.10f,.34f,{.16f,.20f,.24f},transform);
  box(out,side*.53f,-.61f,-.94f,.28f,.37f,.08f,{.18f,.22f,.26f},transform);
  box(out,side*.53f,-.20f,-.94f,.19f,.12f,.085f,{.12f,.16f,.20f},transform);
  for(int i=0;i<7;i++)box(out,side*.53f,-.84f+i*.065f,-1.025f,.23f,.002f,.002f,metal,transform);
  for(float z:{-.72f,-.3f})box(out,side*.53f,-1.14f,z,.025f,.10f,.03f,metal,transform);
  // Side-stick with grip and trigger, decorative until flight axes are connected.
  box(out,side*.95f,-.64f,-.05f,.04f,.11f,.035f,metal,transform);box(out,side*.95f,-.51f,-.05f,.045f,.055f,.055f,edge,transform);
  // Angled windscreen pillars and side window sill.
  box(out,0,0,0,.035f,.59f,.045f,shell,XMMatrixRotationZ(side*.30f)*XMMatrixTranslation(side*.94f,.47f,1.20f)*transform);
  box(out,side*1.01f,.19f,.18f,.045f,.035f,1.0f,shell,transform);
 }
 // Overhead electrical panel, individual switch housings and indicator lamps.
 box(out,0,.87f,.52f,.48f,.06f,.39f,edge,transform);
 for(int row=0;row<1;row++)for(int col=0;col<8;col++){
  float x=-.39f+col*.11f,z=.23f+row*.18f;
  box(out,x,.795f,z,.034f,.012f,.045f,shell,transform);box(out,x,.775f,z,.007f,.023f,.007f,metal,transform);
  box(out,x+.023f,.778f,z+.023f,.005f,.004f,.008f,{.22f,.72f,.47f},transform);
 }
 // Pedestal and paired throttle tracks, flap detent marks.
 box(out,.63f,-.96f,.28f,.24f,.08f,.56f,edge,transform);
 for(float x:{.575f,.685f}){box(out,x,-.83f,.56f,.022f,.08f,.22f,shell,transform);for(int j=0;j<7;j++)box(out,x+.028f,-.743f,.38f+j*.05f,.015f,.001f,.002f,metal,transform);}
 for(int row=0;row<5;row++)for(int col=0;col<4;col++)box(out,.48f+col*.095f,-.865f,-.06f-row*.075f,.027f,.012f,.020f,shell,transform);
 // Screen bezels, screws and ventilation slots.
 for(float x:{-.55f,0.f,.55f}){
  box(out,x,-.735f,.90f,.245f,.169f,.018f,edge,transform);
  cockpitFace(out,x,-.735f,.879f,.224f,.147f,{.016f,.025f,.032f},transform);
  for(float dx:{-.232f,.232f})for(float dy:{-.156f,.156f})cockpitCylinder(out,x+dx,-.735f+dy,.876f,.006f,.002f,metal,transform,12);
  for(int k=0;k<7;k++)cockpitCylinder(out,x-.18f+k*.06f,-.89f,.871f,.009f,.009f,shell,transform,16);
 }
 for(int i=0;i<24;i++)box(out,-.94f+i*.082f,-.97f,.887f,.019f,.002f,.002f,edge,transform);
 for(float x:{-.94f,.94f})cockpitCylinder(out,x,-.37f,.88f,.033f,.025f,metal,transform);
}
