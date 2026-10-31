#pragma once

#include "storage.h"
#include <array>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace sim {

struct FlightSample {
    static constexpr size_t columns = 40;
    std::array<double, columns> values{};

    static const char* header() {
        return "sim_s,x_m,y_m,z_m,ias_mps,tas_mps,gs_mps,mach,baro_alt_m,vs_mps,"
               "pitch_rad,roll_rad,yaw_rad,aoa_rad,beta_rad,load_g,lift_N,drag_N,side_N,"
               "pitch_moment_Nm,yaw_moment_Nm,roll_moment_Nm,pitch_rate,roll_rate,yaw_rate,"
               "left_n1,right_n1,left_egt_C,right_egt_C,left_thrust_N,right_thrust_N,"
               "fuel_pct,battery_fraction,avionics_on,surface_model,left_fault,right_fault,crashed,aircraft_id,engine_count";
    }

    static FlightSample capture(const Snapshot& state) {
        const auto& f = state.flight;
        const auto air = airData(f, state.avionics.qnh);
        const auto& left = f.engines.units[0];
        const auto& right = f.engines.units[1];
        const auto m = f.bodyForceMoment.moment;
        FlightSample sample;
        sample.values = {f.time, f.x, f.y, f.z, air.ias, air.tas, air.groundSpeed,
                         air.mach, air.altitude, f.vy, f.pitch, f.roll, f.yaw, f.aoa,
                         f.beta, f.load, f.liftForce, f.dragForce, f.sideForce,
                         m.x, m.y, m.z, f.pitchRate, f.rollRate, f.yawRate,
                         left.n1(), f.aircraftId ? right.n1() : 0.0,
                         left.egt, f.aircraftId ? right.egt : 0.0,
                         left.thrust, f.aircraftId ? right.thrust : 0.0,
                         f.fuel, state.avionics.charge, state.avionics.powered(f) ? 1.0 : 0.0,
                         f.surfaceModel ? 1.0 : 0.0,
                         double(static_cast<int>(left.fault)),
                         f.aircraftId ? double(static_cast<int>(right.fault)) : 0.0,
                         f.crashed ? 1.0 : 0.0, double(f.aircraftId), double(f.engines.count(f.aircraftId))};
        return sample;
    }

    bool finite() const {
        for (double value : values) if (!std::isfinite(value)) return false;
        return true;
    }

    void write(std::ostream& out) const {
        out << std::setprecision(12);
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) out << ',';
            out << values[i];
        }
        out << '\n';
    }

    static bool parse(const std::string& line, FlightSample& output) {
        std::istringstream input(line);
        FlightSample parsed;
        std::string cell;
        for (size_t i = 0; i < columns; ++i) {
            if (!std::getline(input, cell, ',') || cell.empty()) return false;
            std::istringstream number(cell);
            if (!(number >> parsed.values[i])) return false;
            number >> std::ws;
            if (!number.eof()) return false;
        }
        if (line.empty() || line.back() == ',' || std::getline(input, cell, ',')) return false;
        if (!parsed.finite()) return false;
        output = parsed;
        return true;
    }
};

struct FlightStatistics {
    size_t samples = 0;
    double firstTime = 0.0;
    double lastTime = 0.0;
    double minAltitude = 0.0;
    double maxAltitude = 0.0;
    double maxAirspeed = 0.0;
    double maxAbsRoll = 0.0;
    double maxAbsAoA = 0.0;
    double maxEgt = 0.0;
    double maxAsymmetricThrust = 0.0;
    bool crashed = false;

    bool add(const FlightSample& sample) {
        if (!sample.finite()) return false;
        const auto& v = sample.values;
        if (samples && v[0] < lastTime) return false;
        if (!samples) {
            firstTime = v[0];
            minAltitude = maxAltitude = v[2];
        }
        lastTime = v[0];
        minAltitude = std::min(minAltitude, v[2]);
        maxAltitude = std::max(maxAltitude, v[2]);
        maxAirspeed = std::max(maxAirspeed, v[5]);
        maxAbsRoll = std::max(maxAbsRoll, std::abs(v[11]));
        maxAbsAoA = std::max(maxAbsAoA, std::abs(v[13]));
        maxEgt = std::max({maxEgt, v[27], v[28]});
        if(v[39] > 1.5) maxAsymmetricThrust = std::max(maxAsymmetricThrust, std::abs(v[29] - v[30]));
        crashed = crashed || v[37] > .5;
        ++samples;
        return true;
    }

    void write(std::ostream& out) const {
        out << std::setprecision(10)
            << "samples=" << samples << '\n'
            << "duration_s=" << (samples ? lastTime - firstTime : 0.0) << '\n'
            << "min_altitude_m=" << minAltitude << '\n'
            << "max_altitude_m=" << maxAltitude << '\n'
            << "max_tas_mps=" << maxAirspeed << '\n'
            << "max_abs_roll_deg=" << maxAbsRoll * 180 / pi << '\n'
            << "max_abs_aoa_deg=" << maxAbsAoA * 180 / pi << '\n'
            << "max_egt_C=" << maxEgt << '\n'
            << "max_thrust_difference_N=" << maxAsymmetricThrust << '\n'
            << "crashed=" << crashed << '\n';
    }
};

inline bool analyzeTelemetry(std::istream& input, FlightStatistics& output,
                             std::string& error) {
    std::string line;
    if (!std::getline(input, line)) { error = "missing CSV header"; return false; }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != FlightSample::header()) { error = "unsupported CSV columns/version"; return false; }
    FlightStatistics parsed;
    size_t row = 1;
    while (std::getline(input, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        FlightSample sample;
        if (!FlightSample::parse(line, sample) || !parsed.add(sample)) {
            error = "invalid or non-monotonic sample at line " + std::to_string(row);
            return false;
        }
    }
    if (!input.eof() || !parsed.samples) { error = "empty or unreadable data"; return false; }
    output = parsed;
    return true;
}

inline void writeSurfaceSamples(std::ostream& out, double time, const SurfaceResult& result) {
    for (size_t i = 0; i < result.count; ++i) {
        const auto& s = result.samples[i];
        out << time << ',' << i << ',' << static_cast<int>(s.kind) << ','
            << s.position.x << ',' << s.position.y << ',' << s.position.z << ','
            << s.airspeed << ',' << s.alpha * 180 / pi << ',' << s.beta * 180 / pi << ','
            << s.cl << ',' << s.cd << ',' << s.lift << ',' << s.drag << ','
            << s.force.x << ',' << s.force.y << ',' << s.force.z << ','
            << s.moment.x << ',' << s.moment.y << ',' << s.moment.z << '\n';
    }
}

} // namespace sim
