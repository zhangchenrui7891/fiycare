"""Compile actual main.cpp key-message hook, polling, bindings and Controls.

Foreground/iconic/key APIs are CPU stand-ins. This checks wiring and state,
not Windows message delivery, focus behaviour, keyboard hardware or graphics.
"""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
start=main.index(' if(msg==WM_KEYDOWN||msg==WM_KEYUP){')
event_hook=main[start:main.index(' if(msg==WM_KILLFOCUS)',start)]
start=main.index('   bool focus=GetForegroundWindow()==window;')
polling=main[start:main.index('   if(IsIconic(window))',start)]
start=main.index('    if(pressed[VK_F2])')
bindings=main[start:main.index("    }\n    if(pressed['T'])",start)]
start=main.index('     sim::Controls controls;controls.pitch=')
controls=main[start:main.index('\n',start)]
for hook in ('WM_ENTERMENULOOP','WM_EXITMENULOOP','WM_KILLFOCUS'):
    if f'if(msg=={hook}){{'+('menuPressedCommand=-1;' if hook!='WM_EXITMENULOOP' else '')+'keyboardInput.suspend();' not in main:
        raise RuntimeError(f'Missing keyboard suspension in {hook}')
for text in ('if(msg==WM_ENTERSIZEMOVE){menuPressedCommand=-1;keyboardInput.suspend();cockpitInput.cancel();cockpitMouseDown=false;if(GetCapture()==h)ReleaseCapture();}',
             'keyboardInput.suspend();cockpitInput.cancel();renderer.reset();',
             'case ui::Help:help(window);keyboardInput.suspend();',
             'keyboardInput.suspend();physicsTime.reset();previousMeasured=false;',
             'inline void toggleFullscreen(HWND h){\n keyboardInput.suspend();'):
    if text not in main:
        raise RuntimeError(f'Missing recovery hook: {text}')

prefix=r'''
#include "keyboard_state.h"
#include "cockpit_controls.h"
#include "flight_alerts.h"
#include <iostream>
#include <sstream>
constexpr int VK_BACK=8,VK_SHIFT=16,VK_CONTROL=17,VK_SPACE=32,VK_PRIOR=33,VK_NEXT=34,
 VK_END=35,VK_HOME=36,VK_LEFT=37,VK_UP=38,VK_RIGHT=39,VK_DOWN=40,
 VK_F2=113,VK_F3=114,VK_F6=117,VK_F7=118,VK_F8=119,VK_F9=120;
constexpr int WM_KEYDOWN=0x100,WM_KEYUP=0x101;
input::KeyboardState keyboardInput;
input::KeyboardState::Keys physical{},messagePhysical{};
int foreground=1,keyReads=0;
bool iconic=false,inNativeMenu=false;
std::wstring notice;
std::vector<std::string> logs;
void logLine(const std::string& value){logs.push_back(value);}
int GetForegroundWindow(){return foreground;}
bool IsIconic(int){return iconic;}
int GetAsyncKeyState(int key){++keyReads;return physical[std::size_t(key)]?0x8000:0;}
int GetKeyState(int key){return messagePhysical[std::size_t(key)]?0x8000:0;}
bool key(int key){return (GetAsyncKeyState(key)&0x8000)!=0;}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::string saved(const sim::Snapshot& state){std::ostringstream out;state.write(out);return out.str();}
struct Result{input::KeyboardState::Keys down,pressed;sim::Controls controls;};
void message(int h,int msg,std::size_t w,long long l){
'''+event_hook+r'''
}
void sendKey(std::size_t key,bool down,bool repeat=false){
 if(key<messagePhysical.size())messagePhysical[key]=down;
 message(1,down?WM_KEYDOWN:WM_KEYUP,key,repeat?(1LL<<30):0);
}
Result frame(sim::Snapshot& state,bool focused,bool minimized,bool nativeMenu,
             const input::KeyboardState::Keys& raw){
 const int window=1;foreground=focused?1:2;iconic=minimized;inNativeMenu=nativeMenu;
 physical=raw;auto& flight=state.flight;sim::AlertManager flightAlerts;
 for(std::size_t k=0;k<raw.size();++k)if(raw[k]!=messagePhysical[k])sendKey(k,raw[k]);
'''
ending=r'''
 return {down,pressed,controls};
}
int main(){
 sim::Snapshot state;state.flight.configure(3);input::KeyboardState::Keys raw{};
 frame(state,true,false,false,raw);
 const auto before=saved(state);
 const int recoveryKeys[]={'G','F','I','Q','M','Y','W',VK_F6,VK_F7};
 for(int key:recoveryKeys)raw[std::size_t(key)]=true;
 keyReads=0;frame(state,false,false,false,raw);
 require(keyReads==0,"Do not poll physical keys while another app is foreground");
 auto result=frame(state,true,false,false,raw);
 require(saved(state)==before&&result.controls.throttle==0,"Recovery cannot toggle aircraft systems/faults or deliver held throttle");
 require(!notice.empty()&&!logs.empty(),"Recovery notice and count-only diagnostic are wired");
 for(int frameIndex=0;frameIndex<120;frameIndex++){
  result=frame(state,true,false,false,raw);
  require(saved(state)==before&&result.controls.throttle==0,"Held recovery inputs stay neutral in actual bindings");
 }
 raw.fill(false);frame(state,true,false,false,raw);
 raw['G']=true;frame(state,true,false,false,raw);
 require(!state.flight.gear,"Fresh G still retracts gear through actual main binding");
 frame(state,true,false,false,raw);require(!state.flight.gear,"Held G must not repeat");
 raw.fill(false);frame(state,true,false,false,raw);
 raw['W']=true;result=frame(state,true,false,false,raw);
 require(result.controls.throttle==1,"Fresh W reaches actual continuous Controls construction");
 raw.fill(false);frame(state,true,false,false,raw);
 raw[VK_F6]=true;frame(state,true,false,false,raw);
 require(state.flight.engines.units[0].fault==sim::EngineFault::Flameout,"Fresh F6 still injects left fault exactly as before");
 raw.fill(false);frame(state,true,false,false,raw);
 raw[VK_F7]=true;frame(state,true,false,false,raw);
 require(state.flight.engines.units[1].fault==sim::EngineFault::Flameout,"Fresh F7 still injects right fault");
 raw.fill(false);frame(state,true,false,false,raw);
 const auto after=saved(state);raw['Q']=true;raw['F']=true;
 keyReads=0;frame(state,true,false,true,raw);
 require(keyReads==0,"Native menu suspends polling");
 frame(state,true,false,false,raw);require(saved(state)==after,"Native-menu return does not replay held system keys");
 keyReads=0;frame(state,true,true,false,raw);
 require(keyReads==0,"Minimization suspends polling even if foreground check is true");
 frame(state,true,false,false,raw);require(saved(state)==after,"Restore does not replay held keys");
 keyboardInput.suspend(); // Event hook without an intervening inactive render tick.
 frame(state,true,false,false,raw);require(saved(state)==after,"Event-only suspension also protects actual bindings");
 raw.fill(false);frame(state,true,false,false,raw);
 state.headingHold=state.altitudeHold=state.navigationHold=true;
 raw['Y']=true;keyboardInput.suspend();frame(state,true,false,false,raw);
 require(state.avionics.master&&state.headingHold&&state.altitudeHold&&state.navigationHold,
         "Held avionics key on recovery cannot accidentally disconnect autopilot modes");
 raw.fill(false);frame(state,true,false,false,raw);
 raw['Y']=true;frame(state,true,false,false,raw);
 require(!state.avionics.master&&!state.headingHold&&!state.altitudeHold&&!state.navigationHold,
         "Fresh avionics-off command retains intentional autopilot-disconnect behaviour");
 // Actual event-hook statements retain taps completed before the polling frame.
 keyboardInput.suspend();messagePhysical.fill(false);raw.fill(false);
 state=sim::Snapshot{};state.flight.configure(3);frame(state,true,false,false,raw);
 sendKey('G',true);sendKey('G',false);auto tap=frame(state,true,false,false,raw);
 require(!state.flight.gear&&tap.pressed['G']&&!tap.down['G'],"Actual G binding receives completed tap without holding controls");
 frame(state,true,false,false,raw);require(!state.flight.gear,"Tap must not replay next frame");
 sendKey('W',true);sendKey('W',false);tap=frame(state,true,false,false,raw);
 require(tap.controls.throttle==0,"Completed W tap must not become stuck throttle");
 sendKey(VK_SHIFT,true);sendKey(VK_F6,true);sendKey(VK_F6,false);sendKey(VK_SHIFT,false);
 frame(state,true,false,false,raw);
 require(state.flight.engines.units[0].fault==sim::EngineFault::Fire,"Actual F6 binding uses Shift from event, not released polling state");
 sendKey(VK_CONTROL,true);sendKey(VK_F7,true);sendKey(VK_F7,false);sendKey(VK_CONTROL,false);
 frame(state,true,false,false,raw);
 require(state.flight.engines.units[1].fault==sim::EngineFault::Generator,"Actual F7 binding uses press-time Ctrl snapshot");
 const bool gear=state.flight.gear;
 sendKey('G',true,true);sendKey('G',false);frame(state,true,false,false,raw);
 require(state.flight.gear==gear,"Autorepeat flag filtered by actual message-hook code");
 sendKey('G',true);sendKey('G',false);keyboardInput.suspend();frame(state,true,false,false,raw);
 require(state.flight.gear==gear,"Suspension drops actual pending G tap");
 for(int unavailable=0;unavailable<3;++unavailable){
  frame(state,unavailable!=0,unavailable==1,unavailable==2,raw);
  sendKey('G',true);sendKey('G',false);frame(state,true,false,false,raw);
  require(state.flight.gear==gear,"Inactive/native-menu/minimized messages cannot inject pending gear action");
 }
 // A generic modifier remains down if its other physical side is still held.
 raw[VK_SHIFT]=true;messagePhysical[VK_SHIFT]=true;keyboardInput.suspend();
 frame(state,true,false,false,raw);
 message(1,WM_KEYUP,VK_SHIFT,0); // Fake GetKeyState still reports the other Shift.
 sendKey(VK_F6,true);sendKey(VK_F6,false);frame(state,true,false,false,raw);
 require(state.flight.engines.units[0].fault==sim::EngineFault::Flameout,
         "Releasing one Shift side cannot rearm recovered generic Shift while the other stays held");
 sendKey(VK_SHIFT,false);sendKey(VK_SHIFT,true);sendKey(VK_F6,true);sendKey(VK_F6,false);sendKey(VK_SHIFT,false);
 raw.fill(false);frame(state,true,false,false,raw);
 require(state.flight.engines.units[0].fault==sim::EngineFault::Fire,"All Shift released and repressed rearms modifier snapshot");
 std::cout<<"Actual main polling/bindings/Controls: held gear/flaps/engine/power/fault/throttle recovery stays neutral; fresh commands still work.\n"
          <<"No key API calls in background/native-menu/minimized modes; focus/menu/resize/dialog hooks present.\n"
          <<"Actual message hook: completed G/W taps, Shift+F6, Ctrl+F7, repeat filter and suspend barriers passed.\n"
          <<"CPU API stand-ins only: Windows message delivery, keyboard hardware and real interaction NOT verified.\n";
}
'''
with tempfile.TemporaryDirectory() as folder:
    cpp=Path(folder)/'keyboard.cpp'
    cpp.write_text(prefix+polling+bindings+controls+'\n'+ending)
    exe=Path(folder)/'keyboard'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-pedantic',
                    '-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
