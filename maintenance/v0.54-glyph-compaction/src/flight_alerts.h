#pragma once

#include "storage.h"
#include <algorithm>
#include <string>
#include <vector>

namespace sim {

enum class AlertLevel { Advisory, Caution, Warning };

struct FlightAlert {
    std::string id;
    std::wstring text;
    AlertLevel level = AlertLevel::Advisory;
    bool acknowledged = false;
};

inline std::vector<FlightAlert> collectFlightAlerts(const Snapshot& state) {
    const auto& f = state.flight;
    std::vector<FlightAlert> alerts;
    auto add = [&](const char* id, const wchar_t* text, AlertLevel level) {
        alerts.push_back({id, text, level, false});
    };

    for (int i = 0; i < f.engines.count(f.aircraftId); ++i) {
        const auto& engine = f.engines.units[static_cast<size_t>(i)];
        if (engine.fault == EngineFault::Fire) {
            add(i == 0 ? "engine1_fire" : "engine2_fire",
                i == 0 ? L"一号发动机火警" : L"二号发动机火警", AlertLevel::Warning);
        } else if (engine.fault == EngineFault::Flameout) {
            add(i == 0 ? "engine1_fail" : "engine2_fail",
                i == 0 ? L"一号发动机失效" : L"二号发动机失效", AlertLevel::Warning);
        }
        if (engine.fault == EngineFault::Generator) {
            add(i == 0 ? "generator1" : "generator2",
                i == 0 ? L"一号发电机故障" : L"二号发电机故障", AlertLevel::Caution);
        }
        if (f.engines.initialized && engine.rpm > 1000 && engine.egt > 900) {
            add(i == 0 ? "egt1" : "egt2",
                i == 0 ? L"一号排气温度过高" : L"二号排气温度过高", AlertLevel::Warning);
        }
    }
    if (!state.avionics.busPowered(f)) add("bus", L"电气母线失电", AlertLevel::Warning);
    else if (!state.avionics.master) add("avionics", L"航电主开关关闭", AlertLevel::Advisory);
    if (f.stallWarning && !f.grounded()) add("stall", L"失速警告：减小迎角", AlertLevel::Warning);
    if (f.fuel < 10) add("fuel", L"燃油不足百分之十", AlertLevel::Caution);
    if (!state.avionics.generating(f) && state.avionics.battery && state.avionics.charge < .15) {
        add("battery", L"电池电量低", AlertLevel::Caution);
    }
    const double agl = f.y - terrain(f.x, f.z);
    if (!f.grounded() && agl < 150 && f.vy < -1.0 && !f.gear) {
        add("gear", L"低空下降：检查起落架", AlertLevel::Warning);
    }
    if (!f.grounded() && agl < 150 && f.vy < -6.0) {
        add("sink", L"下降率过大", AlertLevel::Warning);
    }
    std::stable_sort(alerts.begin(), alerts.end(), [](const FlightAlert& a, const FlightAlert& b) {
        return static_cast<int>(a.level) > static_cast<int>(b.level);
    });
    return alerts;
}

// Acknowledgement silences the attention state but never clears an active fault.
// A resolved fault is removed; if it happens again it needs a new acknowledgement.
struct AlertManager {
    std::vector<FlightAlert> active;

    // A different loaded/restarted flight must not inherit old acknowledgements.
    void beginFlight(const Snapshot& state) { active = collectFlightAlerts(state); }

    void update(const Snapshot& state) {
        auto next = collectFlightAlerts(state);
        for (auto& alert : next) {
            const auto old = std::find_if(active.begin(), active.end(), [&](const FlightAlert& a) {
                return a.id == alert.id && a.level == alert.level;
            });
            if (old != active.end()) alert.acknowledged = old->acknowledged;
        }
        active = std::move(next);
    }

    // Highest severity first; within that severity bring a newly unacknowledged
    // fault forward. Keep collection order as a stable tie-breaker.
    const FlightAlert* headline() const {
        const FlightAlert* selected = nullptr;
        for (const auto& alert : active) {
            if (!selected || static_cast<int>(alert.level) > static_cast<int>(selected->level) ||
                (alert.level == selected->level && selected->acknowledged && !alert.acknowledged))
                selected = &alert;
        }
        return selected;
    }

    void acknowledge() {
        for (auto& alert : active) alert.acknowledged = true;
    }

    bool attention() const {
        for (const auto& alert : active) {
            if (!alert.acknowledged && alert.level != AlertLevel::Advisory) return true;
        }
        return false;
    }
};

struct ChecklistItem {
    std::wstring label;
    bool complete;
};

inline std::vector<ChecklistItem> takeoffChecklist(const Snapshot& state) {
    const auto& f = state.flight;
    bool enginesReady = true;
    for (int i = 0; i < f.engines.count(f.aircraftId); ++i) {
        const auto& engine = f.engines.units[static_cast<size_t>(i)];
        enginesReady = enginesReady && engine.enabled &&
                       engine.fault != EngineFault::Fire &&
                       engine.fault != EngineFault::Flameout &&
                       (f.engines.initialized ? engine.rpm >= 650 : f.rpm >= 650);
    }
    enginesReady = enginesReady && f.engine && f.fuel > 10;
    return {{L"航电供电", state.avionics.powered(f)},
            {L"发动机及燃油", enginesReady},
            {L"起落架放下", f.gear},
            {L"客机起飞襟翼", f.aircraftId == 0 || (f.flap >= .25 && f.flap <= .5)},
            {L"停留刹车释放", !state.parkingBrake},
            {L"配平范围", std::abs(f.trim) <= .25}};
}

} // namespace sim
