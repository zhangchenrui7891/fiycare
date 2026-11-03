#include "src/launch_options.h"
#include <iostream>
#include <stdexcept>

void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 try{
  const std::vector<std::wstring> flags={L"--benchmark",L"--low",L"--smoke-test",L"--airliner",
   L"--a330",L"--a350",L"--sunset",L"--no-aa",L"--night",L"--flood-off",
   L"--panel-dim",L"--surface-flight",L"--fullscreen",L"--native",L"--cockpit",
   L"--dx11",L"--dx12",L"--warp",L"--supersample"};
  std::size_t cases=0;
  for(const auto& flag:flags){
   require(launch::Options({flag}).has(flag),"Exact token must match");
   for(const auto& text:std::vector<std::wstring>{flag+L"-old",L"prefix"+flag,flag+L"=1",
      L"C:\\测试 目录\\"+flag+L".txt",L" "+flag,flag+L" ",L"\""+flag+L"\""}){
    require(!launch::Options({text}).has(flag),"Substring/value/path must not enable flag");++cases;
   }
   const launch::Options original({flag+L"-old",L"中文 参数"});
   const auto copy=original.with(flag).with(flag);
   require(copy.has(flag)&&!original.has(flag),"Forced renderer low option must preserve source");
   for(const auto& other:flags)
    require(copy.has(other)==(other==flag),"Forcing one flag must not enable another");
   require(launch::Options({flag,flag}).has(flag),"Duplicates are harmless");
  }
  require(!launch::Options().has(L"--low"),"Default launch keeps ordinary quality");
  require(!launch::Options({L"--BENCHMARK"}).has(L"--benchmark"),"Flags remain case sensitive");
  const auto renderer=launch::Options({L"--benchmark",L"--dx11"}).with(L"--low");
  require(renderer.has(L"--benchmark")&&renderer.has(L"--dx11")&&renderer.has(L"--low"),"Diagnostic renderer retains exact backend");
  require(launch::initialRenderScale(launch::Options())==1,"Default scale is native");
  require(launch::initialRenderScale(launch::Options({L"--supersample"}))==2,"Explicit supersampling");
  for(const auto& opts:std::vector<std::vector<std::wstring>>{{L"--native"},{L"--supersample",L"--native"},{L"--native",L"--supersample"},{L"--supersample-old"},{L"--supersample=2"}})
   require(launch::initialRenderScale(launch::Options(opts))==1,"Native precedence and exact tokens");
  std::cout<<"PASS launch options: 19 flags, "<<cases<<" misleading tokens, forcing/copy and backend checks. Tokenized CPU inputs only.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
