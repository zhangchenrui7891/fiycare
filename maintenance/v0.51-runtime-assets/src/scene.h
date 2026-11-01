#pragma once
#include "graphics_common.h"
#include "simulation.h"
#include "render_stability.h"
#include "scenery.h"
#include <map>
#include <array>
inline void tri(std::vector<Vertex>& out,XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 color){for(auto p:{a,b,c})out.push_back({p.x,p.y,p.z,color.x,color.y,color.z});}
inline void box(std::vector<Vertex>& out,float x,float y,float z,float w,float h,float d,XMFLOAT3 color,const XMMATRIX& transform=XMMatrixIdentity()){
 XMFLOAT3 p[8];for(int i=0;i<8;i++){XMVECTOR v=XMVectorSet(x+((i&1)?w:-w),y+((i&2)?h:-h),z+((i&4)?d:-d),1);XMStoreFloat3(&p[i],XMVector3TransformCoord(v,transform));}
 int indices[]={0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5};
 for(int i=0;i<36;i+=3){float shade=1;tri(out,p[indices[i]],p[indices[i+1]],p[indices[i+2]],{color.x*shade,color.y*shade,color.z*shade});}
}
inline float terrain(float x,float z){return (float)sim::terrain(x,z);}
inline void gateMesh(std::vector<Vertex>& out,float x,float z,float y){
 const XMFLOAT3 gold{1,.8f,.12f};
 box(out,x-75,y,z,2,60,2,gold);box(out,x+75,y,z,2,60,2,gold);
 box(out,x,y+60,z,75,2,2,gold);box(out,x,y-60,z,75,2,2,gold);
}
inline void planeMesh(std::vector<Vertex>& out,const XMMATRIX& transform,const sim::Flight& f){
 auto point=[&](XMFLOAT3 p){XMFLOAT3 v;XMStoreFloat3(&v,XMVector3TransformCoord(XMLoadFloat3(&p),transform));return v;};
 const float stations[]={-3.2f,-2.1f,0.f,2.0f,3.1f};
 const float radii[]={.12f,.28f,.62f,.42f,.08f};
 for(int ring=0;ring<4;ring++)for(int n=0;n<8;n++){
  float a=n*XM_2PI/8,b=(n+1)*XM_2PI/8;
  auto p1=point({std::cos(a)*radii[ring],std::sin(a)*radii[ring],stations[ring]});
  auto p2=point({std::cos(b)*radii[ring],std::sin(b)*radii[ring],stations[ring]});
  auto p3=point({std::cos(a)*radii[ring+1],std::sin(a)*radii[ring+1],stations[ring+1]});
  auto p4=point({std::cos(b)*radii[ring+1],std::sin(b)*radii[ring+1],stations[ring+1]});
  XMFLOAT3 c{.65f+.2f*std::abs(std::sin(a)),.70f+.18f*std::abs(std::sin(a)),.76f+.18f*std::abs(std::sin(a))};
  tri(out,p1,p3,p2,c);tri(out,p2,p3,p4,c);
 }
 const XMFLOAT3 red{.8f,.15f,.08f};
 auto left=XMMatrixRotationZ(-(float)f.wingFlex/5)*transform;
 auto right=XMMatrixRotationZ((float)f.wingFlex/5)*transform;
 box(out,-2.8f,.15f,.1f,2.7f,.08f,.65f,red,left);box(out,2.8f,.15f,.1f,2.7f,.08f,.65f,red,right);
 box(out,0,.30f,-2.4f,1.6f,.06f,.45f,red,transform);box(out,0,.95f,-2.5f,.07f,.65f,.45f,red,transform);
 box(out,0,.5f,.8f,.42f,.16f,.60f,{.10f,.20f,.30f},transform);
 auto prop=XMMatrixRotationZ((float)(f.time*f.rpm*.04))*transform;
 box(out,0,0,3.25f,.08f,1.05f,.04f,{.12f,.12f,.12f},prop);
 if(f.gear){
  box(out,-1,-.95f,0,.07f,.5f,.07f,{.6f,.6f,.6f},transform);box(out,1,-.95f,0,.07f,.5f,.07f,{.6f,.6f,.6f},transform);
  box(out,-1,-1.30f,0,.20f,.25f,.3f,{.05f,.05f,.05f},transform);box(out,1,-1.30f,0,.20f,.25f,.3f,{.05f,.05f,.05f},transform);
  box(out,0,-1.05f,2.1f,.12f,.4f,.15f,{.12f,.12f,.12f},transform);
 }
}

inline void airlinerMesh(std::vector<Vertex>& out,const XMMATRIX& transform,const sim::Flight& f){
 auto p=[&](XMFLOAT3 v){XMFLOAT3 result;XMStoreFloat3(&result,XMVector3TransformCoord(XMLoadFloat3(&v),transform));return result;};
 const float z[]={-15,-12,-7,7,12,15},r[]={.15f,.95f,1.65f,1.65f,1.35f,.15f};
 for(int ring=0;ring<5;ring++)for(int n=0;n<12;n++){
  float a=n*XM_2PI/12,b=(n+1)*XM_2PI/12;
  XMFLOAT3 v1=p({std::cos(a)*r[ring],std::sin(a)*r[ring],z[ring]}),v2=p({std::cos(b)*r[ring],std::sin(b)*r[ring],z[ring]}),v3=p({std::cos(a)*r[ring+1],std::sin(a)*r[ring+1],z[ring+1]}),v4=p({std::cos(b)*r[ring+1],std::sin(b)*r[ring+1],z[ring+1]});
  float shade=.72f+.22f*std::max(0.f,std::sin(a));XMFLOAT3 color{shade,shade+.025f,shade+.055f};tri(out,v1,v3,v2,color);tri(out,v2,v3,v4,color);
 }
 const XMFLOAT3 blue{.05f,.28f,.63f},grey{.57f,.64f,.70f};
 for(int side:{-1,1}){
  const float sx=(float)side,flex=(float)f.wingFlex;
  XMFLOAT3 a=p({sx*1.4f,.0f,3.5f}),b=p({sx*14,flex,-3.5f}),c=p({sx*14,flex,-5.2f}),d=p({sx*1.4f,.0f,-1.5f});tri(out,a,b,c,grey);tri(out,a,c,d,grey);
  box(out,sx*14,flex+.7f,-4.2f,.10f,.8f,.8f,blue,transform);
  box(out,sx*3.8f,.35f,-11.5f,3.5f,.10f,.7f,grey,transform);
  for(int n=0;n<18;n++)box(out,sx*1.62f,.55f,-9+n*1.05f,.055f,.18f,.22f,{.05f,.15f,.22f},transform);
  box(out,sx*1.61f,-.05f,0,.06f,.14f,10.5f,blue,transform);
  const float ex=sx*4.8f,ey=-1.5f;
  for(int n=0;n<12;n++){
   float a1=n*XM_2PI/12,a2=(n+1)*XM_2PI/12;
   XMFLOAT3 a3=p({ex+std::cos(a1)*.72f,ey+std::sin(a1)*.72f,1.2f}),b3=p({ex+std::cos(a2)*.72f,ey+std::sin(a2)*.72f,1.2f}),c3=p({ex+std::cos(a1)*.80f,ey+std::sin(a1)*.80f,4.4f}),d3=p({ex+std::cos(a2)*.80f,ey+std::sin(a2)*.80f,4.4f});
   tri(out,a3,c3,b3,grey);tri(out,b3,c3,d3,grey);tri(out,p({ex,ey,4.42f}),c3,d3,{.06f,.08f,.10f});
  }
  box(out,ex,-.7f,2.5f,.12f,.55f,.8f,grey,transform);
  if(f.gear){box(out,sx*2.7f,-2.55f,-2.5f,.13f,.85f,.13f,grey,transform);box(out,sx*2.7f,-3.35f,-2.5f,.38f,.45f,.7f,{.04f,.04f,.04f},transform);}
 }
 box(out,0,2.8f,-12,.13f,2.8f,1.9f,blue,transform);box(out,0,1.05f,11.5f,.85f,.32f,.60f,{.04f,.13f,.19f},transform);
 if(f.gear){box(out,0,-2.55f,10,.12f,.8f,.12f,grey,transform);box(out,0,-3.35f,10,.25f,.45f,.45f,{.04f,.04f,.04f},transform);}
}

inline void importedPlaneMesh(std::vector<Vertex>& out,const XMMATRIX& transform,const sim::Flight& f,const ImportedModel& model){
 for(size_t t=0;t<model.gear.size();t++){
  if(!f.gear&&model.gear[t])continue;
  for(size_t k=0;k<3;k++){Vertex v=model.vertices[t*3+k];XMFLOAT3 p;XMStoreFloat3(&p,XMVector3TransformCoord(XMVectorSet(v.x,v.y,v.z,1),transform));v.x=p.x;v.y=p.y;v.z=p.z;XMFLOAT3 n;XMStoreFloat3(&n,XMVector3TransformNormal(XMVectorSet(v.nx,v.ny,v.nz,0),transform));v.nx=n.x;v.ny=n.y;v.nz=n.z;out.push_back(v);}
 }
}
inline void a330Gear(std::vector<Vertex>& out,const XMMATRIX& transform){
 const XMFLOAT3 metal{.55f,.57f,.6f},rubber{.035f,.04f,.045f};
 for(float x:{-4.5f,4.5f}){box(out,x,-3.f,-1,.12f,1.1f,.12f,metal,transform);for(float z:{-2.f,-.5f})box(out,x,-4.2f,z,.65f,.6f,.65f,rubber,transform);}
 box(out,0,-3.1f,23,.12f,.9f,.12f,metal,transform);box(out,0,-4.3f,23,.4f,.5f,.5f,rubber,transform);
}
inline void supplementalGear(std::vector<Vertex>& out,const XMMATRIX& transform,const sim::Flight& f){
 const auto d=sim::airframeDimensions(f.aircraftId);const float h=float(f.spec().gearHeight);
 const float radius=std::min(.55f,float(d.length)*.012f);const XMFLOAT3 metal{.58f,.61f,.64f},rubber{.035f,.04f,.045f};
 for(int side:{-1,1}){
  const float x=side*float(d.span)*.08f,z=-float(d.length)*.06f;
  box(out,x,-h+1.2f,z,.10f,1.0f,.10f,metal,transform);
  box(out,x,-h+radius,z,radius*.65f,radius,radius*1.3f,rubber,transform);
 }
 const float z=float(d.length)*.36f;
 box(out,0,-h+1.05f,z,.08f,.85f,.08f,metal,transform);box(out,0,-h+radius*.8f,z,radius*.45f,radius*.8f,radius,rubber,transform);
}
inline std::vector<Vertex> landscape(int tx,int tz,XMFLOAT3 ground,int mode){
 std::vector<Vertex> out;out.reserve(28000);
 auto vertex=[&](float x,float z){
  Vertex v{x,(float)sim::terrain(x,z,mode),z,ground.x,ground.y,ground.z};
  if(mode==1){auto uv=sim::satelliteUV(x,z);if(uv.covered){v.u=uv.u;v.v=uv.v;v.textured=1;v.r=v.g=v.b=1;}}
  else{
   float river=-1500+200*std::sin(z*.0007f);
   if(std::abs(x-river)<120){v.r=.12f;v.g=.32f;v.b=.45f;}
   else if(v.y>440){float snow=std::clamp((v.y-440)/300.f,0.f,1.f);v.r=.5f+.35f*snow;v.g=.5f+.35f*snow;v.b=.48f+.4f*snow;}
   else {float tone=.9f+.1f*std::sin(x*.01f+z*.012f);v.r*=tone;v.g*=tone;v.b*=tone;}
  }float dx=(float)(sim::terrain(x+1,z,mode)-sim::terrain(x-1,z,mode))*.5f,dz=(float)(sim::terrain(x,z+1,mode)-sim::terrain(x,z-1,mode))*.5f;float len=std::sqrt(dx*dx+1+dz*dz);v.nx=-dx/len;v.ny=1/len;v.nz=-dz/len;return v;
 };
 for(int ix=-24;ix<24;ix++)for(int iz=-24;iz<24;iz++){
  float x=tx*400.f+ix*200,z=tz*400.f+iz*200;
  auto a=vertex(x,z),b=vertex(x+200,z),c=vertex(x,z+200),d=vertex(x+200,z+200);if(mode==1&&(!a.textured||!b.textured||!c.textured||!d.textured)){for(auto* v:{&a,&b,&c,&d}){v->textured=0;v->r=ground.x;v->g=ground.y;v->b=ground.z;}}out.insert(out.end(),{a,c,b,b,c,d});
 }
 if(mode==0){
  const int centerX=int(std::floor(tx*400./450.)),centerZ=int(std::floor(tz*400./550.));
  for(int ix=-8;ix<=8;ix++)for(int iz=-7;iz<=7;iz++){
   const auto tree=presentation::treePosition(centerX+ix,centerZ+iz);
   float x=float(tree.x),z=float(tree.z);
   if(std::abs(x)<240&&std::abs(z)<2500)continue;
   if(std::abs(x+1500-200*std::sin(z*.0007f))<170)continue;
   float y=(float)sim::terrain(x,z,mode);box(out,x,y+3,z,.6f,3,.6f,{.28f,.19f,.1f});
   tri(out,{x-4,y+3,z-4},{x+4,y+3,z-4},{x,y+14,z},{.13f,.3f,.12f});tri(out,{x+4,y+3,z-4},{x+4,y+3,z+4},{x,y+14,z},{.1f,.26f,.1f});tri(out,{x+4,y+3,z+4},{x-4,y+3,z+4},{x,y+14,z},{.15f,.34f,.13f});tri(out,{x-4,y+3,z+4},{x-4,y+3,z-4},{x,y+14,z},{.12f,.29f,.11f});
  }
  for(int n=0;n<30;n++){float x=-600-(n%5)*75.f,z=-800+(n/5)*110.f,y=(float)sim::terrain(x,z,mode);box(out,x,y+6,z,15,6,20,{.55f,.48f,.39f});box(out,x,y+13,z,17,1,22,{.45f,.2f,.13f});}
 }
 if(mode==1)scenery::append(out,tx,tz,{ground.x,ground.y,ground.z});
 return out;
}
inline void airportBuildings(std::vector<Vertex>& out){
 box(out,110,.03f,-400,50,.03f,950,{.24f,.25f,.26f});box(out,255,.04f,-250,115,.04f,450,{.31f,.32f,.33f});
 for(float z:{-1200.f,-700.f,-200.f,300.f})box(out,85,.06f,z,50,.03f,12,{.27f,.28f,.29f});
 box(out,350,8,-250,30,8,260,{.65f,.67f,.69f});box(out,350,16.5f,-250,35,.5f,267,{.28f,.38f,.45f});
 for(int n=0;n<8;n++){float z=-470+n*65.f;box(out,300,5,z,25,1.7f,3,{.48f,.58f,.64f});box(out,279,3.5f,z,4,3.5f,4,{.32f,.4f,.46f});}
 box(out,390,12,170,5,12,5,{.68f,.68f,.65f});box(out,390,27,170,9,3,9,{.14f,.29f,.38f});
 for(int n=0;n<3;n++)box(out,260,10,-920-n*100.f,40,10,35,{.5f,.55f,.6f});
 for(int n=0;n<10;n++){float z=-450+n*75.f;box(out,245,1.6f,z,1.3f,1,3,{.78f,.79f,.7f});box(out,245,2.7f,z+1.5f,1.15f,.5f,1,{.2f,.4f,.48f});}
 for(int z=-1300;z<500;z+=60)box(out,110,.12f,(float)z,.18f,.01f,8,{.95f,.75f,.16f});
}
