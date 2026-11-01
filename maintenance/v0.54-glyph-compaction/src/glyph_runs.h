#pragma once
#include <map>
#include <utility>
#include <vector>
struct PixelRun{float x,y,w,h;};
// Input is the row-major, non-overlapping GDI threshold mask. Merge only
// equal-width spans touching vertically, preserving the exact covered area.
// Called once per cached glyph, never by the per-frame draw path.
inline void compactGlyphRuns(std::vector<PixelRun>& runs){
 std::map<std::pair<float,float>,std::size_t> last;
 std::size_t written=0;
 for(std::size_t i=0;i<runs.size();++i){
  const PixelRun run=runs[i];
  const auto key=std::make_pair(run.x,run.w);
  const auto found=last.find(key);
  if(found!=last.end()){
   auto& previous=runs[found->second];
   if(previous.y+previous.h==run.y){previous.h+=run.h;continue;}
  }
  runs[written]=run;last[key]=written++;
 }
 runs.resize(written);
}
