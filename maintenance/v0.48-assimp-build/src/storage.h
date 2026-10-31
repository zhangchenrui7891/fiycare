#pragma once
#include "career.h"
#include "avionics.h"
#include "systems_storage.h"
#include <iomanip>
#include <sstream>
namespace sim {
struct Snapshot {
 Progress career;Flight flight;double clock=12,seasonElapsed=0;int season=0,terrainMode=1;double exposureEV=0,haze=.00016;bool shadows=true,parkingBrake=false,antiAlias=true;
 double panelBrightness=.75;bool cockpitFlood=true;
 Avionics avionics;bool debugPanel=false,showForces=false;
 bool headingHold=false,altitudeHold=false,navigationHold=false;int navigationWaypoint=0;double targetHeading=0,targetAltitude=1000;
 void write(std::ostream& out)const{
  out<<std::setprecision(17)<<"FCAIR_STATE 16\n";
  career.save(out);
  out<<career.stage<<' '<<career.mission<<' '<<career.gate<<' '<<career.elapsed<<' '<<career.raceLap<<'\n';
  const auto& f=flight;
  out<<f.x<<' '<<f.y<<' '<<f.z<<' '<<f.vx<<' '<<f.vy<<' '<<f.vz<<' '<<f.pitch<<' '<<f.roll<<' '<<f.yaw<<' '<<f.throttle<<' '<<f.fuel<<' '<<f.rpm<<' '<<f.flap<<' '<<f.engine<<' '<<f.gear<<' '<<f.crashed<<' '<<f.assist<<' '<<f.aircraftId<<' '<<f.passengers<<'\n';
  out<<clock<<' '<<seasonElapsed<<' '<<season<<' '<<terrainMode<<' '<<exposureEV<<' '<<haze<<' '<<shadows<<' '<<parkingBrake<<' '<<antiAlias<<' '<<headingHold<<' '<<altitudeHold<<' '<<targetHeading<<' '<<targetAltitude<<' '<<navigationHold<<' '<<navigationWaypoint<<' '<<career.departureRunway<<' '<<career.arrivalRunway<<' '<<career.lastFlightScore<<'\n';
  out<<career.arrivalLandingScore<<'\n';
  out<<panelBrightness<<' '<<cockpitFlood<<'\n';
  out<<avionics.battery<<' '<<avionics.generator<<' '<<avionics.master<<' '<<avionics.charge<<' '<<avionics.qnh<<' '<<f.trim<<' '<<f.windX<<' '<<f.windZ<<' '<<f.pitchRate<<' '<<f.rollRate<<'\n';
  writeFlightSystems(out,f.surfaceModel,f.yawRate,f.engines,f.rotation,debugPanel,showForces);
 }
 bool read(std::istream& in){
  Snapshot p;std::string header;int version=0;
  if(!(in>>header>>version)||header!="FCAIR_STATE"||(version<3||version>16))return false;
  // Validate legacy record with the same constraints, then parse the active flight.
  std::string magic;int cv=0;auto& c=p.career;
  if(!(in>>magic>>cv>>c.credits>>c.completed>>c.licensed>>c.fleet>>c.photos>>c.best>>c.maintenance)||magic!="FCAIR"||cv!=2)return false;
  if(!(in>>c.stage>>c.mission>>c.gate>>c.elapsed>>c.raceLap))return false;
  auto& f=p.flight;
  if(!(in>>f.x>>f.y>>f.z>>f.vx>>f.vy>>f.vz>>f.pitch>>f.roll>>f.yaw>>f.throttle>>f.fuel>>f.rpm>>f.flap>>f.engine>>f.gear>>f.crashed>>f.assist))return false;
  if(version>=4&&!(in>>f.aircraftId>>f.passengers))return false;
  if(!(in>>p.clock>>p.seasonElapsed>>p.season))return false;
  if(version>=5&&!(in>>p.terrainMode))return false;
  if(version>=6&&!(in>>p.exposureEV>>p.haze>>p.shadows))return false;
  if(version>=7&&!(in>>p.parkingBrake))return false;
  if(version>=8&&!(in>>p.antiAlias))return false;
  if(version>=9&&!(in>>p.headingHold>>p.altitudeHold>>p.targetHeading>>p.targetAltitude))return false;
  if(version>=10&&!(in>>p.navigationHold>>p.navigationWaypoint))return false;
  if(version>=11&&!(in>>c.departureRunway>>c.arrivalRunway>>c.lastFlightScore))return false;
  if(version>=12&&!(in>>c.arrivalLandingScore))return false;
  if(version>=13&&!(in>>p.panelBrightness>>p.cockpitFlood))return false;
  auto& a=p.avionics;
  if(version>=14&&!(in>>a.battery>>a.generator>>a.master>>a.charge>>a.qnh>>f.trim>>f.windX>>f.windZ>>f.pitchRate>>f.rollRate))return false;
  if(version>=15&&!readFlightSystems(in,f.surfaceModel,f.yawRate,f.engines,f.rotation,p.debugPanel,p.showForces))return false;
  for(double n:{a.charge,a.qnh,f.trim,f.windX,f.windZ,f.pitchRate,f.rollRate})if(!std::isfinite(n))return false;
  if(a.charge<0||a.charge>1||a.qnh<950||a.qnh>1050||std::abs(f.trim)>.5||std::abs(f.windX)>30||std::abs(f.windZ)>30||std::abs(f.pitchRate)>(f.surfaceModel?10:2)||std::abs(f.rollRate)>(f.surfaceModel?10:2))return false;
  if(!std::isfinite(p.panelBrightness)||p.panelBrightness<0||p.panelBrightness>1)return false;
  if(c.arrivalLandingScore< -1||c.arrivalLandingScore>100)return false;
  if(!std::isfinite(p.exposureEV)||p.exposureEV< -2||p.exposureEV>2||!std::isfinite(p.haze)||p.haze<.00004||p.haze>.0006)return false;
  if(p.terrainMode<0||p.terrainMode>1)return false;
  for(double n:{c.best,c.maintenance,c.elapsed,c.raceLap,f.x,f.y,f.z,f.vx,f.vy,f.vz,f.pitch,f.roll,f.yaw,f.throttle,f.fuel,f.rpm,f.flap,p.clock,p.seasonElapsed,p.targetHeading,p.targetAltitude})if(!std::isfinite(n))return false;
  if(c.credits<0||c.credits>100000000||c.completed<0||c.completed>1000000||c.photos<0||c.photos>1000000||c.fleet<1||c.fleet>6||c.maintenance<.3||c.maintenance>1||c.best<0||c.stage<0||c.stage>3||c.mission<0||c.mission>4||c.gate<0||c.gate>4||c.elapsed<0||c.raceLap<0||c.departureRunway<0||c.departureRunway>1||c.arrivalRunway<0||c.arrivalRunway>1||c.departureRunway==c.arrivalRunway||c.lastFlightScore<0||c.lastFlightScore>100)return false;
  if((c.stage==1&&c.gate>=4)||(c.mission>0&&!c.licensed))return false;
  if(std::abs(f.x)>1000000||std::abs(f.z)>1000000||f.y<0||f.y>30000||f.speed()>500||std::abs(f.pitch)>(f.surfaceModel?pi/2+.001:.7)||std::abs(f.roll)>(f.surfaceModel?pi+.001:1.2)||std::abs(f.yaw)>pi+.01||f.throttle<0||f.throttle>1||f.fuel<0||f.fuel>100||f.rpm<0||f.rpm>2601||f.flap<0||f.flap>1)return false;
  if(f.aircraftId<0||f.aircraftId>=aircraftCount||f.passengers<0||f.passengers>aircraft(f.aircraftId).seats)return false;
  if(p.clock<0||p.clock>=24||p.seasonElapsed<0||p.seasonElapsed>300||p.season<0||p.season>3)return false;
  if(p.targetHeading< -pi-.01||p.targetHeading>pi+.01||p.targetAltitude<0||p.targetAltitude>12000||p.navigationWaypoint<0||p.navigationWaypoint>3)return false;
  if(f.crashed)f.crashReason="载入上次事故状态";
  f.condition=c.maintenance;f.payload=c.mission==1?250:(c.mission==2?150:0);
  *this=p;return true;
 }
};
}
