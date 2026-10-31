#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <cmath>
struct Vertex{float x,y,z,r,g,b;float u=0,v=0,textured=0;float nx=0,ny=0,nz=0;};
static_assert(sizeof(Vertex)==48,"Packed mesh vertex layout");
struct AtlasImage{uint32_t width=0,height=0;std::vector<unsigned char> rgba;};
struct ImportedModel{std::vector<Vertex> vertices;std::vector<unsigned char> gear;};
inline uint32_t assetU32(std::istream& in){std::array<unsigned char,4> b{};in.read(reinterpret_cast<char*>(b.data()),4);return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
inline void assetMagic(std::istream& in,const char* expected){char b[8]{};in.read(b,8);if(!in||std::memcmp(b,expected,8))throw std::runtime_error("Invalid asset header");}
inline AtlasImage loadAtlas(const std::filesystem::path& p){
 std::ifstream in(p,std::ios::binary);assetMagic(in,"FCTEX005");AtlasImage a;a.width=assetU32(in);a.height=assetU32(in);
 if(!a.width||!a.height||a.width>4096||a.height>4096)throw std::runtime_error("Invalid atlas dimensions");
 a.rgba.resize(size_t(a.width)*a.height*4);in.read(reinterpret_cast<char*>(a.rgba.data()),static_cast<std::streamsize>(a.rgba.size()));
 if(!in||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Invalid atlas payload");
 return a;
}
inline ImportedModel loadModel(const std::filesystem::path& p){
 std::ifstream in(p,std::ios::binary);char meshMagic[8]{};in.read(meshMagic,8);const bool colored=std::memcmp(meshMagic,"FCMESH07",8)==0;
 if(!in||(!colored&&std::memcmp(meshMagic,"FCMESH06",8)))throw std::runtime_error("Invalid mesh header");
 const auto n=assetU32(in),stride=assetU32(in),triangles=assetU32(in);
 if(!n||n>3000000||n%3||stride!=12||triangles!=n/3)throw std::runtime_error("Invalid mesh dimensions");
 ImportedModel m;m.vertices.resize(n);m.gear.resize(triangles);in.read(reinterpret_cast<char*>(m.vertices.data()),static_cast<std::streamsize>(n*sizeof(Vertex)));in.read(reinterpret_cast<char*>(m.gear.data()),triangles);
 if(!in||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Invalid mesh payload");
 for(const auto& v:m.vertices)if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.r)||!std::isfinite(v.g)||!std::isfinite(v.b)||!std::isfinite(v.u)||!std::isfinite(v.v)||v.u<0||v.u>1||v.v<0||v.v>1||(colored?v.textured!=0:v.textured!=1)||!std::isfinite(v.nx)||!std::isfinite(v.ny)||!std::isfinite(v.nz)||std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)>.02)throw std::runtime_error("Invalid mesh vertex");
 for(auto g:m.gear)if(g>1)throw std::runtime_error("Invalid gear tag");
 return m;
}
