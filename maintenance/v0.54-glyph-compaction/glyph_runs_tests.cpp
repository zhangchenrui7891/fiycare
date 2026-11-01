#include "src/glyph_runs.h"
#include <array>
#include <random>
#include <stdexcept>
#include <iostream>
void check(bool ok){if(!ok)throw std::runtime_error("glyph coverage regression");}
using Mask=std::array<int,1024>;
Mask raster(const std::vector<PixelRun>& runs){
 Mask mask{};
 for(const auto& r:runs)for(int y=int(r.y);y<int(r.y+r.h);++y)
  for(int x=int(r.x);x<int(r.x+r.w);++x){check(x>=0&&x<32&&y>=0&&y<32);mask[size_t(y*32+x)]++;}
 return mask;
}
int main(){
 std::mt19937 rng(54);size_t before=0,after=0;
 for(int test=0;test<3000;++test){
  Mask mask{};for(auto& p:mask)p=(rng()%100)<unsigned(test%101);
  std::vector<PixelRun> runs;
  for(int y=0;y<32;++y)for(int x=0;x<32;){
   if(!mask[size_t(y*32+x)]){++x;continue;}
   int start=x;while(x<32&&mask[size_t(y*32+x)])++x;
   runs.push_back({float(start),float(y),float(x-start),1});
  }
  before+=runs.size();compactGlyphRuns(runs);after+=runs.size();
  check(raster(runs)==mask);auto n=runs.size();compactGlyphRuns(runs);
  check(raster(runs)==mask&&runs.size()==n);
 }
 std::vector<PixelRun> stem;
 for(int y=2;y<15;++y)stem.push_back({1,float(y),2,1});
 compactGlyphRuns(stem);check(stem.size()==1&&stem[0].h==13);
 std::vector<PixelRun> gaps{{1,0,2,1},{1,2,2,1},{1,3,3,1}};
 auto original=raster(gaps);compactGlyphRuns(gaps);check(gaps.size()==3&&raster(gaps)==original);
 std::vector<PixelRun> negative{{-3,-2,2,1},{-3,-1,2,1}};
 compactGlyphRuns(negative);check(negative.size()==1&&negative[0].y==-2&&negative[0].h==2);
 std::cout<<"3000 masks exact coverage, no overlap; idempotence, gaps, width changes, negative bearings passed\n"
          <<"Synthetic rectangles "<<before<<" -> "<<after<<"; 13-row stem vertices 78 -> 6. Not GDI/FPS evidence.\n";
}
