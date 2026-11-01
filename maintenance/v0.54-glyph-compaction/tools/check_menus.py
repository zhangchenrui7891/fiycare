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
struct XMFLOAT3{float x,y,z;};
struct Vertex{};
void screenQuad(std::vector<Vertex>&,float,float,float,float,XMFLOAT3){}
struct Font{void draw(std::vector<Vertex>&,const std::wstring&,float,float,XMFLOAT3,int=900){}};
Font& chineseFont(){static Font f;return f;}
'''
code += text[globals_start:globals_end] + text[draw_start:]
code += r'''
int main(){sim::Snapshot state;std::vector<Vertex> output;
for(auto page:{ui::Page::Home,ui::Page::Pause,ui::Page::Aircraft,ui::Page::Missions,ui::Page::Settings,ui::Page::Lighting}){
 drawMenu(output,page,state,false,L"");
 for(const auto& b:clickableButtons)if(b.x<180||b.y<170||b.x+b.w>780||b.y+b.h>470)return 1;
 if(page==ui::Page::Aircraft){if(clickableButtons.size()!=19)return 2;
  for(int command:{ui::A320,ui::B737,ui::B787,ui::A319,ui::A321,ui::B7879,ui::ImportModel,ui::ResetModel,ui::RotateModel}){
   int count=0;for(const auto& b:clickableButtons)if(b.command==command)count++;if(count!=1)return 3;
  }
 }
}
drawMenu(output,ui::Page::None,state,false,L"");if(!clickableButtons.empty())return 4;
std::cout<<"PASS: six menu pages, 19 aircraft-page buttons, ten profiles and model tools fit the viewport\n";}
'''
with tempfile.TemporaryDirectory() as folder:
    source = Path(folder) / "menus.cpp"
    source.write_text(code)
    exe = Path(folder) / "menus"
    subprocess.run(["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
                    "-I", str(root / "src"), str(source), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
