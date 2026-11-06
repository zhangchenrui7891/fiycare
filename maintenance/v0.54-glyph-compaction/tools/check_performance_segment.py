"""Execute production rolling-window sampling and boundary statements (CPU only)."""
from pathlib import Path
import os, subprocess, sys, tempfile
root=Path(__file__).resolve().parents[1]
main=Path(sys.argv[1]).read_text() if len(sys.argv)>1 else (root/'src/main.cpp').read_text()
sampling=main[main.index('   if(measured&&previousMeasured)'):main.index('   if(benchmark&&measured')]
boundary='\n'.join(line.strip() for line in main.splitlines() if any(key in line for key in (
    'const auto frameExtent=', 'const timing::FrameContext frameContext{', 'if(frameContextGate.changed(frameContext))')))
if boundary:
    assert main.index('switch(cmd)') < main.index('const timing::FrameContext frameContext{') < main.index('const auto frameStats=') < main.index('if(reportTime>=5&&perf)')
code=r'''
#include "performance.h"
#include "launch_options.h"
#include <iostream>
#include <stdexcept>
using UINT=unsigned;
struct {int aircraftId=3;} flight;
struct {int terrainMode=1;bool antiAlias=true;} state;
enum class Page {None,Pause}; Page menuPage=Page::None;
int camera=1;bool low=false,wasActive=true,walking=false,powerSaver=false;
long clientWidth=1600,clientHeight=900;unsigned appliedScale=1;
timing::FrameWindow frameWindow;timing::FrameContextGate frameContextGate;
bool previousMeasured=false;double reportTime=0;
void frame(double wallDt,bool measured=true){
''' + sampling + r'''
 previousMeasured=measured;
''' + boundary + r'''
}
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void mature(){for(int i=0;i<60;++i)frame(.1);check(reportTime>=5,"Stable frames must reach reporting threshold");}
void clean(){check(frameWindow.summary().samples==0&&reportTime==0,"Configuration transition retained old samples/report timer");
 frame(.3);check(frameWindow.summary().samples==0,"Transition rendering interval must be discarded");
 frame(.02);const auto s=frameWindow.summary();check(s.samples==1&&s.maxMs==20&&s.fps==50,"First stable sample contains old configuration or transition time");}
int main(){try{
 frame(.1);mature();state.antiAlias=false;frame(.1);clean();
 mature();camera=0;frame(.1);clean();
 mature();flight.aircraftId=2;frame(.1);clean();
 mature();powerSaver=true;frame(.1);clean();
 mature();appliedScale=2;frame(.1);clean();
 mature();menuPage=Page::Pause;frame(.1);clean();
 mature();menuPage=Page::None;frame(.1);clean();
 mature();wasActive=false;frame(.1);clean();
 mature();frame(.5,false);check(frameWindow.summary().samples==0&&reportTime==0,"Focus loss retained samples");
 frame(.5,true);check(frameWindow.summary().samples==0,"Focus recovery interval retained");
 frame(.02);check(frameWindow.summary().samples==1,"Focus recovery never resumed");
 std::cout<<"PASS actual main sampling/boundary statements: 8 configuration changes, transition exclusion, five-second restart, focus recovery. CPU only.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d);(p/'test.cpp').write_text(code)
    flags=['-fsanitize=address,undefined','-g'] if os.environ.get('FCAIR_SANITIZE') else []
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic',*flags,'-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    sys.exit(subprocess.run([str(p/'test')]).returncode)
