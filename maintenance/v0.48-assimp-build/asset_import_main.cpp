#include "src/asset_import.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--formats") {
            std::cout << aircraftImporterFormats() << '\n'; return 0;
        }
        if (argc < 3) {
            std::cout << "asset_import input_model output.fcmesh [--length metres] [--height metres] [--yaw degrees]\n"
                         "asset_import --formats\n";
            return 0;
        }
        AircraftImportOptions options;
        for (int i = 3; i < argc; ++i) {
            const std::string key = argv[i];
            if (++i >= argc) throw std::runtime_error("Missing option value");
            size_t end = 0;
            const std::string text = argv[i]; const double value = std::stod(text, &end);
            if (end != text.size() || !std::isfinite(value)) throw std::runtime_error("Invalid option number");
            if (key == "--length") options.lengthMeters = value;
            else if (key == "--height") options.groundOffset = value;
            else if (key == "--yaw") options.yawDegrees = value;
            else throw std::runtime_error("Unknown import option");
        }
        const auto result = importAircraftModel(argv[1], options);
        saveAircraftModel(result.model, argv[2]);
        writeImportReport(result.report, std::string(argv[2])+".report.txt");
        std::cout << "triangles=" << result.report.importedTriangles << " textures=" << result.report.decodedTextures << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "asset_import: " << e.what() << '\n'; return 1;
    }
}
