#include "src/assets.h"
#include "src/frame_geometry.h"
#include "src/fleet_assets.h"
#include "src/ui_state.h"
#include "src/model_transform.h"
#include "src/texture_quality.h"
#include <chrono>
#include <iostream>
#include <sstream>
void need(bool ok){if(!ok)throw std::runtime_error("Fleet variant regression");}
int main(int argc,char**argv){need(argc==2);for(int id=0;id<sim::aircraftCount;id++){sim::Snapshot s;s.flight.configure(id);s.flight.passengers=s.flight.spec().seats;std::stringstream save;s.write(save);sim::Snapshot restored;need(restored.read(save)&&restored.flight.aircraftId==id);auto d=sim::airframeDimensions(id);need(d.length>0&&d.span>0);if(id<7)continue;auto profile=sim::fleetAsset(id);need(profile!=nullptr);auto model=loadModel(std::filesystem::path(argv[1])/profile->nativeName);float lo=1e9,hi=-1e9;for(auto&v:model.vertices){lo=std::min(lo,v.z);hi=std::max(hi,v.z);}need(std::abs(hi-lo-profile->lengthMeters)<.02);s.flight.y=1500;s.flight.vz=100;s.flight.gear=false;s.flight.flap=0;s.flight.throttle=.6;sim::Controls c;for(int i=0;i<1000;i++)s.flight.step(.01,c);need(std::isfinite(s.flight.y)&&std::isfinite(s.flight.vz));}need(ui::aircraftIdForCommand(ui::A319)==7&&ui::aircraftIdForCommand(ui::A321)==8&&ui::aircraftIdForCommand(ui::B7879)==9);need(ui::aircraftIdForCommand(ui::B777)==10);
 const auto assets=std::filesystem::path(argv[1]);
 auto model=loadModel(assets/"b777.fcmesh");
 need(model.indices.size()==model.vertices.size()&&model.uniqueVertices.size()*2<model.vertices.size());
 need(std::count(model.gear.begin(),model.gear.end(),1)>0);
 need(std::any_of(model.vertices.begin(),model.vertices.end(),[](const Vertex& v){return v.textured==5;}));
 const auto tail=std::max_element(model.vertices.begin(),model.vertices.end(),[](const Vertex& a,const Vertex& b){return a.y<b.y;});
 need(tail->z<-20);
 auto verifyIndex=[](const ImportedModel& mesh){
  for(size_t i=0;i<mesh.vertices.size();i++)
   need(std::memcmp(&mesh.vertices[i],&mesh.uniqueVertices.at(mesh.indices.at(i)),sizeof(Vertex))==0);
 };
 verifyIndex(model);
 verifyIndex(rotateAircraftQuarterTurn(model,73.86,5.5));
 // Reconstruct the draw sequence around the indexed span, preserving the
 // orientation, texture fields, and ordering of each triangle.
 presentation::FrameGeometry<Vertex> frame;
 const std::vector<Vertex> terrain(6,Vertex{1,2,3,1,1,1});
 const std::vector<Vertex> trailing(3,Vertex{3,2,1,1,1,1});
 auto transform=[](Vertex v){v.x+=100;v.y-=25;return v;};
 for(bool gearDown:{true,false,true}){
  frame.beginFrame(terrain);
  auto& indexed=frame.indexedScene;
  need(indexed.indices.empty()&&indexed.vertexCount==0);
  indexed.append(frame.scene,model,gearDown,transform);
  frame.scene.insert(frame.scene.end(),trailing.begin(),trailing.end());
  need(indexed.firstVertex==terrain.size()&&indexed.vertexCount==23447);
  need(indexed.indices.size()==(gearDown?91548:62748));
  std::vector<Vertex> reconstructed(frame.scene.begin(),frame.scene.begin()+indexed.firstVertex);
  for(auto i:indexed.indices){
   need(i<indexed.vertexCount);
   reconstructed.push_back(frame.scene[indexed.firstVertex+i]);
  }
  reconstructed.insert(reconstructed.end(),frame.scene.begin()+indexed.firstVertex+indexed.vertexCount,frame.scene.end());
  auto expected=terrain;
  for(size_t t=0;t<model.gear.size();++t){
   if(!gearDown&&model.gear[t])continue;
   for(size_t k=0;k<3;++k)expected.push_back(transform(model.vertices[t*3+k]));
  }
  expected.insert(expected.end(),trailing.begin(),trailing.end());
  need(reconstructed.size()==expected.size());
  need(std::memcmp(reconstructed.data(),expected.data(),expected.size()*sizeof(Vertex))==0);
  std::cout<<"Gear "<<(gearDown?"down":"up")<<": "
           << indexed.indices.size()*sizeof(Vertex)<<" -> "
           << indexed.vertexCount*sizeof(Vertex)+indexed.indices.size()*sizeof(std::uint32_t)
           <<" bytes for model upload.\n";
 }
 frame.beginFrame(terrain);
 need(frame.indexedScene.indices.empty()&&frame.indexedScene.vertexCount==0);
 std::string warning;
 const auto textures=loadFleetTextures(assets,warning);
 need(warning.empty());
 need(textures.subresources.size()==6*textures.mipCount&&textures.subresources[5*textures.mipCount].width==1024);
 const auto original=loadAtlas(assets/"b777.fctex");
 need(textures.subresources[5*textures.mipCount].rgba==original.rgba);
 struct Temporary {
  std::filesystem::path path=std::filesystem::temp_directory_path()/
   ("fcair_livery_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Temporary(){std::filesystem::create_directory(path);}
  ~Temporary(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
 } temporary;
 const auto base=temporary.path/"atlas.fctex", livery=temporary.path/"b777.fctex";
 // Tiny valid atlas and a wrong-sized livery exercise the same startup paths.
 const auto writeTexture=[](const std::filesystem::path& file,unsigned char side){
  std::ofstream output(file,std::ios::binary);
  const unsigned char dimensions[]={side,0,0,0,side,0,0,0};
  output.write("FCTEX005",8);
  output.write(reinterpret_cast<const char*>(dimensions),sizeof(dimensions));
  for(int i=0;i<side*side*4;++i)output.put(char(i));
  need(bool(output));
 };
 const auto rejectMissingBase=[&]{
  try{loadFleetTextures(temporary.path,warning);}
  catch(const std::runtime_error& error){return std::string(error.what()).find("atlas.fctex")!=std::string::npos;}
  return false;
 };
 need(rejectMissingBase());
 std::ofstream(base)<<"bad";
 need(rejectMissingBase());
 writeTexture(base,8);
 const auto checkFallback=[&](const char* expectedReason){
  const auto result=loadFleetTextures(temporary.path,warning);
  need(warning.find("b777.fctex")!=std::string::npos&&warning.find(expectedReason)!=std::string::npos);
  need(result.mipCount==3&&result.subresources.size()==18);
  const auto unchanged=buildMaterialTextures(loadAtlas(base));
  for(size_t i=0;i<unchanged.subresources.size();++i)
   need(result.subresources[i].rgba==unchanged.subresources[i].rgba);
  for(size_t i=15;i<18;++i){
   const auto& mip=result.subresources[i];
   const auto expectedSide=4u>>(i-15);
   need(mip.width==expectedSide&&mip.height==expectedSide&&mip.rgba.size()==expectedSide*expectedSide*4);
   need(std::all_of(mip.rgba.begin(),mip.rgba.end(),[](unsigned char value){return value==255;}));
  }
 };
 checkFallback("Could not open texture file");
 std::ofstream(livery)<<"bad";
 checkFallback("Invalid asset header");
 writeTexture(livery,2);
 checkFallback("resolution mismatch");
 writeTexture(livery,4);
 std::filesystem::resize_file(livery,17);
 checkFallback("payload size");
 writeTexture(livery,4);
 const auto recovered=loadFleetTextures(temporary.path,warning);
 need(warning.empty()&&recovered.subresources[15].rgba==loadAtlas(livery).rgba);
 std::cout<<"PASS: missing, corrupt, truncated and wrong-size optional livery; required atlas errors and restored livery.\n";
 std::cout<<"PASS: 11 aircraft, 777 scale, orientation, gear, indexed geometry, and texture mip chain.\n";
}
