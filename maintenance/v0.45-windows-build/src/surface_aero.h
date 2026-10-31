#pragma once

#include "math3d.h"
#include "rigid_body.h"
#include "aerodynamics.h"
#include <array>

namespace sim {

enum class SurfaceKind { Wing, HorizontalTail, VerticalTail, Fuselage };

struct SurfaceDefinition {
    SurfaceKind kind = SurfaceKind::Wing;
    V3 position;
    double area = 0.0;
    double dihedral = 0.0;
    double side = 0.0;
};

struct SurfaceAircraft {
    static constexpr size_t capacity = 16;
    std::array<SurfaceDefinition, capacity> surfaces{};
    size_t count = 0;
};

inline SurfaceAircraft makeSurfaceAircraft(int id) {
    SurfaceAircraft geometry;
    const auto d = airframeDimensions(id);
    const double area = aircraft(id).wingArea;
    constexpr int halfPanels = 6;
    for (int side : {-1, 1}) {
        for (int panel = 0; panel < halfPanels; ++panel) {
            const double fraction = (panel + .5) / halfPanels;
            const double x = side * fraction * d.span * .5;
            // Equal-area strips. Swept-wing pressure centres require calibration;
            // place the main wing at CG longitudinally to avoid invented trim moments.
            geometry.surfaces[geometry.count++] = {
                SurfaceKind::Wing, {x, d.wingHeight, 0},
                area / (2 * halfPanels), 5.0 * pi / 180.0, double(side)};
        }
    }
    for (int side : {-1, 1}) {
        geometry.surfaces[geometry.count++] = {
            SurfaceKind::HorizontalTail,
            {side * d.span * .12, d.wingHeight * .5, -d.tailArm},
            area * .09, 0, double(side)};
    }
    geometry.surfaces[geometry.count++] = {
        SurfaceKind::VerticalTail, {0, d.span * .065, -d.tailArm},
        area * .10, 0, 0};
    geometry.surfaces[geometry.count++] = {
        SurfaceKind::Fuselage, {0, 0, 0}, area * .1, 0, 0};
    return geometry;
}

struct SurfaceInput {
    int aircraftId = 0;
    V3 bodyAirVelocity;
    V3 angularVelocity;
    double density = 1.225;
    double groundHeight = 1000.0;
    double pitchControl = 0.0;
    double rollControl = 0.0;
    double yawControl = 0.0;
    double flap = 0.0;
    bool gear = true;
};

struct SurfaceSample {
    SurfaceKind kind = SurfaceKind::Wing;
    V3 position;
    V3 localAirVelocity;
    V3 force;
    V3 moment;
    double airspeed = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    double lift = 0.0;
    double drag = 0.0;
    double cl = 0.0;
    double cd = 0.0;
    double separation = 0.0;
};

struct SurfaceResult {
    ForceMoment total;
    std::array<SurfaceSample, SurfaceAircraft::capacity> samples{};
    size_t count = 0;
    double totalLift = 0.0;
    double totalDrag = 0.0;
    double meanSeparation = 0.0;
    double leftLift = 0.0;
    double rightLift = 0.0;

    bool finite() const {
        if (!total.force.finite() || !total.moment.finite()) return false;
        for (size_t i = 0; i < count; ++i) {
            const auto& s = samples[i];
            if (!s.force.finite() || !s.moment.finite() ||
                !std::isfinite(s.alpha) || !std::isfinite(s.beta) ||
                !std::isfinite(s.lift) || !std::isfinite(s.drag)) return false;
        }
        return true;
    }
};

inline SurfaceSample evaluateSurface(const SurfaceDefinition& def,
                                     const SurfaceInput& input) {
    SurfaceSample result;
    result.kind = def.kind;
    result.position = def.position;
    // Each surface sees the flow induced by rigid-body angular motion.
    result.localAirVelocity = input.bodyAirVelocity +
                              input.angularVelocity.cross(def.position);
    result.airspeed = result.localAirVelocity.length();
    if (result.airspeed < .05) return result;

    const V3 direction = result.localAirVelocity / result.airspeed;
    const double cosine = std::cos(def.dihedral);
    const double sine = std::sin(def.dihedral) * def.side;
    const V3 right{cosine, sine, 0};
    const V3 up{-sine, cosine, 0};
    result.alpha = std::atan2(-result.localAirVelocity.dot(up),
                              result.localAirVelocity.z);
    result.beta = std::asin(std::clamp(direction.x, -1.0, 1.0));
    const double q = .5 * std::max(0.0, input.density) *
                     result.airspeed * result.airspeed;

    if (def.kind == SurfaceKind::Wing) {
        const auto coefficient = aeroCoefficients(
            input.aircraftId, result.alpha, result.beta, input.flap,
            input.gear, input.groundHeight + def.position.y);
        const double aileron = -.65 * input.rollControl * def.side;
        result.cl = coefficient.cl + aileron * (1.0 - coefficient.separation);
        const auto tune = aeroTuning(input.aircraftId);
        const double deltaInduced = (result.cl*result.cl - coefficient.cl*coefficient.cl) /
                                    (pi * tune.aspectRatio * tune.efficiency);
        result.cd = std::max(.001, coefficient.cd + deltaInduced * coefficient.groundFactor);
        result.separation = coefficient.separation;
        result.lift = q * def.area * result.cl;
        result.drag = q * def.area * result.cd;
        const V3 liftDirection = direction.cross(right).normalizedOrZero();
        result.force = liftDirection * result.lift - direction * result.drag;
    } else if (def.kind == SurfaceKind::HorizontalTail) {
        // A simple stabilizer incidence and elevator model, with local angular flow.
        const double effective = result.alpha - .035 - input.pitchControl * .22;
        result.cl = std::clamp(3.6 * effective, -1.35, 1.35);
        result.cd = .012 + result.cl*result.cl / (pi * 4.0 * .75);
        result.lift = q * def.area * result.cl;
        result.drag = q * def.area * result.cd;
        result.force = direction.cross({1, 0, 0}).normalizedOrZero() * result.lift -
                       direction * result.drag;
    } else if (def.kind == SurfaceKind::VerticalTail) {
        const double sideCoefficient = -.9 * std::sin(result.beta) -
                                        input.yawControl * .65;
        result.cl = sideCoefficient;
        result.cd = .012 + sideCoefficient*sideCoefficient * .16;
        result.lift = q * def.area * sideCoefficient;
        result.drag = q * def.area * result.cd;
        const V3 lateral = (V3{1, 0, 0} - direction * direction.x).normalizedOrZero();
        result.force = lateral * result.lift - direction * result.drag;
    } else {
        result.cd = .08 + .8 * direction.y*direction.y +
                    .8 * direction.x*direction.x;
        result.drag = q * def.area * result.cd;
        result.force = direction * -result.drag;
    }
    result.moment = def.position.cross(result.force);
    return result;
}

inline SurfaceResult solveSurfaces(const SurfaceInput& input) {
    const auto geometry = makeSurfaceAircraft(input.aircraftId);
    SurfaceResult result;
    result.count = geometry.count;
    for (size_t i = 0; i < geometry.count; ++i) {
        auto sample = evaluateSurface(geometry.surfaces[i], input);
        result.samples[i] = sample;
        result.total.addAt(sample.force, sample.position);
        if (sample.kind != SurfaceKind::VerticalTail) result.totalLift += sample.lift;
        result.totalDrag += sample.drag;
        if (sample.kind == SurfaceKind::Wing) {
            result.meanSeparation += sample.separation / 12.0;
            if (sample.position.x < 0) result.leftLift += sample.lift;
            else result.rightLift += sample.lift;
        }
    }
    return result;
}

} // namespace sim
