#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace timing {
// CPU wall-clock frame intervals, including Present/wait; never GPU timings.
struct FrameStats {double fps=0,p95Ms=0,maxMs=0;std::size_t samples=0;};
class FrameWindow {
 std::array<double,300> frames{};
 std::size_t count=0,next=0;
public:
 void reset(){count=next=0;}
 void add(double seconds){
  if(!std::isfinite(seconds)||seconds<=0)return;
  frames[next]=seconds;next=(next+1)%frames.size();count=std::min(count+1,frames.size());
 }
 FrameStats summary()const{
  FrameStats s;s.samples=count;if(!count)return s;
  auto selected=frames;double total=0,maximum=0;
  for(std::size_t i=0;i<count;++i){total+=frames[i];maximum=std::max(maximum,frames[i]);}
  // HUD needs one nearest-rank percentile, not a fully sorted window.
  // Work on the copy: ring order and future expiration must stay unchanged.
  const auto rank=(95*count+99)/100-1;
  std::nth_element(selected.begin(),selected.begin()+rank,selected.begin()+count);
  s.fps=count/total;s.p95Ms=selected[rank]*1000;s.maxMs=maximum*1000;return s;
 }
};
}
