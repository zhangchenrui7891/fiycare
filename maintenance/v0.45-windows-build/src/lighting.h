#pragma once
#include <array>
#include <algorithm>
#include <cmath>
using LightVec=std::array<float,4>;
struct LightingFrame {
 LightVec eye{},sun{},sunColor{},zenith{},horizon{},options{},right{},up{},forward{},plane{},shape{};
};
inline LightingFrame makeLighting(double hour,double exposureEV,double haze,bool shadows,double panelBrightness=.75,bool cockpitFlood=true){
 constexpr double pi=3.14159265358979323846;double a=(hour-6)*pi/12;
 float sy=(float)std::sin(a),sx=(float)std::cos(a),day=std::clamp((sy+.08f)/.3f,0.f,1.f),warm=1-std::clamp(sy*3,0.f,1.f);
 LightingFrame f;f.sun={sx*.8f,sy,sx*.6f,0};
 f.sunColor={1.7f*day,1.7f*day*(1-.35f*warm),1.7f*day*(1-.65f*warm),day};
 f.zenith={.004f+.035f*day,.008f+.16f*day,.025f+.38f*day,0};
 f.horizon={.015f+day*(.42f+.15f*warm),.02f+day*(.53f-.12f*warm),.04f+day*(.62f-.33f*warm),0};
 // Reserved W channels: sun.w = display dimmer, horizon.w = cabin flood.
 f.sun[3]=std::isfinite(panelBrightness)?float(std::clamp(panelBrightness,0.,1.)):.75f;
 f.horizon[3]=cockpitFlood?1.f:0.f;
 f.options={(float)std::exp2(std::clamp(exposureEV,-2.0,2.0)),(float)std::clamp(haze,.00004,.0006),0,shadows?1.f:0.f};return f;
}
inline LightingFrame lightingFrame=makeLighting(12,0,.00016,true);
