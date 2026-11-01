#define near
#define far
// Match the legacy Windows SDK macros before including production scenery.
#include "src/scenery.h"
#undef near
#undef far
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <set>
#include <tuple>

void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
using Position=std::tuple<float,float,float>;
std::set<Position> positions(const std::vector<Vertex>& mesh) {
    std::set<Position> out;
    for(const auto& v:mesh)out.emplace(v.x,v.y,v.z);
    return out;
}
void geometry(const std::vector<Vertex>& mesh) {
    require(mesh.size()%3==0,"Complete triangles");
    for(std::size_t i=0;i<mesh.size();i+=3) {
        const auto a=mesh[i],b=mesh[i+1],c=mesh[i+2];
        const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
        const float vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
        const float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
        const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
        require(length>1e-5f,"Non-degenerate triangles");
        require((nx*a.nx+ny*a.ny+nz*a.nz)/length>.999f,"Normals agree with winding");
        for(const auto& v:{a,b,c}) {
            const float data[]={v.x,v.y,v.z,v.r,v.g,v.b,v.nx,v.ny,v.nz};
            for(auto f:data)require(std::isfinite(f),"Finite vertex values");
            require(v.r>=0&&v.r<=1&&v.g>=0&&v.g<=1&&v.b>=0&&v.b<=1,"Valid colors");
            require(std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.001f,"Unit normals");
            require(v.textured==0&&v.u==0&&v.v==0,"No aircraft/satellite atlas leakage");
            require(v.y>=float(sim::terrain(v.x,v.z,1))-.5f,"Grounded feature foundations");
            require(scenery::allowed(v.x,v.z,0,0),"All vertices remain in safe inland envelope");
            for(const auto& r:sim::runways)
                require(!(std::abs(v.x-float(r.x))<=float(r.halfWidth)+150 &&
                          std::abs(v.z-float(r.z))<=float(r.halfLength)+1500),"Clear runway approaches");
        }
    }
}
int main(int argc,char** argv) {
    try {
        const scenery::Color summer{.24f,.5f,.2f};
        std::vector<Vertex> mesh;std::size_t peak=0,total=0;unsigned cases=0;
        for(int x=-14;x<=8;x+=2)for(int z=-15;z<=15;z+=2) {
            mesh.clear();scenery::append(mesh,x,z,summer);geometry(mesh);
            require(mesh.size()<=scenery::vertexBudget,"Bounded added geometry");
            peak=std::max(peak,mesh.size());total+=mesh.size();++cases;
        }
        mesh.clear();scenery::append(mesh,0,0,summer);
        require(mesh.size()>6000,"Visible surroundings at departure airport");
        const auto first=mesh;mesh.clear();scenery::append(mesh,0,0,summer);
        require(mesh.size()==first.size()&&std::memcmp(mesh.data(),first.data(),mesh.size()*sizeof(Vertex))==0,"Repeatable full mesh");
        // Hash/world footprints do not depend on camera or season. Compare actual
        // buildings against both a near and a 2800m-distant mesh, ignoring LOD bands.
        std::vector<Vertex> near,far,building;
        scenery::append(near,-4,0,summer);scenery::append(far,-4,7,summer);
        const auto n=scenery::seed(-4,0);const auto district=n%5;
        if(district==0)scenery::building(building,-1575,35,11,17,6,true,false,{.64f,.59f,.49f});
        else if(district==1)scenery::building(building,-1645,0,55,36,12,false,false,{.57f,.61f,.62f});
        else for(int k=0;k<3;k++) {
            const float bx=-1700+float(k)*95,bz=float(int((n>>(k*4+3))&31u)-16)*3;
            const float h=district==4?24+float((n>>(k+12))&15u):7+float((n>>(k+12))&7u);
            scenery::building(building,bx,bz,17,23,h,district!=4,false,{.66f,.63f,.55f});
        }
        auto np=positions(near),fp=positions(far);
        for(const auto& v:building)require(np.count({v.x,v.y,v.z})&&fp.count({v.x,v.y,v.z}),"Building anchors and heights persist across camera LODs");
        std::vector<Vertex> winter;scenery::append(winter,-4,0,{.83f,.88f,.92f});
        require(near.size()==winter.size(),"Season preserves geometry count");
        bool recolored=false;
        for(std::size_t i=0;i<near.size();i++) {
            require(near[i].x==winter[i].x&&near[i].y==winter[i].y&&near[i].z==winter[i].z,"Season preserves positions");
            recolored|=near[i].r!=winter[i].r;
        }
        require(recolored,"Season changes foliage/field palette");
        // Output may already contain terrain. Only scenery's own addition is budgeted.
        Vertex sentinel{1,2,3,.1f,.2f,.3f};std::vector<Vertex> prefixed(60000,sentinel);
        scenery::append(prefixed,0,0,summer);
        require(prefixed.size()==60000+first.size(),"Terrain prefix retained and separately budgeted");
        for(std::size_t i=0;i<60000;i++)require(std::memcmp(&prefixed[i],&sentinel,sizeof(Vertex))==0,"Prefix unchanged");
        for(int key:{std::numeric_limits<int>::min(),-100,100,std::numeric_limits<int>::max()}) {
            auto before=prefixed.size();scenery::append(prefixed,key,key,summer);
            require(prefixed.size()==before,"Out-of-area keys safely empty");
        }
        require(!scenery::allowed(0,0,20,20)&&!scenery::allowed(3500,2500,20,20),"Airport footprints excluded");
        require(!scenery::allowed(-700,0,40,40),"Full footprint, not just center, is checked");
        // Optional actual mesh dump for a CPU projection preview; no GPU claim.
        if(argc==2) {
            std::ofstream out(argv[1]);
            auto emit=[&](const Vertex& v){out<<v.x<<' '<<v.y<<' '<<v.z<<' '<<v.r<<' '<<v.g<<' '<<v.b<<' '<<v.nx<<' '<<v.ny<<' '<<v.nz<<' '<<v.u<<' '<<v.v<<' '<<v.textured<<'\n';};
            auto ground=[](float x,float z){
                auto uv=sim::satelliteUV(x,z);
                return Vertex{x,float(sim::terrain(x,z,1)),z,1,1,1,uv.u,uv.v,uv.covered?1.f:0.f,0,1,0};
            };
            for(int x=-4800;x<4800;x+=200)for(int z=-4800;z<4800;z+=200){
                auto a=ground(float(x),float(z)),b=ground(float(x+200),float(z));
                auto c=ground(float(x),float(z+200)),d=ground(float(x+200),float(z+200));
                for(const auto& v:{a,c,b,b,c,d})emit(v);
            }
            for(const auto& v:first)emit(v);
            require(bool(out),"Preview mesh dump written");
        }
        std::cout<<"PASS offline scenery: "<<cases<<" camera keys, "<<total<<" checked vertices; peak "<<peak
                 <<" / "<<scenery::vertexBudget<<" budget; airport mesh "<<first.size()
                 <<". Determinism, LOD footprints, seasons, normals and clear approaches verified. CPU only.\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
