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
  std::cout<<"PASS actual Windows GDI font: "<<rows<<" metric primary rows, "<<vertices
           <<" glyph vertices fit 300x21 logical text bounds. No screen/GPU/HD6000 validation.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
