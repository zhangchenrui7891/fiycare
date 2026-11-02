#pragma once
#include "storage.h"
namespace ui {
enum Command {
 Continue=100,ToggleMenu,Save,Load,Restart,Exit,Trainer,Airliner,PassengersLess,PassengersMore,PassengersFull,PassengersEmpty,
 Missions,Settings,Home,PowerSaver,Assist,CameraChase,CameraCockpit,CameraTop,Help,AircraftPage,A330,A350,TerrainSatellite,TerrainMountains,LightingPage,ExposureLess,ExposureMore,LightReset,HazeToggle,ShadowsToggle,Noon,Sunset,AntiAlias,Fullscreen,SuperSample,A320,B737,B787,ImportModel,ResetModel,RotateModel,
 A319=170,A321,B7879,Account,
 License=200,Cargo,Passenger,Race,Photo
};
enum class Page{None,Home,Pause,Aircraft,Missions,Settings,Lighting};
inline int aircraftIdForCommand(int command){
 switch(command){case Trainer:return 0;case Airliner:return 1;case A330:return 2;case A350:return 3;case A320:return 4;case B737:return 5;case B787:return 6;case A319:return 7;case A321:return 8;case B7879:return 9;default:return -1;}
}
inline bool switchAircraft(sim::Snapshot& state,int id){
 if(id<0||id>=sim::aircraftCount||!state.career.parked(state.flight))return false;
 state.parkingBrake=false;state.flight=sim::Flight{};state.flight.configure(id);
 state.career.resetFlight(state.flight);
 return true;
}
inline bool setPassengers(sim::Snapshot& state,int count){
 if(state.flight.aircraftId==0||!state.career.parked(state.flight))return false;
 state.flight.passengers=std::clamp(count,0,state.flight.spec().seats);return true;
}
inline void restart(sim::Snapshot& state){
 const int aircraft=state.flight.aircraftId,pax=state.flight.passengers;const double fuel=state.flight.fuel;
 if(state.flight.crashed){state.career.credits=std::max(0,state.career.credits-500);state.career.maintenance=std::max(.3,state.career.maintenance-.10);}
 state.parkingBrake=false;state.flight=sim::Flight{};state.flight.configure(aircraft);state.flight.passengers=pax;state.flight.fuel=fuel;
 state.career.resetFlight(state.flight);
}
}
