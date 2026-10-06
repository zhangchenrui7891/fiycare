#pragma once
#include "simulation.h"
namespace sim {
inline const char* tutorialHint(const Flight& f,int stage){
 if(f.crashed)return "CRASH - R RESTART | F1 HELP";
 if(stage==3)return "MISSION COMPLETE - SELECT 1-5 WHILE PARKED";
 if(f.fuel<10)return "LOW FUEL - LAND AND PRESS H TO REFUEL";
 if(f.stalled)return "STALL - LOWER NOSE AND ADD POWER";
 if(stage==0&&f.speed()<29)return "TAKEOFF - HOLD W FOR FULL POWER, KEEP RUNWAY CENTER";
 if(stage==0&&f.y<10)return "ROTATE - SHORT DOWN ARROW INPUT, AIM FOR 7 DEG PITCH";
 if(stage==1)return "CLIMB / TARGET - KEEP BANK BELOW 30 DEG";
 if(stage==2&&f.y>80)return "RETURN - RUNWAY X 0, Z -700 TO 700. GEAR DOWN, F FLAP";
 if(stage==2&&f.y>2)return "LAND - THROTTLE LOW, DESCENT UNDER 5 M/S";
 if(stage==2)return "BRAKE - HOLD SPACE UNTIL BELOW 1 M/S";
 return "F1 HELP | C CAMERA | P PAUSE | F5 SAVE";
}
}
