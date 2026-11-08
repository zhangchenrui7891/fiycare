#include "src/cockpit_controls.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::string saved(const sim::Snapshot& s){std::ostringstream out;s.write(out);return out.str();}
void applyAll(sim::Snapshot& s,const std::vector<cockpit::Event>& events){for(const auto& e:events)cockpit::apply(s,e.action,e.value);}
struct Screen{float x,y;};
Screen project(cockpit::Point p){
 const float sy=std::sin(cockpit::lookYaw),cy=std::cos(cockpit::lookYaw),sp=std::sin(cockpit::lookPitch),cp=std::cos(cockpit::lookPitch);
 const float right=p.x*cy-p.z*sy,up=-p.x*sy*sp+p.y*cp-p.z*cy*sp,forward=p.x*sy*cp+p.y*sp+p.z*cy*cp;
 return {480*(1+right/forward/(.57735027f*cockpit::viewportAspect)),270*(1-up/forward/.57735027f)};
}
void dragBurst(){
 cockpit::SpatialInput input;input.enabled=true;input.throttle=.5;
 const auto point=project(cockpit::solids(.5)[5].center);require(input.press(point.x,point.y)&&input.dragging,"Actual pick starts throttle drag");
 // Exact old move algorithm: clamp every delta, append every absolute value.
 std::vector<cockpit::Event> legacy{{cockpit::Action::Throttle,.5}};
 double oldThrottle=.5;float oldY=point.y;
 for(int i=0;i<100000;++i){
  const float y=point.y+float((i%1600)-800)*.5f;
  oldThrottle=std::clamp(oldThrottle+(oldY-y)/160.,0.,1.);oldY=y;legacy.push_back({cockpit::Action::Throttle,oldThrottle});
  input.move(y);require(input.pending.size()==1,"One uninterrupted motion burst keeps one pending absolute position");
  require(input.throttle==oldThrottle,"Every clamped/reversed drag delta is still evaluated in order");
 }
 input.release();require(!input.dragging&&input.pending.size()==1,"Release preserves final position for next frame");
 const auto count=input.pending.size();input.move(point.y);require(input.pending.size()==count,"Release stops later motion");
 sim::Snapshot oldState,newState;applyAll(oldState,legacy);applyAll(newState,input.pending);
 require(saved(oldState)==saved(newState),"100001 old throttle applications and one new application produce identical Snapshot");
 std::cout<<"CPU burst: "<<legacy.size()<<" legacy pending events -> "<<input.pending.size()
          <<"; absolute position and final Snapshot identical. Not Windows/RAM/FPS measurement.\n";
 input.cancel();require(!input.enabled&&!input.dragging&&input.pending.empty(),"Cancel disables and clears all pending input");
 require(!input.press(point.x,point.y),"Disabled input rejects new press");
}
void barriers(){
 using A=cockpit::Action;cockpit::Input q;
 q.enqueue(A::Throttle,.1);q.enqueue(A::Throttle,.2);q.enqueue(A::Gear);
 q.enqueue(A::Throttle,.3);q.enqueue(A::Throttle,.4);q.enqueue(A::Gear);q.enqueue(A::Throttle,.5);
 require(q.pending.size()==5&&q.pending[0].value==.2&&q.pending[1].action==A::Gear&&q.pending[2].value==.4&&
         q.pending[3].action==A::Gear&&q.pending[4].value==.5,"Discrete switches stay ordered between throttle runs");
 sim::Snapshot s;applyAll(s,q.pending);require(s.flight.gear&&s.flight.throttle==.5,"Two gear toggles not collapsed");
 q.pending.clear();for(int i=0;i<10000;++i)q.enqueue(A::Gear);
 require(q.pending.size()==10000,"Discrete queue is deliberately not capped or coalesced");
 q.pending.clear();for(double value:{.2,double(NAN),.8,double(INFINITY),.6})q.enqueue(A::Throttle,value);
 require(q.pending.size()==5,"Nonfinite values form barriers rather than erasing earlier valid positions");
 sim::Snapshot off;off.avionics.master=false;off.headingHold=off.altitudeHold=off.navigationHold=true;
 auto original=off;const std::vector<cockpit::Event> legacy={{A::Throttle,.2},{A::Throttle,NAN},{A::Throttle,.8},{A::Throttle,INFINITY},{A::Throttle,.6}};
 applyAll(off,q.pending);applyAll(original,legacy);require(saved(off)==saved(original),"Nonfinite ignored-value and power-disconnect semantics preserved");
 q.pending.clear();q.enqueue(A::Throttle,.1);applyAll(s,q.pending);q.pending.clear();q.enqueue(A::Throttle,.9);
 require(s.flight.throttle==.1&&q.pending.size()==1&&q.pending.back().value==.9,"Consumed frame is not rewritten by a later frame");
}
void mixedEquivalence(){
 using A=cockpit::Action;unsigned rng=0x39ab12u;int batches=0;
 for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(bool surface:{false,true})for(bool powered:{false,true}){
  sim::Snapshot oldState;oldState.flight.configure(aircraft);oldState.flight.setSurfaceModel(surface);
  oldState.avionics.master=powered;oldState.headingHold=oldState.altitudeHold=oldState.navigationHold=true;auto newState=oldState;
  cockpit::Input q;std::vector<cockpit::Event> old;
  for(int i=0;i<3000;++i){
   rng=rng*1664525u+1013904223u;
   const A action=(i%4==0)?static_cast<A>((rng>>16)%(static_cast<unsigned>(A::ResetFaults)+1)):A::Throttle;
   double value=double(rng%1601)/1000-.3;if(i%113==0)value=NAN;if(i%127==0)value=INFINITY;
   old.push_back({action,value});q.enqueue(action,value);
   if(i%37==36||i==2999){
    applyAll(oldState,old);applyAll(newState,q.pending);
    require(saved(oldState)==saved(newState),"Mixed commands/power/faults/absolute values preserve final Snapshot per consumption batch");
    old.clear();q.pending.clear();++batches;
   }
  }
 }
 std::cout<<sim::aircraftCount*12000<<" deterministic mixed events in "<<batches<<" batches: "<<sim::aircraftCount<<" profiles x2 models x2 power states, final Snapshot equivalence passed.\n";
}
void pointerOwnership(){
 for(bool leftReleasedFirst:{false,true}){
  cockpit::SpatialInput input;input.enabled=true;input.throttle=.5;
  const auto point=project(cockpit::solids(.5)[5].center);
  require(input.press(point.x,point.y)&&input.dragging,"Left drag acquires throttle");
  input.move(point.y-16);const double selected=input.throttle;
  require(!input.beginLook()&&!input.looking&&input.dragging,"Right press cannot steal active throttle drag");
  if(leftReleasedFirst){input.release();input.endLook();}
  else{input.endLook();require(input.dragging,"Ignored right release cannot terminate throttle drag");input.release();}
  require(!input.dragging&&!input.looking,"Both release orders leave pointer idle");
  sim::Snapshot s;applyAll(s,input.pending);
  require(std::abs(s.flight.throttle-selected)<1e-12,"Release keeps final pending throttle value");
  input.pending.clear();require(input.beginLook(),"Idle pointer accepts look");
  require(!input.press(point.x,point.y)&&input.pending.empty(),"Left press during look cannot queue throttle");
  const auto gear=project(cockpit::solids(selected)[1].center);
  require(!input.press(gear.x,gear.y)&&input.pending.empty(),"Left press during look cannot toggle a switch");
  input.release();require(input.looking,"Left release cannot end right-button look");
  input.endLook();require(!input.looking,"Right release ends look");
  require(input.press(point.x,point.y),"New left press works after look ends");
  input.loseCapture();require(!input.dragging&&!input.looking&&!input.pending.empty(),"Capture loss ends gestures but preserves completed input");
  require(input.beginLook(),"Look works after capture loss");input.cancel();
  require(!input.looking&&!input.dragging&&input.pending.empty()&&!input.beginLook(),"Focus/menu cancellation clears gestures and pending input");
 }
 std::cout<<"Pointer ownership: both release orders, switch suppression, capture loss, cancellation and recovery passed. CPU event-state tests only.\n";
}
int main(){try{pointerOwnership();dragBurst();barriers();mixedEquivalence();
 std::cout<<"Cockpit input coalescing CPU checks passed; no real Windows mouse/capture/rendering verification.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
