"""Compile actual landscape/box/tri statements with identity CPU math adapters.

Compares existing terrain vertices and fictional-mode geometry to frozen v0.49.
Does not compile DirectX headers or claim a Windows program/GPU check.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/scene.h').read_text()
helpers = source[source.index('inline void tri('):source.index('inline float terrain(')]
landscape = source[source.index('inline std::vector<Vertex> landscape('):source.index('inline void airportBuildings(')]
code = r'''
#include "assets.h"
#include "simulation.h"
#include "render_stability.h"
#include "scenery.h"
#include <iostream>
#include <stdexcept>
struct XMFLOAT3{float x,y,z;};
using XMVECTOR=XMFLOAT3;
using XMMATRIX=int;
int XMMatrixIdentity(){return 0;}
XMFLOAT3 XMVectorSet(float x,float y,float z,float){return {x,y,z};}
XMFLOAT3 XMVector3TransformCoord(XMFLOAT3 p,int){return p;}
void XMStoreFloat3(XMFLOAT3* out,XMFLOAT3 p){*out=p;}
''' + helpers + '\n' + landscape + '\n' + (root / 'tests/reference/landscape_v049.h').read_text() + r'''
int main(){
 unsigned cases=0;
 for(int mode:{0,1})for(int x:{0,-4,100})for(int z:{0,7})
  for(const auto& color:{XMFLOAT3{.24f,.5f,.2f},XMFLOAT3{.83f,.88f,.92f}}){
   auto before=legacyLandscape(x,z,color,mode),after=landscape(x,z,color,mode);
   if(after.size()<before.size()||std::memcmp(before.data(),after.data(),before.size()*sizeof(Vertex)))
    throw std::runtime_error("Original terrain/fictional geometry changed");
   if(mode==0&&after.size()!=before.size())throw std::runtime_error("Scenery leaked into fictional mode");
   if(mode==1&&x<=0&&after.size()<=before.size())throw std::runtime_error("Scenery missing in satellite mode");
   if(after.size()>before.size()+scenery::vertexBudget)throw std::runtime_error("Integrated budget exceeded");
   ++cases;
  }
 std::cout<<"PASS actual landscape statements: "<<cases
          <<" mode/camera/season cases; v0.49 ground bytes and fictional geometry preserved. Identity CPU math only.\n";
}
'''
with tempfile.TemporaryDirectory(prefix='fcair-scenery-') as folder:
    source_path = Path(folder) / 'check.cpp'
    executable = Path(folder) / 'check'
    source_path.write_text(code)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-O2', '-Wall', '-Wextra',
                    '-Werror', '-pedantic', '-pthread', '-I', str(root / 'src'), str(source_path),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
