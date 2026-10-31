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
  auto text=instruments::primary(100,1500,.4,-.4*pi/180);
  require(text.speedAltitude==L"IAS 360 km/h ALT 1500 m","Metric units and speed conversion");
  require(text.headingVertical==L"HDG 000° VS +0.4 m/s","Heading and fractional VS units");
  text=instruments::primary(0,-25,-2.3,pi);
  require(text.speedAltitude==L"IAS 0 km/h ALT -25 m"&&text.headingVertical==L"HDG 180° VS -2.3 m/s","Ground/negative barometric altitude/descent");
  require(instruments::heading(std::numeric_limits<double>::max()).size()==3,"Large finite heading is bounded before rounding");
  std::cout<<"PASS 40001 heading-range cases plus cardinal/north-boundary, unit conversion, fractional VS, invalid and signed-zero cases. Text/CPU only.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
