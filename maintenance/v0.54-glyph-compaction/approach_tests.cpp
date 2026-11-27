#include "src/approach.h"
#include "src/run_files.h"
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void close(double a,double b,const char* message){require(std::abs(a-b)<1e-8,message);}
std::string saved(const sim::Snapshot& state){std::ostringstream out;state.write(out);return out.str();}

// Exact v0.33 geometry excerpt for deterministic regression, not Windows evidence.
sim::ApproachGuidance legacy(const sim::Flight& f){
 const auto& r=sim::nearestRunway(f.x,f.z);sim::ApproachGuidance g;g.runway=&r;
 g.distance=std::abs(f.z-r.z);g.localizer=f.x-r.x;
 g.desiredAltitude=std::tan(3*sim::pi/180)*std::max(0.0,g.distance-80)+f.spec().gearHeight;
 g.glideslope=f.y-g.desiredAltitude;
 g.valid=g.distance<=12000&&std::abs(g.localizer)<=2500&&f.y<=1500;
 return g;
}
sim::Flight inbound(int airport,int direction,double distance){
 sim::Flight f;f.configure(3);const auto& r=sim::runways[airport];
 f.x=r.x;f.z=r.z-direction*r.halfLength-direction*distance;
 f.yaw=direction==1?0:sim::pi;f.vz=direction*90.;
 f.y=f.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*distance;
 return f;
}
void legacyDefects(){
 auto f=inbound(0,1,0);const auto old=legacy(f),now=sim::approachGuidance(f,0);
 require(old.distance==1800&&now.distance==0,"Legacy measures from centre, new measures from threshold");
 require(old.desiredAltitude-now.desiredAltitude>74,"Legacy path at threshold differs by over 74m from chosen training clearance");
 std::cout<<"Legacy PVG threshold: centre-distance="<<old.distance<<"m, path delta="
          <<old.desiredAltitude-now.desiredAltitude<<"m relative to chosen training clearance.\n";
 f=inbound(0,-1,1000);f.x=120;
 require(legacy(f).localizer>0&&sim::approachGuidance(f,0).localizer<0,"Reciprocal approach fixes aircraft-right sign");
 f=inbound(0,1,1000);f.yaw=sim::pi;f.vz=-90;
 require(legacy(f).valid&&!sim::approachGuidance(f,0).valid,"Outbound false approach suppressed");
 f.z=0;require(legacy(f).valid&&!sim::approachGuidance(f,0).valid,"In-runway false glide cue suppressed");
}
void bothAirportsAndEnds(){
 int samples=0;
 for(int aircraft=0;aircraft<sim::aircraftCount;++aircraft)for(int airport=0;airport<2;++airport)
  for(int direction:{1,-1})for(double distance:{0.,10.,100.,1000.,5000.,12000.}){
   auto f=inbound(airport,direction,distance);f.aircraftId=aircraft;
   f.y=f.spec().gearHeight+sim::trainingThresholdClearance+std::tan(3*sim::pi/180)*distance;
   auto g=sim::approachGuidance(f,airport);
   require(g.valid&&g.runwayIndex==airport&&g.direction==direction,"Each aircraft and reciprocal runway end selectable");
   close(g.distance,distance,"Threshold distance exact");close(g.glideslope,0,"Chosen training glide path centred");
   close(g.desiredAltitude-f.spec().gearHeight-std::tan(3*sim::pi/180)*distance,
         sim::trainingThresholdClearance,"Threshold clearance independent of runway length and aircraft gear height");
   f.x+=direction*120;f.y+=40;g=sim::approachGuidance(f,airport);
   close(g.localizer,120,"Aircraft right of inbound course is positive in both directions");
   close(g.glideslope,40,"High is positive in both directions");
   f.x-=direction*240;f.y-=80;g=sim::approachGuidance(f,airport);
   // The selected path may extend below the flat ground at very short distance.
   if(f.y>=0){require(g.valid,"Low nonnegative-altitude sample valid");close(g.localizer,-120,"Aircraft left is negative");close(g.glideslope,-40,"Low is negative");}
   ++samples;
  }
 std::cout<<sim::aircraftCount<<" aircraft x2 airports x2 directions x6 distances: "<<samples<<" threshold, slope and signed-deviation samples passed.\n";
}
void selectionPowerAndPersistence(){
 sim::Snapshot state;state.flight=inbound(1,1,5000);
 require(sim::nearestRunwayIndex(state.flight.x,state.flight.z)==0,"Test geometry is nearer departure centre");
 state.career.mission=2;state.career.stage=1;state.career.arrivalRunway=1;state.career.licensed=true;
 const auto before=saved(state);auto g=sim::approachGuidance(state);
 require(g.valid&&g.runwayIndex==1,"Passenger approach stays on destination despite closer departure centre");
 for(int frame=0;frame<1000;++frame)require(sim::approachGuidance(state).runwayIndex==1,"Destination selection does not drift");
 require(saved(state)==before,"Guidance has no Snapshot mutation");
 state.avionics.master=false;require(!sim::approachGuidance(state).valid,"Unpowered avionics hide guidance");
 state.avionics.master=true;state.avionics.generator=false;state.avionics.battery=false;
 require(!sim::approachGuidance(state).valid,"No electric bus hides guidance");
 state.avionics.battery=true;state.avionics.charge=1;
 require(sim::approachGuidance(state).valid,"Battery restores guidance");
 const auto airborne=state.flight;state.flight.y=state.flight.spec().gearHeight;
 require(!sim::approachGuidance(state).valid,"Grounded cockpit does not show approach cues");state.flight=airborne;
 state.career.arrivalRunway=-1;require(!sim::approachGuidance(state).valid,"Invalid mission arrival does not silently select nearby runway");
 state.career.arrivalRunway=1;
 std::stringstream stream;state.write(stream);sim::Snapshot loaded;
 require(loaded.read(stream)&&sim::approachGuidance(loaded).runwayIndex==1,"Existing v16 state restores same arrival without new fields");
 state.career.mission=0;state.flight=inbound(0,1,1000);
 require(sim::approachGuidance(state).runwayIndex==0,"Non-passenger training auto-selects eligible runway end");
}
void groundTrackAcquisition(){
 for(int airport:{0,1})for(int direction:{1,-1})for(int crosswind:{-1,1}){
  auto f=inbound(airport,direction,2000);
  for(double angle:{30.,59.,61.,89.,91.}){
   const double radians=angle*sim::pi/180;
   f.vx=crosswind*90*std::sin(radians);
   f.vz=direction*90*std::cos(radians);
   require(sim::approachGuidance(f,airport).valid==(angle<60),
           "Ground track must be inside the inbound cone for both ends and crosswind signs");
  }
  f.vx=crosswind*4;f.vz=0;
  require(sim::approachGuidance(f,airport).valid,"Heading remains decisive below the track speed floor");
  f.vx=crosswind*6;
  require(!sim::approachGuidance(f,airport).valid,"Moving broadside above the speed floor hides guidance");
 }
 sim::Snapshot mission;
 mission.career.mission=2;mission.career.stage=1;mission.career.arrivalRunway=1;
 mission.flight=inbound(1,-1,3000);
 mission.flight.vx=90;mission.flight.vz=-2;
 require(!sim::approachGuidance(mission).valid,"Passenger cockpit shares the ground-track guard");
 mission.flight.vz=-90;
 require(sim::approachGuidance(mission).valid,"Valid destination approach reacquires immediately");
 std::cout<<"Both airports, reciprocal ends and crosswind signs: acquisition and reacquisition checks passed.\n";
}
void validityAndContinuity(){
 auto f=inbound(0,1,1000);
 f.yaw=sim::pi/2;require(!sim::approachGuidance(f,0).valid,"Crossing heading is not inbound");
 f.yaw=0;f.vz=-10;require(!sim::approachGuidance(f,0).valid,"Wrong-way ground track is not inbound");
 f.vz=90;f.x=2500.01;require(!sim::approachGuidance(f,0).valid,"Outside lateral envelope hidden");
 f.x=0;f.y=1500.01;require(!sim::approachGuidance(f,0).valid,"Outside altitude envelope hidden");
 f=inbound(0,1,12000.01);require(!sim::approachGuidance(f,0).valid,"Beyond range hidden");
 f=inbound(0,1,-.01);require(!sim::approachGuidance(f,0).valid,"Past threshold hidden, no flare/autoland cue");
 f=inbound(0,1,0);require(sim::approachGuidance(f,0).valid,"Exactly at threshold remains valid");
 f=inbound(0,1,1000);require(!sim::approachGuidance(f,-2).valid&&!sim::approachGuidance(f,2).valid,"Selected airport bounds guarded");
 f.crashed=true;require(!sim::approachGuidance(f,0).valid,"Crashed flight has no guidance");
 f=inbound(0,1,1000);f.x=std::numeric_limits<double>::quiet_NaN();require(!sim::approachGuidance(f).valid,"NaN position rejected");
 f=inbound(0,1,1000);f.yaw=std::numeric_limits<double>::infinity();require(!sim::approachGuidance(f).valid,"Infinite heading rejected");
 double last=0;int count=0;
 for(int step=0;step<=12000;++step){
  f=inbound(1,-1,double(step));const auto g=sim::approachGuidance(f,1);
  require(g.valid,"One-metre reciprocal-path sweep remains valid");
  if(step)close(g.desiredAltitude-last,std::tan(3*sim::pi/180),"No runway-centre cusp or path discontinuity");
  last=g.desiredAltitude;++count;
 }
 std::cout<<"12,001 one-metre path samples: continuous slope; selection/power/save and invalid envelopes passed.\n";
}
void displayDeflections(){
 sim::ApproachGuidance g;g.valid=true;g.distance=0;
 auto d=sim::approachDeflection(g);require(d.valid&&d.lateral==0&&d.vertical==0,"Centered threshold display");
 g.localizer=3;g.glideslope=1;d=sim::approachDeflection(g);
 close(d.lateral,-.1,"Right of course commands a left diamond");close(d.vertical,.1,"High aircraft commands a lower diamond");
 g.localizer=-3;g.glideslope=-1;d=sim::approachDeflection(g);
 close(d.lateral,.1,"Left of course commands a right diamond");close(d.vertical,-.1,"Low aircraft commands an upper diamond");
 for(double distance:{0.,100.,1000.,5000.,12000.})for(int sign:{-1,1}){
  g.distance=distance;g.localizer=sign*2500.;g.glideslope=sign*1000.;d=sim::approachDeflection(g);
  require(d.valid&&d.lateral==-sign&&d.vertical==sign,"Large deviations clamp to scale ends");
 }
 g.distance=1000;g.localizer=10;g.glideslope=5;d=sim::approachDeflection(g);
 close(d.lateral,-10/(1000*std::tan(2.5*sim::pi/180)),"Lateral range scales with threshold distance");
 close(d.vertical,5/(1000*std::tan(.7*sim::pi/180)),"Vertical range scales with threshold distance");
 for(int field=0;field<3;++field)for(double invalid:{double(NAN),double(INFINITY),-double(INFINITY)}){
  auto bad=g;(field==0?bad.distance:field==1?bad.localizer:bad.glideslope)=invalid;
  require(!sim::approachDeflection(bad).valid,"Nonfinite deviation cannot reach display geometry");
 }
 g.valid=false;require(!sim::approachDeflection(g).valid,"Inactive guidance has no scale");
 g.valid=true;g.distance=-1;require(!sim::approachDeflection(g).valid,"Negative threshold distance rejected");
 std::cout<<"PFD approach deflection signs, distance sensitivity, clamps and invalid inputs passed. Training geometry only.\n";
}
void touchdownRegression(){
 // Inside the taxi tolerance but still clear of the surface: do not score yet.
 sim::Flight low;
 low.configure(2);
 low.y+=.008;low.vy=-.1;
 require(low.grounded(),"Fixture must start inside the old grounded tolerance");
 require(!low.step(.01,{}),"Approaching the floor is not a touchdown");
 std::optional<double> impact;
 for(int i=0;i<20&&!impact;++i)impact=low.step(.01,{});
 require(impact&&*impact<-.1&&low.vy==0,"Report the integrated sink rate before ground clamping");
 for(int i=0;i<100;++i)require(!low.step(.01,{}),"No duplicate contact while parked");

 sim::Flight hard;
 hard.configure(2);hard.y+=.008;hard.vy=-6;
 const auto hardImpact=hard.step(.01,{});
 require(hardImpact&&hard.crashed&&!sim::assessLanding(hard,*hardImpact,hard.roll,hard.yaw).valid,
         "Crash and scoring use the same impact");

 auto practice=sim::pudongA330Final();
 auto& flight=practice.flight;
 int contacts=0;
 std::optional<double> previousRemaining;
 for(int i=0;i<15000&&!practice.career.completed;++i){
  sim::Controls input;
  if(contacts){flight.throttle=0;input.brake=true;}
  const auto hit=flight.step(.01,input);
  if(hit){
   ++contacts;
   const auto result=sim::assessLanding(flight,*hit,flight.roll,flight.yaw);
   require(result.valid&&result.sinkRate>0&&result.score>0,"A330 runway contact scored");
   practice.career.recordTouchdown(flight,result);
   std::cout<<"A330 model touchdown: t="<<(i+1)*.01<<" s, signed vy="<<*hit
            <<" m/s, runway z="<<flight.z<<" m, score="<<result.score<<"\n";
  }
  if(auto remaining=sim::remainingRunway(flight)){
   require(contacts>0&&(!previousRemaining||*remaining<=*previousRemaining),"Runway remaining decreases during rollout");
   if(!previousRemaining)std::cout<<"A330 rollout initial remaining: "<<*remaining<<" m\n";
   previousRemaining=remaining;
  }
  practice.career.update(flight,.01);
 }
 require(previousRemaining.has_value()&&!sim::remainingRunway(flight),"Stopped rollout hides distance");
 require(contacts==1&&!flight.crashed&&practice.career.parked(flight)&&practice.career.completed==1
         &&practice.career.arrivalLandingScore>0,"One contact, arrival score retained at stop");
}

void sessionCollision(const std::filesystem::path& root){
 constexpr std::time_t midnight=1704067200;
 const auto first=runfiles::createPracticeSession(root,midnight);
 std::ofstream(first/"startup.log")<<"first";
 const auto second=runfiles::createPracticeSession(root,midnight);
 const auto third=runfiles::createPracticeSession(root,midnight);
 std::string retained;
 std::ifstream(first/"startup.log")>>retained;
 require(first.filename()=="2024-01-01_00-00-00Z"&&second.filename()=="2024-01-01_00-00-00Z-1"
         &&third.filename()=="2024-01-01_00-00-00Z-2"&&retained=="first",
         "Colliding session times must keep the prior recording");
}
int main(int argc,char** argv){
 try{
  if(argc==3&&std::string(argv[1])=="--session-test"){
   sessionCollision(argv[2]);
   std::cout<<"Three sessions at the same UTC second retained independent output.\n";
   return 0;
  }
  for(int id=0;id<sim::aircraftCount;++id){
   for(double speed:{-65.,-2.,0.,2.,65.}){
    for(double dt:{.005,.01,.02}){
     auto coast=sim::pudongA330Rollout().flight;coast.configure(id);
     coast.z=0;coast.vz=speed;coast.yaw=speed<0?sim::pi:0.;
     auto braked=coast;
     sim::Controls input;input.brake=true;
     coast.step(dt,{});braked.step(dt,input);
     const double removed=std::abs(coast.vz)-std::abs(braked.vz);
     require(removed>=-1e-10&&removed<=3.5*dt+1e-9,"Extra wheel-brake impulse is bounded");
     require(coast.vz*braked.vz>=0,"Wheel braking must not reverse travel");
     if(coast.y>coast.spec().gearHeight)close(removed,0,"No wheel braking after liftoff");
     else if(std::abs(speed)==65.)close(removed,3.5*dt,"High-speed cap is active");
     if(std::abs(speed)==2.)close(braked.vz,coast.vz*std::exp((.025-coast.spec().brakeRate)*dt),"Low-speed response retained");
    }
   }
  }
  for(int releaseSteps:{0,200}){
   auto attempt=sim::pudongA330Rollout();
   auto& aircraft=attempt.flight;
   sim::RolloutReview review(aircraft);
   int events=0,brakingSteps=0;
   for(int step=0;step<12000&&!review.finished;++step){
    sim::Controls controls;controls.brake=step<100||step>=100+releaseSteps;
    aircraft.step(.01,controls);
    brakingSteps+=controls.brake;
    events+=review.observe(attempt,controls.brake,.01);
   }
   require(events==1&&attempt.career.parked(aircraft),"Exactly one rollout completion");
   close(review.brakeSeconds,brakingSteps*.01,"Count applied braking steps");
   close(aircraft.time-review.brakeSeconds,releaseSteps*.01,"Released interval excluded");
   close(review.forwardMetres(aircraft),aircraft.z+1400,"Distance from actual spawn");
   close(review.maxCenterlineError,0,"Straight rollout remains centered");
   require(!review.observe(attempt,true,.01),"Stop event cannot repeat");
   std::cout<<"Rollout review: release="<<releaseSteps*.01<<" s, total="<<aircraft.time<<" s, brakes="<<review.brakeSeconds<<" s, forward="<<review.forwardMetres(aircraft)<<" m\n";
   attempt=sim::pudongA330Rollout();review=sim::RolloutReview(attempt.flight);
   require(!review.finished&&review.brakeSeconds==0&&review.maxCenterlineError==0,"Restart clears prior result");
   aircraft.x=4;review.observe(attempt,false,.01);aircraft.x=-7;review.observe(attempt,false,.01);
   aircraft.x=0;review.observe(attempt,false,.01);close(review.maxCenterlineError,7,"Maximum absolute lateral deviation retained");
  }
  auto rollout=sim::pudongA330Rollout();
  auto& f=rollout.flight;
  require(f.grounded()&&rollout.avionics.powered(f)&&!rollout.parkingBrake&&f.throttle==0,"Powered ground start with brakes released");
  close(sim::remainingRunway(f).value(),3200,"Initial runway length");
  sim::Controls braking;braking.brake=true;
  for(int i=0;i<12000&&!rollout.career.parked(f);++i){
   require(!f.step(.01,braking)&&!f.crashed,"Rollout stays grounded without a new contact");
   rollout.career.update(f,.01);
  }
  require(rollout.career.parked(f)&&rollout.career.completed==0&&rollout.career.credits==0,"Stop without fictitious career payout");
  std::cout<<"Rollout brake test: time="<<f.time<<" s, distance="<<f.z+1400<<" m, speed="<<f.speed()<<" m/s\n";
  const runfiles::RunFiles rolloutFiles("profile",false,false,"",true,runfiles::Practice::Rollout);
  require(!rolloutFiles.gameSavesEnabled()&&rolloutFiles.folder()==std::filesystem::path("profile")/"practice"/"a330-rollout","Separate rollout records");
  touchdownRegression();
  auto training=sim::pudongA330Final();
  const auto guidance=sim::approachGuidance(training);
  require(training.flight.aircraftId==2&&guidance.valid&&guidance.runwayIndex==0,"A330 on selected PVG final");
  close(guidance.distance,6000,"Distance to threshold");
  close(guidance.localizer,0,"Centered localizer");close(guidance.glideslope,0,"Centered glidepath");
  require(training.flight.gear&&training.flight.flap==.5&&!training.parkingBrake&&training.flight.engines.totalThrust>0,"Landing configuration and engines");
  double maxPathError=0;
  for(int i=0;i<6000;++i){
   training.flight.step(.01,{});
   const auto current=sim::approachGuidance(training);
   require(current.valid&&!training.flight.stallWarning,"Practice guidance and stall margin during descent");
   maxPathError=std::max(maxPathError,std::abs(current.glideslope));
  }
  require(maxPathError<8,"Neutral controls stay near the training glidepath for sixty seconds");
  std::cout<<"A330 virtual 60s: maximum vertical path error "<<maxPathError<<" m; threshold distance "<<sim::approachGuidance(training).distance<<" m. CPU only.\n";
  require(!training.flight.crashed&&!training.flight.grounded()&&training.flight.z>sim::pudongA330Final().flight.z+4500,"Sixty seconds of live flight physics");
  const runfiles::RunFiles practice("profile",false,false,"",true,runfiles::Practice::Final);
  require(!practice.gameSavesEnabled()&&practice.folder()==std::filesystem::path("profile")/"practice"/"a330-final","Practice is isolated from career files");
  displayDeflections();legacyDefects();bothAirportsAndEnds();selectionPowerAndPersistence();groundTrackAcquisition();validityAndContinuity();
  std::cout<<"Training approach CPU geometry passed; not Windows, radio ILS, certified procedure or autoland validation.\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
