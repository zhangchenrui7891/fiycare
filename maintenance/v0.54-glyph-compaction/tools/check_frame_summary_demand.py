"""Run the production summary decision with a counted window and an eager reference."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/main.cpp').read_text()
decision = main[main.index('   const bool reportDue='):main.index('   if(reportDue){')]
source = r'''
#include "performance.h"
#include <iostream>
#include <stdexcept>
namespace ui { enum class Page {None, Pause}; }
struct CountedWindow : timing::FrameWindow {
 int calls=0;
 timing::FrameStats summary(){++calls;return timing::FrameWindow::summary();}
};
void run(bool debug,bool menu,bool csv){
 CountedWindow frameWindow;
 timing::FrameWindow eager;
 struct {bool debugPanel;} state{debug};
 auto menuPage=menu?ui::Page::Pause:ui::Page::None;
 bool perf=csv;
 double reportTime=0;
 int reports=0;
 for(int i=0;i<1920;++i){
  const double dt=(i%2?9.:7.)/256.; // Exact binary intervals; 60 simulated seconds.
  frameWindow.add(dt);eager.add(dt);reportTime+=dt;
  const auto expected=eager.summary();
  DECISION
  if(reportDue){++reports;reportTime=0;}
  if(reportDue||(debug&&!menu)){
   if(expected.samples!=frameStats.samples||expected.fps!=frameStats.fps||
      expected.p95Ms!=frameStats.p95Ms||expected.maxMs!=frameStats.maxMs)
    throw std::runtime_error("Consumer read different rolling statistics");
  }
 }
 const int expectedReports=csv?12:0;
 if(reports!=expectedReports||frameWindow.calls!=(debug&&!menu?1920:expectedReports))
  throw std::runtime_error("Summary count or report cadence differs");
 std::cout<<"debug="<<debug<<" menu="<<menu<<" csv="<<csv<<" eager=1920 actual="<<frameWindow.calls
          <<" reports="<<reports<<"\n";
}
int main(){for(bool debug:{false,true})for(bool menu:{false,true})for(bool csv:{false,true})run(debug,menu,csv);}
'''.replace('DECISION', decision)
with tempfile.TemporaryDirectory() as folder:
    path = Path(folder)
    (path / 'probe.cpp').write_text(source)
    subprocess.run(['g++', '-std=c++17', '-O2', '-I', str(root / 'src'), str(path / 'probe.cpp'), '-o', str(path / 'probe')], check=True)
    subprocess.run([str(path / 'probe')], check=True)
