#pragma once

#include "engines.h"
#include "rigid_body.h"
#include <istream>
#include <ostream>
#include <string>

namespace sim {

inline void writeEngineUnit(std::ostream& out, const EngineUnit& unit) {
    out << unit.enabled << ' ' << static_cast<int>(unit.fault) << ' '
        << static_cast<int>(unit.phase) << ' ' << unit.rpm << ' '
        << unit.egt << ' ' << unit.oilTemperature << ' '
        << unit.throttleOffset << '\n';
}

inline bool readEngineUnit(std::istream& in, EngineUnit& target) {
    EngineUnit unit;
    int fault = 0;
    int phase = 0;
    if (!(in >> unit.enabled >> fault >> phase >> unit.rpm >> unit.egt >>
          unit.oilTemperature >> unit.throttleOffset)) return false;
    if (fault < 0 || fault > 3 || phase < 0 || phase > 4) return false;
    unit.fault = static_cast<EngineFault>(fault);
    unit.phase = static_cast<EnginePhase>(phase);
    if (!unit.valid()) return false;
    target = unit;
    return true;
}

inline void writeFlightSystems(std::ostream& out, bool surfaceModel, double yawRate,
                              const EngineSystem& engines, const RigidRotation& rotation,
                              bool debugPanel, bool showForces) {
    out << "SYSTEMS 1\n" << surfaceModel << ' ' << yawRate << ' '
        << engines.initialized << ' ' << debugPanel << ' ' << showForces << '\n';
    for (const auto& unit : engines.units) writeEngineUnit(out, unit);
    const auto& q = rotation.orientation;
    const auto& w = rotation.angularVelocity;
    out << rotation.initialized << ' ' << q.w << ' ' << q.x << ' '
        << q.y << ' ' << q.z << ' ' << w.x << ' ' << w.y << ' ' << w.z << '\n'
        << "END_SYSTEMS\n";
}

inline bool readFlightSystems(std::istream& in, bool& surfaceModel, double& yawRate,
                             EngineSystem& engines, RigidRotation& rotation,
                             bool& debugPanel, bool& showForces) {
    std::string tag;
    int version = 0;
    bool mode = false, debug = false, forces = false;
    double rate = 0.0;
    EngineSystem parsedEngines;
    RigidRotation parsedRotation;
    if (!(in >> tag >> version) || tag != "SYSTEMS" || version != 1) return false;
    if (!(in >> mode >> rate >> parsedEngines.initialized >> debug >> forces)) return false;
    if (!std::isfinite(rate) || std::abs(rate) > 10) return false;
    for (auto& unit : parsedEngines.units) {
        if (!readEngineUnit(in, unit)) return false;
    }
    auto& q = parsedRotation.orientation;
    auto& w = parsedRotation.angularVelocity;
    if (!(in >> parsedRotation.initialized >> q.w >> q.x >> q.y >> q.z >>
          w.x >> w.y >> w.z)) return false;
    std::string end;
    if (!(in >> end) || end != "END_SYSTEMS") return false;
    if (!parsedRotation.valid()) return false;
    surfaceModel = mode;
    yawRate = rate;
    engines = parsedEngines;
    rotation = parsedRotation;
    debugPanel = debug;
    showForces = forces;
    return true;
}

} // namespace sim
