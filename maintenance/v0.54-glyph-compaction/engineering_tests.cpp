#include "src/autopilot.h"
#include "src/cockpit_controls.h"
#include "src/fault_scenarios.h"
#include "src/flight_alerts.h"
#include "src/telemetry.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

void expect(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

bool near(double a, double b, double tolerance = 1e-8) {
    return std::abs(a - b) <= tolerance;
}

void rotationTests() {
    for (double pitch : {-.6, 0.0, .6}) {
        for (double roll : {-1.1, 0.0, 1.1}) {
            for (double yaw : {-2.4, 0.0, 2.4}) {
                const auto q = sim::Quaternion::fromEuler(pitch, roll, yaw);
                const auto fwd = q.rotate({0, 0, 1});
                const auto right = q.rotate({1, 0, 0});
                const auto up = q.rotate({0, 1, 0});
                expect(near(fwd.length(), 1) && near(right.length(), 1) && near(up.length(), 1),
                       "Attitude basis retains unit lengths");
                expect(near(fwd.dot(right), 0) && near(fwd.dot(up), 0) && near(right.dot(up), 0),
                       "Attitude basis is orthogonal");
                const auto e = q.euler();
                expect(near(e.x, pitch) && near(e.y, roll) && near(e.z, yaw),
                       "Quaternion/Euler agree with visual attitude convention");
                const sim::V3 v{3.2, -5.0, 12.7};
                expect((q.inverseRotate(q.rotate(v)) - v).length() < 1e-8,
                       "Body/world transform roundtrip");
            }
        }
    }
    sim::RigidRotation body;
    body.capture(0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 100; ++i) {
        expect(body.integrate({2, 0, 0}, {2, 3, 4}, .01), "Finite torque integration");
    }
    expect(near(body.angularVelocity.x, 1, 1e-6), "Constant single-axis torque matches angular impulse");
    expect(std::abs(body.orientation.euler().x + .5) < .006,
           "Constant torque orientation approaches analytic half-angle result");
    expect(near(body.orientation.normSquared(), 1), "Quaternion remains normalized");
    expect(!body.integrate({0, 0, 0}, {0, 1, 1}, .01), "Zero inertia rejected");
    expect(!body.integrate({0, 0, 0}, {1, 1, 1}, .1), "Oversized dynamics step rejected");
    const auto priorRate = body.angularVelocity;
    expect(!body.integrate({1e9, 0, 0}, {2, 3, 4}, .01) &&
           (body.angularVelocity-priorRate).length() == 0,
           "Out-of-envelope integration preserves the last valid state for saving");
    sim::ForceMoment applied;
    applied.addAt({0, 0, 100}, {-2, 0, 0});
    expect(near(applied.moment.y, 200), "Left engine generates positive yaw torque");
}

void surfaceTests() {
    for (int id = 0; id < 4; ++id) {
        const auto geometry = sim::makeSurfaceAircraft(id);
        expect(geometry.count == 16, "Sixteen surface samples per aircraft");
        double wingArea = 0;
        for (size_t i = 0; i < geometry.count; ++i) {
            if (geometry.surfaces[i].kind == sim::SurfaceKind::Wing) wingArea += geometry.surfaces[i].area;
        }
        expect(near(wingArea, sim::aircraft(id).wingArea, 1e-6), "Panel areas conserve configured main-wing area");
        sim::SurfaceInput input;
        input.aircraftId = id;
        input.gear = false;
        input.bodyAirVelocity = {0, -3, 90};
        const auto symmetric = sim::solveSurfaces(input);
        expect(symmetric.finite() && near(symmetric.leftLift, symmetric.rightLift, 1e-5),
               "Symmetric aircraft produces equal main-wing lift");
        expect(std::abs(symmetric.total.moment.y) < 1e-4 && std::abs(symmetric.total.moment.z) < 1e-4,
               "Symmetric flow has no yaw or roll moment");
        input.rollControl = .5;
        auto roll = sim::solveSurfaces(input);
        expect(roll.total.moment.z < 0 && roll.leftLift > roll.rightLift,
               "Right roll command creates correct differential lift and moment");
        input.rollControl = 0;
        input.pitchControl = .5;
        auto pitch = sim::solveSurfaces(input);
        expect(pitch.total.moment.x < symmetric.total.moment.x, "Nose-up elevator reduces pitch-axis torque");
        input.pitchControl = 0;
        input.yawControl = .5;
        auto yaw = sim::solveSurfaces(input);
        expect(yaw.total.moment.y > 0, "Right rudder generates positive yaw torque");
        input.yawControl = 0;
        input.angularVelocity = {0, 0, .2};
        auto damp = sim::solveSurfaces(input);
        expect(damp.total.moment.z < 0, "Local rotational flow damps roll rate");
        expect((damp.samples[0].localAirVelocity-damp.samples[6].localAirVelocity).length() > .1,
               "Opposite wings sample different rotational airflow");
        input.angularVelocity = {};
        input.bodyAirVelocity = {};
        const auto zero = sim::solveSurfaces(input);
        expect(zero.total.force.length() == 0 && zero.finite(), "Zero flow has no invented aerodynamic force");
    }
}

void engineTests() {
    sim::EngineSystem engines;
    double fuel = 100;
    for (int i = 0; i < 3000; ++i) engines.update(3, .01, true, .7, fuel, 1, 0);
    const auto& left = engines.units[0];
    const auto& right = engines.units[1];
    expect(near(left.thrust, right.thrust) && near(engines.bodyForces.moment.y, 0),
           "Balanced twin engines produce no yaw torque");
    const double full = engines.totalThrust;
    engines.inject(0, sim::EngineFault::Flameout, 3);
    engines.update(3, .01, true, .7, fuel, 1, 0);
    expect(near(left.thrust, 0) && right.thrust > full*.49,
           "One engine failure preserves other engine thrust");
    expect(engines.bodyForces.moment.y < 0 && engines.anyGenerator(3, true, fuel),
           "Asymmetric thrust yaws toward failed engine while other generator survives");
    engines.inject(1, sim::EngineFault::Generator, 3);
    engines.update(3, .01, true, .7, fuel, 1, 0);
    expect(!engines.anyGenerator(3, true, fuel) && right.thrust > 0,
           "Generator fault removes electrical power without stopping combustion");
    engines.inject(1, sim::EngineFault::Fire, 3);
    const double oldEgt = right.egt;
    engines.update(3, 1, true, .7, fuel, 1, 0);
    expect(right.egt > oldEgt && right.thrust == 0 && right.phase == sim::EnginePhase::Failed,
           "Fire raises EGT and removes failed-engine thrust");
    engines.resetFaults();
    expect(left.fault == sim::EngineFault::None && right.fault == sim::EngineFault::None,
           "Training fault reset clears both engines");
    const double priorFuel = fuel;
    for (int i = 0; i < 1000; ++i) engines.update(3, .01, false, 1, fuel, 1, 0);
    expect(fuel == priorFuel && engines.totalThrust == 0, "Master-off stops fuel use and thrust");
    sim::EngineSystem trainer;
    trainer.inject(1, sim::EngineFault::Fire, 0);
    expect(trainer.units[1].fault == sim::EngineFault::None, "Trainer rejects nonexistent second-engine fault");
}

void faultAndAlertTests() {
    sim::Snapshot state;
    state.flight.configure(3);
    sim::FaultSchedule schedule;
    expect(schedule.set({{2, 1, sim::EngineFault::Generator}, {1, 0, sim::EngineFault::Flameout}}, 3),
           "Out-of-order fault plan is sorted");
    expect(schedule.apply(.9, state.flight) == 0, "Fault does not happen early");
    expect(schedule.apply(1, state.flight) == 1 && schedule.apply(1.5, state.flight) == 0,
           "Scheduled fault fires exactly once");
    expect(schedule.apply(2, state.flight) == 1 && schedule.complete(), "Remaining fault completes schedule");
    expect(!schedule.set({{0, 1, sim::EngineFault::Fire}}, 0), "Invalid plan rejected without replacing active schedule");
    sim::AlertManager alerts;
    alerts.update(state);
    expect(alerts.attention(), "Fresh engine failure needs acknowledgement");
    alerts.acknowledge(); alerts.update(state);
    expect(!alerts.attention() && !alerts.active.empty(), "Acknowledgement retains active fault indication");
    state.flight.engines.resetFaults(); alerts.update(state);
    state.flight.engines.inject(0, sim::EngineFault::Fire, 3); alerts.update(state);
    expect(alerts.attention() && alerts.active.front().level == sim::AlertLevel::Warning,
           "Resolved fault can reappear as new higher-priority warning");
}

void storageAndTelemetryTests() {
    sim::Snapshot state;
    state.flight.configure(3);
    state.flight.y = 1500; state.flight.vz = 100;
    state.flight.setSurfaceModel(true);
    for (int i = 0; i < 30; ++i) state.flight.step(.01, {});
    state.flight.engines.inject(1, sim::EngineFault::Generator, 3);
    state.flight.engines.units[0].enabled = false;
    state.debugPanel = state.showForces = true;
    std::stringstream out;
    state.write(out);
    sim::Snapshot loaded;
    expect(loaded.read(out) && loaded.flight.surfaceModel && loaded.debugPanel && loaded.showForces,
           "v15 preserves dynamics and engineering switches");
    expect(!loaded.flight.engines.units[0].enabled &&
           loaded.flight.engines.units[1].fault == sim::EngineFault::Generator,
           "v15 preserves independent engine commands and faults");
    expect((loaded.flight.rotation.angularVelocity-state.flight.rotation.angularVelocity).length() < 1e-10,
           "v15 preserves body angular rates");
    auto uninterrupted = state.flight;
    auto resumed = loaded.flight;
    for (int i = 0; i < 100; ++i) { uninterrupted.step(.01, {}); resumed.step(.01, {}); }
    expect(near(uninterrupted.y, resumed.y, 1e-7) && near(uninterrupted.pitch, resumed.pitch, 1e-7),
           "Saved rigid-body flight resumes with same trajectory");
    auto broken = out.str();
    broken.resize(broken.size()-8);
    std::stringstream truncated(broken);
    loaded.flight.x = 123;
    expect(!loaded.read(truncated) && loaded.flight.x == 123, "Incomplete v15 tail rejected atomically");
    auto legacy = out.str();
    legacy.replace(0, 14, "FCAIR_STATE 14");
    legacy.erase(legacy.find("SYSTEMS"));
    std::stringstream old(legacy);
    expect(loaded.read(old) && !loaded.flight.surfaceModel && !loaded.flight.engines.initialized,
           "v14 migration uses existing flight model and lazy engine migration");
    auto sample = sim::FlightSample::capture(state);
    std::ostringstream row; sample.write(row);
    sim::FlightSample parsed;
    auto line = row.str(); line.pop_back();
    expect(sim::FlightSample::parse(line, parsed) && parsed.finite(), "CSV numeric sample roundtrip");
    expect(!sim::FlightSample::parse(line+",", parsed), "CSV trailing empty field rejected");
    expect(!sim::FlightSample::parse("nan,broken", parsed), "Malformed/nonfinite CSV rejected");
    std::stringstream csv; csv << sim::FlightSample::header() << '\n';
    sample.values[0] = 0; sample.write(csv); sample.values[0] = 1; sample.write(csv);
    sim::FlightStatistics stats; std::string error;
    expect(sim::analyzeTelemetry(csv, stats, error) && stats.samples == 2 && near(stats.lastTime, 1),
           "CSV analyzer summarizes valid flight");
    std::stringstream reversed; reversed << sim::FlightSample::header() << '\n';
    sample.values[0] = 2; sample.write(reversed); sample.values[0] = 1; sample.write(reversed);
    expect(!sim::analyzeTelemetry(reversed, stats, error), "Nonmonotonic telemetry rejected");
}

void fullFlightTests() {
    for (int id = 0; id < 4; ++id) {
        sim::Snapshot state;
        auto& f = state.flight;
        f.configure(id); f.setSurfaceModel(true); f.flap = id ? .5 : 0;
        double maximumAltitude = f.y;
        for (int step = 0; step < 9000; ++step) {
            sim::Controls c;
            c.throttle = f.throttle < 1 ? 1 : 0;
            const double target = f.speed() < f.spec().rotationSpeed ? 0 : .12;
            c.pitch = std::clamp((target-f.pitch)*6-f.pitchRate*.9, -1.0, 1.0);
            f.step(.01, c);
            expect(!f.crashed && f.rotation.valid(), "Ninety-second surface-model takeoff remains valid");
            maximumAltitude = std::max(maximumAltitude, f.y);
        }
        expect(maximumAltitude > 200, "All four surface-model aircraft climb above 200 metres");
        std::cout << "surface_takeoff aircraft=" << id << " max_altitude_m=" << maximumAltitude << '\n';
    }
    sim::Snapshot state;
    auto& f = state.flight;
    f.configure(3); f.setSurfaceModel(true); f.y = 1500; f.z = 0;
    f.vz = 100; f.gear = false; f.throttle = .3; f.rpm = 1270;
    state.targetAltitude = 1500; state.headingHold = state.altitudeHold = true;
    double maxBank = 0;
    for (int step = 0; step < 9000; ++step) {
        if (step == 1500) f.engines.inject(0, sim::EngineFault::Flameout, 3);
        sim::Controls c;
        sim::updateAutopilot(state, c);
        f.step(.01, c); state.avionics.step(.01, f);
        expect(!f.crashed && f.surfaceResult.finite(), "Controlled single-engine failure remains finite and airborne");
        maxBank = std::max(maxBank, std::abs(f.roll));
    }
    expect(std::abs(f.y-1500) < 25 && maxBank < .1,
           "A350 scripted engine-out AP compensates yaw and maintains approximate altitude");
    state.avionics.master = false;
    sim::Controls c; sim::updateAutopilot(state, c);
    expect(!state.headingHold && !state.altitudeHold, "Power loss releases AP after engine-out flight");
}

int main() {
    try {
        rotationTests(); surfaceTests(); engineTests(); faultAndAlertTests();
        storageAndTelemetryTests(); fullFlightTests();
        std::cout << "PASS: rigid-body, panel aerodynamics, independent engines, faults, alerts, v15, CSV, four takeoffs, engine-out\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
