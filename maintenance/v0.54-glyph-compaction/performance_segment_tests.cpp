#include "src/performance.h"
#include <iostream>
#include <stdexcept>
#include <vector>

void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 const timing::FrameContext base{3,1,1,0,true,false,true,false,false,1600,900};
 std::vector<timing::FrameContext> changes;
 auto c=base;c.aircraft=2;changes.push_back(c);
 c=base;c.camera=0;changes.push_back(c);
 c=base;c.terrain=0;changes.push_back(c);
 c=base;c.menu=1;changes.push_back(c);
 c=base;c.antiAlias=false;changes.push_back(c);
 c=base;c.low=true;changes.push_back(c);
 c=base;c.active=false;changes.push_back(c);
 c=base;c.walking=true;changes.push_back(c);
 c=base;c.powerSaver=true;changes.push_back(c);
 c=base;c.width=3200;changes.push_back(c);
 c=base;c.height=1800;changes.push_back(c);
 for(const auto& changed:changes){
  timing::FrameContextGate gate;
  check(gate.changed(base),"First configuration must establish a boundary");
  for(int i=0;i<600;++i)check(!gate.changed(base),"Stable configuration must retain samples");
  check(gate.changed(changed),"Configuration change was not detected");
  check(!gate.changed(changed),"Stable new configuration reset repeatedly");
  check(gate.changed(base),"Returning to previous configuration must reset again");
 }
 std::cout<<"PASS 11 independent configuration transitions and 6600 stable-frame checks. CPU only.\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
