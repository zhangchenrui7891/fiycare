#include "src/chinese_text.h"
#include "src/autopilot_feedback.h"
#include <iostream>
int main(){
 try{
  ChineseFont font;std::size_t vertices=0;
  for(int bits=1;bits<8;++bits){
   const sim::AutopilotModes lost{bool(bits&1),bool(bits&2),bool(bits&4)};
   const auto text=sim::autopilotDisconnectMessage(lost);
   if(text.size()>46)throw std::runtime_error("AP message exceeds existing cockpit notice character limit");
   std::vector<Vertex> mesh;font.draw(mesh,text,12,495,{1,1,1},930);
   if(mesh.empty())throw std::runtime_error("AP message has no GDI glyph geometry");
   for(const auto& v:mesh){
    const float x=(v.x+1)*480,y=(1-v.y)*270;
    if(x<11.99f||x>662.01f||y<494.99f||y>516.01f)
     throw std::runtime_error("AP message exceeds notice row or reaches the performance overlay");
   }
   vertices+=mesh.size();
  }
  std::cout<<"PASS actual Windows GDI: seven AP channel messages, "<<vertices
           <<" glyph vertices fit existing notice row. Not screen/GPU/HD6000 validation.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
