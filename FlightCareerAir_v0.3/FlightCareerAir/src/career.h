#pragma once
#include "simulation.h"
#include <array>
#include <istream>
#include <ostream>
namespace sim {
struct Waypoint { double x,z; };
struct Progress {
 int credits=0,completed=0,stage=0,mission=0,gate=0,fleet=1,photos=0;
 bool licensed=false; double elapsed=0,best=0,raceLap=0,maintenance=1;
 std::array<Waypoint,4> route{{{0,1200},{900,1700},{1200,400},{0,-200}}};
 bool parked(const Flight& f)const{return !f.crashed&&f.y<=1.61&&f.speed()<1&&std::abs(f.x)<22&&std::abs(f.z)<700;}
 bool select(int m,const Flight& f){if(!parked(f)||m<0||m>4||(!licensed&&m!=0))return false;mission=m;stage=gate=0;elapsed=raceLap=0;return true;}
 const char* name()const{const char* n[]={"LICENSE","CARGO","SIGHTSEEING","RACE","PHOTO TOUR"};return n[std::clamp(mission,0,4)];}
 void reward(int value){credits+=value;completed++;stage=3;}
 void update(const Flight& f,double dt){
  if(f.crashed||stage==3||mission<0||mission>4||gate<0||gate>4)return;
  if(stage>0&&stage<2)elapsed+=std::max(0.0,dt);
  if(stage==0&&f.y>30)stage=1;
  if(mission==0){if(stage==1&&f.y>200)stage=2;}
  else if(stage==1&&gate<4){
   auto p=route[gate]; double radius=mission==3?140:300;
   if(std::hypot(f.x-p.x,f.z-p.z)<radius&&f.y>80&&f.y<650){
    if(mission!=4&&++gate>=4){stage=2;raceLap=elapsed;}
   }
  }
  if(stage==2&&parked(f)){
   int values[]={1500,2400,1800,2600,2000};reward(values[mission]);licensed=true;
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
 bool service(Flight& f){if(!parked(f)||credits<300)return false;credits-=300;maintenance=1;f.fuel=100;return true;}
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
