#pragma once

#include "math3d.h"
#include "aircraft.h"
#include "aerodynamics.h"

namespace sim {

struct AirframeDimensions {
    double span;
    double length;
    double tailArm;
    double engineArm;
    double wingHeight;
};

inline AirframeDimensions airframeDimensions(int id) {
    const auto& spec = aircraft(id);
    const double span = std::sqrt(spec.wingArea * aeroTuning(id).aspectRatio);
    static constexpr double lengths[] = {7.0, 30.0, 63.7, 66.8, 37.57, 39.47, 62.8, 33.84, 44.51, 62.8};
    static_assert(sizeof(lengths)/sizeof(lengths[0])==aircraftCount,"Dimension table size");
    const double length = lengths[std::clamp(id, 0, aircraftCount-1)];
    return {span, length, length * .32,
            id == 0 ? 0.0 : span * .19, span * .025};
}

inline V3 approximateInertia(int id, double mass) {
    const auto d = airframeDimensions(id);
    // Prototype mass distribution, not manufacturer inertia data.
    return {mass * d.length*d.length * .055,
            mass * (d.length*d.length + d.span*d.span) * .045,
            mass * d.span*d.span * .055};
}

struct RigidRotation {
    Quaternion orientation;
    V3 angularVelocity;
    V3 lastAngularAcceleration;
    bool initialized = false;

    void capture(double pitch, double roll, double yaw,
                 double pitchRate, double rollRate, double yawRate) {
        orientation = Quaternion::fromEuler(pitch, roll, yaw);
        angularVelocity = {-pitchRate, yawRate, -rollRate};
        lastAngularAcceleration = {};
        initialized = true;
    }

    bool valid() const {
        return orientation.finite() && angularVelocity.finite() &&
               std::abs(orientation.normSquared() - 1.0) < .001 &&
               angularVelocity.length() < 10.0;
    }

    bool integrate(V3 torque, V3 inertia, double dt) {
        if (!torque.finite() || !inertia.finite() ||
            !std::isfinite(dt) || dt <= 0.0 || dt > .02 ||
            inertia.x <= 0 || inertia.y <= 0 || inertia.z <= 0) {
            return false;
        }

        const RigidRotation previous = *this;
        const V3 momentum{inertia.x * angularVelocity.x,
                          inertia.y * angularVelocity.y,
                          inertia.z * angularVelocity.z};
        const V3 residual = torque - angularVelocity.cross(momentum);
        lastAngularAcceleration = {residual.x / inertia.x,
                                   residual.y / inertia.y,
                                   residual.z / inertia.z};
        angularVelocity += lastAngularAcceleration * dt;
        orientation.integrateBodyRate(angularVelocity, dt);
        if (!valid()) {
            *this = previous;
            return false;
        }
        return true;
    }
};

} // namespace sim
