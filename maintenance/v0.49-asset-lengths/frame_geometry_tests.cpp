#include "src/assets.h"
#include "src/frame_geometry.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool ok,const char* reason) {
    if(!ok)throw std::runtime_error(reason);
}
template<class Action> void rejects(Action action,const char* reason) {
    bool caught=false;
    try {action();} catch(const std::exception&) {caught=true;}
    require(caught,reason);
}

void boundsAndEmptyRanges() {
    using Batch=presentation::VertexBatch<Vertex>;
    using Span=presentation::VertexSpan<Vertex>;
    Vertex marker{};
    const std::vector<Vertex> none;
    Batch empty({},none,none,none);
    empty.copyTo(nullptr,0);
    require(empty.bytes()==0&&empty.layout.total==0,"Empty batch is harmless");
    Batch sky(Span(&marker,1),none,none,none);
    std::vector<unsigned char> guard(sizeof(Vertex)+32,0xA5);
    sky.copyTo(guard.data()+16,1);
    require(std::all_of(guard.begin(),guard.begin()+16,[](auto b){return b==0xA5;})&&
            std::all_of(guard.end()-16,guard.end(),[](auto b){return b==0xA5;}),
            "Upload preserves guard bytes");
    auto before=guard;
    rejects([&]{sky.copyTo(guard.data()+16,0);},"Short destinations rejected");
    require(guard==before,"Invalid destination is rejected before any write");
    rejects([&]{sky.copyTo(nullptr,1);},"Null destination rejected");
    rejects([&]{Batch invalid(Span(nullptr,1),none,none,none);},"Null source rejected");
    const auto maximum=std::numeric_limits<std::uint32_t>::max()/sizeof(Vertex);
    Batch largest(Span(&marker,maximum),none,none,none); // validate layout only
    require(largest.bytes()<=std::numeric_limits<std::uint32_t>::max(),"Largest batch is representable");
    rejects([&]{Batch invalid(Span(&marker,maximum),Span(&marker,1),none,none);},
            "Aggregated GPU byte limit checked before copy");
    rejects([&]{Batch invalid(Span(&marker,std::numeric_limits<std::size_t>::max()),none,none,none);},
            "Host size overflow rejected");
}

void realFleetFrames(const std::filesystem::path& assets) {
    const Vertex sky[6]{};
    const std::vector<Vertex> terrain(28000,Vertex{1,2,3,.2f,.3f,.4f});
    presentation::FrameGeometry<Vertex> frame;
    for(const auto* filename:{"a330.fcmesh","a350.fcmesh","a320.fcmesh","b737.fcmesh","b787.fcmesh"}) {
        const auto model=loadModel(assets/filename);
        frame.beginFrame(terrain);
        frame.scene.insert(frame.scene.end(),model.vertices.begin(),model.vertices.end());
        frame.cabin.assign(24000,Vertex{4,5,6,.7f,.8f,.9f});
        frame.hud.assign(6000,Vertex{7,8,9,1,1,1});
        presentation::VertexBatch<Vertex> batch(sky,frame.scene,frame.cabin,frame.hud);
        // v0.29's staging order, retained as an independent byte-level oracle.
        std::vector<Vertex> reference(std::begin(sky),std::end(sky));
        reference.insert(reference.end(),frame.scene.begin(),frame.scene.end());
        reference.insert(reference.end(),frame.cabin.begin(),frame.cabin.end());
        reference.insert(reference.end(),frame.hud.begin(),frame.hud.end());
        std::vector<Vertex> mapped(batch.layout.total+2);
        std::fill_n(reinterpret_cast<unsigned char*>(mapped.data()),mapped.size()*sizeof(Vertex),0xA5);
        batch.copyTo(mapped.data()+1,batch.layout.total);
        require(batch.bytes()==reference.size()*sizeof(Vertex)&&
                std::memcmp(mapped.data()+1,reference.data(),batch.bytes())==0,
                "Fleet vertices match legacy concatenation byte for byte");
        require(batch.layout.scene==6&&batch.layout.cabin==6+frame.scene.size()&&
                batch.layout.hud==6+frame.scene.size()+frame.cabin.size(),"Draw ranges retain order");
        const auto* sceneAddress=frame.scene.data();
        const auto* cabinAddress=frame.cabin.data();
        const auto* hudAddress=frame.hud.data();
        const auto retained=frame.retainedBytes();
        for(int n=0;n<120;n++) {
            frame.beginFrame(terrain);
            require(frame.scene.size()==terrain.size()&&frame.cabin.empty()&&frame.hud.empty(),
                    "View/menu changes cannot retain last frame's vertices");
            frame.scene.insert(frame.scene.end(),model.vertices.begin(),model.vertices.end());
            frame.cabin.assign(24000,Vertex{4,5,6,.7f,.8f,.9f});
            frame.hud.assign(6000,Vertex{7,8,9,1,1,1});
            presentation::VertexBatch<Vertex> next(sky,frame.scene,frame.cabin,frame.hud);
            next.copyTo(mapped.data()+1,batch.layout.total);
            require(frame.scene.data()==sceneAddress&&frame.cabin.data()==cabinAddress&&
                    frame.hud.data()==hudAddress&&frame.retainedBytes()==retained,
                    "Warm frame vectors do not reallocate");
            require(std::memcmp(mapped.data()+1,reference.data(),next.bytes())==0,
                    "Reusable frames preserve every vertex");
        }
        const auto* first=reinterpret_cast<const unsigned char*>(mapped.data());
        const auto* last=reinterpret_cast<const unsigned char*>(&mapped.back());
        for(std::size_t i=0;i<sizeof(Vertex);i++)require(first[i]==0xA5&&last[i]==0xA5,"Fleet upload guards intact");
        require(std::memcmp(terrain.data(),frame.scene.data(),terrain.size()*sizeof(Vertex))==0,
                "Terrain source remains unchanged");
        std::cout<<filename<<": "<<model.vertices.size()<<" model vertices; 120 warm frames, 0 geometry-buffer reallocations; "
                 <<batch.bytes()<<" DX12 intermediate bytes removed per frame\n";
    }
    frame.beginFrame({});
    presentation::VertexBatch<Vertex> skyOnly(sky,frame.scene,frame.cabin,frame.hud);
    require(skyOnly.layout.total==6&&frame.scene.empty()&&frame.cabin.empty()&&frame.hud.empty(),
            "Empty terrain recovery contains only this frame's data");
}

int main(int argc,char** argv) {
    try {
        boundsAndEmptyRanges();
        realFleetFrames(argc>1?std::filesystem::path(argv[1]):std::filesystem::path("assets"));
        std::cout<<"Frame geometry checks passed. CPU staging evidence only; no GPU/FPS claim.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
