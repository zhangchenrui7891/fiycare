#pragma once

#include "aircraft.h"
#include "math3d.h"
#include "rigid_body.h"
#include <array>

namespace sim {

enum class EngineFault { None = 0, Flameout = 1, Fire = 2, Generator = 3 };
enum class EnginePhase { Off, Starting, Idle, Running, Failed };

inline const char* enginePhaseName(EnginePhase phase) {
    switch (phase) {
        case EnginePhase::Off: return "OFF";
        case EnginePhase::Starting: return "START";
        case EnginePhase::Idle: return "IDLE";
        case EnginePhase::Running: return "RUN";
        case EnginePhase::Failed: return "FAIL";
    }
    return "OFF";
}

struct EngineUnit {
    bool enabled = true;
    EngineFault fault = EngineFault::None;
    EnginePhase phase = EnginePhase::Off;
    double rpm = 0.0;
    double egt = 15.0;
    double oilTemperature = 15.0;
    double oilPressure = 0.0;
    double thrust = 0.0;
    double fuelPercentPerSecond = 0.0;
    double throttleOffset = 0.0;

    bool combustion(bool master, double fuel) const {
        return master && enabled && fuel > 0.0 &&
               fault != EngineFault::Flameout && fault != EngineFault::Fire;
    }

    bool generatorAvailable(bool master, double fuel) const {
        return combustion(master, fuel) && rpm > 1000.0 &&
               fault != EngineFault::Generator;
    }

    double n1() const { return std::clamp(rpm / 26.0, 0.0, 100.0); }

    void update(double dt, const AircraftSpec& spec, int count,
                bool master, double fuel, double lever, double condition) {
        const bool burning = combustion(master, fuel);
        const double demand = std::clamp(lever + throttleOffset, 0.0, 1.0);
        const double target = burning ? 700.0 + demand * 1900.0 : 0.0;
        rpm += (target - rpm) * std::min(1.0, dt * spec.spoolRate);
        thrust = std::max(0.0, (rpm - 700.0) / 1900.0) *
                 spec.maxThrust / count * std::clamp(condition, .3, 1.0);
        if (!burning) thrust = 0.0;
        fuelPercentPerSecond = burning ? (.001 + demand * spec.fuelRate) / count : 0.0;

        double targetEgt = burning ? 300.0 + demand * 480.0 : 15.0;
        if (fault == EngineFault::Fire) targetEgt = 1100.0;
        egt += (targetEgt - egt) * std::min(1.0, dt * .35);
        const double targetOil = burning ? 65.0 + demand * 45.0 : 15.0;
        oilTemperature += (targetOil - oilTemperature) * std::min(1.0, dt * .025);
        oilPressure = burning ? 20.0 + n1() * .5 : 0.0;

        if (fault == EngineFault::Flameout || fault == EngineFault::Fire) {
            phase = EnginePhase::Failed;
        } else if (!burning) {
            phase = EnginePhase::Off;
        } else if (rpm < 650.0) {
            phase = EnginePhase::Starting;
        } else {
            phase = demand < .02 ? EnginePhase::Idle : EnginePhase::Running;
        }
    }

    bool valid() const {
        const int failure = static_cast<int>(fault);
        return failure >= 0 && failure <= 3 && std::isfinite(rpm) &&
               rpm >= 0 && rpm <= 2601 && std::isfinite(egt) &&
               egt >= -100 && egt <= 1200 && std::isfinite(oilTemperature) &&
               oilTemperature >= -100 && oilTemperature <= 200 &&
               std::isfinite(throttleOffset) && std::abs(throttleOffset) <= .5;
    }
};

struct EngineSystem {
    std::array<EngineUnit, 2> units{};
    bool initialized = false;
    double totalThrust = 0.0;
    double fuelFlow = 0.0;
    double averageRpm = 0.0;
    ForceMoment bodyForces;

    int count(int aircraftId) const { return aircraftId == 0 ? 1 : 2; }

    void initialize(double legacyRpm) {
        for (auto& unit : units) unit.rpm = legacyRpm;
        initialized = true;
    }

    void update(int id, double dt, bool master, double lever,
                double& fuel, double condition, double legacyRpm) {
        if (!initialized) initialize(legacyRpm);
        totalThrust = fuelFlow = averageRpm = 0.0;
        bodyForces = {};
        const int n = count(id);
        const double arm = airframeDimensions(id).engineArm;
        for (int i = 0; i < n; ++i) {
            auto& unit = units[static_cast<size_t>(i)];
            unit.update(dt, aircraft(id), n, master, fuel, lever, condition);
            totalThrust += unit.thrust;
            fuelFlow += unit.fuelPercentPerSecond;
            averageRpm += unit.rpm / n;
            const V3 mount{n == 1 ? 0.0 : (i == 0 ? -arm : arm), 0.0, 0.0};
            bodyForces.addAt({0, 0, unit.thrust}, mount);
        }
        fuel = std::max(0.0, fuel - fuelFlow * dt);
    }

    bool anyGenerator(int id, bool master, double fuel) const {
        for (int i = 0; i < count(id); ++i) {
            if (units[static_cast<size_t>(i)].generatorAvailable(master, fuel)) return true;
        }
        return false;
    }

    void inject(int index, EngineFault fault, int id) {
        if (index >= 0 && index < count(id)) units[static_cast<size_t>(index)].fault = fault;
    }

    void resetFaults() {
        for (auto& unit : units) unit.fault = EngineFault::None;
    }

    bool valid() const {
        return units[0].valid() && units[1].valid();
    }
};

} // namespace sim
