"""New recovery regression; not a recreation of the lost v0.41 script.

Executes the actual save lambdas with portable filesystem API substitutes.
Does not execute Windows CopyFileW/MoveFileExW or the entire message loop.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
start = main.index('  auto save=[&](){')
save = main[start:main.index('  SetProcessDPIAware();', start)]
start = main.index('  auto saveTracks=[&](){')
tracks = main[start:main.index('\n', start)]
for wiring in ('const runfiles::RunFiles runFiles(dataFolder(),benchmark,smoke);',
               'folder=runFiles.folder();', 'autoSave>=30',
               'case ui::Save:save();saveTracks();',
               'if(!smoke&&!benchmark){save();saveTracks();}'):
    if wiring not in main:
        raise RuntimeError('Actual save routing changed; review recovery test: ' + wiring)

code = r'''
#include "run_files.h"
#include "storage.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
constexpr int FALSE=0, MOVEFILE_REPLACE_EXISTING=1, MOVEFILE_WRITE_THROUGH=2;
bool CopyFileW(const char* a,const char* b,int){
 std::filesystem::copy_file(a,b,std::filesystem::copy_options::overwrite_existing);return true;
}
bool MoveFileExW(const char* a,const char* b,int){std::filesystem::rename(a,b);return true;}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string read(const std::filesystem::path& p){std::ifstream in(p);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char** argv){
 if(argc!=2)return 1;
 const auto base=std::filesystem::path(argv[1])/u8"玩家 空间";
 for(int mode=0;mode<4;++mode){
  const bool benchmark=(mode&1)!=0,smoke=(mode&2)!=0;
  const auto game=base/std::to_string(mode);std::filesystem::create_directories(game);
  for(const char* n:{"state.txt","state.bak","career.txt","tracks.txt"})std::ofstream(game/n)<<"PLAYER-UNCHANGED";
  const runfiles::RunFiles runFiles(game,benchmark,smoke);
  const auto expected=benchmark?game/"diagnostics"/"benchmark":smoke?game/"diagnostics"/"smoke":game;
  require(runFiles.folder()==expected,"Mode routing and benchmark precedence");
  require(runFiles.gameSavesEnabled()==(mode==0),"Diagnostic gate");
  auto folder=runFiles.folder();std::filesystem::create_directories(folder);
  sim::Snapshot state;state.flight.configure(3);state.flight.x=123.5;
  bool backupEligible=true;
  struct Mark{float x,z;};std::deque<Mark> marks{{1,2},{3,4}};
''' + save + tracks + r'''
  for(int i=0;i<4;++i){save();saveTracks();}
  if(mode){
   for(const char* n:{"state.txt","state.bak","career.txt","tracks.txt"})
    require(read(game/n)=="PLAYER-UNCHANGED","Diagnostic write touched player file");
   require(std::filesystem::is_empty(folder),"Diagnostic save created a file");
  }else{
   std::ostringstream encoded;state.write(encoded);
   require(read(game/"state.txt")==encoded.str(),"Normal main lambda wrote full state");
   require(read(game/"state.bak")==encoded.str(),"Normal repeated backup");
   require(read(game/"tracks.txt")=="1 2\n3 4\n","Actual track lambda output");
   std::ifstream in(game/"state.txt");sim::Snapshot restored;
   require(restored.read(in)&&restored.flight.x==123.5,"v16 state readback");
   require(!std::filesystem::exists(game/"state.tmp"),"No leftover committed temp");
   require(read(game/"career.txt")=="PLAYER-UNCHANGED","Unrelated legacy file unchanged");
  }
 }
 std::cout<<"PASS rebuilt storage: four modes, actual save/track lambdas, repeated commits, state readback. CPU API substitutes only.\n";
}
'''
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'storage.cpp'
    source.write_text(code)
    executable = Path(directory) / 'storage'
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O2']
    subprocess.run(['g++', '-std=c++17', '-pthread', '-Wall', '-Wextra', '-Werror', '-pedantic',
                    *flags, '-I', str(root / 'src'), str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable), str(Path(directory) / 'data')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
