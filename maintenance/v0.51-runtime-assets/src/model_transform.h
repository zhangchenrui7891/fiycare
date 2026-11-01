#pragma once

#include "assets.h"
#include <algorithm>

inline ImportedModel rotateAircraftQuarterTurn(const ImportedModel& source,
                                               double lengthMeters,double groundOffset){
    if(source.vertices.empty()||!std::isfinite(lengthMeters)||lengthMeters<=0||
       !std::isfinite(groundOffset))throw std::runtime_error("Invalid model transform");
    auto result=source;
    float minX=1e20f,minY=1e20f,minZ=1e20f,maxX=-1e20f,maxZ=-1e20f;
    for(auto& v:result.vertices){
        const float x=v.x;v.x=v.z;v.z=-x;
        const float nx=v.nx;v.nx=v.nz;v.nz=-nx;
        minX=std::min(minX,v.x);maxX=std::max(maxX,v.x);minY=std::min(minY,v.y);
        minZ=std::min(minZ,v.z);maxZ=std::max(maxZ,v.z);
    }
    if(maxZ-minZ<1e-5)throw std::runtime_error("Degenerate longitudinal extent");
    const double scale=lengthMeters/(maxZ-minZ);
    for(auto& v:result.vertices){
        v.x=float((v.x-(minX+maxX)*.5)*scale);v.z=float((v.z-(minZ+maxZ)*.5)*scale);
        v.y=float((v.y-minY)*scale-groundOffset);
    }
    return result;
}
