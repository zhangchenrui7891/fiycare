"""Compile actual cockpit approach statements; verify state selection and labels.

This exercises CPU formatting and guidance. It does not execute Windows/GPU UI.
The rebuilt cockpit text check uses CPU glyph/math substitutes. The original
full cockpit geometry Python driver was not recoverable; see RECOVERY_V042.md.
"""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
cockpit=(root/'src/cockpit.h').read_text()
start=cockpit.index(' const auto approach=sim::approachGuidance(state);')
hud=cockpit[start:cockpit.index('\n }',start)].replace('cockpitText(out,','capture(')
if 'approachGuidance(flight)' in main or 'approachGuidance(f)' in cockpit:
    raise RuntimeError('User-visible approach cannot bypass arrival/power context')

code=r'''
#include "approach.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
std::wstring captured;
struct Color{float x,y,z;};
void capture(const std::wstring& value,float,float,float,float,int,Color){captured=value;}
std::wstring drawHud(const sim::Snapshot& state){
 captured.clear();const int transform=0;
 auto n=[](double value){return std::to_wstring(int(std::lround(value)));};
'''+hud+r'''
 return captured;
}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void setup(sim::Snapshot& state,int direction){
 const auto& r=sim::runways[1];state.flight.configure(3);
 state.flight.x=r.x;state.flight.z=r.z-direction*(r.halfLength+5000);
 state.flight.yaw=direction==1?0:sim::pi;state.flight.vz=direction*90.;
 state.flight.y=state.flight.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*5000;
 state.career.mission=2;state.career.stage=1;state.career.arrivalRunway=1;
}
int main(){
 sim::Snapshot state;setup(state,1);auto text=drawHud(state);
 require(text.find(sim::runways[1].name)!=std::wstring::npos,
         "Actual HUD uses destination and threshold distance, not nearest centre");
 require(text.find(L"训练进近")!=std::wstring::npos&&text.find(L" 正")!=std::wstring::npos,"Actual HUD identifies training course");
 setup(state,-1);state.flight.x+=120;text=drawHud(state);
 require(text.find(L" 反")!=std::wstring::npos&&text.find(L"LOC 左 120")!=std::wstring::npos,
         "Actual reciprocal HUD displays reversed aircraft-relative sign");
 state.avionics.master=false;require(drawHud(state).empty(),"HUD hides when avionics are off");
 state.avionics.master=true;state.flight.y=state.flight.spec().gearHeight;
 require(drawHud(state).empty(),"HUD hides when grounded");
 setup(state,1);state.flight.yaw=sim::pi;state.flight.vz=-90;
 require(drawHud(state).empty(),"Outbound HUD hides invalid cue");
 setup(state,1);state.career.arrivalRunway=9;
 require(drawHud(state).empty(),"Invalid destination is not replaced silently");
 std::cout<<"Actual cockpit approach statements: destination/threshold distance, reciprocal sign/course, power/ground/outbound/bounds passed.\n"
          <<"CPU text formatting only; no Windows message, font rasterization, GPU or screen-layout verification.\n";
}
'''
with tempfile.TemporaryDirectory() as folder:
    source=Path(folder)/'approach_hud.cpp'
    source.write_text(code)
    executable=Path(folder)/'approach_hud'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-pedantic',
                    '-I',str(root/'src'),str(source),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
