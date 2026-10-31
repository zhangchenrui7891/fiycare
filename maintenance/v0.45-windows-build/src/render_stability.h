#pragma once
#include "simulation.h"
#include "frame_timing.h"

namespace presentation {

// D32_FLOAT reverse depth: nearest fragments win with GREATER, clear = 0.
inline constexpr float depthClear = 0.f;
inline constexpr float worldNear = .1f, worldFar = 6000.f;
inline constexpr float cabinNear = .04f, cabinFar = 8.f;

struct DepthProjection {
    float scale, offset;
    static DepthProjection reversed(float nearDistance, float farDistance) {
        const float scale = nearDistance / (nearDistance-farDistance);
        return {scale, -farDistance*scale};
    }
    float at(float distance) const { return scale + offset/distance; }
};

inline sim::Quaternion interpolate(sim::Quaternion a, sim::Quaternion b, double t) {
    // Antipodal quaternions describe the same orientation. Follow the short arc,
    // including when the aircraft crosses the -pi/+pi heading boundary.
    double dot = a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
    if (dot < 0) { b={-b.w,-b.x,-b.y,-b.z}; dot=-dot; }
    double left=1-t, right=t;
    if (dot < .9995) {
        const double angle=std::acos(std::clamp(dot,-1.,1.));
        const double divisor=std::sin(angle);
        left=std::sin((1-t)*angle)/divisor;
        right=std::sin(t*angle)/divisor;
    }
    sim::Quaternion result{a.w*left+b.w*right,a.x*left+b.x*right,
                           a.y*left+b.y*right,a.z*left+b.z*right};
    result.normalize();
    return result;
}

struct Pose {
    sim::V3 position;
    sim::Quaternion orientation;
    double time=0, flex=0;
    static Pose capture(const sim::Flight& f) {
        return {{f.x,f.y,f.z},sim::Quaternion::fromEuler(f.pitch,f.roll,f.yaw),
                f.time,f.wingFlex};
    }
};

struct FlightHistory {
    Pose previous, current;
    int aircraftId=-1;
    bool surfaceModel=false, crashed=false;
    void reset(const sim::Flight& f) {
        previous=current=Pose::capture(f);
        aircraftId=f.aircraftId; surfaceModel=f.surfaceModel; crashed=f.crashed;
    }
    void push(const sim::Flight& f) {
        auto next=Pose::capture(f);
        if (aircraftId!=f.aircraftId || surfaceModel!=f.surfaceModel ||
            crashed!=f.crashed || next.time<current.time ||
            (next.position-current.position).length()>100) { reset(f); return; }
        previous=current; current=next;
    }
    sim::Flight sample(const sim::Flight& authoritative, double fraction) const {
        sim::Flight visual=authoritative;
        const double t=std::isfinite(fraction)?std::clamp(fraction,0.,1.):1.;
        const auto p=previous.position*(1-t)+current.position*t;
        const auto e=interpolate(previous.orientation,current.orientation,t).euler();
        visual.x=p.x; visual.y=p.y; visual.z=p.z;
        visual.pitch=e.x; visual.roll=e.y; visual.yaw=e.z;
        visual.time=previous.time+(current.time-previous.time)*t;
        visual.wingFlex=previous.flex+(current.flex-previous.flex)*t;
        return visual;
    }
};

// Anchored to world lattice cells, independent of the asynchronously rebuilt
// terrain tile. Crossing a tile boundary must not relocate every visible tree.
inline sim::V3 treePosition(int cellX, int cellZ) {
    return {cellX*450.+90.,0,cellZ*550.};
}
} // namespace presentation
