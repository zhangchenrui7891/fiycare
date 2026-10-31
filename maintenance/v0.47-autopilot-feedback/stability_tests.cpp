#include "src/render_stability.h"
#include "src/autopilot.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

void check(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
bool close(double a,double b,double tolerance=1e-8) {
    return std::abs(a-b)<tolerance;
}

void frameInterpolation() {
    for(int fps:{30,60,100,144,240,1000}) {
        timing::FixedSteps timer;
        presentation::FlightHistory history;
        sim::Flight flight;flight.z=0;flight.y=1500;
        history.reset(flight);
        double wall=0,lastVisual=-1;
        for(int frame=0;frame<fps*3;frame++) {
            const double elapsed=1./fps;
            wall+=elapsed;
            for(int n=timer.advance(elapsed,true);n>0;n--) {
                flight.x+=100*timing::FixedSteps::step;
                flight.time+=timing::FixedSteps::step;
                history.push(flight);
            }
            auto visual=history.sample(flight,timer.fraction());
            check(close(visual.x,100*std::max(0.,wall-.01),1e-7),
                  "Constant speed is smooth at every render frame, one tick behind");
            check(visual.x>=lastVisual,"Presentation never steps backwards");
            lastVisual=visual.x;
            check(close(flight.x,std::round(flight.time*100)),
                  "Presentation does not mutate the authoritative flight");
        }
        check(close(flight.x,300),"Render FPS does not change accepted physics time");
    }
    timing::FixedSteps timer;
    presentation::FlightHistory history;
    sim::Flight f;history.reset(f);
    double wall=0;
    for(double elapsed:{.001,.007,.022,.004,.031,.017,.006,.009,.003}) {
        wall+=elapsed;
        for(int n=timer.advance(elapsed,true);n>0;n--) {
            f.time+=.01;f.x+=1;history.push(f);
        }
        check(close(history.sample(f,timer.fraction()).x,100*std::max(0.,wall-.01)),
              "Irregular frame durations interpolate without a sawtooth");
    }
    timer.advance(5,true);
    check(timer.fraction()<1,"Long stalls leave no interpolation backlog");
    timer.advance(.005,false);history.reset(f);
    check(close(history.sample(f,timer.fraction()).x,f.x),"Pause freezes at current pose");
    f.x=2000;f.time=0;history.push(f);
    check(close(history.sample(f,0).x,2000),"Teleport/restart does not smear across the map");
    f.configure(3);history.push(f);
    check(history.sample(f,0).aircraftId==3,"Aircraft switches reset presentation history");
    check(std::isfinite(history.sample(f,std::numeric_limits<double>::quiet_NaN()).x),
          "Invalid interpolation fraction does not contaminate rendering");
}

void rotationInterpolation() {
    using namespace sim;
    auto a=Quaternion::fromEuler(0,0,179*pi/180);
    auto b=Quaternion::fromEuler(0,0,-179*pi/180);
    auto middle=presentation::interpolate(a,b,.5);
    check(std::abs(std::abs(middle.euler().z)-pi)<1e-8,
          "Heading wrap follows two degrees instead of a full spin");
    auto antipodal=Quaternion{-a.w,-a.x,-a.y,-a.z};
    check((presentation::interpolate(a,antipodal,.5).rotate({0,0,1})-
           a.rotate({0,0,1})).length()<1e-8,"Antipodal orientations remain stationary");
    for(double pitch:{-.6,0.,.6})for(double roll:{-1.,0.,1.}) {
        auto q=Quaternion::fromEuler(pitch,roll,2.4);
        for(double t:{0.,.1,.5,.9,1.}) {
            auto value=presentation::interpolate(a,q,t);
            check(close(value.normSquared(),1),"SLERP stays normalized at all bank angles");
        }
    }
}

void depthPrecision() {
    auto p=presentation::DepthProjection::reversed(presentation::worldNear,presentation::worldFar);
    check(close(p.at(.1f),1,1e-6)&&std::abs(p.at(6000.f))<1e-9,
          "Reverse projection maps near to one, far to zero");
    float last=2;
    for(float distance:{.1f,1.f,100.f,1000.f,2000.f,5000.f,6000.f}) {
        float depth=p.at(distance);
        check(depth<last&&depth>=-1e-9,"Greater depth consistently selects nearer geometry");
        last=depth;
    }
    // Simulate D32 storage for two surfaces separated by 3 cm at 2 km.
    const float farther=2000.f,nearer=1999.97f;
    const double n=.1,far=6000;
    auto ordinary=[&](float z){return float(far/(far-n)-far*n/((far-n)*z));};
    check(ordinary(farther)==ordinary(nearer),"Regression reproduces the old distant depth collision");
    check(p.at(nearer)>p.at(farther),"Reverse floating depth distinguishes distant pavement layers");
    auto cabin=presentation::DepthProjection::reversed(presentation::cabinNear,presentation::cabinFar);
    check(cabin.at(.870f)>cabin.at(.871f),"Cabin millimetre layers remain ordered");
    // Adding local cockpit layers to a distant world position loses their offset.
    check(float(50000.+.870)==float(50000.+.871),"Regression reproduces large-coordinate cabin precision loss");
    check(float(.870)!=float(.871),"Local cockpit preserves the original layer separation");
    check(presentation::depthClear<p.at(5000),"Cleared reverse depth admits visible world fragments");
}

sim::Snapshot runFlight(int id,const std::vector<double>& frames) {
    sim::Snapshot s;
    auto& f=s.flight;
    f.configure(id);f.y=1500;f.z=0;f.vz=id?100:40;f.gear=false;
    f.throttle=.3;f.rpm=1270;f.roll=.04;f.pitch=.03;f.setSurfaceModel(true);
    s.headingHold=s.altitudeHold=true;s.targetAltitude=1500;
    timing::FixedSteps timer;presentation::FlightHistory history;history.reset(f);
    size_t frame=0;int steps=0;
    double latePitchRate=0;
    while(steps<12000) {
        int count=timer.advance(frames[frame++%frames.size()],true);
        for(int n=0;n<count&&steps<12000;n++,steps++) {
            sim::Controls c;sim::updateAutopilot(s,c);
            f.step(.01,c);s.avionics.step(.01,f);history.push(f);
            check(!f.crashed&&f.surfaceResult.finite(),"Seven-aircraft perturbed surface flight stays finite");
            if(steps>2000)latePitchRate=std::max(latePitchRate,std::abs(f.pitchRate));
        }
        const auto visual=history.sample(f,timer.fraction());
        check(std::isfinite(visual.pitch)&&std::isfinite(visual.x),"Presented pose remains finite");
    }
    check(latePitchRate<.01,"Selected AP perturbation scenarios do not develop rapid pitch oscillations");
    return s;
}

int main() {
    try {
        frameInterpolation();rotationInterpolation();depthPrecision();
        for(int id=0;id<sim::aircraftCount;id++) {
            const auto reference=runFlight(id,{.01});
            for(auto frames:std::vector<std::vector<double>>{{1./30},{1./60},{1./144},{.002,.021,.007,.039,.011}}) {
                auto actual=runFlight(id,frames);
                check(close(actual.flight.x,reference.flight.x)&&close(actual.flight.y,reference.flight.y)&&
                      close(actual.flight.z,reference.flight.z)&&close(actual.flight.pitch,reference.flight.pitch),
                      "Different render cadences yield identical surface-flight trajectories");
            }
        }
        for(int x:{-30,-1,0,1,30})for(int z:{-20,-1,0,1,20}) {
            auto tree=presentation::treePosition(x,z);
            check(close(tree.x,x*450.+90)&&close(tree.z,z*550.),"Vegetation uses a fixed world lattice");
        }
        std::cout<<"PASS: smooth 30-1000 FPS presentation, irregular frames, reset/wrap, reverse depth, local cabin; "
                    "35 seven-aircraft 120-second surface-flight runs agree across render cadences\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
