#include "src/model_restore.h"
#include <iostream>
#include <chrono>
#include <string>
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/("fcair-restore-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 try{
  std::filesystem::create_directories(dir);
  std::map<int,ImportedModel> models;ImportedModel old;old.vertices.resize(3);old.vertices[0].x=42;old.gear={1};models.emplace(2,old);
  std::ofstream(dir/"custom.fcmesh")<<"persistent custom data";
  for(int failure=0;failure<3;++failure){
   auto path=dir/"default.fcmesh";
   if(failure==1)std::ofstream(path)<<"corrupt";
   if(failure==2){std::ofstream out(path,std::ios::binary);out.write("FCMESH07",8);uint32_t dims[]={3,12,1};out.write(reinterpret_cast<char*>(dims),12);}
   bool threw=false;try{restoreDefaultModel(models,2,path);}catch(const std::exception&){threw=true;}
   check(threw,"Missing/corrupt/truncated default must fail");
   check(models.size()==1&&models.at(2).vertices[0].x==42&&models.at(2).gear[0]==1,"Failure must preserve current model");
   check(std::filesystem::file_size(dir/"custom.fcmesh")==22,"Failure must preserve persistent custom model");
  }
  auto path=dir/"valid.fcmesh";
  {std::ofstream out(path,std::ios::binary);out.write("FCMESH07",8);uint32_t dims[]={3,12,1};out.write(reinterpret_cast<char*>(dims),12);Vertex v[3]{};for(auto& x:v)x.ny=1;v[0].x=7;out.write(reinterpret_cast<char*>(v),sizeof(v));out.put(0);}
  restoreDefaultModel(models,2,path);check(models.at(2).vertices[0].x==7&&models.at(2).gear[0]==0,"Valid default replaces custom model");
  restoreDefaultModel(models,3,path);check(models.size()==2&&models.at(3).vertices[0].x==7,"Absent model can be restored");
  std::filesystem::remove_all(dir);std::cout<<"PASS: missing, corrupt and truncated defaults preserve current model; valid replace/insert succeed. CPU filesystem test.\n";
 }catch(const std::exception& e){std::filesystem::remove_all(dir);std::cerr<<e.what()<<'\n';return 1;}
}
