"""Compile actual renderer selection with CPU API/renderer substitutes.
No Windows or GPU execution. Windows parser is covered by a separate Windows target.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
main = (root/'src/main.cpp').read_text()
graphics = (root/'src/graphics.h').read_text()
assert 'renderScale=launch::initialRenderScale(args);' in main
assert '--supersample' in (root/'run_high_quality.bat').read_text()
assert 'args.find(' not in main+graphics
assert 'const auto args=launch::currentOptions();' in main
assert main.count('createRenderer(window,renderArgs)') == 2
assert 'target_link_libraries(FlightCareerAir PRIVATE' in (root/'CMakeLists.txt').read_text()
assert 'comdlg32 shell32)' in (root/'CMakeLists.txt').read_text()
render_options = next(l.strip() for l in main.splitlines() if 'const auto renderArgs=' in l)
renderer = '\n'.join(l for l in graphics.splitlines() if not l.startswith(('#include','#pragma')))
code = r'''
#include "launch_options.h"
#include <iostream>
#include <memory>
#include <stdexcept>
using UINT=unsigned int;using HWND=int;
struct RECT {long left=0,top=0,right=800,bottom=450;};
void GetClientRect(HWND,RECT*){}
void logLine(const std::string&){} void logShaderBytecodeCache(){}
struct IRenderer {UINT width=0,height=0;bool software=false;virtual ~IRenderer()=default;virtual int backend()const=0;};
bool fail12=false;int starts12=0,starts11=0;
struct Renderer12:IRenderer{void init(HWND,bool s){++starts12;if(fail12)throw std::runtime_error("simulated DX12 failure");software=s;}int backend()const override{return 12;}};
struct Renderer11:IRenderer{void init(HWND,bool s){++starts11;software=s;}int backend()const override{return 11;}};
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
''' + renderer + r'''
int main(){
 try{
  auto a=createRenderer(0,launch::Options({L"--dx11-extra",L"--low=1",L"C:\\--warp.txt"}));
  require(a->backend()==12&&a->width==800&&a->height==450&&!a->software,"Misleading options must not change default backend/quality");
  renderScale=2;
  auto high=createRenderer(0,launch::Options());require(high->width==1600&&high->height==900,"Explicit supersampling keeps four times the pixels");
  fail12=true;auto fallback=createRenderer(0,launch::Options());require(fallback->backend()==11&&fallback->width==1600&&fallback->height==900,"Fallback preserves explicit scale");fail12=false;
  renderScale=1;
  auto nativeAgain=createRenderer(0,launch::Options());require(nativeAgain->width==800&&nativeAgain->height==450,"Switching back recreates native resolution");
  auto b=createRenderer(0,launch::Options({L"--dx11",L"--low",L"--warp"}));
  require(b->backend()==11&&b->width==640&&b->height==360&&b->software,"Exact diagnostic options");
  auto c=createRenderer(0,launch::Options({L"--dx11",L"--dx12"}));
  require(c->backend()==11,"Existing DX11 precedence retained");
  fail12=true;
  auto d=createRenderer(0,launch::Options());require(d->backend()==11,"Ordinary DX12 failure falls back");
  const int before=starts11;bool rejected=false;
  try{auto forced=createRenderer(0,launch::Options({L"--dx12"}));(void)forced;}catch(const std::runtime_error&){rejected=true;}
  require(rejected&&starts11==before,"Forced DX12 failure must not silently fallback");
  const launch::Options args({L"--benchmark",L"--dx11"});const bool benchmark=args.has(L"--benchmark");
''' + render_options + r'''
  require(!args.has(L"--low")&&renderArgs.has(L"--low"),"Benchmark adds exact renderer flag without mutating args");
  for(int resize=0;resize<2;++resize){auto r=createRenderer(0,renderArgs);require(r->width==640&&r->backend()==11,"Recreation retains benchmark render options");}
  std::cout<<"PASS actual renderer selection: misleading/exact flags, DX11 precedence, fallback/forced failure, benchmark copy and recreation. CPU renderer/API substitutes only.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
'''
with tempfile.TemporaryDirectory(prefix='fcair-launch-') as tmp:
    src=Path(tmp)/'check.cpp'; exe=Path(tmp)/'check';src.write_text(code)
    flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if os.environ.get('FCAIR_SANITIZE') else []
    subprocess.run(['g++','-std=c++17','-O1','-Wall','-Wextra','-Werror','-pedantic',*flags,'-I',str(root/'src'),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
