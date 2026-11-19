#include "src/flight_instruments.h"
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 try{
  constexpr double pi=3.14159265358979323846;
  require(instruments::heading(0)==L"000","North");
  require(instruments::heading(pi/2)==L"090"&&instruments::heading(-pi/2)==L"270","East/west");
  require(instruments::heading(pi)==L"180"&&instruments::heading(-pi)==L"180","South");
  require(instruments::heading(359.6*pi/180)==L"000"&&instruments::heading(-.4*pi/180)==L"000","North rounding must not show 360");
  require(instruments::heading(359.4*pi/180)==L"359"&&instruments::heading(-.6*pi/180)==L"359","Near-north directions");
  for(int i=-20000;i<=20000;++i){
   const auto text=instruments::heading(i*pi/997);
   require(text.size()==3,"Heading fixed width");
   for(wchar_t c:text)require(c>=L'0'&&c<=L'9',"Heading digits");
   const int value=std::stoi(text);require(value>=0&&value<360,"Heading range for multiple revolutions");
  }
  for(double invalid:{NAN,INFINITY,-INFINITY}){
   require(instruments::heading(invalid)==L"---","Invalid heading placeholder");
   require(instruments::whole(invalid)==L"---"&&instruments::verticalSpeed(invalid)==L"---","Invalid instrument placeholders");
  }
  require(instruments::verticalSpeed(.4)==L"+0.4"&&instruments::verticalSpeed(-.4)==L"-0.4","Small climb/descent must remain visible");
  require(instruments::verticalSpeed(-.049)==L"0.0"&&instruments::verticalSpeed(.049)==L"0.0","No negative or positive signed zero");
  require(instruments::verticalSpeed(-0.)==L"0.0","Signed zero");
  require(instruments::whole(12.5)==L"13"&&instruments::whole(-12.5)==L"-13","Whole readings preserve nearest-away rounding");
  const double east[]={0,10,0,-10};
  const double north[]={10,0,-10,0};
  const wchar_t* tracks[]={L"000",L"090",L"180",L"270"};
  for(int i=0;i<4;++i){
   const auto ground=instruments::groundMotion(east[i],north[i]);
   require(ground.track==tracks[i]&&ground.speed==L"36","Ground-track cardinals and km/h conversion");
  }
  const auto diagonal=instruments::groundMotion(30,40);
  require(diagonal.track==L"037"&&diagonal.speed==L"180","Crosswind ground motion uses both velocity components");
  require(instruments::groundMotion(0,0).track==L"---"&&instruments::groundMotion(0,0).speed==L"0","Parked track hidden but zero ground speed visible");
  require(instruments::groundMotion(4.99,0).track==L"---"&&instruments::groundMotion(5,0).track==L"090","Low-speed track boundary");
  for(double invalid:{double(NAN),double(INFINITY),-double(INFINITY),std::numeric_limits<double>::max()}){
   const auto ground=instruments::groundMotion(invalid,invalid);
   require(ground.track==L"---"&&ground.speed==L"---","Invalid or overflowed ground motion hidden");
  }
  require(instruments::aboveGround(1234,1000)==L"234","AGL subtracts terrain from geometric altitude");
  require(instruments::aboveGround(750,0)==L"750"&&instruments::aboveGround(750.001,0).empty(),"AGL range uses unrounded height with inclusive 750m boundary");
  require(instruments::aboveGround(0,0)==L"0"&&instruments::aboveGround(1.6,0)==L"2","AGL is aircraft reference height, not wheel clearance");
  require(instruments::aboveGround(249.49,126)==L"123"&&instruments::aboveGround(249.5,126)==L"124","AGL uses whole-metre rounding");
  require(instruments::aboveGround(0,1)==L"---","Below-terrain reference is invalid rather than false zero");
  for(double invalid:{double(NAN),double(INFINITY),-double(INFINITY)}){
   require(instruments::aboveGround(invalid,0)==L"---"&&instruments::aboveGround(0,invalid)==L"---","Invalid geometric or terrain input uses AGL placeholder");
  }
  require(instruments::aboveGround(std::numeric_limits<double>::max(),-std::numeric_limits<double>::max())==L"---","AGL subtraction overflow rejected");
  auto text=instruments::primary(100,1500,.4,-.4*pi/180);
  require(text.speedAltitude==L"IAS 360 km/h ALT 1500 m","Metric units and speed conversion");
  require(text.headingVertical==L"HDG 000° VS +0.4 m/s","Heading and fractional VS units");
  text=instruments::primary(0,-25,-2.3,pi);
  require(text.speedAltitude==L"IAS 0 km/h ALT -25 m"&&text.headingVertical==L"HDG 180° VS -2.3 m/s","Ground/negative barometric altitude/descent");
  require(instruments::heading(std::numeric_limits<double>::max()).size()==3,"Large finite heading is bounded before rounding");
  std::cout<<"PASS 40001 heading-range cases plus cardinal/north-boundary, unit conversion, fractional VS, invalid and signed-zero cases. AGL terrain/range/rounding/invalid cases passed. Text/CPU only.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
