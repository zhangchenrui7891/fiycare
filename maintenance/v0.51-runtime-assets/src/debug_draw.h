#pragma once

#include "scene.h"

inline XMFLOAT3 debugPoint(sim::V3 p) {
    return {float(p.x), float(p.y), float(p.z)};
}

inline void debugArrow(std::vector<Vertex>& out, sim::V3 origin,
                       sim::V3 vector, XMFLOAT3 color, double thickness) {
    const double length = vector.length();
    if (!vector.finite() || length < .001) return;
    const sim::V3 direction = vector / length;
    const sim::V3 axis = std::abs(direction.y) < .9 ? sim::V3{0, 1, 0} : sim::V3{1, 0, 0};
    const sim::V3 right = direction.cross(axis).normalizedOrZero() * thickness;
    const sim::V3 up = direction.cross(right).normalizedOrZero() * thickness;
    const sim::V3 tip = origin + vector;
    const sim::V3 neck = tip - direction * std::min(length * .25, thickness * 7);
    const size_t start = out.size();
    for (sim::V3 offset : {right, -right, up, -up}) {
        tri(out, debugPoint(origin + offset), debugPoint(origin - offset), debugPoint(neck + offset), color);
        tri(out, debugPoint(origin - offset), debugPoint(neck - offset), debugPoint(neck + offset), color);
        tri(out, debugPoint(neck + offset * 3), debugPoint(neck - offset * 3), debugPoint(tip), color);
    }
    for (size_t i = start; i < out.size(); ++i) out[i].textured = 2;
}

inline void drawFlightForces(std::vector<Vertex>& out, const sim::Flight& f) {
    const auto orientation = sim::Quaternion::fromEuler(f.pitch, f.roll, f.yaw);
    const sim::V3 position{f.x, f.y, f.z};
    const auto dimensions = sim::airframeDimensions(f.aircraftId);
    const double mass = f.spec().emptyMass + f.payload + f.passengers * 85;
    const double scale = dimensions.span * .7 / (mass * 9.81);
    if (f.surfaceModel) {
        for (size_t i = 0; i < f.surfaceResult.count; ++i) {
            const auto& sample = f.surfaceResult.samples[i];
            const auto origin = position + orientation.rotate(sample.position);
            sim::V3 vector = orientation.rotate(sample.force) * scale * 6;
            const double maximum = dimensions.span * .65;
            if (vector.length() > maximum) vector = vector.unit() * maximum;
            const XMFLOAT3 color = sample.kind == sim::SurfaceKind::Wing ? XMFLOAT3{.2f, 1, .3f} :
                                     sample.kind == sim::SurfaceKind::VerticalTail ? XMFLOAT3{1, .8f, .15f} :
                                     XMFLOAT3{.4f, .55f, 1};
            debugArrow(out, origin, vector, color, dimensions.span * .0018);
        }
    } else {
        debugArrow(out, position, orientation.rotate({0, f.liftForce, -f.dragForce}) * scale,
                   {.2f, 1, .3f}, dimensions.span * .003);
    }
    const int count = f.engines.count(f.aircraftId);
    for (int i = 0; i < count; ++i) {
        const sim::V3 mount{count == 1 ? 0.0 : (i == 0 ? -dimensions.engineArm : dimensions.engineArm), 0, 0};
        const auto thrust = orientation.rotate({0, 0, f.engines.units[size_t(i)].thrust * scale});
        debugArrow(out, position + orientation.rotate(mount), thrust, {1, .3f, .1f}, dimensions.span * .003);
    }
    const sim::V3 airflow{f.windX-f.vx, -f.vy, f.windZ-f.vz};
    const auto airArrow = airflow.normalizedOrZero() * dimensions.span * .5;
    debugArrow(out, position + sim::V3{0, dimensions.span*.15, 0}, airArrow,
               {.1f, .9f, 1}, dimensions.span * .003);
}
