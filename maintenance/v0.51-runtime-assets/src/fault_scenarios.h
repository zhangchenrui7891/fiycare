#pragma once

#include "simulation.h"
#include <algorithm>
#include <vector>

namespace sim {

struct ScheduledEngineFault {
    double time = 0.0;
    int engineIndex = 0;
    EngineFault fault = EngineFault::Flameout;
};

// Used by headless regression scenarios, independent of the rendering frame rate.
struct FaultSchedule {
    std::vector<ScheduledEngineFault> events;
    size_t next = 0;

    bool set(std::vector<ScheduledEngineFault> input, int aircraftId) {
        for (const auto& event : input) {
            const int code = static_cast<int>(event.fault);
            if (!std::isfinite(event.time) || event.time < 0 ||
                event.engineIndex < 0 || event.engineIndex >= (aircraftId == 0 ? 1 : 2) ||
                code < 0 || code > 3) return false;
        }
        std::stable_sort(input.begin(), input.end(), [](const auto& a, const auto& b) {
            return a.time < b.time;
        });
        events = std::move(input);
        next = 0;
        return true;
    }

    size_t apply(double simulationTime, Flight& flight) {
        size_t applied = 0;
        while (next < events.size() && events[next].time <= simulationTime + 1e-9) {
            const auto& event = events[next++];
            flight.engines.inject(event.engineIndex, event.fault, flight.aircraftId);
            ++applied;
        }
        return applied;
    }

    bool complete() const { return next == events.size(); }
};

} // namespace sim
