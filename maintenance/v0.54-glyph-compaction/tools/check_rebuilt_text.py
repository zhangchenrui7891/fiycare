"""Fresh recovery checks for actual draw/screenQuad/cockpitText statements.

Uses deterministic glyphs and affine CPU math substitutes. No GDI, DirectX
execution or GPU pixel comparisons; the original lost scripts stay missing.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
font = (root / 'src/chinese_text.h').read_text()
cockpit = (root / 'src/cockpit.h').read_text()
quad = font[font.index('inline void screenQuad'):font.index('\nstruct ChineseFont')]
draw = font[font.index(' void draw('):font.index('\n};\ninline ChineseFont&')]
text = cockpit[cockpit.index('inline void cockpitText'):cockpit.index('\n#include "cockpit_avionics.h"')]
code = r'''
#include "assets.h"
#include <array>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
struct XMFLOAT3{float x,y,z;};
struct XMMATRIX{float sx,sy,sz,tx,ty,tz;};
XMFLOAT3 XMLoadFloat3(const XMFLOAT3* p){return *p;}
XMFLOAT3 XMVector3TransformCoord(XMFLOAT3 p,const XMMATRIX& m){
 return {p.x*m.sx+m.tx,p.y*m.sy+m.ty,p.z*m.sz+m.tz};
}
void XMStoreFloat3(XMFLOAT3* p,XMFLOAT3 v){*p=v;}
struct PixelRun{float x,y,w,h;};
''' + quad + r'''
struct TestFont{
 struct Glyph{int advance;std::vector<PixelRun> runs;};
 std::map<wchar_t,Glyph> cache;
 wchar_t fail=0;
 TestFont(){
  cache[L'A']={9,{{1,2,3,1},{1,3,1,5}}};
  cache[L'中']={16,{{1,2,12,1},{1,3,1,9},{12,3,1,9}}};
  cache[L' ']={6,{}};
  cache[L'?']={16,{{1,2,12,1},{1,14,12,1},{1,2,1,13},{12,2,1,13}}};
 }
 const Glyph& get(wchar_t c){
  if(fail&&c==fail)throw std::runtime_error("injected glyph failure");
  auto p=cache.find(c);return p==cache.end()?cache.at(L'?'):p->second;
 }
''' + draw + r'''
};
TestFont& chineseFont(){static TestFont value;return value;}
''' + text + r'''
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
bool equal(const Vertex& a,const Vertex& b){
 return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.r==b.r&&a.g==b.g&&a.b==b.b&&
        a.u==b.u&&a.v==b.v&&a.textured==b.textured&&a.nx==b.nx&&a.ny==b.ny&&a.nz==b.nz;
}
void same(const std::vector<Vertex>& a,const std::vector<Vertex>& b){
 require(a.size()==b.size(),"Vertex count");
 for(size_t i=0;i<a.size();++i)require(equal(a[i],b[i]),"All 12 vertex fields");
}
// Former temporary-vector pipeline, supplied as a reference for recovery.
void reference(std::vector<Vertex>& out,const std::wstring& s,float x,float y,float z,float w,
               const XMMATRIX& matrix,XMFLOAT3 color){
 std::vector<Vertex> glyphs;chineseFont().draw(glyphs,s,0,0,color,900);
 for(auto v:glyphs){
  XMFLOAT3 p{x+(v.x+1)*480*w/300,y-(1-v.y)*270*w/300,z};
  p=XMVector3TransformCoord(p,matrix);v.x=p.x;v.y=p.y;v.z=p.z;v.textured=2;
  out.push_back(v);
 }
}
int main(){
 auto& f=chineseFont();std::vector<Vertex> out;
 f.draw(out,L"A A\n中",0,0,{.4f,1,.75f},900);
 require(out.size()==42,"Two A glyphs, a Chinese glyph, no visible space/newline");
 require(out[12].x==(16.f/480-1),"Space advances without emitting vertices");
 require(out[24].y==(1-23.f/270),"Newline resets x and advances y by 21");
 out.clear();f.draw(out,L"AA",0,0,{1,1,1},9);
 require(out[12].y==(1-23.f/270),"Width wraps second glyph");
 const std::array<std::wstring,8> strings={L"",L" ",L"A",L"中 A?",L"A\n中\nA",L"\n",std::wstring(250,L'A'),L"未知字形"};
 const std::array<XMMATRIX,3> matrices={XMMATRIX{1,1,1,0,0,0},XMMATRIX{2,.3f,-1,3,-4,5},XMMATRIX{-.7f,1.2f,.1f,-2,4,-8}};
 const Vertex prefix{1,2,3,.1f,.2f,.3f,.4f,.5f,4,.6f,.7f,.8f};
 unsigned cases=0;size_t vertices=0;
 for(const auto& s:strings)for(const auto& m:matrices)for(float width:{0.f,.12f,.66f,1.4f})for(int existing:{0,3}){
  std::vector<Vertex> actual(size_t(existing),prefix),expected=actual;
  cockpitText(actual,s,.3f,-.8f,.7f,width,m,{.4f,1,.75f});
  reference(expected,s,.3f,-.8f,.7f,width,m,{.4f,1,.75f});same(actual,expected);
  vertices+=actual.size()-size_t(existing);++cases;
 }
 out={prefix};const auto original=out;f.fail=L'中';bool threw=false;
 try{cockpitText(out,L"A中",0,0,0,1,matrices[0]);}catch(const std::runtime_error&){threw=true;}
 require(threw,"Glyph exception propagated");same(out,original);
 f.fail=0;cockpitText(out,L"A中",0,0,0,1,matrices[0]);
 std::vector<Vertex> expected=original;reference(expected,L"A中",0,0,0,1,matrices[0],{.4f,1,.75f});same(out,expected);
 std::cout<<"PASS rebuilt text: "<<cases<<" cases, "<<vertices<<" generated vertices, layout, prefix, rollback/retry. CPU glyph/matrix substitutes only.\n";
}
'''
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'text.cpp'
    source.write_text(code)
    executable = Path(directory) / 'text'
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O2']
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic', *flags,
                    '-I', str(root / 'src'), str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
