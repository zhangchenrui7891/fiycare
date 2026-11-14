#pragma once

#include "aircraft.h"
#include <algorithm>
#include <string>

namespace sim {

struct FleetAsset {
    int aircraftId;
    const char* nativeName;
    const char* sourceName;
    const wchar_t* credit;
    double lengthMeters;
    double bodyGroundOffset;
    double yawDegrees;
};

inline const FleetAsset* fleetAsset(int id) {
    // Registered visual assets and flight parameters are separate on purpose.
    // Downloading geometry does not supply certified aircraft performance data.
    static constexpr FleetAsset assets[] = {
        {2, "a330.fcmesh", "a330-300_plane.glb", L"用户提供A330模型", 63.66, 2.8, 0},
        {3, "a350.fcmesh", "airbus_a350-900.glb", L"用户提供A350模型", 66.8, 7.5, 180},
        {4, "a320.fcmesh", "amvlab/A320_nologo.glb", L"amvlab · CC BY 4.0", 37.57, 2.5, -90},
        {5, "b737.fcmesh", "amvlab/B737_nologo.glb", L"amvlab · CC BY 4.0", 39.47, 2.0, -90},
        {6, "b787.fcmesh", "amvlab/B787_nologo.glb", L"amvlab · CC BY 4.0", 62.8, 3.8, -90},
        {7,"a319.fcmesh","amvlab/A320_nologo.glb",L"amvlab CC BY 4.0 · A320缩短近似",33.84,2.5,-90},
        {8,"a321.fcmesh","amvlab/A320_nologo.glb",L"amvlab CC BY 4.0 · A320加长近似",44.51,2.5,-90},
        {9,"b7879.fcmesh","amvlab/B787_nologo.glb",L"amvlab CC BY 4.0 · B787系列近似",62.8,3.8,-90},
        {10,"b777.fcmesh","flightgear777/777-300ER.ac",L"FlightGear 777 authors · GPL v2",73.86,5.5,-90}
    };
    for (const auto& asset : assets) if (asset.aircraftId == id) return &asset;
    return nullptr;
}

inline double visualLength(int id) {
    if (const auto* asset = fleetAsset(id)) return asset->lengthMeters;
    return id == 0 ? 7.0 : 30.0;
}

} // namespace sim
