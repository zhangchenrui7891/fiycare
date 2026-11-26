"""Execute the actual main overlay routing with CPU font/window stand-ins."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root/'src/main.cpp').read_text()
cockpit = (root/'src/cockpit.h').read_text()
overlay = main[main.index('   const bool inCockpit='):main.index('   if(smoke&&smokeFrames==2)')]
panel = (root/'src/engineering_panel.h').read_text()
panel = '\n'.join(line for line in panel.splitlines() if not line.startswith(('#include', '#pragma')))
warning = cockpit[cockpit.index('inline void drawCockpit('):]
if 'hudText(hud,display.str())' in main or '跑道：中心X=0' in main:
    raise RuntimeError('Verbose flight overlay was reintroduced')
if 'int camera=smoke?0:1;' not in main:
    raise RuntimeError('First launch must select the glass cockpit')
code = r'''
#include "approach.h"
#include "flight_instruments.h"
#include "flight_alerts.h"
#include <vector>
#include <stdexcept>
#include <iostream>
struct XMFLOAT3{float x,y,z;};
struct Vertex{};
struct Label{std::wstring text;float x,y;};std::vector<Label> labels;
void screenQuad(std::vector<Vertex>&,float,float,float,float,XMFLOAT3){}
struct Font{void draw(std::vector<Vertex>&,const std::wstring& s,float x,float y,XMFLOAT3,int=900){labels.push_back({s,x,y});}};
Font& chineseFont(){static Font value;return value;}
std::wstring wideUtf8(const std::string& s){return std::wstring(s.begin(),s.end());}
''' + warning + panel + r'''
namespace ui{enum class Page{None,Pause};}
sim::Snapshot state;auto& flight=state.flight;
struct Renderer{const char* name(){return "DX12";}} rendererValue,*renderer=&rendererValue;
struct Stats{double fps=23,p95Ms=43.2;}frameStats;
std::vector<Vertex> hud;int camera=1;bool largeCockpitReadout=false,walking=false,paused=false,powerSaver=false;
ui::Page menuPage=ui::Page::None;sim::AlertManager flightAlerts;std::wstring notice,lastOverlayNotice,saveError;
double overlayNoticeUntil=0,wallElapsed=0;std::map<int,int> customModels;
void drawMenu(std::vector<Vertex>&,ui::Page,const sim::Snapshot&,bool,const std::wstring&){}

void frame(){labels.clear();
''' + overlay + r'''
}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool has(const std::wstring& token){for(const auto& l:labels)if(l.text.find(token)!=std::wstring::npos)return true;return false;}
int main(){
 frame();require(labels.size()==2&&!has(L"IAS"),"Normal cockpit shows only help and attribution");
 largeCockpitReadout=true;frame();require(has(L"IAS")&&has(L"VS "),"Large cockpit readout");
 const auto savedState=state;
 state=sim::pudongA330Final();frame();require(has(L"入口 6000 m")&&!has(L"余长"),"Inbound threshold distance");
 flight.z=-1700;frame();require(!has(L"入口")&&!has(L"余长"),"Airborne beyond threshold");
 flight.y=flight.spec().gearHeight;frame();require(has(L"余长 3500 m")&&!has(L"入口"),"Forward rollout");
 flight.vz=-30;frame();require(has(L"余长 100 m"),"Reverse rollout");
 flight.vx=40;flight.vz=5;frame();require(!has(L"余长"),"Crossing runway");
 flight.vx=0;flight.vz=0;frame();require(!has(L"余长"),"Stopped");
 flight.vz=30;flight.x=36;frame();require(!has(L"余长"),"Outside runway");
 flight.x=3500;flight.z=3200;frame();require(has(L"余长 0 m"),"Second runway exact end");
 state.avionics.master=false;frame();require(!has(L"余长"),"Unpowered");
 state=savedState;
 const double elevation=sim::terrain(flight.x,flight.z);
 const double initialY=flight.y;
 flight.y=elevation+334.;frame();require(has(L"AGL 334 m"),"Final approach geometric height");
 state.avionics.qnh+=10;frame();require(has(L"AGL 334 m"),"AGL is independent of pressure");
 state.avionics.qnh-=10;
 flight.y=elevation+750;frame();require(has(L"AGL 750 m"),"Ceiling inclusive");
 flight.y=elevation+750.1;frame();require(!has(L"AGL")&&has(L"IAS"),"High altitude keeps two rows");
 flight.y=elevation-1;frame();require(has(L"AGL --- m"),"Invalid height does not indicate touchdown");
 flight.y=elevation+2;frame();require(has(L"AGL 2 m"),"Ground reference point keeps gear offset");
 for(const auto& l:labels)if(l.text==L"AGL 2 m")require(l.x==344&&l.y==112,"Dedicated AGL column");
 camera=0;frame();require(!has(L"AGL"),"No AGL overlay in chase view");camera=1;
 menuPage=ui::Page::Pause;frame();require(!has(L"AGL"),"No AGL over menu");menuPage=ui::Page::None;
 flight.crashed=true;frame();require(!has(L"AGL"),"No AGL after crash");flight.crashed=false;
 flight.y=initialY;
 state.avionics.master=false;frame();require(!has(L"AGL")&&!has(L"IAS")&&has(L"航电断电"),"Power loss blanks large readout");
 state.avionics.master=true;walking=true;frame();require(!has(L"IAS"),"Walking hides readout");
 walking=false;largeCockpitReadout=false;
 camera=0;frame();require(has(L"IAS")&&has(L"ALT")&&labels.size()==3,"External view has one compact flight row");
 camera=1;notice=L"AP DISC";wallElapsed=1;frame();require(has(notice),"Changed event is visible");
 wallElapsed=7;frame();require(!has(notice),"Event expires after five seconds");
 notice=L"SAVED";frame();require(has(notice),"New notification restarts timer");
 saveError=notice=L"SAVE FAILED";wallElapsed=100;frame();wallElapsed=200;frame();require(has(saveError),"Save error persists beyond normal notification timeout");
 saveError.clear();frame();require(!has(L"SAVE FAILED"),"Cleared save error stops persistent overlay");
 state.avionics.master=false;frame();require(has(L"航电断电"),"Power loss warning survives minimal HUD");
 state.avionics.master=true;state.flight.stallWarning=true;frame();require(has(L"失速警告"),"Stall warning survives minimal HUD");
 state.flight.crashed=true;state.flight.crashReason="CRASH";frame();require(has(L"CRASH"),"Crash reason survives minimal HUD");
 state.flight.crashed=false;state.flight.stallWarning=false;paused=true;frame();require(has(L"已暂停"),"Pause is not confused with disabled cockpit input");
 paused=false;state.debugPanel=true;frame();require(has(L"P95")&&has(L"DX12"),"Performance is available on F2 only");
 state.flight.y=sim::terrain(state.flight.x,state.flight.z)+400;state.flight.vy=-2;
 state.flight.gear=false;state.flight.flap=0;state.parkingBrake=true;
 flightAlerts.update(state);frame();require(has(L"着陆检查")&&has(L"[待办] 起落架放下")&&has(L"[待办] 着陆襟翼")&&has(L"[待办] 停留刹车释放"),
                 "Real F2 panel selects and binds landing checklist");
 for(const auto& item:labels)if(item.x==655)require(item.y>=121&&item.y+19<=469,"Checklist stays within the engineering panel");
 state.flight.vy=0;flightAlerts.update(state);frame();require(has(L"着陆检查"),"Level approach preserves visible landing checklist");
 state.flight.vy=1;flightAlerts.update(state);frame();require(has(L"起飞检查")&&!has(L"着陆检查"),"Go-around changes the visible checklist");
 state.flight.vy=0;state.parkingBrake=false;
 state.debugPanel=false;frame();require(!has(L"P95")&&!has(L"帧/秒"),"Performance hidden by default");
 menuPage=ui::Page::Pause;frame();require(labels.size()==1,"Menu excludes flight overlay but preserves attribution");
 menuPage=ui::Page::None;state.debugPanel=false;
 flightAlerts.active={{"old",L"OLD WARNING",sim::AlertLevel::Warning,true},{"new",L"NEW WARNING",sim::AlertLevel::Warning,false}};
 frame();require(has(L"NEW WARNING")&&!has(L"OLD WARNING"),"Actual panel foregrounds new same-severity warning");
 flightAlerts.acknowledge();frame();require(has(L"OLD WARNING")&&has(L"已确认"),"Acknowledgement preserves active faults");
 flightAlerts.active.clear();
 std::cout<<"PASS real main overlay statements: cockpit/exterior/menu, timed notifications, power/stall/crash/pause, F2 and required credits. CPU font adapters only.\n";
}
'''
code = code.replace('#include <vector>', '#include <vector>\n#include <map>')
with tempfile.TemporaryDirectory() as folder:
    source = Path(folder)/'overlay.cpp'
    source.write_text(code)
    exe = Path(folder)/'overlay'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-pedantic',
                    '-I',str(root/'src'),str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
