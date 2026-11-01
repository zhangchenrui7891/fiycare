#pragma once
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace instruments {
inline std::wstring heading(double radians){
 if(!std::isfinite(radians))return L"---";
 constexpr double turn=6.28318530717958647692;
 const double degrees=std::fmod(radians,turn)*360/turn;
 const int rounded=(static_cast<int>(std::lround(degrees))+360)%360;
 std::wstring text(3,L'0');
 text[0]=wchar_t(L'0'+rounded/100);text[1]=wchar_t(L'0'+rounded/10%10);text[2]=wchar_t(L'0'+rounded%10);
 return text;
}
inline std::wstring whole(double value){
 if(!std::isfinite(value))return L"---";
 if(std::abs(value)<.5)value=0; // Avoid a displayed negative zero.
 std::wostringstream out;out.imbue(std::locale::classic());
 out<<std::fixed<<std::setprecision(0)<<std::round(value);return out.str();
}
inline std::wstring verticalSpeed(double metresPerSecond){
 if(!std::isfinite(metresPerSecond))return L"---";
 if(std::abs(metresPerSecond)<.05)metresPerSecond=0;
 std::wostringstream out;out.imbue(std::locale::classic());
 if(metresPerSecond>0)out<<L'+';
 out<<std::fixed<<std::setprecision(1)<<metresPerSecond;return out.str();
}
struct PrimaryText {std::wstring speedAltitude,headingVertical;};
inline PrimaryText primary(double iasMetresPerSecond,double pressureAltitudeMetres,
                           double verticalMetresPerSecond,double headingRadians){
 return {L"IAS "+whole(iasMetresPerSecond*3.6)+L" km/h ALT "+whole(pressureAltitudeMetres)+L" m",
         L"HDG "+heading(headingRadians)+L"° VS "+verticalSpeed(verticalMetresPerSecond)+L" m/s"};
}
}
