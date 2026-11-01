"""Compile the production glass displays with CPU math/text adapters.

Checks real geometry, labels, power gating and data bindings. Optional --preview
exports an orthographic layout preview, not a screenshot of the Windows game.
"""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
code = r'''
#include "assets.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
struct XMFLOAT3{float x,y,z;};
using XMMATRIX=int;
XMFLOAT3 XMLoadFloat3(const XMFLOAT3* p){return *p;}
XMFLOAT3 XMVector3TransformCoord(XMFLOAT3 p,int){return p;}
void XMStoreFloat3(XMFLOAT3* p,XMFLOAT3 v){*p=v;}
void tri(std::vector<Vertex>& out,XMFLOAT3 a,XMFLOAT3 b,XMFLOAT3 c,XMFLOAT3 color){
 for(auto p:{a,b,c})out.push_back({p.x,p.y,p.z,color.x,color.y,color.z});
}
// Emulate legacy SDK macros to catch the v0.50 class of Windows build failure.
#define near
#define far
#include "cockpit_avionics.h"
#undef near
#undef far
struct Label{std::wstring text;float x,y,z,w;XMFLOAT3 color;};
std::vector<Label> labels;
void label(const std::wstring& s,float x,float y,float z,float w,XMFLOAT3 c){labels.push_back({s,x,y,z,w,c});}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool contains(const std::wstring& token){for(const auto& l:labels)if(l.text.find(token)!=std::wstring::npos)return true;return false;}
void validate(const std::vector<Vertex>& out){
 require(out.size()%3==0&&out.size()<16000,"Display triangle budget");
 for(const auto& v:out){
  require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"Finite vertices");
  require(v.x>=-.775f&&v.x<=.775f&&v.y>=-.883f&&v.y<=-.587f,"Display geometry within bezels");
  require(v.textured==2&&v.z>.870f&&v.z<.875f,"Self-lit glass layers");
 }
 for(const auto& l:labels){
  require(l.y<=-.588f&&l.y>=-.883f&&l.z<.872f,"Text on glass face");
  float center=l.x<-.326f?-.55f:l.x>.326f?.55f:0.f;
  require(l.x>=center-.2241f&&l.x+l.text.size()*9*l.w/300<=center+.2241f,"Conservative text width within bezel");
 }
}
int main(int argc,char** argv){
 std::vector<Vertex> out;int cases=0;
 for(int id=0;id<sim::aircraftCount;id++)for(double pitch:{-.8,0.,.8})for(double roll:{-2.8,-.6,0.,.6,2.8})for(double yaw:{-sim::pi,0.,6.28})for(bool on:{false,true}){
  sim::Snapshot state;state.flight.configure(id);state.flight.y=2400;state.flight.vz=90;state.flight.vy=-.3;state.flight.pitch=pitch;state.flight.roll=roll;state.flight.yaw=yaw;state.flight.rpm=2100;state.avionics.master=on;
  out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
  require(on?!labels.empty():labels.empty(),"Powered state gates all flight text");
  if(!on)require(out.size()==18,"Power loss leaves only three dark rectangles");
  if(on){require(contains(L"km/h")&&contains(L"ALT m")&&contains(L"VS -0.3 m/s"),"Metric units and precise vertical speed");require(contains(L"RPM")&&!contains(L"N1"),"Actual RPM, no invented jet N1");}
  ++cases;
 }
 sim::Snapshot state;state.flight.configure(3);state.flight.rpm=2250;state.flight.vz=90;state.flight.y=1234;state.flight.vy=1.2;state.flight.pitch=.035;state.flight.roll=-.12;state.flight.throttle=.84;state.flight.flap=.5;state.flight.fuel=87;state.headingHold=true;state.navigationHold=true;state.flight.z=1000;
 out.clear();labels.clear();glass::draw(out,state,0,label);
 require(contains(L"2250")&&contains(L"FUEL 87%")&&contains(L"FLAP 50%")&&contains(L"THR 84%"),"Flight state feeds engine/system glass");
 state.navigationWaypoint=99;out.clear();labels.clear();glass::draw(out,state,0,label);require(contains(L"WPT INVALID"),"Invalid waypoint never dereferenced");
 state.navigationWaypoint=0;state.flight.stallWarning=true;out.clear();labels.clear();glass::draw(out,state,0,label);require(contains(L"STALL"),"Stall warning retained");
 state.flight.stallWarning=false;out.clear();labels.clear();glass::draw(out,state,0,label);validate(out);
 if(argc>1){std::ofstream file(argv[1]);file<<"{\"vertices\":[";bool first=true;for(const auto& v:out){if(!first)file<<',';first=false;file<<'['<<v.x<<','<<v.y<<','<<v.z<<','<<v.r<<','<<v.g<<','<<v.b<<']';}
  file<<"],\"labels\":[";first=true;for(const auto& l:labels){if(!first)file<<',';first=false;file<<"{\"text\":\"";for(wchar_t c:l.text)file<<char(c);file<<"\",\"x\":"<<l.x<<",\"y\":"<<l.y<<",\"w\":"<<l.w<<",\"color\":["<<l.color.x<<','<<l.color.y<<','<<l.color.z<<"]}";}file<<"]}";
 }
 std::cout<<"PASS production glass: "<<cases<<" aircraft/pitch/bank/heading/power cases; units, RPM, fuel/flap/throttle bindings, bounds, vertex budget, invalid waypoint and stall warning. CPU adapters only.\n";
}
'''
# Also compile the complete production cockpit integration, including controls,
# mechanical geometry, text binding and display material conversion.
scene = (root/'src/scene.h').read_text()
box = scene[scene.index('inline void box('):scene.index('inline float terrain(')]
detail = (root/'src/cockpit_detail.h').read_text().replace('#pragma once', '').replace('#include "scene.h"', '')
body = (root/'src/cockpit.h').read_text()
body = '\n'.join(line for line in body.splitlines() if not line.startswith(('#include', '#pragma')))
font = (root/'src/chinese_text.h').read_text()
quad = font[font.index('inline void screenQuad'):font.index('\nstruct ChineseFont')]
integration = r'''
#include "cockpit_controls.h"
constexpr float XM_2PI=6.283185307f;
using XMVECTOR=XMFLOAT3;
int XMMatrixIdentity(){return 0;}
int XMMatrixRotationZ(float){return 0;}
int XMMatrixTranslation(float,float,float){return 0;}
XMFLOAT3 XMVectorSet(float x,float y,float z,float){return {x,y,z};}
struct TestFont{void draw(std::vector<Vertex>&,const std::wstring&,float,float,XMFLOAT3={1,1,1},float=900){}};
TestFont& chineseFont(){static TestFont f;return f;}
''' + quad + box + detail + body + r'''
void checkCockpitIntegration(){
 for(int id=0;id<sim::aircraftCount;id++)for(bool on:{false,true}){
  sim::Snapshot state;state.flight.configure(id);state.avionics.master=on;std::vector<Vertex> geometry;
  cockpitMesh(geometry,state,0);
  require(!geometry.empty(),"Full cockpit draws geometry");
  for(const auto& v:geometry)require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&(v.textured==3||v.textured==4),"Cockpit finite and material integration");
 }
}
'''
code = code.replace('int main(int argc,char** argv){', integration + '\nint main(int argc,char** argv){\n checkCockpitIntegration();')
with tempfile.TemporaryDirectory(prefix='fcair-glass-') as folder:
    source = Path(folder) / 'glass.cpp'
    exe = Path(folder) / 'glass'
    source.write_text(code)
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O2']
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic', *flags,
                    '-I', str(root / 'src'), str(source), '-o', str(exe)], check=True)
    preview = root / 'validation_v052/glass_layout.json'
    subprocess.run([str(exe), *([str(preview)] if '--preview' in sys.argv else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})

if '--preview' in sys.argv:
    from PIL import Image, ImageDraw, ImageFont
    data = json.loads(preview.read_text())
    image = Image.new('RGB', (1580, 490), '#111a23')
    draw = ImageDraw.Draw(image)
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    draw.text((32, 18), 'FLIGHTCAREER AIR  /  GLASS COCKPIT', font=ImageFont.truetype(font_path, 22), fill='#dae7f0')
    draw.text((32, 53), 'CPU layout preview - production geometry, substitute font; not an in-game screenshot', font=ImageFont.truetype(font_path, 14), fill='#8fa7b9')
    scale = 940
    def point(x, y): return (790+x*scale, 108+(-.588-y)*scale)
    def color(c): return tuple(max(0,min(255,round(v*255))) for v in c)
    vertices = data['vertices']
    for center in [-.55, 0, .55]:
        a=point(center-.237,-.58); b=point(center+.237,-.89)
        draw.rounded_rectangle((*a,*b), radius=9, fill='#26313c', outline='#4a5865', width=2)
    for i in range(0,len(vertices),3):
        v=vertices[i:i+3]
        draw.polygon([point(p[0],p[1]) for p in v],fill=color(v[0][3:]))
    for l in data['labels']:
        font=ImageFont.truetype(font_path,max(8,round(16*l['w']/300*scale)))
        draw.text(point(l['x'],l['y']),l['text'],font=font,fill=color(l['color']))
    for x,title in [(77,'PRIMARY FLIGHT DISPLAY'),(595,'NAVIGATION DISPLAY'),(1110,'ENGINE / SYSTEM DISPLAY')]:
        draw.text((x,419),title,font=ImageFont.truetype(font_path,16),fill='#9fbdcd')
    image.save(root/'validation_v052/glass_preview.png')
    print('Saved CPU glass layout preview.')
