#pragma once
#include <algorithm>
#include <cmath>
namespace mapdisplay {
struct Segment {double ax,ay,bx,by;};
// Liang-Barsky clipping. Transactional on rejection, no allocation.
inline bool clip(Segment& s,double left,double top,double right,double bottom){
 for(double v:{s.ax,s.ay,s.bx,s.by,left,top,right,bottom})
  if(!std::isfinite(v))return false;
 if(left>right||top>bottom)return false;
 const double dx=s.bx-s.ax,dy=s.by-s.ay;
 if(!std::isfinite(dx)||!std::isfinite(dy))return false;
 double lo=0,hi=1;
 auto edge=[&](double p,double q){
  if(p==0)return q>=0;
  const double t=q/p;
  if(p<0)lo=std::max(lo,t);else hi=std::min(hi,t);
  return lo<=hi;
 };
 if(!edge(-dx,s.ax-left)||!edge(dx,right-s.ax)||
    !edge(-dy,s.ay-top)||!edge(dy,bottom-s.ay))return false;
 Segment result{s.ax+lo*dx,s.ay+lo*dy,s.ax+hi*dx,s.ay+hi*dy};
 // Bound tiny round-off at viewport edges.
 result.ax=std::clamp(result.ax,left,right);result.bx=std::clamp(result.bx,left,right);
 result.ay=std::clamp(result.ay,top,bottom);result.by=std::clamp(result.by,top,bottom);
 s=result;return true;
}
}
