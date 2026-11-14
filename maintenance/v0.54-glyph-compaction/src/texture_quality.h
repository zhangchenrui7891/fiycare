#pragma once
#include "assets.h"
#include <algorithm>
#include <utility>

// Each source region occupies its own GPU array slice, so filtering never
// crosses from satellite imagery into an aircraft material, even at 1x1 LOD.
struct MaterialTextures {
 uint32_t mipCount=0;
 std::vector<AtlasImage> subresources; // slice*mipCount + mip (D3D ordering)
};
inline std::vector<AtlasImage> textureMips(AtlasImage base){
 if(!base.width||!base.height||base.rgba.size()!=size_t(base.width)*base.height*4)
  throw std::runtime_error("Invalid mipmap source");
 std::array<float,256> linear{};
 for(size_t i=0;i<linear.size();i++)linear[i]=std::pow(float(i)/255,2.2f);
 std::vector<AtlasImage> chain;chain.push_back(std::move(base));
 while(chain.back().width>1||chain.back().height>1){
  const auto& src=chain.back();AtlasImage dst;
  dst.width=std::max(1u,src.width/2);dst.height=std::max(1u,src.height/2);dst.rgba.resize(size_t(dst.width)*dst.height*4);
  for(uint32_t y=0;y<dst.height;y++)for(uint32_t x=0;x<dst.width;x++){
   // Area bounds include the final row/column of odd-sized sources.
   const uint32_t x0=x*src.width/dst.width,x1=(x+1)*src.width/dst.width;
   const uint32_t y0=y*src.height/dst.height,y1=(y+1)*src.height/dst.height;
   float sum[4]{};const float count=float((x1-x0)*(y1-y0));
   for(uint32_t sy=y0;sy<y1;sy++)for(uint32_t sx=x0;sx<x1;sx++){
    const auto* pixel=&src.rgba[(size_t(sy)*src.width+sx)*4];
    for(int c=0;c<3;c++)sum[c]+=linear[pixel[c]];
    sum[3]+=pixel[3];
   }
   auto* pixel=&dst.rgba[(size_t(y)*dst.width+x)*4];
   for(int c=0;c<3;c++)pixel[c]=static_cast<unsigned char>(std::lround(std::pow(sum[c]/count,1.f/2.2f)*255));
   pixel[3]=static_cast<unsigned char>(std::lround(sum[3]/count));
  }
  chain.push_back(std::move(dst));
 }
 return chain;
}
inline MaterialTextures buildMaterialTextures(const AtlasImage& atlas){
 if(atlas.width<4||atlas.width!=atlas.height||(atlas.width&(atlas.width-1))||atlas.width>4096||atlas.rgba.size()!=size_t(atlas.width)*atlas.height*4)
  throw std::runtime_error("Material atlas must be square power-of-two RGBA");
 const uint32_t side=atlas.width/2;
 const uint32_t regions[5][3]={{0,0,side},{side,0,side},{0,side,side},{side,side,side/2},{side+side/2,side,side/2}};
 MaterialTextures result;
 for(const auto& region:regions){
  AtlasImage base;base.width=base.height=side;base.rgba.resize(size_t(side)*side*4);
  // The two smaller source regions are replicated exactly to the common
  // array resolution. This changes storage size, not source detail.
  for(uint32_t y=0;y<side;y++)for(uint32_t x=0;x<side;x++){
   const size_t src=(size_t(region[1]+y*region[2]/side)*atlas.width+region[0]+x*region[2]/side)*4;
   std::copy_n(atlas.rgba.data()+src,4,base.rgba.data()+(size_t(y)*side+x)*4);
  }
  auto chain=textureMips(std::move(base));result.mipCount=static_cast<uint32_t>(chain.size());
  for(auto& mip:chain)result.subresources.push_back(std::move(mip));
 }
 return result;
}

inline MaterialTextures buildFleetTextures(const AtlasImage& atlas,AtlasImage aircraft){
 auto result=buildMaterialTextures(atlas);
 const auto side=result.subresources[0].width;
 if(aircraft.width!=side||aircraft.height!=side)throw std::runtime_error("Aircraft texture resolution mismatch");
 for(auto& mip:textureMips(std::move(aircraft))) result.subresources.push_back(std::move(mip));
 return result;
}
