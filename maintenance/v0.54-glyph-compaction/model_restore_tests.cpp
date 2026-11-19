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
  const auto missing=dir/"missing.fcmesh",bad=dir/"bad.fcmesh";
  std::ofstream(bad)<<"broken";
  auto preferred=loadStartupModel(missing,path);
  check(preferred.custom&&preferred.model&&preferred.model->vertices[0].x==7&&preferred.warning.empty(),"Valid custom bypasses unavailable default");
  preferred=loadStartupModel(path,bad);
  check(!preferred.custom&&preferred.model&&!preferred.warning.empty(),"Damaged custom falls back to valid default with warning");
  preferred=loadStartupModel(path,missing);
  check(!preferred.custom&&preferred.model&&preferred.warning.empty(),"Absent custom loads default");
  preferred=loadStartupModel({},missing);
  check(!preferred.custom&&!preferred.model&&preferred.warning.empty(),"Procedural aircraft needs no mesh");
  preferred=loadStartupModel({},bad);
  check(!preferred.model&&!preferred.warning.empty(),"Damaged custom on procedural aircraft leaves procedural fallback");
  preferred=loadStartupModel({},path);
  check(preferred.custom&&preferred.model,"Procedural aircraft accepts valid custom");
  const auto oversized=dir/std::string(4096,'x');
  bool probeFailed=false;
  try{(void)std::filesystem::exists(oversized);}catch(const std::filesystem::filesystem_error&){probeFailed=true;}
#ifndef _WIN32
  check(probeFailed,"Linux oversized component must exercise a real filesystem probe exception");
#endif
  if(probeFailed){
   preferred=loadStartupModel(path,oversized);
   check(!preferred.custom&&preferred.model&&preferred.model->vertices[0].x==7&&!preferred.warning.empty(),"Probe failure falls back to valid default and retains diagnostic");
   preferred=loadStartupModel({},oversized);
   check(!preferred.custom&&!preferred.model&&!preferred.warning.empty(),"Probe failure preserves procedural fallback");
   bool requiredFailed=false;try{loadStartupModel(missing,oversized);}catch(const std::runtime_error&){requiredFailed=true;}
   check(requiredFailed,"Missing required default remains fatal after custom probe failure");
   check(models.at(2).vertices[0].x==7&&std::filesystem::file_size(dir/"custom.fcmesh")==22,"Probe failure does not mutate installed models or persistent custom data");
   std::cout<<"Real filesystem probe exception: default/procedural fallback, diagnostic, required-default failure and existing data preservation passed.\n";
  }else std::cout<<"Probe-exception case not exercised on this platform; ordinary fallback cases still run.\n";
  bool failed=false;try{loadStartupModel(missing,bad);}catch(const std::runtime_error&){failed=true;}
  check(failed,"No valid custom or required default must still report startup failure");
  // Startup only requests the active profile. Other paths remain unopened,
  // and revisiting a ready profile never replaces a live imported/edited mesh.
  std::map<int,ImportedModel> lazy;std::set<int> ready,custom;
  check(ensureAircraftModel(lazy,ready,custom,3,path,missing).empty(),"Active default loads without warning");
  check(lazy.size()==1&&ready.size()==1&&ready.count(3)&&custom.empty(),"Only requested profile becomes resident");
  lazy.at(3).vertices[0].x=99;
  check(ensureAircraftModel(lazy,ready,custom,3,missing,bad).empty()&&lazy.at(3).vertices[0].x==99,"Cached profile skips disk and preserves edits");
  ensureAircraftModel(lazy,ready,custom,0,{},missing);
  check(ready.count(0)&&lazy.size()==1,"Procedural profile remembered without mesh allocation");
  check(ensureAircraftModel(lazy,ready,custom,0,missing,bad).empty(),"Revisiting procedural profile skips disk");
  ensureAircraftModel(lazy,ready,custom,2,missing,path);
  check(lazy.size()==2&&custom.count(2),"New profile loads custom before missing default");
  check(!ensureAircraftModel(lazy,ready,custom,4,path,bad).empty()&&!custom.count(4)&&ready.count(4),"Corrupt custom fallback is retained and marked ready");
  bool lazyFailed=false;
  try{ensureAircraftModel(lazy,ready,custom,10,missing,bad);}catch(const std::runtime_error&){lazyFailed=true;}
  check(lazyFailed&&!ready.count(10)&&!lazy.count(10)&&!custom.count(10),"Failed request leaves profile retryable");
  check(lazy.size()==3&&ready.size()==4&&lazy.at(3).vertices[0].x==99&&custom.count(2),"Failed selection preserves previous resident models and custom markers");
  ensureAircraftModel(lazy,ready,custom,10,path,missing);
  check(ready.count(10)&&lazy.at(10).vertices[0].x==7,"Repair and retry succeeds");
  std::cout<<"On-demand profiles: one active mesh, cached edits/procedural skip, custom fallback, failure isolation and repair/retry passed.\n";
  std::filesystem::remove_all(dir);std::cout<<"PASS: missing, corrupt and truncated defaults preserve current model; valid replace/insert succeed. Custom-first startup, default/procedural fallbacks and unrecoverable failure passed. CPU filesystem test.\n";
 }catch(const std::exception& e){std::filesystem::remove_all(dir);std::cerr<<e.what()<<'\n';return 1;}
}
