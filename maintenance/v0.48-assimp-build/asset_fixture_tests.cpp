#include "src/asset_import.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct Temporary {
 std::filesystem::path path=std::filesystem::temp_directory_path()/
  ("fcair_fixture_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Temporary(){std::filesystem::create_directories(path);}
 ~Temporary(){std::error_code error;std::filesystem::remove_all(path,error);}
};
void fixture(const std::filesystem::path& folder,bool unicode){
 std::filesystem::create_directories(folder);
 const auto modelName=unicode?u8"飞机 模型.obj":"plane.obj";
 const auto materialName=unicode?u8"机身 材质.mtl":"body.mtl";
 const auto textureName=unicode?u8"蓝色 贴图.ppm":"blue.ppm";
 {std::ofstream out(folder/std::filesystem::u8path(textureName),std::ios::binary);
  out<<"P6\n1 1\n255\n";const unsigned char rgb[]={51,102,204};
  out.write(reinterpret_cast<const char*>(rgb),3);require(bool(out),"PPM fixture written");}
 {std::ofstream out(folder/std::filesystem::u8path(materialName));
  out<<"newmtl body\nKd 1 1 1\nmap_Kd "<<textureName<<"\n";require(bool(out),"MTL fixture written");}
 const auto model=folder/std::filesystem::u8path(modelName);
 {std::ofstream out(model);out<<"mtllib "<<materialName<<"\n"
   "v -1 0 -1\nv 1 0 -1\nv 0 0 1\nvt 0 0\nvt 1 0\nvt 0 1\n"
   "usemtl body\nf 1/1 3/3 2/2\n";require(bool(out),"OBJ fixture written");}
 AircraftImportOptions options;options.lengthMeters=4;options.groundOffset=1;
 const auto imported=importAircraftModel(model,options);
 require(imported.report.importedTriangles==1&&imported.model.vertices.size()==3,"Real Assimp OBJ import");
 require(imported.report.decodedTextures==1,"External UTF-8 texture path resolves and bakes");
 require(imported.report.source==modelName,"UTF-8 source name retained");
 for(const auto& v:imported.model.vertices)
  require(std::abs(v.r-.2f)<.001&&std::abs(v.g-.4f)<.001&&std::abs(v.b-.8f)<.001,"Actual decoded pixel is baked into vertex colors");
 const auto packed=folder/std::filesystem::u8path(unicode?u8"已转换 飞机.fcmesh":"converted.fcmesh");
 saveAircraftModel(imported.model,packed);
 const auto loaded=loadModel(packed);
 require(loaded.vertices.size()==3&&std::abs(loaded.vertices[0].b-.8f)<.001,"Native mesh roundtrip on Unicode path");
 options.bakeTextures=false;const auto untextured=importAircraftModel(model,options);
 require(untextured.model.vertices.size()==3&&untextured.report.decodedTextures==0,"Explicit texture-bake disable retained");
 options.lengthMeters=0;bool rejected=false;
 try{importAircraftModel(model,options);}catch(const std::exception&){rejected=true;}
 require(rejected,"Invalid import scale rejected");
}
int main(){
 try{
  Temporary files;fixture(files.path/"ascii",false);
  fixture(files.path/std::filesystem::u8path(u8"中文 目录"),true);
  std::cout<<"PASS actual Assimp import: ASCII/Unicode model, MTL and external texture; baked RGB, native roundtrip, bake-disable and invalid scale. Synthetic fixtures, no GPU or flight test.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
