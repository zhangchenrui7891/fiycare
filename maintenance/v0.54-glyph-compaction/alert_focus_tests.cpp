#include "src/flight_alerts.h"
#include <iostream>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("Alert headline priority regression");}
int main(){
 sim::AlertManager alerts;check(!alerts.headline());
 // Use actual collection and acknowledgement transitions on every twin-engine type.
 int transitions=0;
 for(int id=1;id<sim::aircraftCount;++id){
  sim::Snapshot state;state.flight.configure(id);
  auto& engines=state.flight.engines;
  engines.units[0].fault=sim::EngineFault::Flameout;
  alerts.active.clear();alerts.update(state);
  check(alerts.headline()&&alerts.headline()->id=="engine1_fail");
  alerts.acknowledge();check(!alerts.attention());
  engines.units[1].fault=sim::EngineFault::Flameout;alerts.update(state);
  check(alerts.attention()&&alerts.headline()->id=="engine2_fail"&&!alerts.headline()->acknowledged);
  alerts.update(state);check(alerts.headline()->id=="engine2_fail");
  alerts.acknowledge();check(alerts.headline()->id=="engine1_fail"&&alerts.headline()->acknowledged);
  engines.units[1].fault=sim::EngineFault::None;alerts.update(state);
  engines.units[1].fault=sim::EngineFault::Flameout;alerts.update(state);
  check(alerts.headline()->id=="engine2_fail"&&!alerts.headline()->acknowledged);
  engines.units[0].fault=sim::EngineFault::None;alerts.update(state);
  check(alerts.headline()->id=="engine2_fail");
  transitions+=7;
 }
 // Exhaust severity/ack combinations, independent of list ordering.
 int combinations=0;
 for(int a=0;a<3;++a)for(int b=0;b<3;++b)for(bool aa:{false,true})for(bool ab:{false,true}){
  alerts.active={{"a",L"A",static_cast<sim::AlertLevel>(a),aa},{"b",L"B",static_cast<sim::AlertLevel>(b),ab}};
  const char* expected=b>a||(a==b&&aa&&!ab)?"b":"a";
  check(alerts.headline()->id==expected);
  check(alerts.active[0].id=="a"&&alerts.active[0].acknowledged==aa&&alerts.active[1].acknowledged==ab);
  ++combinations;
 }
 alerts.active={{"warn",L"W",sim::AlertLevel::Warning,true},{"caution",L"C",sim::AlertLevel::Caution,false}};
 check(alerts.headline()->id=="warn"&&alerts.attention());
 std::cout<<transitions<<" actual fault transitions and "<<combinations<<" severity/ack combinations passed; stable ordering and no mutation. CPU only.\n";
}
