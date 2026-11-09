#include "src/performance.h"
#include "src/frame_timing.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

// Frozen v0.42 full-sort oracle, including physical ring summation order.
struct Reference {
 std::array<double,300> frames{};
 std::size_t count=0,next=0;
 void reset(){count=next=0;}
 void add(double dt){
  if(!std::isfinite(dt)||dt<=0)return;
  frames[next]=dt;next=(next+1)%frames.size();count=std::min(count+1,frames.size());
 }
 timing::FrameStats summary()const{
  timing::FrameStats s;s.samples=count;if(!count)return s;
  auto sorted=frames;double total=0;
  for(std::size_t i=0;i<count;++i)total+=frames[i];
  std::sort(sorted.begin(),sorted.begin()+count);
  s.fps=count/total;s.p95Ms=sorted[(95*count+99)/100-1]*1000;s.maxMs=sorted[count-1]*1000;return s;
 }
};
std::size_t checks=0;
void compare(const timing::FrameWindow& actual,const Reference& expected){
 const auto a=actual.summary(),b=expected.summary();
 if(a.samples!=b.samples||a.fps!=b.fps||a.p95Ms!=b.p95Ms||a.maxMs!=b.maxMs)
  throw std::runtime_error("Rolling statistics differ from full-sort oracle");
 ++checks;
}
void sequence(const std::vector<double>& inputs){
 timing::FrameWindow actual;Reference expected;
 compare(actual,expected);
 for(std::size_t i=0;i<inputs.size();++i){
  actual.add(inputs[i]);expected.add(inputs[i]);
  compare(actual,expected);compare(actual,expected); // summary must not mutate the ring
  if(i%619==618){actual.reset();expected.reset();compare(actual,expected);}
 }
 actual.reset();expected.reset();compare(actual,expected);
}
int main(){
 try{
  for(bool saver:{false,true}){
   double elapsed=0;
   for(int i=0;i<1000;++i){
    const double work=(i%2)?0.005:0.12;
    const double delay=timing::frameDelay(work,saver,false);
    if(delay<0||std::abs(work+delay-std::max(.1,work))>1e-12)
     throw std::runtime_error("Background pacing must subtract work and never add backlog");
    elapsed+=work+delay;
   }
   if(std::abs(elapsed-110)>1e-8)throw std::runtime_error("Background frame budget differs across power settings");
   if(timing::frameDelay(.01,saver,true)!=(saver?.04:0.))throw std::runtime_error("Focus recovery must immediately restore foreground policy");
  }
  for(double bad:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
   if(timing::frameDelay(bad,false,false)!=0)throw std::runtime_error("Invalid elapsed time cannot produce a sleep request");
  std::cout<<"Background pacing: 2000 simulated budgets, foreground recovery, overruns and invalid times passed; no wall-clock/FPS/power measurement.\n";
  std::uint32_t rng=48271;
  for(int pattern=0;pattern<7;++pattern){
   std::vector<double> inputs;
   for(int i=0;i<6000;++i){
    rng=rng*1664525u+1013904223u;
    double dt=.016;
    switch(pattern){
     case 0:dt=(i%601+1)*.0001;break;
     case 1:dt=(601-i%601)*.0001;break;
     case 2:dt=(rng%100000+1)*.000001;break;
     case 3:dt=(rng%4+1)*.01;break;
     case 4:dt=i%299==0?5:.02;break;
     case 5:dt=(i%2)?1e100:1e-100;break;
     default:break;
    }
    inputs.push_back(dt);
   }
   sequence(inputs);
  }
  sequence({NAN,INFINITY,-INFINITY,0.,-0.,-.01,.01,NAN,.02,
            std::numeric_limits<double>::min(),std::numeric_limits<double>::max()});
  // No resets here: verify a spike expires after exactly 300 replacements.
  timing::FrameWindow actual;Reference expected;
  actual.add(5);expected.add(5);
  for(int i=0;i<900;++i){compare(actual,expected);actual.add(.02);expected.add(.02);}
  compare(actual,expected);
  if(actual.summary().maxMs!=20)throw std::runtime_error("Old spike did not expire");
  std::cout<<"PASS "<<checks<<" exact oracle comparisons: all partial window sizes, repeated queries, ring wraps, resets, duplicates, long frames, rejected samples. CPU only.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
