// Converts the FlightGear 777-300ER exterior. See FLIGHTGEAR_777.md.
#include "asset_import.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace {
void recordNames(const aiNode& node,std::string prefix,std::vector<std::string>& names){
 prefix+="/"+std::string(node.mName.C_Str());
 for(unsigned i=0;i<node.mNumMeshes;i++)names.at(node.mMeshes[i])=prefix;
 for(unsigned i=0;i<node.mNumChildren;i++)recordNames(*node.mChildren[i],prefix,names);
}
// AC3D is Y-up. The source nose is -X; the game uses +Z.
aiVector3D gameAxes(aiVector3D point){return {point.z,point.y,-point.x};}
bool retractableGear(std::string name){
 std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
 for(const auto* word:{"gear","wheel","strut","axle","damper"})
  if(name.find(word)!=std::string::npos)return true;
 return false;
}
}

int main(int argc,char** argv){
 try{
  if(argc!=3)throw std::runtime_error("Usage: import_fg777 777-300ER.ac b777.fcmesh");
  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY,true);
  const auto* scene=importer.ReadFile(argv[1],aiProcess_Triangulate|aiProcess_PreTransformVertices|
    aiProcess_GenSmoothNormals|aiProcess_SortByPType);
  if(!scene||!scene->HasMeshes())throw std::runtime_error(importer.GetErrorString());
  std::vector<std::string> names(scene->mNumMeshes);
  recordNames(*scene->mRootNode,"",names);
  ImportedModel model;
  aiVector3D low(1e9f,1e9f,1e9f),high(-1e9f,-1e9f,-1e9f);
  size_t skipped=0;
  for(unsigned m=0;m<scene->mNumMeshes;m++){
   const auto& mesh=*scene->mMeshes[m];
   const auto& material=*scene->mMaterials[mesh.mMaterialIndex];
   aiString texture;
   material.GetTexture(aiTextureType_DIFFUSE,0,&texture);
   const std::string name=texture.C_Str();
   // The remaining textured meshes are FlightGear light sprites.
   if(!name.empty()&&name!="paint1.png"){skipped+=mesh.mNumFaces;continue;}
   aiColor4D color(1,1,1,1);
   aiGetMaterialColor(&material,AI_MATKEY_COLOR_DIFFUSE,&color);
   const bool textured=name=="paint1.png"&&mesh.HasTextureCoords(0);
   const bool gear=retractableGear(names[m]);
   for(unsigned f=0;f<mesh.mNumFaces;f++){
    const auto& face=mesh.mFaces[f];
    if(face.mNumIndices!=3){++skipped;continue;}
    aiVector3D points[3];
    for(int k=0;k<3;k++)points[k]=gameAxes(mesh.mVertices[face.mIndices[k]]);
    auto normal=(points[1]-points[0])^(points[2]-points[0]);
    if(normal.SquareLength()<1e-14f){++skipped;continue;}
    normal.Normalize();
    for(int k=0;k<3;k++){
     const auto i=face.mIndices[k];
     const auto p=points[k];
     auto n=mesh.HasNormals()?gameAxes(mesh.mNormals[i]):normal;
     if(n.SquareLength()<1e-12f)n=normal;
     n.Normalize();
     const auto uv=textured?mesh.mTextureCoords[0][i]:aiVector3D();
     model.vertices.push_back({p.x,p.y,p.z,color.r,color.g,color.b,
       std::clamp(uv.x,0.f,1.f),std::clamp(1-uv.y,0.f,1.f),textured?5.f:0.f,n.x,n.y,n.z});
     low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);low.z=std::min(low.z,p.z);
     high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);high.z=std::max(high.z,p.z);
    }
    model.gear.push_back(gear);
   }
  }
  if(model.vertices.empty()||high.z-low.z<1e-5f)throw std::runtime_error("Empty or degenerate exterior");
  const float scale=73.86f/(high.z-low.z);
  for(auto& v:model.vertices){
   v.x=(v.x-(low.x+high.x)*.5f)*scale;
   v.y=(v.y-low.y)*scale-5.5f;
   v.z=(v.z-(low.z+high.z)*.5f)*scale;
  }
  saveAircraftModel(model,argv[2]);
  const auto verified=loadModel(argv[2]);
  std::cout<<"triangles="<<verified.gear.size()<<" gear="<<std::count(verified.gear.begin(),verified.gear.end(),1)
           <<" skipped="<<skipped<<" corners="<<verified.vertices.size()<<" unique="<<verified.uniqueVertices.size()<<'\n';
  return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
