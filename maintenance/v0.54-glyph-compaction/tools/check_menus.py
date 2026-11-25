"""Compile the real menu layout with native window calls removed; check button bounds."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
text = (root / "src/menus.h").read_text()
globals_start = text.index("struct MenuButton")
globals_end = text.index("inline HMENU createChineseMenu")
draw_start = text.index("inline void drawMenu")
code = r'''
#include "ui_state.h"
#include <deque>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <optional>
#include <tuple>
struct XMFLOAT3{float x,y,z;};
struct Vertex{};
void screenQuad(std::vector<Vertex>&,float,float,float,float,XMFLOAT3){}
struct Font{void draw(std::vector<Vertex>&,const std::wstring&,float,float,XMFLOAT3,int=900){}};
Font& chineseFont(){static Font f;return f;}
'''
code += r"""
using UINT=unsigned;using HMENU=int;
constexpr UINT MF_BYCOMMAND=0,MF_CHECKED=8,MF_UNCHECKED=0;
std::map<int,UINT> checks;
int writes=0,failCommand=-1;
UINT CheckMenuItem(HMENU,int cmd,UINT flags){
 ++writes;if(cmd==failCommand)return UINT(-1);
 checks[cmd]=flags;return 0;
}
"""
code += text[globals_start:globals_end] + text[text.index("struct NativeMenuChecks"):draw_start] + text[draw_start:]
code += r'''
int main(){sim::Snapshot state;std::vector<Vertex> output;
NativeMenuChecks native;
sim::worldMode.store(1);
state.flight.configure(2);state.flight.assist=true;
auto sync=[&](bool saver){native.update(1,state.flight,saver);};
for(int i=0;i<1200;i++)sync(true);
if(writes!=15||checks.size()!=15||checks[ui::A330]!=8||checks[ui::TerrainSatellite]!=8||checks[ui::Assist]!=8||checks[ui::PowerSaver]!=8)return 7;
for(int cmd:{ui::Trainer,ui::Airliner,ui::A330,ui::A350,ui::A320,ui::B737,ui::B787,ui::A319,ui::A321,ui::B7879,ui::B777}){
 state.flight.configure(ui::aircraftIdForCommand(cmd));sync(true);
 int selected=0;for(auto kv:checks)if(kv.second==8)selected++;
 if(checks[cmd]!=8||selected!=3+int(state.flight.assist))return 8;
}
sim::worldMode.store(0);sync(true);
if(checks[ui::TerrainMountains]!=8||checks[ui::TerrainSatellite]!=0)return 9;
state.flight.assist=!state.flight.assist;sync(true);if(checks[ui::Assist]!=(state.flight.assist?8u:0u))return 10;
failCommand=ui::PowerSaver;sync(false);failCommand=-1;sync(false);
if(checks[ui::PowerSaver]!=0)return 11;
const int stable=writes;sync(false);if(writes!=stable)return 12;
std::cout<<"PASS native checkmarks: 1200 stable updates, 15 writes; all aircraft, terrain, cap, assist and failure retry verified. CPU adapter.\n";
for(auto page:{ui::Page::Home,ui::Page::Pause,ui::Page::Aircraft,ui::Page::Missions,ui::Page::Settings,ui::Page::Lighting}){
 drawMenu(output,page,state,false,L"");
 for(const auto& b:clickableButtons)if(b.x<180||b.y<170||b.x+b.w>780||b.y+b.h>470)return 1;
 if(page==ui::Page::Home||page==ui::Page::Pause){
  if(clickableButtons.size()!=10||clickableButtons.front().command!=ui::Continue)return 5;
  for(size_t i=0;i<clickableButtons.size();i++)for(size_t j=i+1;j<clickableButtons.size();j++){
   const auto& a=clickableButtons[i];const auto& b=clickableButtons[j];
   if(a.command==b.command||(a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h))return 6;
  }
 }
 if(page==ui::Page::Aircraft){if(clickableButtons.size()!=20)return 2;
  for(int command:{ui::A320,ui::B737,ui::B787,ui::A319,ui::A321,ui::B7879,ui::B777,ui::ImportModel,ui::ResetModel,ui::RotateModel}){
   int count=0;for(const auto& b:clickableButtons)if(b.command==command)count++;if(count!=1)return 3;
  }
 }
}
drawMenu(output,ui::Page::None,state,false,L"");if(!clickableButtons.empty())return 4;
std::cout<<"PASS: six menu pages, 20 aircraft-page buttons, eleven profiles and model tools fit the viewport\n";}
'''
with tempfile.TemporaryDirectory() as folder:
    source = Path(folder) / "menus.cpp"
    source.write_text(code)
    exe = Path(folder) / "menus"
    subprocess.run(["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    "-I", str(root / "src"), str(source), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
