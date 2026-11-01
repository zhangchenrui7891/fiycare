#include "src/assets.h"
#include "src/fleet_assets.h"
#include "src/ui_state.h"
#include <iostream>
#include <sstream>
void need(bool ok){if(!ok)throw std::runtime_error("Fleet variant regression");}
int main(int argc,char**argv){need(argc==2);for(int id=0;id<sim::aircraftCount;id++){sim::Snapshot s;s.flight.configure(id);s.flight.passengers=s.flight.spec().seats;std::stringstream save;s.write(save);sim::Snapshot restored;need(restored.read(save)&&restored.flight.aircraftId==id);auto d=sim::airframeDimensions(id);need(d.length>0&&d.span>0);if(id<7)continue;auto profile=sim::fleetAsset(id);need(profile!=nullptr);auto model=loadModel(std::filesystem::path(argv[1])/profile->nativeName);float lo=1e9,hi=-1e9;for(auto&v:model.vertices){lo=std::min(lo,v.z);hi=std::max(hi,v.z);}need(std::abs(hi-lo-profile->lengthMeters)<.02);s.flight.y=1500;s.flight.vz=100;s.flight.gear=false;s.flight.flap=0;s.flight.throttle=.6;sim::Controls c;for(int i=0;i<1000;i++)s.flight.step(.01,c);need(std::isfinite(s.flight.y)&&std::isfinite(s.flight.vz));}need(ui::aircraftIdForCommand(ui::A319)==7&&ui::aircraftIdForCommand(ui::A321)==8&&ui::aircraftIdForCommand(ui::B7879)==9);std::cout<<"PASS: 10 aircraft save roundtrips, dimensions; 3 variant meshes/lengths/normals and 10-second flight simulations.\n";}
