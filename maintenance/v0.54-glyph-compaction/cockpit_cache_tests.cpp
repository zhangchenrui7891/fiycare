#include "src/cockpit.h"
#include <cstring>
#include <iostream>

int main(){
 try{
  const XMMATRIX transforms[]={XMMatrixIdentity(),XMMatrixTranslation(2,-3,4),
      XMMatrixRotationZ(.35f)*XMMatrixTranslation(-1,2,3),
      XMMatrixScaling(1.2f,.8f,1.5f)*XMMatrixRotationY(-.6f)};
  std::size_t cases=0;
  for(bool b777:{false,true})for(const auto& transform:transforms){
   std::vector<Vertex> reference,actual;
   buildCockpitInterior(reference,transform,b777);
   for(int frame=0;frame<3;++frame){
    actual.assign(1,Vertex{});
    cockpitInterior(actual,transform,b777);
    if(actual.size()!=reference.size()+1||
       std::memcmp(actual.data()+1,reference.data(),reference.size()*sizeof(Vertex))!=0)
     throw std::runtime_error("Cached cockpit output changed geometry, order, transform or caller prefix");
    ++cases;
   }
  }
  for(bool b777:{false,true})for(const auto& transform:transforms){
   std::vector<Vertex> reference,actual;
   buildCockpitFixedMesh(reference,transform,b777);
   for(int frame=0;frame<3;++frame){
    actual.assign(1,Vertex{});
    cockpitFixedMesh(actual,transform,b777);
    if(actual.size()!=reference.size()+1||
       std::memcmp(actual.data()+1,reference.data(),reference.size()*sizeof(Vertex))!=0)
     throw std::runtime_error("Fixed cockpit cache changed transformed geometry or final materials");
    ++cases;
   }
  }
  std::cout<<"PASS "<<cases<<" interior byte comparisons using Windows DirectXMath, both variants and identity/translation/rotation/scaling. CPU only.\n";
  return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
