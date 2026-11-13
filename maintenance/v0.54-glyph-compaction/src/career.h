#pragma once
#include "simulation.h"
#include "flight_review.h"
#include <array>
#include <istream>
#include <ostream>
namespace sim {
struct Waypoint { double x,z; };
struct Progress {
 int credits=0,completed=0,stage=0,mission=0,gate=0,fleet=1,photos=0,departureRunway=0,arrivalRunway=1,lastFlightScore=0;
 bool licensed=false; double elapsed=0,best=0,raceLap=0,maintenance=1;
 int arrivalLandingScore=-1;
 double timePoints()const{return std::clamp(100.0-elapsed/18.0,35.0,100.0)*.3;}
 double fuelPoints(const Flight& f)const{return std::clamp(f.fuel,0.0,100.0)*.2;}
 double landingPoints()const{return std::clamp(arrivalLandingScore,0,100)*.5;}
 void writeFlightReport(std::ostream& out,const Flight& f,int income)const{
  if(mission!=2||stage!=3)return;
  out<<"FlightCareerAir v0.20 passenger flight report\n"
     <<"departure_runway="<<departureRunway<<"\narrival_runway="<<arrivalRunway
     <<"\nelapsed_seconds="<<elapsed<<"\nfuel_remaining_percent="<<f.fuel
     <<"\ntime_points_of_30="<<timePoints()<<"\nfuel_points_of_20="<<fuelPoints(f)
     <<"\ntouchdown_recorded="<<(arrivalLandingScore>=0?"yes":"no")
     <<"\nlanding_points_of_50="<<landingPoints()<<"\nrounded_total_of_100="<<lastFlightScore
     <<"\ncredited_income="<<income<<"\n---\n";
 }
 void recordTouchdown(const Flight& f,const LandingResult& result){
  if(mission==2&&stage==1&&arrivalRunway>=0&&arrivalRunway<2&&nearestRunwayIndex(f.x,f.z)==arrivalRunway)
   arrivalLandingScore=result.valid?std::clamp(result.score,0,100):-1;
 }
 std::array<Waypoint,4> route{{{0,1200},{900,1700},{1200,400},{0,-200}}};
 bool parked(const Flight& f)const{return !f.crashed&&f.grounded()&&f.speed()<1&&onRunway(f.x,f.z);}
 // Start a fresh attempt from the aircraft's current spawn. Retain career
 // history and mission type, but never reuse an old airport pair or score.
 void resetFlight(const Flight& f){
  stage=gate=0;elapsed=raceLap=0;lastFlightScore=0;arrivalLandingScore=-1;
  departureRunway=nearestRunwayIndex(f.x,f.z);arrivalRunway=departureRunway?0:1;
 }
 bool select(int m,const Flight& f){
  if(!parked(f)||m<0||m>4||(!licensed&&m!=0))return false;
  mission=m;resetFlight(f);
  return true;
 }
 const wchar_t* name()const{const wchar_t* n[]={L"执照训练",L"货物运输",L"旅客航班",L"计时竞速",L"摄影巡游"};return n[std::clamp(mission,0,4)];}
 void reward(int value){credits+=value;completed++;stage=3;}
 void update(const Flight& f,double dt){
  if(f.crashed||stage==3||mission<0||mission>4||gate<0||gate>4||arrivalRunway<0||arrivalRunway>=2)return;
  if(stage>0&&stage<2)elapsed+=std::max(0.0,dt);
  if(stage==0&&f.y>30)stage=1;
  if(mission==0){if(stage==1&&f.y>200)stage=2;}
  else if(mission==2&&stage==1){
   const auto& destination=runways[arrivalRunway];
   if(parked(f)&&nearestRunwayIndex(f.x,f.z)==arrivalRunway){
    // Missing touchdown evidence never grants a perfect landing score.
    lastFlightScore=int(std::lround(timePoints()+fuelPoints(f)+landingPoints()));
    reward(1800+f.passengers*30+lastFlightScore*8);licensed=true;
    maintenance=std::max(.3,maintenance-.03);credits+=(fleet-1)*120;
   }else if(std::hypot(f.x-destination.x,f.z-destination.z)<900&&f.y<350)gate=1;
  }
  else if(stage==1&&gate<4){
   auto p=route[gate]; double radius=mission==3?140:300;
   if(std::hypot(f.x-p.x,f.z-p.z)<radius&&f.y>80&&f.y<650){
    if(mission!=4&&++gate>=4){stage=2;raceLap=elapsed;}
   }
  }
  if(mission!=2&&stage==2&&parked(f)){
   int values[]={1500,2400,1800,2600,2000};reward(values[mission]+(mission==2&&f.aircraftId>0?f.passengers*30:0));licensed=true;
   maintenance=std::max(.3,maintenance-.03);
   credits+=(fleet-1)*120;
   if(mission==3&&(best==0||raceLap<best))best=raceLap;
  }
 }
 bool canPhotograph(const Flight& f)const{
  if(mission!=4||stage!=1||f.crashed||gate<0||gate>=4)return false;
  auto p=route[gate];return std::hypot(f.x-p.x,f.z-p.z)<=350&&f.y>=80&&f.y<=650;
 }
 bool photograph(const Flight& f){
  if(!canPhotograph(f))return false;
  photos++;if(++gate>=4)stage=2;return true;
 }
 bool buy(const Flight& f){if(!licensed||!parked(f)||credits<6000||fleet>=6)return false;credits-=6000;fleet++;return true;}
 bool service(Flight& f){if(!parked(f)||credits<300)return false;credits-=300;maintenance=1;f.fuel=100;f.engines.resetFaults();return true;}
 void load(std::istream& in){
  Progress p;std::string magic;int version;
  if(!(in>>magic>>version)||magic!="FCAIR"||version!=2)return;
  if(!(in>>p.credits>>p.completed>>p.licensed>>p.fleet>>p.photos>>p.best>>p.maintenance))return;
  if(p.credits<0||p.credits>100000000||p.completed<0||p.fleet<1||p.fleet>6||p.photos<0||!std::isfinite(p.best)||p.best<0||!std::isfinite(p.maintenance)||p.maintenance<.3||p.maintenance>1)return;
  *this=p;
 }
 void save(std::ostream& out)const{out<<"FCAIR 2\n"<<credits<<' '<<completed<<' '<<licensed<<' '<<fleet<<' '<<photos<<' '<<best<<' '<<maintenance<<'\n';}
};
}
