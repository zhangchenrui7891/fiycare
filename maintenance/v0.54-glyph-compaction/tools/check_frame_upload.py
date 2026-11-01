"""Compile actual renderer upload statements with CPU mapping stand-ins.

This checks integration and uploaded bytes, NOT Windows compilation or GPU use.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source11 = (root / 'src/renderer11.h').read_text()
source12 = (root / 'src/renderer12.h').read_text()
start = '  const presentation::VertexBatch<Vertex> batch('
upload11 = source11[source11.index(start):source11.index('  float clear[]=')]
begin12 = source12.index(start)
upload12 = source12[begin12:source12.index('check(allocator->Reset());', begin12)]
prefix = r'''
#include "assets.h"
#include "frame_geometry.h"
#include "vertex_capacity.h"
#include <iostream>
using UINT=unsigned;
const Vertex skyVertices[6]{};
struct D3D11_MAPPED_SUBRESOURCE {void* pData=nullptr;};
constexpr int D3D11_MAP_WRITE_DISCARD=4;
void check(int status) {if(status)throw std::runtime_error("CPU map adapter failed");}
struct Buffer {int Get() const{return 0;}};
struct Context {
 std::vector<Vertex>& memory;int maps=0,unmaps=0;
 int Map(int,UINT,int mode,UINT,D3D11_MAPPED_SUBRESOURCE* mapped) {
  if(mode!=D3D11_MAP_WRITE_DISCARD)return 1;
  mapped->pData=memory.data();++maps;return 0;
 }
 void Unmap(int,UINT) {++unmaps;}
};
struct Upload11 {
 std::size_t capacity=0;std::vector<Vertex> memory;
 Context mapping{memory};Context* context=&mapping;Buffer vertices;
 void ensureVertexBuffer(std::size_t required) {
  capacity=growVertexCapacity(capacity,required,sizeof(Vertex));memory.resize(capacity);
 }
 void upload(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,
             const std::vector<Vertex>& cabin) {
'''
middle = r'''
 }
};
struct Upload12 {
 std::size_t capacity=0;std::vector<Vertex> memory;Vertex* mapped=nullptr;
 void ensureVertexBuffer(std::size_t required) {
  capacity=growVertexCapacity(capacity,required,sizeof(Vertex));memory.resize(capacity);mapped=memory.data();
 }
 void upload(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,
             const std::vector<Vertex>& cabin) {
'''
ending = r'''
 }
};
int main(int argc,char** argv) {
 if(argc!=2)return 1;
 Upload11 dx11;Upload12 dx12;
 for(const auto* file:{"a330.fcmesh","a350.fcmesh","a320.fcmesh","b737.fcmesh","b787.fcmesh"}) {
  auto model=loadModel(std::filesystem::path(argv[1])/file);
  for(int view=0;view<4;view++) {
   std::vector<Vertex> scene=view==3?std::vector<Vertex>{}:model.vertices;
   std::vector<Vertex> cabin(view==1?24000:0,Vertex{1,2,3,1,1,1});
   std::vector<Vertex> hud(view==2?0:6000,Vertex{4,5,6,.1f,.2f,.3f});
   std::vector<Vertex> expected(std::begin(skyVertices),std::end(skyVertices));
   expected.insert(expected.end(),scene.begin(),scene.end());
   expected.insert(expected.end(),cabin.begin(),cabin.end());
   expected.insert(expected.end(),hud.begin(),hud.end());
   dx11.upload(scene,hud,cabin);dx12.upload(scene,hud,cabin);
   if(std::memcmp(dx11.memory.data(),expected.data(),expected.size()*sizeof(Vertex))||
      std::memcmp(dx12.memory.data(),expected.data(),expected.size()*sizeof(Vertex)))return 2;
  }
 }
 if(dx11.mapping.maps!=20||dx11.mapping.unmaps!=20)return 3;
 std::cout<<"Actual DX11/DX12 upload statements: 5 fleet assets x 4 view/empty combinations; bytes match legacy staging.\n"
          <<"CPU stand-ins only: Windows APIs, GPU fences and rendering remain unverified.\n";
}
'''
with tempfile.TemporaryDirectory() as folder:
    source = Path(folder) / 'upload.cpp'
    source.write_text(prefix + upload11 + middle + upload12 + ending)
    exe = Path(folder) / 'upload'
    subprocess.run(['g++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-pedantic', '-I', str(root / 'src'), str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe), str(root / 'assets')], check=True)
