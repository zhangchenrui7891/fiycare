#include "model_restore.h"
#include "fleet_assets.h"
#include <iostream>
size_t bytes(const ImportedModel& m){return m.vertices.capacity()*sizeof(Vertex)+m.gear.capacity()+m.uniqueVertices.capacity()*sizeof(Vertex)+m.indices.capacity()*sizeof(uint32_t);}
int main(int argc,char**argv){if(argc!=2)return 1;std::filesystem::path assets=argv[1];size_t eager=0;int count=0;
for(int id=0;id<sim::aircraftCount;++id){if(const auto* p=sim::fleetAsset(id)){auto m=loadModel(assets/p->nativeName);eager+=bytes(m);++count;}}
std::cout<<"Stock assets, no custom models. CPU vector capacity bytes only; excludes textures, allocators, renderer, GPU and process working set.\nold_eager_meshes="<<count<<" old_eager_vector_capacity_bytes="<<eager<<'\n';
for(int id:{0,3,10}){std::map<int,ImportedModel> models;std::set<int> ready,custom;const auto* p=sim::fleetAsset(id);ensureAircraftModel(models,ready,custom,id,p?assets/p->nativeName:std::filesystem::path{},assets/"absent-custom.fcmesh");size_t active=0;for(const auto& pair:models)active+=bytes(pair.second);std::cout<<"initial_profile="<<id<<" resident_meshes="<<models.size()<<" vector_capacity_bytes="<<active<<" avoided_capacity_bytes="<<eager-active<<'\n';}
}
