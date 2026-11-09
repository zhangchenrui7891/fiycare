"""Exercise actual modal-exit hook and frame accounting with CPU Win32 stand-ins.

Optional main.cpp argument supports testing the preceding release. No Windows
message delivery, interactive resize, GPU or wall-clock performance is tested.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = Path(sys.argv[1]).read_text() if len(sys.argv) > 1 else (root/'src/main.cpp').read_text()
hook = '\n'.join(line for line in main.splitlines() if 'if(msg==WM_EXITSIZEMOVE)' in line)
start = main.index('   double dt=std::min(.25,wallDt);last=now;')
accounting = main[start:main.index('   bool focus=GetForegroundWindow()==window;', start)]
code = r'''
#include "frame_timing.h"
#include "performance.h"
#include "benchmark.h"
#include "keyboard_state.h"
#include <iostream>
#include <stdexcept>
constexpr int WM_EXITSIZEMOVE=0x232;
input::KeyboardState keyboardInput;
bool sizeMoveJustClosed=false,nativeMenuJustClosed=false,inNativeMenu=false,iconic=false;
int foreground=1;
int GetForegroundWindow(){return foreground;}
bool IsIconic(int){return iconic;}
void message(int msg){(void)msg;
''' + hook + r'''
}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Loop {
 timing::FrameWindow frameWindow;
 timing::BenchmarkSamples benchmarkSamples;
 timing::FixedSteps physicsTime;
 bool previousMeasured=true,benchmark=true,benchmarkStarted=false,wasActive=true;
 double last=0,reportTime=0,wallElapsed=0;
 double frame(double wallDt){
  const double now=last+wallDt;const int window=1;
''' + accounting + r'''
  return dt;
 }
};
int main(){try{
 int cases=0;
 for(bool bench:{false,true})for(bool recreated:{false,true})for(double wait:{.001,1.,30.}){
  sizeMoveJustClosed=nativeMenuJustClosed=false;foreground=1;iconic=false;
  Loop loop;loop.benchmark=bench;loop.frame(.02);
  loop.physicsTime.advance(.005,true);
  const auto samples=loop.benchmarkSamples.storedSamples();
  // EXITSIZEMOVE is sent for a move/cancel too, without a size change.
  message(WM_EXITSIZEMOVE);
  if(recreated){loop.previousMeasured=false;loop.physicsTime.reset();loop.wasActive=false;}
  const double dt=loop.frame(wait);
  require(dt==0&&!loop.wasActive&&loop.physicsTime.remainder==0,
   "Modal move/resize return must discard physics time even if client size is unchanged");
  require(loop.frameWindow.summary().samples==0&&loop.reportTime==0,
   "Modal wait must not enter rolling statistics");
  require(loop.benchmarkSamples.storedSamples()==samples,"Modal wait must not enter benchmark samples");
  require(!sizeMoveJustClosed&&!nativeMenuJustClosed&&!loop.previousMeasured,"Return boundary consumed once");
  require(loop.wallElapsed==.02+wait,"Real elapsed timeline remains intact");
  loop.frame(.02); // First clean interval establishes a measurement boundary.
  require(loop.frameWindow.summary().samples==0,"No sample straddles return boundary");
  require(loop.frame(.02)==.02&&loop.frameWindow.summary().samples==1,
   "Normal frame timing and samples resume");
  require(loop.benchmarkSamples.storedSamples()==samples+(bench?1u:0u),"Benchmark resumes on clean interval");
  ++cases;
 }
 Loop ordinary;sizeMoveJustClosed=false;
 require(ordinary.frame(1)==.25&&ordinary.frameWindow.summary().maxMs==1000,
  "Ordinary slow frames must remain visible, not be filtered as modal waits");
 require(ordinary.physicsTime.advance(.25,true)==8,"Existing physics stall cap retained");
 nativeMenuJustClosed=true;
 require(ordinary.frame(3)==0&&!ordinary.wasActive&&ordinary.frameWindow.summary().samples==0,
  "Existing native menu recovery retained");
 input::KeyboardState::Keys keys{};keyboardInput.sample(keys,true);
 keyboardInput.keyDown('G',false,{});message(WM_EXITSIZEMOVE);keys['W']=true;
 keyboardInput.sample(keys,true);
 require(!keyboardInput.pressed()['G']&&!keyboardInput.down()['W']&&keyboardInput.resumed(),
  "Modal exit discards queued actions and gates held keys");
 keys['W']=false;keyboardInput.sample(keys,true);keys['W']=true;keyboardInput.sample(keys,true);
 require(keyboardInput.down()['W'],"Fresh input works after release");
 std::cout<<"PASS "<<cases<<" modal timing cases; ordinary stalls, native menu, benchmark recovery and key gating. Actual main.cpp statements with CPU Win32 stand-ins only.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory(prefix='fcair-window-timing-') as tmp:
    cpp = Path(tmp)/'check.cpp'
    exe = Path(tmp)/'check'
    cpp.write_text(code)
    flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if os.environ.get('FCAIR_SANITIZE') else []
    subprocess.run(['g++', '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror', '-pedantic', *flags,
                    '-I', str(root/'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
