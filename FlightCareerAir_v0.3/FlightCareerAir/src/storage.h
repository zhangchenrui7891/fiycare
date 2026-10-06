#pragma once
#include "career.h"
#include <iomanip>
#include <sstream>
namespace sim {
struct Snapshot {
 Progress career;Flight flight;double clock=12,seasonElapsed=0;int season=0;
 void write(std::ostream& out)const{
  out<<std::setprecision(17)<<"FCAIR_STATE 3\n";
  career.save(out);
  out<<career.stage<<' '<<career.mission<<' '<<career.gate<<' '<<career.elapsed<<' '<<career.raceLap<<'\n';
  const auto& f=flight;
  out<<f.x<<' '<<f.y<<' '<<f.z<<' '<<f.vx<<' '<<f.vy<<' '<<f.vz<<' '<<f.pitch<<' '<<f.roll<<' '<<f.yaw<<' '<<f.throttle<<' '<<f.fuel<<' '<<f.rpm<<' '<<f.flap<<' '<<f.engine<<' '<<f.gear<<' '<<f.crashed<<' '<<f.assist<<'\n';
  out<<clock<<' '<<seasonElapsed<<' '<<season<<'\n';
 }
 bool read(std::istream& in){
  Snapshot p;std::string header;int version=0;
  if(!(in>>header>>version)||header!="FCAIR_STATE"||version!=3)return false;
  // Validate legacy record with the same constraints, then parse the active flight.
  std::string magic;int cv=0;auto& c=p.career;
  if(!(in>>magic>>cv>>c.credits>>c.completed>>c.licensed>>c.fleet>>c.photos>>c.best>>c.maintenance)||magic!="FCAIR"||cv!=2)return false;
  if(!(in>>c.stage>>c.mission>>c.gate>>c.elapsed>>c.raceLap))return false;
  auto& f=p.flight;
  if(!(in>>f.x>>f.y>>f.z>>f.vx>>f.vy>>f.vz>>f.pitch>>f.roll>>f.yaw>>f.throttle>>f.fuel>>f.rpm>>f.flap>>f.engine>>f.gear>>f.crashed>>f.assist>>p.clock>>p.seasonElapsed>>p.season))return false;
  for(double n:{c.best,c.maintenance,c.elapsed,c.raceLap,f.x,f.y,f.z,f.vx,f.vy,f.vz,f.pitch,f.roll,f.yaw,f.throttle,f.fuel,f.rpm,f.flap,p.clock,p.seasonElapsed})if(!std::isfinite(n))return false;
  if(c.credits<0||c.credits>100000000||c.completed<0||c.completed>1000000||c.photos<0||c.photos>1000000||c.fleet<1||c.fleet>6||c.maintenance<.3||c.maintenance>1||c.best<0||c.stage<0||c.stage>3||c.mission<0||c.mission>4||c.gate<0||c.gate>4||c.elapsed<0||c.raceLap<0)return false;
  if((c.stage==1&&c.gate>=4)||(c.mission>0&&!c.licensed))return false;
  if(std::abs(f.x)>1000000||std::abs(f.z)>1000000||f.y<0||f.y>30000||f.speed()>500||std::abs(f.pitch)>.7||std::abs(f.roll)>1.2||std::abs(f.yaw)>pi+.01||f.throttle<0||f.throttle>1||f.fuel<0||f.fuel>100||f.rpm<0||f.rpm>2601||f.flap<0||f.flap>1)return false;
  if(p.clock<0||p.clock>=24||p.seasonElapsed<0||p.seasonElapsed>300||p.season<0||p.season>3)return false;
  if(f.crashed)f.crashReason="SAVED ACCIDENT";
  f.condition=c.maintenance;f.payload=c.mission==1?250:(c.mission==2?150:0);
  *this=p;return true;
 }
};
}
