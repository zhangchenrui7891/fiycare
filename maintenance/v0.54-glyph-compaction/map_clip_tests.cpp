#include "src/map_clip.h"
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("Map clipping regression");}
bool close(double a,double b){return std::abs(a-b)<1e-8;}
int main(){
 using mapdisplay::Segment;using mapdisplay::clip;
 Segment crossing{-10,5,20,5};check(clip(crossing,0,0,10,10));
 check(close(crossing.ax,0)&&close(crossing.bx,10)&&close(crossing.ay,5));
 Segment vertical{5,-10,5,20};check(clip(vertical,0,0,10,10)&&close(vertical.ay,0)&&close(vertical.by,10));
 Segment miss{-5,-5,-2,20};check(!clip(miss,0,0,10,10)&&miss.ax==-5);
 Segment point{5,5,5,5};check(clip(point,0,0,10,10));
 Segment bad{0,0,std::numeric_limits<double>::infinity(),1};check(!clip(bad,0,0,10,10));
 Segment huge{-std::numeric_limits<double>::max(),0,std::numeric_limits<double>::max(),0};check(!clip(huge,0,0,10,10));
 int count=0;
 for(int ax=-15;ax<=15;ax+=3)for(int ay=-15;ay<=15;ay+=3)
 for(int bx=-15;bx<=15;bx+=3)for(int by=-15;by<=15;by+=3){
  Segment s{double(ax),double(ay),double(bx),double(by)},reverse{s.bx,s.by,s.ax,s.ay};
  bool visible=clip(s,-5,-5,5,5);check(visible==clip(reverse,-5,-5,5,5));
  if(visible){
   check(close(s.ax,reverse.bx)&&close(s.ay,reverse.by)&&close(s.bx,reverse.ax)&&close(s.by,reverse.ay));
   check(s.ax>=-5&&s.ax<=5&&s.ay>=-5&&s.ay<=5&&s.bx>=-5&&s.bx<=5&&s.by>=-5&&s.by<=5);
   check(close((s.ax-ax)*(by-ay),(s.ay-ay)*(bx-ax)));
   Segment twice=s;check(clip(twice,-5,-5,5,5)&&close(twice.ax,s.ax)&&close(twice.by,s.by));
  }
  // Every sampled original point inside the rectangle must survive clipping.
  for(int i=0;i<=20;++i){
   double x=ax+(bx-ax)*i/20.,y=ay+(by-ay)*i/20.;
   if(x>=-5&&x<=5&&y>=-5&&y<=5)check(visible);
  }
  ++count;
 }
 std::cout<<count<<" segment grid cases: reversal, bounds, collinearity, idempotence, sampled coverage; crossing/parallel/point/invalid cases passed. CPU only.\n";
}
