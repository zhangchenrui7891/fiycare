"""Exercise actual main.cpp mouse hook with CPU-only Win32 API stand-ins."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
main = Path(sys.argv[1]).read_text() if len(sys.argv)>1 else (root/'src/main.cpp').read_text()
start = main.index(' if(msg==WM_CAPTURECHANGED)')
hook = main[start:main.index(' return DefWindowProc', start)].replace(';}return 0;', ';}\n return 0;')
resize_start=main.index(' if(msg==WM_ENTERSIZEMOVE)')
hook=main[resize_start:main.index('\n',resize_start)]+'\n'+hook
for event in ('WM_KILLFOCUS','WM_ENTERMENULOOP'):
    i=main.index(' if(msg=='+event+')');hook=main[i:main.index('\n',i)]+'\n'+hook
prefix = r'''
#include "cockpit_controls.h"
#include "keyboard_state.h"
#include <iostream>
#include <stdexcept>
constexpr int WM_CAPTURECHANGED=1,WM_RBUTTONDOWN=2,WM_RBUTTONUP=3,
 WM_MOUSEMOVE=4,WM_LBUTTONDOWN=5,WM_LBUTTONUP=6,WM_ENTERSIZEMOVE=7,WM_KILLFOCUS=8,WM_ENTERMENULOOP=9;
struct RECT{long left=0,top=0,right=960,bottom=540;};
struct Button{float x,y,w,h;int command;};
std::vector<Button> clickableButtons;
std::vector<int> menuCommands;
cockpit::SpatialInput cockpitInput;
input::KeyboardState keyboardInput;
bool cockpitMouseDown=false,lookDragging=false,inNativeMenu=false;
int lookX=0,lookY=0,capture=0,menuPressedCommand=-1;
int message(int h,int msg,unsigned long l);
unsigned long LOWORD(unsigned long l){return l&65535;}
unsigned long HIWORD(unsigned long l){return (l>>16)&65535;}
long clientWidth=960,clientHeight=540;
void GetClientRect(int,RECT* r){r->right=clientWidth;r->bottom=clientHeight;}
int GetCapture(){return capture;}
void SetCapture(int h){capture=h;}
void ReleaseCapture(){const int previous=capture;capture=0;if(previous)message(previous,WM_CAPTURECHANGED,0);}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void send(int msg,int x,int y){message(1,msg,(unsigned long(y)&65535)<<16|(unsigned long(x)&65535));}
int message(int h,int msg,unsigned long l){
'''
# Functional-style casts with a two-word type are not accepted by C++.
prefix=prefix.replace('unsigned long(y)','static_cast<unsigned long>(y)').replace('unsigned long(x)','static_cast<unsigned long>(x)')
ending=r'''
 return -1;
}
int main(){try{
 cockpitInput.cancel();clickableButtons={{10,10,100,40,101},{200,10,100,40,102}};
 send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Release without matching press must not activate menu");
 send(WM_LBUTTONDOWN,150,20);send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Dragging in from outside must not activate menu");
 send(WM_LBUTTONDOWN,20,20);require(capture==1,"Menu press captures pointer");send(WM_LBUTTONUP,210,20);require(menuCommands.empty()&&capture==0,"Release on another button cancels");
 send(WM_LBUTTONDOWN,20,20);send(WM_LBUTTONUP,150,20);send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Outside release consumes click");
 send(WM_LBUTTONDOWN,20,20);ReleaseCapture();send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Capture loss cancels menu press");
 send(WM_LBUTTONDOWN,20,20);send(WM_ENTERSIZEMOVE,20,20);send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Resize cancels menu press");
 for(int event:{WM_KILLFOCUS,WM_ENTERMENULOOP}){send(WM_LBUTTONDOWN,20,20);send(event,20,20);send(WM_LBUTTONUP,20,20);require(menuCommands.empty(),"Focus loss/native menu cancels click");}
 send(WM_LBUTTONDOWN,20,20);send(WM_LBUTTONUP,20,20);require(menuCommands.size()==1&&menuCommands[0]==101&&capture==0,"Same-button press/release activates once");send(WM_LBUTTONUP,20,20);require(menuCommands.size()==1,"Duplicate release must not repeat command");
 clickableButtons.clear();menuCommands.clear();
 std::cout<<"Menu clicks: unmatched/outside/other-button releases, capture loss, resize and normal click passed.\n";

 const auto p=cockpit::solids(.5)[5].center;
 const float sp=std::sin(cockpit::lookPitch),cp=std::cos(cockpit::lookPitch);
 const float up=p.y*cp-p.z*sp,forward=p.y*sp+p.z*cp;
 const int x=int(480*(1+p.x/forward/(.57735027f*cockpit::viewportAspect)));
 const int y=int(270*(1-up/forward/.57735027f));
 for(bool look:{false,true})for(bool owned:{false,true}){
  cockpitInput={};cockpitInput.enabled=true;cockpitInput.throttle=.5;
  send(look?WM_RBUTTONDOWN:WM_LBUTTONDOWN,x,y);
  if(!look)send(WM_MOUSEMOVE,x,y-16);
  if(!owned)capture=2; // Must not release another window's capture.
  send(WM_ENTERSIZEMOVE,x,y);
  require(!cockpitInput.enabled&&!cockpitInput.dragging&&!cockpitInput.looking&&!cockpitMouseDown&&cockpitInput.pending.empty(),"Resize entry clears gestures and queued cockpit commands");
  require(capture==(owned?0:2),"Resize releases only this window's capture");
  const float yaw=cockpit::lookYaw,pitch=cockpit::lookPitch;
  const double throttle=cockpitInput.throttle;
  clientWidth=1920;clientHeight=1080;send(WM_MOUSEMOVE,x,y-80);
  require(cockpitInput.throttle==throttle&&cockpitInput.pending.empty()&&cockpit::lookYaw==yaw&&cockpit::lookPitch==pitch,"Resized coordinates cannot move stale throttle or look gesture");
  cockpitInput.enabled=true;send(WM_MOUSEMOVE,x,y-100);
  require(cockpitInput.pending.empty(),"Re-enable alone cannot resume old drag");
  cockpitInput.throttle=.5;send(WM_LBUTTONDOWN,x*2,y*2);send(WM_MOUSEMOVE,x*2,(y-16)*2);
  require(cockpitInput.dragging&&std::abs(cockpitInput.throttle-.6)<1e-9,"Fresh press works at resized dimensions");
  cockpitInput.cancel();ReleaseCapture();clientWidth=960;clientHeight=540;
 }
 std::cout<<"Resize entry: throttle/look, own/foreign capture, changed dimensions and fresh-press recovery passed.\n";
 for(bool leftFirst:{false,true}){
  cockpitInput={};cockpitInput.enabled=true;cockpitInput.throttle=.5;
  cockpitMouseDown=false;lookDragging=false;capture=0;
  const float yaw=cockpit::lookYaw,pitch=cockpit::lookPitch;
  send(WM_LBUTTONDOWN,x,y);require(cockpitInput.dragging&&capture==1,"Left press starts captured throttle drag");
  send(WM_RBUTTONDOWN,x,y);send(WM_MOUSEMOVE,x+10,y-16);
  require(cockpit::lookYaw==yaw&&cockpit::lookPitch==pitch,"Right press during throttle drag must not rotate view");
  require(std::abs(cockpitInput.throttle-.6)<1e-9,"Motion still reaches throttle while right button is held");
  if(leftFirst){send(WM_LBUTTONUP,x+10,y-16);send(WM_RBUTTONUP,x+10,y-16);}
  else{send(WM_RBUTTONUP,x+10,y-16);require(capture==1&&cockpitInput.dragging,"Ignored right release preserves left capture");send(WM_LBUTTONUP,x+10,y-16);}
  require(!cockpitInput.dragging&&capture==0,"Both release orders finish drag");
  require(cockpitInput.pending.size()==1&&std::abs(cockpitInput.pending[0].value-.6)<1e-9,"Final absolute throttle survives capture-change callback");
 }
 cockpitInput={};cockpitInput.enabled=true;cockpitInput.throttle=.5;capture=0;
 clickableButtons={{0,0,960,540,99}};menuCommands.clear();
 send(WM_RBUTTONDOWN,x,y);send(WM_LBUTTONDOWN,x,y);send(WM_LBUTTONUP,x,y);
 require(capture==1&&cockpitInput.pending.empty()&&menuCommands.empty(),"Look blocks left cockpit and menu actions without releasing capture");
 const float before=cockpit::lookYaw;send(WM_MOUSEMOVE,x+10,y);
 require(cockpit::lookYaw>before,"Look continues after ignored left release");
 send(WM_RBUTTONUP,x+10,y);require(capture==0,"Right release finishes look");
 const float after=cockpit::lookYaw;send(WM_MOUSEMOVE,x+20,y);
 require(cockpit::lookYaw==after,"Released look does not stick");
 send(WM_RBUTTONDOWN,x,y);ReleaseCapture();send(WM_MOUSEMOVE,x+30,y);
 require(cockpit::lookYaw==after,"Capture loss stops look");
 std::cout<<"Actual mouse hook: throttle-first both release orders, look-first left suppression, capture callbacks and idle recovery passed.\nCPU Win32 stand-ins only; no Windows message delivery, real mouse or GPU verification.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory() as folder:
    cpp=Path(folder)/'pointer.cpp';cpp.write_text(prefix+hook+ending)
    exe=Path(folder)/'pointer'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-pedantic',*(['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if os.environ.get('FCAIR_SANITIZE') else []),'-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
