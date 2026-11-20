#include "src/chinese_text.h"
#include "src/flight_instruments.h"
#include <iostream>
#include <limits>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 try{
  ChineseFont font;std::size_t rows=0,vertices=0;
  for(double speed:{0.,100.,500.})for(double altitude:{-500.,0.,30000.})
   for(double vs:{-500.,-.4,0.,.4,500.}){
    const auto text=instruments::primary(speed,altitude,vs,6.28);
    for(const auto& line:{text.speedAltitude,text.headingVertical}){
     std::vector<Vertex> mesh;font.draw(mesh,line,0,0,{1,1,1},900);
     require(!mesh.empty(),"GDI primary label has visible glyph geometry");
     for(const auto& v:mesh){
      const float x=(v.x+1)*480,y=(1-v.y)*270;
      require(x>=-.01f&&x<=300.01f,"Primary label exceeds its 300-pixel cockpit scale width");
      require(y>=-.01f&&y<=21.01f,"Primary label exceeds a single text row");
     }
     vertices+=mesh.size();++rows;
    }
   }
  for(double x:{0.,30.,-30.,400.})for(double z:{0.,40.,-40.,400.}){
   const auto ground=instruments::groundMotion(x,z);
   for(const auto& line:{L"TRK "+ground.track,L"XTK -2500 m   GS "+ground.speed+L" km/h"}){
    std::vector<Vertex> mesh;font.draw(mesh,line,0,0,{1,1,1},900);
    require(!mesh.empty(),"Ground-motion label has visible Windows glyphs");
    for(const auto& v:mesh){
     const float logicalX=(v.x+1)*480*.63f,logicalY=(1-v.y)*270*.63f;
     require(logicalX>=-.01f&&logicalX<=292.f&&logicalY>=-.01f&&logicalY<=16.f,
             "Ground-motion label exceeds the NAV footer space");
    }
    ++rows;vertices+=mesh.size();
   }
  }
  for(double height:{0.,1.6,123.,749.6,750.,751.,double(NAN)}){
   const auto value=instruments::aboveGround(height,0);
   if(value.empty())continue;
   std::vector<Vertex> mesh;font.draw(mesh,L"AGL "+value+L" m",0,0,{1,1,1},900);
   require(!mesh.empty(),"AGL GDI label has visible glyph geometry");
   for(const auto& v:mesh){
    const float x=(v.x+1)*480*.70f,y=(1-v.y)*270*.70f;
    require(x>=-.01f&&x<=84.01f&&y>=-.01f&&y<=16.01f,"AGL text exceeds reserved PFD box");
   }
   ++rows;vertices+=mesh.size();
  }
  for(const auto* text:{L"TRAIN",L"LOC",L"G/S"}){
   std::vector<Vertex> mesh;font.draw(mesh,text,0,0,{1,1,1},900);
   require(!mesh.empty(),"Approach scale label has visible GDI glyphs");
   for(const auto& v:mesh){const float x=(v.x+1)*480*.50f,y=(1-v.y)*270*.50f;
    require(x>=-.01f&&x<=24.01f&&y>=-.01f&&y<=10.01f,"Approach scale label exceeds reserved box");}
   ++rows;vertices+=mesh.size();
  }
  std::cout<<"PASS actual Windows GDI font: "<<rows<<" metric primary rows, "<<vertices
           <<" glyph vertices fit 300x21 logical text bounds. No screen/GPU/HD6000 validation.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
