#pragma once
#include "storage.h"
#include <string>

namespace sim {
struct AutopilotModes {
 bool heading=false,altitude=false,navigation=false;
 bool any()const{return heading||altitude||navigation;}
};
inline AutopilotModes autopilotModes(const Snapshot& state){
 return {state.headingHold,state.altitudeHold,state.navigationHold};
}
inline AutopilotModes disconnectedAutopilotModes(AutopilotModes before,AutopilotModes after){
 return {before.heading&&!after.heading,before.altitude&&!after.altitude,
         before.navigation&&!after.navigation};
}
inline std::wstring autopilotDisconnectMessage(AutopilotModes lost){
 if(!lost.any())return {};
 std::wstring channels;
 auto add=[&](const wchar_t* name){if(!channels.empty())channels+=L"/";channels+=name;};
 if(lost.heading)add(L"航向");
 if(lost.altitude)add(L"高度");
 if(lost.navigation)add(L"导航");
 return L"自动驾驶通道断开："+channels+L"；请接管对应操纵。";
}
}
