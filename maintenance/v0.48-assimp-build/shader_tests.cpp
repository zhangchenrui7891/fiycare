#include "src/graphics_common.h"
#include <iostream>
int main(){
 try{
  auto vs=compileShader("vs","vs_5_0"),ps=compileShader("ps","ps_5_0");
  auto postVS=compilePost("postVS","vs_5_0"),postPS=compilePost("postPS","ps_5_0");
  for(int repeat=0;repeat<20;repeat++){
   if(compileShader("vs","vs_5_0").Get()!=vs.Get()||compileShader("ps","ps_5_0").Get()!=ps.Get()||
      compilePost("postVS","vs_5_0").Get()!=postVS.Get()||compilePost("postPS","ps_5_0").Get()!=postPS.Get())
    throw std::runtime_error("Repeated shader request did not reuse its bytecode blob");
  }
  const auto& stats=shaderBytecodeCache().stats();
  if(stats.compilations!=4||stats.hits!=80||stats.failures)throw std::runtime_error("Unexpected shader compilation/cache counters");
  std::cout<<"PASS: four SM5 programs compiled; 80 repeat requests reused the same blobs (CPU compiler, no GPU)\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
