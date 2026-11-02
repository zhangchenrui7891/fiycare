"""Execute the production ResetModel switch body with portable UI stand-ins."""
from pathlib import Path
import tempfile, subprocess, sys
root=Path(__file__).resolve().parents[1]
main=Path(sys.argv[1]).read_text() if len(sys.argv)>1 else (root/'src/main.cpp').read_text()
body=main.split('     case ui::ResetModel:{',1)[1].split('     case ui::PassengersLess:',1)[0]
code=r'''
#include "src/model_restore.h"
#include "src/ui_state.h"
#include "src/fleet_assets.h"
#include <set>
#include <iostream>
sim::Snapshot state;auto& career=state.career;auto& flight=state.flight;
std::filesystem::path folder;std::map<int,ImportedModel> aircraftModels;std::set<int> customModels;
std::wstring notice;void logLine(const std::string&){}
std::filesystem::path assetFolder(){return folder;}
void invoke(){switch(int(ui::ResetModel)){case ui::ResetModel:{
'''+body+r'''
}}
int main(int argc,char** argv){if(argc!=2)return 2;folder=argv[1];flight.configure(2);
ImportedModel old;old.vertices.resize(3);old.vertices[0].x=42;aircraftModels[2]=old;customModels.insert(2);
auto path=folder/"custom_aircraft_2.fcmesh";std::ofstream(path)<<"keep me";
try{invoke();}catch(const std::exception&){std::cerr<<"FAIL: ResetModel exception escapes the menu\n";return 1;}
if(aircraftModels.at(2).vertices[0].x!=42||!customModels.count(2)||!std::filesystem::exists(path)||notice.empty())return 3;
std::cout<<"PASS: production menu catches missing default, preserves model/file/tag and sets notice. CPU stand-ins only.\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'check.cpp').write_text(code)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic','-I',str(root),str(p/'check.cpp'),'-o',str(p/'check')],check=True)
 sys.exit(subprocess.run([str(p/'check'),d]).returncode)
