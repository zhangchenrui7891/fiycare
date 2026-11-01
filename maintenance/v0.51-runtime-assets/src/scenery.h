#pragma once
#include "assets.h"
#include "world.h"
#include <algorithm>
#include <array>
#include <cstdint>

// Illustrative, offline scenery west of the training airport. These are NOT
// surveyed footprints, a DEM, collision objects or an airport-layout database.
namespace scenery {
constexpr std::size_t vertexBudget=48000;
constexpr float cellSize=400, viewRadius=4200;
struct Color {float r,g,b;};
struct Point {float x,y,z;};
inline std::uint32_t seed(int x,int z) {
    auto n=std::uint32_t(x)*0x9e3779b9u ^ std::uint32_t(z)*0x85ebca6bu;
    n^=n>>16; n*=0x7feb352du; n^=n>>15; n*=0x846ca68bu;
    return n^(n>>16);
}
inline bool allowed(float x,float z,float halfX,float halfZ) {
    // Conservative inland envelope; do not grow illustrative buildings out
    // into the photographed coast or either airport/approach corridor.
    if(x-halfX < -4800 || x+halfX > -720 || std::abs(z)+halfZ > 4400)return false;
    for(const auto& r:sim::runways) {
        if(std::abs(x-float(r.x)) < 620+halfX &&
           std::abs(z-float(r.z)) < float(r.halfLength)+900+halfZ)return false;
        if(std::abs(x-float(r.x)) < 200+halfX &&
           std::abs(z-float(r.z)) < float(r.halfLength)+2000+halfZ)return false;
    }
    return true;
}
inline void triangle(std::vector<Vertex>& out,Point a,Point b,Point c,Color color) {
    const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
    const float vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
    float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
    if(length<1e-6f)return;
    nx/=length;ny/=length;nz/=length;
    for(auto p:{a,b,c})out.push_back({p.x,p.y,p.z,color.r,color.g,color.b,0,0,0,nx,ny,nz});
}
inline void quad(std::vector<Vertex>& out,Point a,Point b,Point c,Point d,Color color) {
    triangle(out,a,b,c,color);triangle(out,a,c,d,color);
}
inline void cuboid(std::vector<Vertex>& out,float x,float y,float z,float w,float h,float d,Color color) {
    // No invisible bottom face. Explicit outward normals permit directional light.
    quad(out,{x-w,y,z-d},{x-w,y,z+d},{x-w,y+h,z+d},{x-w,y+h,z-d},color);
    quad(out,{x+w,y,z+d},{x+w,y,z-d},{x+w,y+h,z-d},{x+w,y+h,z+d},color);
    quad(out,{x+w,y,z-d},{x-w,y,z-d},{x-w,y+h,z-d},{x+w,y+h,z-d},color);
    quad(out,{x-w,y,z+d},{x+w,y,z+d},{x+w,y+h,z+d},{x-w,y+h,z+d},color);
    quad(out,{x-w,y+h,z-d},{x-w,y+h,z+d},{x+w,y+h,z+d},{x+w,y+h,z-d},color);
}
inline void surface(std::vector<Vertex>& out,float x,float z,float w,float d,Color color,float lift=.25f) {
    auto p=[&](float px,float pz){return Point{px,float(sim::terrain(px,pz,1))+lift,pz};};
    quad(out,p(x-w,z-d),p(x-w,z+d),p(x+w,z+d),p(x+w,z-d),color);
}
inline void roof(std::vector<Vertex>& out,float x,float y,float z,float w,float d,Color color) {
    const float rise=4;
    quad(out,{x-w,y,z-d},{x-w,y,z+d},{x,y+rise,z+d},{x,y+rise,z-d},color);
    quad(out,{x,y+rise,z-d},{x,y+rise,z+d},{x+w,y,z+d},{x+w,y,z-d},color);
    triangle(out,{x-w,y,z-d},{x,y+rise,z-d},{x+w,y,z-d},color);
    triangle(out,{x+w,y,z+d},{x,y+rise,z+d},{x-w,y,z+d},color);
}
inline void tree(std::vector<Vertex>& out,float x,float z,float height,Color foliage,bool highDetail) {
    const float y=float(sim::terrain(x,z,1));
    if(highDetail)cuboid(out,x,y,z,.45f,height*.6f,.45f,{.28f,.21f,.13f});
    const float base=y+height*.35f,w=height*.35f;
    const Point top{x,y+height,z};
    triangle(out,{x-w,base,z-w},top,{x+w,base,z-w},foliage);
    triangle(out,{x+w,base,z-w},top,{x+w,base,z+w},foliage);
    triangle(out,{x+w,base,z+w},top,{x-w,base,z+w},foliage);
    triangle(out,{x-w,base,z+w},top,{x-w,base,z-w},foliage);
}
inline void building(std::vector<Vertex>& out,float x,float z,float w,float d,float h,
                     bool pitched,bool highDetail,Color wall) {
    const float y=float(sim::terrain(x,z,1))-.3f;
    cuboid(out,x,y,z,w,h,d,wall);
    if(pitched)roof(out,x,y+h,z,w+1,d+1,{.36f,.22f,.16f});
    if(highDetail) {
        // Two narrow glazing bands on the long facades, omitted at middle/far LOD.
        for(float side:{-1.f,1.f}) {
            const float face=z+side*(d+.04f);
            if(side<0)quad(out,{x-w*.8f,y+h*.55f,face},{x-w*.8f,y+h*.75f,face},
                              {x+w*.8f,y+h*.75f,face},{x+w*.8f,y+h*.55f,face},{.17f,.28f,.32f});
            else quad(out,{x-w*.8f,y+h*.55f,face},{x+w*.8f,y+h*.55f,face},
                          {x+w*.8f,y+h*.75f,face},{x-w*.8f,y+h*.75f,face},{.17f,.28f,.32f});
        }
    }
}
struct Cell {int x,z;float distance;};
inline void append(std::vector<Vertex>& out,int anchorX,int anchorZ,Color foliage) {
    // Bound input before multiplying cell indices; invalid/extreme camera keys
    // safely retain terrain alone. There is intentionally no global RNG/cache.
    if(anchorX < -24 || anchorX > 12 || anchorZ < -24 || anchorZ > 24)return;
    std::array<Cell,441> cells{};
    std::size_t count=0;
    for(int dx=-10;dx<=10;dx++)for(int dz=-10;dz<=10;dz++) {
        const int cx=anchorX+dx,cz=anchorZ+dz;
        const float distance=std::hypot(float(dx)*cellSize,float(dz)*cellSize);
        if(distance<=viewRadius && allowed(cx*cellSize,cz*cellSize,195,195))
            cells[count++]={cx,cz,distance};
    }
    std::sort(cells.begin(),cells.begin()+count,[](const Cell& a,const Cell& b){
        if(a.distance!=b.distance)return a.distance<b.distance;
        return a.x!=b.x?a.x<b.x:a.z<b.z;
    });
    const auto initial=out.size();
    std::vector<Vertex> cell;cell.reserve(512);
    for(std::size_t i=0;i<count;i++) {
        const auto c=cells[i];const auto n=seed(c.x,c.z);
        const float x=c.x*cellSize,z=c.z*cellSize;
        const bool highDetail=c.distance<=1600,middle=c.distance<=2800;
        cell.clear();
        // Shared cell-edge roads join exactly. Small field plots leave the
        // existing satellite image visible between the illustrative features.
        if(middle) {
            surface(cell,x-192,z,3,200,{.26f,.27f,.26f});
            surface(cell,x,z-192,200,3,{.26f,.27f,.26f});
        }
        const auto district=n%5;
        if(district==0 || c.x<=-10) {
            if(middle)for(float offset:{-85.f,85.f})
                surface(cell,x+offset,z,70,145,{foliage.r*.85f,foliage.g*.85f,foliage.b*.7f},.18f);
            building(cell,x+25,z+35,11,17,6,true,highDetail,{.64f,.59f,.49f});
        } else if(district==1) {
            building(cell,x-45,z,55,36,12,false,highDetail,{.57f,.61f,.62f});
            if(highDetail)cuboid(cell,x-45,float(sim::terrain(x-45,z,1))+11.7f,z,57,.7f,38,{.25f,.35f,.39f});
        } else {
            for(int k=0;k<3;k++) {
                const float bx=x-100+float(k)*95;
                const float bz=z+float(int((n>>(k*4+3))&31u)-16)*3;
                const float h=district==4?24+float((n>>(k+12))&15u):7+float((n>>(k+12))&7u);
                building(cell,bx,bz,17,23,h,district!=4,highDetail,{.66f,.63f,.55f});
            }
        }
        if(middle)for(int k=0;k<4;k++)
            tree(cell,x-125+float(k)*80,z+135,10+float((n>>(k*3))&7u),foliage,highDetail);
        if(out.size()-initial+cell.size()>vertexBudget)break; // whole cells, nearest first
        out.insert(out.end(),cell.begin(),cell.end());
    }
}
} // namespace scenery
