#include "src/asset_import.h"
#include "src/fleet_assets.h"
#include "src/model_transform.h"
#include "src/storage.h"
#include "src/ui_state.h"
#include "src/vertex_capacity.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

void assetRequire(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}

struct TemporaryAssets {
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("fcair_import_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TemporaryAssets(){std::filesystem::create_directories(path);}
    ~TemporaryAssets(){std::error_code error;std::filesystem::remove_all(path,error);}
};

void importFixtures(const std::filesystem::path& folder){
    const auto obj=folder/"triangle.obj";
    {std::ofstream out(obj);out<<"v -1 0 -1\nv 1 0 -1\nv 0 0 1\nf 1 3 2\n";}
    AircraftImportOptions options;options.lengthMeters=4;options.groundOffset=1;
    auto result=importAircraftModel(obj,options);
    assetRequire(result.model.vertices.size()==3&&result.report.importedTriangles==1,"OBJ parser and triangulation work");
    saveAircraftModel(result.model,folder/"triangle.fcmesh");
    auto loaded=loadModel(folder/"triangle.fcmesh");
    assetRequire(loaded.vertices.size()==3&&loaded.vertices[0].textured==0,"Color mesh v7 roundtrip");
    const auto turned=rotateAircraftQuarterTurn(result.model,4,1);assetRequire(turned.vertices.size()==3,"Model orientation editor preserves triangles");
    options.lengthMeters=0;bool rejected=false;
    try{importAircraftModel(obj,options);}catch(const std::exception&){rejected=true;}
    assetRequire(rejected,"Invalid physical scale rejected");
    const auto bad=folder/"broken.glb";{std::ofstream out(bad);out<<"glTF broken";}
    rejected=false;options.lengthMeters=4;
    try{importAircraftModel(bad,options);}catch(const std::exception&){rejected=true;}
    assetRequire(rejected,"Truncated GLB rejected by importer");
    {std::ofstream out(folder/"bad.fcmesh",std::ios::binary);out<<"FCMESH07";}
    rejected=false;try{loadModel(folder/"bad.fcmesh");}catch(const std::exception&){rejected=true;}
    assetRequire(rejected,"Truncated native mesh rejected");
    const auto formats=aircraftImporterFormats();
    for(const auto* extension:{"*.glb","*.obj","*.fbx","*.stl","*.dae"})
        assetRequire(formats.find(extension)!=std::string::npos,"Required import formats actually compiled");
}

void newFleet(const std::filesystem::path& assets,const std::filesystem::path& temporary){
    assetRequire(sim::aircraftCount==11,"Eleven registered aircraft");
    assetRequire(ui::aircraftIdForCommand(ui::A320)==4&&ui::aircraftIdForCommand(ui::B737)==5&&ui::aircraftIdForCommand(ui::B787)==6,"New menu commands select the right profiles");
    for(int id=4;id<10;id++){
        const auto* profile=sim::fleetAsset(id);assetRequire(profile!=nullptr,"Registered mesh profile exists");
        AircraftImportOptions options;options.lengthMeters=profile->lengthMeters;options.groundOffset=profile->bodyGroundOffset;options.yawDegrees=profile->yawDegrees;
        const auto source=assets/"source"/profile->sourceName;
        const auto result=importAircraftModel(source,options);
        assetRequire(result.report.decodedTextures==1&&result.report.importedTriangles>1000,"Downloaded mesh geometry and embedded texture really imported");
        float low=1e10f,high=-1e10f,minColor=1,maxColor=0;
        for(const auto& v:result.model.vertices){
            low=std::min(low,v.z);high=std::max(high,v.z);
            minColor=std::min({minColor,v.r,v.g,v.b});maxColor=std::max({maxColor,v.r,v.g,v.b});
            assetRequire(v.textured==0&&std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.01,"Native vertex colors and normalized normals");
        }
        assetRequire(std::abs((high-low)-profile->lengthMeters)<.01,"Imported plane length in metres");
        float maxY=-1e10f;double tailZ=0;for(const auto& vertex:result.model.vertices)if(vertex.y>maxY){maxY=vertex.y;tailZ=vertex.z;}
        assetRequire(tailZ<-profile->lengthMeters*.2,"Vertical tail stays behind CG; imported nose points +Z");
        assetRequire(maxColor-minColor>.1f,"Texture bake preserves material variation");
        const auto packed=temporary/(std::to_string(id)+".fcmesh");saveAircraftModel(result.model,packed);
        const auto loaded=loadModel(packed);const auto shipped=loadModel(assets/profile->nativeName);
        assetRequire(loaded.vertices.size()==result.model.vertices.size()&&shipped.vertices.size()==loaded.vertices.size(),"Shipped converted models match source triangle counts");
        sim::Snapshot state;assetRequire(ui::switchAircraft(state,id),"New aircraft selectable while parked");
        state.flight.passengers=state.flight.spec().seats;
        std::stringstream saved;state.write(saved);sim::Snapshot restored;
        assetRequire(restored.read(saved)&&restored.flight.aircraftId==id&&restored.flight.passengers==state.flight.spec().seats,"New aircraft id and capacity survive v16 save");
        for(bool surface:{false,true}){
            sim::Flight flight;flight.configure(id);flight.passengers=flight.spec().seats;flight.flap=.5;flight.setSurfaceModel(surface);
            double maxAltitude=flight.y;
            for(int step=0;step<9000;step++){
                sim::Controls control;control.throttle=flight.throttle<1?1:0;
                const double target=flight.speed()<flight.spec().rotationSpeed?0:.12;
                control.pitch=std::clamp((target-flight.pitch)*6-flight.pitchRate*.9,-1.,1.);
                flight.step(.01,control);maxAltitude=std::max(maxAltitude,flight.y);
                assetRequire(!flight.crashed&&std::isfinite(flight.y),"New aircraft full-load takeoff stable in both models");
            }
            assetRequire(maxAltitude>200,"New full-load aircraft climbs through 200 metres");
            std::cout<<"aircraft="<<id<<" surface="<<surface<<" full_load_max_altitude="<<maxAltitude<<'\n';
        }
    }
}

void bufferGrowth(){
    assetRequire(growVertexCapacity(320000,319999,48)==320000,"Small scenes retain existing allocation");
    assetRequire(growVertexCapacity(320000,600000,48)>=600000,"Large model vertex buffer grows");
    assetRequire(growVertexCapacity(0,1,48)>=4096,"Empty buffer starts with bounded capacity");
    bool rejected=false;try{growVertexCapacity(0,std::numeric_limits<uint32_t>::max(),48);}catch(const std::exception&){rejected=true;}
    assetRequire(rejected,"DirectX byte range overflow rejected");
}

int main(int argc,char** argv){
    try{
        if(argc!=2)throw std::runtime_error("Expected assets directory");
        TemporaryAssets temporary;importFixtures(temporary.path);bufferGrowth();newFleet(argv[1],temporary.path);
        std::cout<<"PASS: compiled multi-format importer, three downloaded aircraft, texture baking, native assets, seven-profile saves and full-load flights\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
