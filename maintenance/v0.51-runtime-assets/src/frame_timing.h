#pragma once
#include <algorithm>
#include <cmath>

namespace timing {
// Retain the fractional 100 Hz step, but discard long stalls instead of
// creating a backlog that delays the next input/render frame.
struct FixedSteps {
 static constexpr double step=.01;
 static constexpr int maxSteps=8;
 double remainder=0;
 double acceptedSeconds=0;
 int advance(double seconds,bool active) {
  acceptedSeconds=0;
  if(!active||!std::isfinite(seconds)||seconds<0){remainder=0;return 0;}
  acceptedSeconds=std::min(seconds,step*maxSteps);
  double total=remainder+acceptedSeconds;
  int count=std::min(maxSteps,static_cast<int>((total+1e-12)/step));
  remainder=std::max(0.0,total-count*step);
  return count;
 }
 double fraction()const{return std::clamp(remainder/step,0.0,1.0);}
 void reset(){remainder=0;acceptedSeconds=0;}
};
}
