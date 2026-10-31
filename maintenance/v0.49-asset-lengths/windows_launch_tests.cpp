#include "src/windows_launch.h"
#include <iostream>
#include <stdexcept>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 try{
  const auto exact=launch::fromWindowsCommandLine(L"\"C:\\飞行 项目\\--low\\FlightCareerAir.exe\" --dx11 \"--benchmark\"\t--no-aa");
  require(exact.has(L"--dx11")&&exact.has(L"--benchmark")&&exact.has(L"--no-aa"),"Quoted exact flags and whitespace");
  require(!exact.has(L"--low"),"Executable path excluded");
  const auto misleading=launch::fromWindowsCommandLine(L"FlightCareerAir.exe --benchmark-old --low=1 --dx11-extra \"C:\\中文 目录\\--warp.txt\"");
  require(!misleading.has(L"--benchmark")&&!misleading.has(L"--low")&&!misleading.has(L"--dx11")&&!misleading.has(L"--warp"),"Misleading substrings ignored");
  require(!launch::fromWindowsCommandLine(L"\"C:\\含 空格\\--benchmark.exe\"").has(L"--benchmark"),"Executable-only launch has no flags");
  const auto blanks=launch::fromWindowsCommandLine(L"app.exe \"\" --smoke-test --smoke-test");
  require(blanks.has(L"--smoke-test")&&!blanks.has(L"--low"),"Empty token and duplicate flags");
  std::cout<<"PASS actual Windows CommandLineToArgvW wrapper: executable exclusion, Unicode/quoted paths, quoted flags, misleading substrings, duplicates. No full application/GPU test.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
