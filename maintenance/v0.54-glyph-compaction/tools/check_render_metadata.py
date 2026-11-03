"""Execute actual performance CSV statements with CPU-only renderer stand-ins."""
from pathlib import Path
import subprocess,tempfile,sys,os
root=Path(__file__).resolve().parents[1]
main=Path(sys.argv[1]).read_text() if len(sys.argv)>1 else (root/'src/main.cpp').read_text()
header=next(x.strip() for x in main.splitlines() if 'perf<<"elapsed_s,' in x)
row=next(x.strip() for x in main.splitlines() if 'perf<<std::fixed' in x)
extent=next((x.strip() for x in main.splitlines() if 'const auto loggedExtent=' in x),'')
code=r'''
#include "launch_options.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using UINT=unsigned;struct Renderer{const char* name(){return "DX11";}} instance;auto* renderer=&instance;
struct{int aircraftId=3;}flight;struct{int terrainMode=1;bool antiAlias=true;}state;
struct{int samples=100;double fps=30,p95Ms=33,maxMs=40;}frameStats;
std::vector<std::string> split(std::string s){std::vector<std::string> a;std::istringstream in(s);std::string x;while(std::getline(in,x,','))a.push_back(x);return a;}
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{
 int count=0;double wallElapsed=5;int camera=1;bool wasActive=true;
 for(bool low:{false,true})for(bool powerSaver:{false,true})for(unsigned appliedScale:{1u,2u})for(long clientWidth:{800L,1600L}){
 long clientHeight=clientWidth*9/16;
 std::ostringstream perf;
''' +header+r'''
 std::string h=perf.str();check(h.find("render_width,render_height,software_frame_cap")!=std::string::npos,"Missing render dimensions and cap in CSV header");perf.str("");perf.clear();
'''+extent+'\n'+row+r'''
 auto fields=split(perf.str());check(fields.size()==15,"CSV has 15 fields");
 check(std::stoi(fields[6])==int(low),"low_mode must describe fixed low rendering, not frame cap");
 check(std::stoul(fields[12])==(low?640u:unsigned(clientWidth)*appliedScale),"Width must use applied scale");
 check(std::stoul(fields[13])==(low?360u:unsigned(clientHeight)*appliedScale),"Height must use applied scale");
 check(std::stoi(fields[14])==(powerSaver?20:0),"Cap must be recorded separately");++count;
 }
 std::cout<<"PASS: "<<count<<" actual CSV header/row cases, including uncapped low benchmark and capped native. CPU-only metadata.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 flags=['-fsanitize=address,undefined','-g'] if os.environ.get('FCAIR_SANITIZE') else []
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic',*flags,'-I',str(root/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 sys.exit(subprocess.run([str(p/'test')]).returncode)
