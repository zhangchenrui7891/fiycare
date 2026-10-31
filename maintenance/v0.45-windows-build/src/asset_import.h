#pragma once

#include "assets.h"
#include <filesystem>
#include <string>

struct AircraftImportOptions {
    double lengthMeters = 37.57;
    double groundOffset = 4.0;
    // Rotation around source Y after node transforms; native nose points +Z.
    double yawDegrees = 0.0;
    size_t maximumTriangles = 1000000;
    bool bakeTextures = true;
};

struct AircraftImportReport {
    std::string source;
    std::string format;
    size_t sourceMeshes = 0;
    size_t importedTriangles = 0;
    size_t sourceMaterials = 0;
    size_t decodedTextures = 0;
    size_t gearTriangles = 0;
    double originalLength = 0;
    double scale = 1;
    std::string warnings;
};

struct AircraftImportResult {
    ImportedModel model;
    AircraftImportReport report;
};

AircraftImportResult importAircraftModel(const std::filesystem::path& path,
                                        const AircraftImportOptions& options);
void saveAircraftModel(const ImportedModel& model, const std::filesystem::path& path);
void writeImportReport(const AircraftImportReport& report, const std::filesystem::path& path);
std::string aircraftImporterFormats();
