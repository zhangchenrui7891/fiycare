#pragma once
#include "graphics_common.h"
#include "simulation.h"
#include <map>
#include <array>
inline void tri(std::vector<Vertex>& out,XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 color){for(auto p:{a,b,c})out.push_back({p.x,p.y,p.z,color.x,color.y,color.z});}
inline void box(std::vector<Vertex>& out,float x,float y,float z,float w,float h,float d,XMFLOAT3 color,const XMMATRIX& transform=XMMatrixIdentity()){
 XMFLOAT3 p[8];for(int i=0;i<8;i++){XMVECTOR v=XMVectorSet(x+((i&1)?w:-w),y+((i&2)?h:-h),z+((i&4)?d:-d),1);XMStoreFloat3(&p[i],XMVector3TransformCoord(v,transform));}
 int indices[]={0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5};
 for(int i=0;i<36;i+=3){float shade=.65f+.35f*(i/6)/5;tri(out,p[indices[i]],p[indices[i+1]],p[indices[i+2]],{color.x*shade,color.y*shade,color.z*shade});}
}
inline float terrain(float x,float z){return (float)sim::terrain(x,z);}
inline void hudText(std::vector<Vertex>& out,const std::string& text){
 static const std::map<char,std::array<int,7>> font={
 {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'C',{14,17,16,16,16,17,14}},
 {'D',{30,17,17,17,17,17,30}},{'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,14}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},
 {'J',{7,2,2,2,18,18,12}},{'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},{'O',{14,17,17,17,17,17,14}},
 {'P',{30,17,17,30,16,16,16}},{'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},{'U',{17,17,17,17,17,17,14}},
 {'V',{17,17,17,17,17,10,4}},{'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},
 {'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},{'8',{14,17,17,14,17,17,14}},
 {'9',{14,17,17,15,1,1,14}},{'-',{0,0,0,31,0,0,0}},{'/',{1,2,2,4,8,8,16}},
 {'.',{0,0,0,0,0,4,4}},{'%',{17,2,4,8,16,17,0}},{'|',{4,4,4,4,4,4,4}},{':',{0,4,4,0,4,4,0}},
 {'+',{0,4,4,31,4,4,0}}};
 auto quad=[&](float x,float y,float w,float h,XMFLOAT3 color){
  float l=x/480-1,r=(x+w)/480-1,t=1-y/270,b=1-(y+h)/270;
  tri(out,{l,t,0},{l,b,0},{r,t,0},color);tri(out,{r,t,0},{l,b,0},{r,b,0},color);
 };
 quad(5,5,950,172,{.025f,.04f,.065f});int x=12,y=12;
 for(char c:text){if(c=='\n'){x=12;y+=16;continue;}auto found=font.find(c);
  if(found!=font.end())for(int row=0;row<7;row++)for(int col=0;col<5;col++)if(found->second[row]&(1<<(4-col)))quad((float)(x+col),(float)(y+row*2),1,2,{.9f,.95f,.8f});
  x+=6;
 }
}

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
