#include "src/arrival_navigation.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void close(double a,double b,const char* why){require(std::abs(a-b)<1e-8,why);}
std::string encoded(const sim::Snapshot& s){std::ostringstream out;s.write(out);return out.str();}
int main(){try{
 int cases=0;
 for(int id=0;id<sim::aircraftCount;++id)for(int airport:{0,1}){
  sim::Snapshot s;s.flight.configure(id);s.career.mission=2;s.career.stage=1;s.career.arrivalRunway=airport;s.career.departureRunway=1-airport;s.career.licensed=true;
  const double dx[]={0,100,0,-100,300},dz[]={100,0,-100,0,400};
  const double bearings[]={0,sim::pi/2,sim::pi,-sim::pi/2,std::atan2(300.,400.)};
  for(int direction=0;direction<5;++direction){
   s.flight.x=sim::runways[airport].x-dx[direction];s.flight.z=sim::runways[airport].z-dz[direction];
   const auto before=encoded(s);const auto n=sim::arrivalNavigation(s);
   require(n.active&&n.valid&&n.airport==airport,"Active mission keeps selected destination");
   close(n.distance,direction==4?500:100,"Horizontal distance to airport centre");
   close(n.bearing,bearings[direction],"World bearing to destination");
   require(encoded(s)==before,"Query cannot change flight, career or autopilot");++cases;
  }
  s.flight.x=sim::runways[airport].x;s.flight.z=sim::runways[airport].z;
  close(sim::arrivalNavigation(s).distance,0,"At airport centre remains finite");
  const auto n=sim::arrivalNavigation(s);s.flight.yaw=sim::pi;s.avionics.qnh=950;
  close(sim::arrivalNavigation(s).bearing,n.bearing,"Map bearing is independent of aircraft yaw and QNH");
  std::stringstream save;s.write(save);sim::Snapshot loaded;require(loaded.read(save),"Existing save roundtrip");
  require(sim::arrivalNavigation(loaded).airport==airport,"Arrival target persists without format changes");
 }
 sim::Snapshot s;s.career.mission=2;s.career.stage=1;s.career.arrivalRunway=1;
 require(sim::nearestRunwayIndex(s.flight.x,s.flight.z)==0&&sim::arrivalNavigation(s).airport==1,"Nearby departure cannot replace arrival");
 for(int mission=0;mission<=4;++mission)for(int stage=0;stage<=3;++stage){
  s.career.mission=mission;s.career.stage=stage;
  require(sim::arrivalNavigation(s).active==(mission==2&&stage==1),"Only active passenger flight supplies arrival footer");
 }
 s.career.mission=2;s.career.stage=1;
 for(int invalid:{-1,2,std::numeric_limits<int>::max()}){s.career.arrivalRunway=invalid;const auto n=sim::arrivalNavigation(s);require(n.active&&!n.valid,"Bad destination must remain invalid, not select nearest");}
 s.career.arrivalRunway=1;
 for(double invalid:{double(NAN),double(INFINITY),-double(INFINITY)}){
  s.flight.x=invalid;s.flight.z=0;require(!sim::arrivalNavigation(s).valid,"Nonfinite X rejected");
  s.flight.x=0;s.flight.z=invalid;require(!sim::arrivalNavigation(s).valid,"Nonfinite Z rejected");
 }
 s.flight.x=s.flight.z=-std::numeric_limits<double>::max();require(!sim::arrivalNavigation(s).valid,"Overflowed distance rejected");
 std::cout<<"PASS "<<cases<<" aircraft/destination/bearing cases; lifecycle, save, read-only state, selection and invalid input. CPU geometry only.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
