#pragma once
#include "storage.h"
#include <iterator>
namespace sim {
struct ArrivalNavigation {
 bool active=false,valid=false;
 int airport=-1;
 double distance=0,bearing=0;
};
// Read-only map information to the selected illustrative airport centre.
// Independent of training waypoint/AP guidance and approach-threshold geometry.
inline ArrivalNavigation arrivalNavigation(const Snapshot& state){
 ArrivalNavigation n;n.active=state.career.mission==2&&state.career.stage==1;
 if(!n.active)return n;
 n.airport=state.career.arrivalRunway;
 if(n.airport<0||n.airport>=int(std::size(runways))||
    !std::isfinite(state.flight.x)||!std::isfinite(state.flight.z))return n;
 const auto& r=runways[n.airport];
 const double dx=r.x-state.flight.x,dz=r.z-state.flight.z;
 n.distance=std::hypot(dx,dz);n.bearing=std::atan2(dx,dz);
 n.valid=std::isfinite(n.distance)&&std::isfinite(n.bearing);
 return n;
}
}
