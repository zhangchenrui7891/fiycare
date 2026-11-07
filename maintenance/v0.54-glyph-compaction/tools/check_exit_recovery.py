"""Compile actual exit decision and WM_CLOSE hook against CPU API substitutes.

Checks save-before-destroy, cancellation/retry and diagnostic isolation. This
is not a Windows message-loop, modal dialog, input-device or GPU acceptance test.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
start = main.index('  // Exit recovery:')
handler = main[start:main.index('  // End exit recovery.', start)]
start = main.index(' if(msg==WM_CLOSE)')
hook = main[start:main.index('\n', start)]
for wiring in ('case ui::Exit:exitApproved=requestExit();break;',
               '   if(exitApproved)break;\n   if(!IsWindow(window))continue;',
               'if(!smoke&&!benchmark&&!exitApproved&&!trySave())exitWithoutSave=true;'):
    if wiring not in main:
        raise RuntimeError('Review exit routing: ' + wiring)
assert main.index('exitApproved=requestExit();if(exitApproved)break;continue;') < main.index('   RECT currentClient{};')
assert 'if(std::find(menuCommands.begin(),menuCommands.end(),ui::Exit)!=menuCommands.end())' in main
assert main.index('   if(exitApproved)break;') < main.index('   bool cockpitEnabled=')
code = r'''
#include <chrono>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
constexpr int WM_CLOSE=16,MB_YESNO=4,MB_ICONWARNING=48,MB_DEFBUTTON2=256,IDYES=6,IDNO=7;
namespace ui {constexpr int Exit=9;}
std::deque<int> menuCommands;
std::vector<std::string> events;
bool alive=true,saveOK=true,destroyOK=true,exitWithoutSave=false;
int answer=IDNO,capture=1,dialogCount=0,saveCount=0;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void logLine(const std::string& text){events.push_back(text);}
struct Keyboard{int count=0;void suspend(){++count;}} keyboardInput;
struct Cockpit{int count=0;void cancel(){++count;}} cockpitInput;
struct Physics{int count=0;void reset(){++count;}} physicsTime;
int GetCapture(){return capture;}
void ReleaseCapture(){capture=0;}
bool DestroyWindow(int){events.push_back("destroy");require(alive,"Cannot destroy twice");if(destroyOK)alive=false;return destroyOK;}
int MessageBoxW(int owner,const wchar_t* message,const wchar_t*,int flags){
 require(owner==1&&alive,"Failure prompt needs live owner window");
 require(flags==(MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2),"Default must be No");
 require(std::wstring(message).find(L"F5")!=std::wstring::npos,"Prompt must explain recovery");
 require(saveCount==1,"Prompt only after failed save attempt");
 menuCommands.push_back(ui::Exit);menuCommands.push_back(99);
 ++dialogCount;events.push_back("prompt");return answer;
}
int windowMessage(int msg){
''' + hook + r'''
 return -1;
}
int main(){
 for(int scenario=0;scenario<8;++scenario){
  const int window=1;const bool smoke=scenario==4,benchmark=scenario==5;
  bool paused=false,wasActive=true,previousMeasured=true;
  std::wstring notice;auto last=std::chrono::steady_clock::time_point{};
  alive=true;saveOK=scenario==0||scenario==6;destroyOK=scenario!=6;
  answer=scenario==2?IDYES:scenario==7?0:IDNO;capture=1;dialogCount=saveCount=0;
  exitWithoutSave=false;events.clear();menuCommands.clear();
  keyboardInput.count=cockpitInput.count=physicsTime.count=0;
  auto trySave=[&](){require(alive,"Save must precede window destruction");++saveCount;events.push_back("save");return saveOK;};
''' + handler + r'''
  require(windowMessage(WM_CLOSE)==0&&menuCommands.front()==ui::Exit,"Window close routes to shared exit command");
  bool approved=requestExit();
  const bool diagnostic=smoke||benchmark;
  const bool shouldExit=scenario==0||scenario==2||diagnostic;
  require(approved==shouldExit&&alive!=shouldExit,"Only successful save or explicit Yes may close");
  require(saveCount==(diagnostic?0:1),"Exactly one save attempt; diagnostics never save");
  require(dialogCount==(!saveOK&&!diagnostic?1:0),"No spurious prompt on success or diagnostics");
  require(exitWithoutSave==(scenario==2),"Explicit unsaved exit must be logged separately");
  require(menuCommands.empty(),"Drop duplicate close and stale continue commands, including modal-loop arrivals");
  require(paused&&!wasActive&&!previousMeasured&&last!=std::chrono::steady_clock::time_point{},"Cancel leaves paused and resets timing");
  require(keyboardInput.count==2&&cockpitInput.count==2&&physicsTime.count==1&&capture==0,"Modal exit neutralizes held input, mouse capture and pending clicks");
  if(scenario==0)require(events==std::vector<std::string>{"save","destroy"},"Save before destroy, no duplicate save");
  if(scenario==2)require(events==std::vector<std::string>{"save","prompt","destroy"},"Unsaved close requires explicit prompt acceptance");
  if(scenario==3){
   saveOK=true;events.clear();menuCommands.push_back(ui::Exit);saveCount=0;
   require(requestExit()&&!alive&&!exitWithoutSave&&saveCount==1,"Retry after disk recovery saves retained flight before exiting");
   require(dialogCount==1,"Successful retry must not show another failure prompt");
  }
  if(scenario==6)require(!notice.empty(),"Failed DestroyWindow stays open with a retry message");
 }
 std::cout<<"PASS actual exit handler: save-before-destroy, default-No cancellation, explicit unsaved exit, retry, diagnostics bypass, duplicate commands, input/timing reset, destroy/dialog failure. CPU substitutes only.\n";
}
'''
with tempfile.TemporaryDirectory(prefix='fcair-exit-') as d:
    source=Path(d)/'exit.cpp';source.write_text(code)
    binary=Path(d)/'exit_check'
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
