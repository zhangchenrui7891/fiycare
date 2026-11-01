#pragma once
#include "simulation.h"
namespace sim {
inline const wchar_t* tutorialHint(const Flight& f,int stage){
 if(f.crashed)return L"事故：按R重新起飞，可在菜单查看操作说明";
 if(stage==3)return L"任务已完成：停稳后选择下一项任务";
 if(f.fuel<10)return L"燃油不足：请尽快着陆，在跑道停稳后按H补给";
 if(f.stalled)return L"失速警告：降低机头，增加推力";
 if(stage==0&&f.speed()<f.spec().rotationSpeed)return f.aircraftId>0?L"客机滑跑：满推力，保持中心线，约220公里/时开始抬头":L"教练机滑跑：按W加满油门，约110公里/时抬头";
 if(stage==0&&f.y<30)return L"起飞：短按↓抬头，保持约7～9度俯仰";
 if(stage==1)return L"爬升或前往航点：留意高度、方位和距离";
 if(stage==2&&f.y>80)return L"返场：对准跑道中心，放下起落架和襟翼";
 if(stage==2&&f.y>f.spec().gearHeight+1)return L"进近：下降率小于5米/秒，保持低坡度";
 if(stage==2)return L"接地刹车：按住空格，速度低于1米/秒完成任务";
 return L"Tab打开菜单 · F1操作说明 · P暂停 · F5保存";
}
}
