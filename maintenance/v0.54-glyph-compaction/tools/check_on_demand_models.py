"""Execute actual startup/selection/load blocks with CPU filesystem fixtures.
No Win32 event loop, renderer, target-hardware or startup-time validation.
"""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
startup = main[main.index('  std::map<int,ImportedModel> aircraftModels;'):main.index('  struct Mark')]
selection = main[main.index('     {\n      auto candidate=state;'):main.index('     case ui::ImportModel:')]
load = main[main.index('     case ui::Load:{')+len('     case ui::Load:'):main.index('     case ui::Restart:')]
code = r'''
#include "ui_state.h"
#include "fleet_assets.h"
#include "model_restore.h"
#include <chrono>
#include <iostream>
#include <sstream>
void logLine(const std::string&){}
std::filesystem::path base;
std::filesystem::path assetFolder(){return base/"assets";}
struct Reset {void reset(){}};
struct Alerts {void beginFlight(const sim::Snapshot&){}};
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void mesh(const std::filesystem::path& path){
 std::ofstream out(path,std::ios::binary);out.write("FCMESH07",8);uint32_t dims[]={3,12,1};out.write(reinterpret_cast<char*>(dims),12);
 Vertex v[3]{};for(auto& a:v)a.ny=1;out.write(reinterpret_cast<char*>(v),sizeof(v));out.put(0);
}
std::string encoded(const sim::Snapshot& state){std::ostringstream out;state.write(out);return out.str();}
int main(int argc,char**argv){try{
 check(argc==2,"Fixture path required");base=argv[1];std::filesystem::create_directories(assetFolder());
 auto folder=base/"player";std::filesystem::create_directories(folder);
 sim::Snapshot state;auto& flight=state.flight;
''' + startup + r'''
 check(readyModels.size()==1&&readyModels.count(0)&&aircraftModels.empty(),"Startup trainer must not open missing unrelated defaults");
 Alerts flightAlerts;Reset physicsTime;bool walking=false,previousMeasured=true,backupEligible=false;double dt=99;
 auto last=std::chrono::steady_clock::now();std::wstring notice;int saves=0;auto trySave=[&](){++saves;};
 auto menuPage=ui::Page::None;int cmd=ui::A350;
 auto select=[&](){do {
''' + selection + r'''
 }while(false);};
 const auto before=encoded(state);select();
 check(encoded(state)==before&&saves==0&&!readyModels.count(3),"Missing selected default must preserve flight and avoid saving");
 check(dt==0&&!previousMeasured,"Failed disk I/O excluded from simulation timing");
 mesh(assetFolder()/"a350.fcmesh");select();
 check(flight.aircraftId==3&&readyModels.count(3)&&aircraftModels.size()==1&&saves==1,"Repair allows actual menu selection and save");
 std::filesystem::remove(assetFolder()/"a350.fcmesh");cmd=ui::Trainer;select();cmd=ui::A350;select();
 check(flight.aircraftId==3&&saves==3,"Revisit uses cache even when disk mesh is unavailable");
 sim::Snapshot saved;saved.flight.configure(10);saved.flight.fuel=42;
 {std::ofstream out(folder/L"state.txt");saved.write(out);}
 auto load=[&](){do {
''' + load + r'''
 }while(false);};
 const auto active=encoded(state);load();
 check(encoded(state)==active&&!backupEligible&&!readyModels.count(10),"Read transaction preserves flight when saved model is missing");
 mesh(assetFolder()/"b777.fcmesh");load();
 check(flight.aircraftId==10&&flight.fuel==42&&readyModels.count(10)&&backupEligible&&menuPage==ui::Page::Pause,"Repaired saved profile loads before flight commit");
 check(readyModels.size()==3&&aircraftModels.size()==2,"Only three visited profiles and two external meshes resident");
 std::cout<<"Actual main blocks: lazy startup, switch failure/retry/cache, save-load failure/retry, state preservation and timing reset passed. CPU only.\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory(prefix='fcair-demand-') as folder:
    folder=Path(folder); source=folder/'driver.cpp'; source.write_text(code)
    binary=folder/'driver'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-I',str(root/'src'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(folder/'fixture')],check=True)
