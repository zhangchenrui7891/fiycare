#pragma once
#include <algorithm>
namespace sim {
inline constexpr int aircraftCount=11;
struct AircraftSpec {
 const wchar_t* name;double emptyMass,wingArea,maxThrust,baseDrag,flapLift,gearHeight,rotationSpeed,controlScale,brakeRate,spoolRate,fuelRate;
 int seats;double spawnZ;
};
inline const AircraftSpec& aircraft(int id){
 static const AircraftSpec specs[]={
  {L"轻型教练机",1050,16.2,2800,.027,.60,1.6,29,1.0,1.4,2.0,.008,3,-500},
  {L"双发支线客机 · 70座",24000,78,110000,.026,.90,3.8,58,.55,.5,.65,.025,70,-1600},
  {L"空客 A330-300 · 300座",120000,361.6,620000,.025,.90,4.8,68,.38,.48,.55,.04,300,-1600},
  {L"空客 A350-900 · 330座",140000,443,750000,.024,.90,7.5,70,.36,.48,.55,.04,330,-1600},
  {L"空客 A320 · 180座",42600,122.6,240000,.026,.90,4.0,65,.50,.50,.70,.028,180,-1600},
  {L"波音 B737 · 180座",41400,124.6,242000,.027,.90,3.5,65,.50,.50,.70,.028,180,-1600},
  {L"波音 B787 · 290座",120000,377,640000,.024,.90,5.3,69,.39,.48,.58,.038,290,-1600},
  {L"空客 A319 · 132座（近似）",40800,122.6,220000,.026,.90,4.0,62,.52,.52,.72,.026,132,-1600},
  {L"空客 A321 · 220座（近似）",48500,122.6,290000,.027,.92,4.0,68,.47,.50,.68,.031,220,-1600},
  {L"波音 B787-9 · 296座（近似）",128850,377,660000,.024,.90,5.3,70,.38,.48,.58,.039,296,-1600},
  {L"波音 777-300ER",167800,436.8,1026000,.026,.90,5.5,75,.34,.46,.52,.05,365,-1600}
 };
 static_assert(sizeof(specs)/sizeof(specs[0])==aircraftCount,"Aircraft table size");
 return specs[std::clamp(id,0,aircraftCount-1)];
}
}
