#include "src/autopilot.h"
#include "src/fault_scenarios.h"
#include "src/flight_alerts.h"
#include "src/telemetry.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

struct Options {
    int aircraft = 3;
    double seconds = 60;
    double failLeft = -1;
    double wind = 0;
    bool surface = false;
    bool takeoff = false;
    fs::path output = "flight_lab_output";
};

double numeric(const std::string& text) {
    size_t end = 0;
    const double value = std::stod(text, &end);
    if (end != text.size() || !std::isfinite(value)) throw std::runtime_error("invalid numeric argument");
    return value;
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 2; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--surface") { options.surface = true; continue; }
        if (key == "--takeoff") { options.takeoff = true; continue; }
        if (++i >= argc) throw std::runtime_error("missing argument value");
        const std::string value = argv[i];
        if (key == "--aircraft") {
            const double id = numeric(value);
            if (id < 0 || id >= sim::aircraftCount || id != std::floor(id)) throw std::runtime_error("aircraft must be 0..9");
            options.aircraft = static_cast<int>(id);
        } else if (key == "--seconds") options.seconds = numeric(value);
        else if (key == "--fail-left") options.failLeft = numeric(value);
        else if (key == "--wind") options.wind = numeric(value);
        else if (key == "--output") options.output = value;
        else throw std::runtime_error("unknown option: " + key);
    }
    if (options.seconds < 1 || options.seconds > 3600 ||
        std::abs(options.wind) > 30 || options.failLeft < -1 ||
        options.failLeft > options.seconds) throw std::runtime_error("scenario parameter outside range");
    return options;
}

void requireStream(const std::ios& stream) {
    if (!stream) throw std::runtime_error("cannot write laboratory output");
}

void runSweep(const fs::path& folder) {
    fs::create_directories(folder);
    std::ofstream out(folder / "surface_sweep.csv");
    requireStream(out);
    out << "aircraft,flap,gear,aoa_deg,speed_mps,lift_N,drag_N,pitch_moment_Nm,left_lift_N,right_lift_N,separation\n";
    for (int id = 0; id < sim::aircraftCount; ++id) {
        for (double flap : {0.0, .5, 1.0}) {
            for (bool gear : {false, true}) {
                for (int angle = -20; angle <= 40; ++angle) {
                    const double alpha = angle * sim::pi / 180;
                    const double speed = id == 0 ? 45 : 100;
                    sim::SurfaceInput input;
                    input.aircraftId = id;
                    input.bodyAirVelocity = {0, -std::sin(alpha) * speed, std::cos(alpha) * speed};
                    input.density = sim::atmosphere(1500).density;
                    input.groundHeight = 1500;
                    input.flap = flap;
                    input.gear = gear;
                    const auto r = sim::solveSurfaces(input);
                    if (!r.finite()) throw std::runtime_error("nonfinite aerodynamic sweep");
                    out << id << ',' << flap << ',' << gear << ',' << angle << ',' << speed << ','
                        << r.totalLift << ',' << r.totalDrag << ',' << r.total.moment.x << ','
                        << r.leftLift << ',' << r.rightLift << ',' << r.meanSeparation << '\n';
                }
            }
        }
    }
    out.flush();
    requireStream(out);
    std::cout << "Wrote " << (folder / "surface_sweep.csv").string() << '\n';
}

bool runScenario(const Options& options) {
    fs::create_directories(options.output);
    std::ofstream csv(options.output / "flight.csv");
    std::ofstream surfaces(options.output / "surfaces.csv");
    std::ofstream events(options.output / "events.txt");
    requireStream(csv); requireStream(surfaces); requireStream(events);
    csv << sim::FlightSample::header() << '\n';
    surfaces << "sim_s,surface,kind,x_m,y_m,z_m,airspeed_mps,aoa_deg,beta_deg,cl,cd,lift_N,drag_N,fx_N,fy_N,fz_N,mx_Nm,my_Nm,mz_Nm\n";

    sim::Snapshot state;
    auto& f = state.flight;
    f.configure(options.aircraft);
    f.setSurfaceModel(options.surface);
    f.windX = options.wind;
    if (options.takeoff) {
        f.flap = options.aircraft ? .5 : 0;
    } else {
        f.y = 1500;
        f.z = 0;
        f.gear = false;
        f.vz = options.aircraft ? 100 : 48;
        f.throttle = options.aircraft ? .3 : .65;
        f.rpm = 700 + f.throttle * 1900;
        state.targetAltitude = f.y;
        state.targetHeading = 0;
        state.altitudeHold = state.headingHold = true;
    }
    sim::FaultSchedule faults;
    if (options.failLeft >= 0) {
        if (!faults.set({{options.failLeft, 0, sim::EngineFault::Flameout}}, options.aircraft)) {
            throw std::runtime_error("invalid fault schedule");
        }
    }
    sim::AlertManager alerts;
    sim::FlightStatistics stats;
    const int steps = static_cast<int>(options.seconds * 100);
    const auto initial = sim::FlightSample::capture(state);
    initial.write(csv); stats.add(initial);
    size_t priorAlerts = 0;
    double maxAltitude = f.y;
    for (int step = 0; step < steps && !f.crashed; ++step) {
        if (faults.apply(f.time, f)) events << "fault at " << f.time << " seconds\n";
        sim::Controls controls;
        if (options.takeoff) {
            controls.throttle = f.throttle < 1 ? 1 : 0;
            const double target = f.speed() < f.spec().rotationSpeed ? 0 : .12;
            controls.pitch = std::clamp((target-f.pitch)*6-f.pitchRate*.9, -1.0, 1.0);
        } else {
            sim::updateAutopilot(state, controls);
        }
        f.step(.01, controls);
        state.avionics.step(.01, f);
        alerts.update(state);
        maxAltitude = std::max(maxAltitude, f.y);
        if (alerts.active.size() != priorAlerts) {
            events << "t=" << f.time << " active_alerts=" << alerts.active.size() << '\n';
            priorAlerts = alerts.active.size();
        }
        if ((step+1) % 10 == 0 || f.crashed) {
            const auto sample = sim::FlightSample::capture(state);
            if (!stats.add(sample)) throw std::runtime_error("nonfinite scenario sample");
            sample.write(csv);
            if (options.surface) sim::writeSurfaceSamples(surfaces, f.time, f.surfaceResult);
        }
    }
    std::ofstream summary(options.output / "summary.txt");
    summary << "model=" << (options.surface ? "surface-rigid" : "established") << '\n'
            << "aircraft=" << options.aircraft << '\n'
            << "requested_seconds=" << options.seconds << '\n'
            << "max_true_altitude_m=" << maxAltitude << '\n';
    stats.write(summary);
    if (f.crashed) summary << "crash_reason=" << f.crashReason << '\n';
    std::ofstream save(options.output / "final_state.txt");
    state.write(save);
    for (auto* file : {&csv, &surfaces, &events, &summary, &save}) {
        file->flush(); requireStream(*file);
    }
    stats.write(std::cout);
    std::cout << "Output: " << options.output.string() << '\n';
    if (f.crashed) std::cout << "CRASH: " << f.crashReason << '\n';
    return !f.crashed;
}

void analyzeFile(const fs::path& input, const fs::path& report) {
    std::ifstream csv(input);
    if (!csv) throw std::runtime_error("cannot open telemetry CSV");
    sim::FlightStatistics stats;
    std::string error;
    if (!sim::analyzeTelemetry(csv, stats, error)) throw std::runtime_error(error);
    std::ofstream output(report);
    stats.write(output);
    output.flush(); requireStream(output);
    stats.write(std::cout);
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cout << "flight_lab sweep [output_directory]\n"
                         "flight_lab scenario [--aircraft 0..9] [--seconds 60] [--surface]\n"
                         "  [--takeoff] [--fail-left seconds] [--wind mps] [--output directory]\n"
                         "flight_lab analyze input.csv report.txt\n";
            return 0;
        }
        const std::string command = argv[1];
        if (command == "sweep" && argc <= 3) runSweep(argc == 3 ? argv[2] : "flight_lab_output");
        else if (command == "scenario") return runScenario(parseOptions(argc, argv)) ? 0 : 2;
        else if (command == "analyze" && argc == 4) analyzeFile(argv[2], argv[3]);
        else throw std::runtime_error("invalid command or argument count");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "flight_lab: " << e.what() << '\n';
        return 1;
    }
}
