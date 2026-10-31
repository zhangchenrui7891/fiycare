#include "asset_import.h"
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <memory>
#include <unordered_map>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 8192
#include "../third_party/assimp/contrib/stb/stb_image.h"

namespace {
struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;

    aiColor4D sample(aiVector3D uv) const {
        if (!width || !height || pixels.empty()) return {1, 1, 1, 1};
        const float u = uv.x - std::floor(uv.x);
        const float v = uv.y - std::floor(uv.y);
        const int x = std::clamp(int(u * width), 0, width-1);
        const int y = std::clamp(int((1-v) * height), 0, height-1);
        const size_t i = (size_t(y)*width+x)*4;
        return {pixels[i]/255.f, pixels[i+1]/255.f, pixels[i+2]/255.f, pixels[i+3]/255.f};
    }
};

Image decode(const aiScene* scene, const aiString& name, const std::filesystem::path& base) {
    Image image;
    int channels = 0;
    unsigned char* data = nullptr;
    if (const aiTexture* embedded = scene->GetEmbeddedTexture(name.C_Str())) {
        if (embedded->mHeight) {
            if (embedded->mWidth > 8192 || embedded->mHeight > 8192) return image;
            image.width = int(embedded->mWidth);
            image.height = int(embedded->mHeight);
            image.pixels.resize(size_t(image.width)*image.height*4);
            for (size_t i = 0; i < size_t(image.width)*image.height; ++i) {
                image.pixels[i*4] = embedded->pcData[i].r;
                image.pixels[i*4+1] = embedded->pcData[i].g;
                image.pixels[i*4+2] = embedded->pcData[i].b;
                image.pixels[i*4+3] = embedded->pcData[i].a;
            }
            return image;
        }
        if (embedded->mWidth <= size_t(std::numeric_limits<int>::max())) {
            data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(embedded->pcData),
                                        int(embedded->mWidth), &image.width, &image.height, &channels, 4);
        }
    } else {
        // Only resolve textures within the source directory; never fetch remote URIs.
        std::string relative = name.C_Str();
        std::replace(relative.begin(), relative.end(), '\\', '/');
        const auto requested = std::filesystem::u8path(relative);
        bool parent = requested.is_absolute();
        for (const auto& component : requested) if (component == "..") parent = true;
        if (!parent && relative.find("://") == std::string::npos) {
            std::ifstream in(base/requested, std::ios::binary);
            std::vector<unsigned char> encoded;
            if (in) {
                in.seekg(0, std::ios::end);
                const auto size = in.tellg();
                if (size > 0 && size < 64*1024*1024) {
                    encoded.resize(size_t(size)); in.seekg(0);
                    in.read(reinterpret_cast<char*>(encoded.data()), std::streamsize(encoded.size()));
                    if (in) data = stbi_load_from_memory(encoded.data(), int(encoded.size()), &image.width, &image.height, &channels, 4);
                }
            }
        }
    }
    if (data) {
        image.pixels.assign(data, data+size_t(image.width)*image.height*4);
        stbi_image_free(data);
    }
    return image;
}

aiColor4D multiplyColor(aiColor4D a,aiColor4D b){return {a.r*b.r,a.g*b.g,a.b*b.b,a.a*b.a};}

bool gearName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return name.find("landinggear") != std::string::npos || name.find("landing_gear") != std::string::npos ||
           name.find("wheel") != std::string::npos || name.find("tire") != std::string::npos;
}

void putU32(std::ostream& out, uint32_t value) {
    unsigned char bytes[4] = {static_cast<unsigned char>(value), static_cast<unsigned char>(value >> 8),
                              static_cast<unsigned char>(value >> 16), static_cast<unsigned char>(value >> 24)};
    out.write(reinterpret_cast<const char*>(bytes), 4);
}
}

AircraftImportResult importAircraftModel(const std::filesystem::path& path,
                                        const AircraftImportOptions& options) {
    if (!std::isfinite(options.lengthMeters) || options.lengthMeters < 1 || options.lengthMeters > 150 ||
        !std::isfinite(options.groundOffset) || options.groundOffset < 0 || options.groundOffset > 20 ||
        !std::isfinite(options.yawDegrees) || !options.maximumTriangles || options.maximumTriangles > 1000000) {
        throw std::runtime_error("Invalid aircraft import options");
    }
    const auto size = std::filesystem::file_size(path);
    if (size == 0 || size > 256*1024*1024) throw std::runtime_error("Model file outside supported size range");
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    const unsigned flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                           aiProcess_PreTransformVertices | aiProcess_SortByPType |
                           aiProcess_ValidateDataStructure | aiProcess_FindInvalidData;
    const aiScene* scene = importer.ReadFile(path.u8string(), flags);
    if (!scene || !scene->HasMeshes()) throw std::runtime_error(std::string("Assimp: ") + importer.GetErrorString());

    AircraftImportResult result;
    auto& report = result.report;
    report.source = path.filename().u8string();
    report.format = path.extension().string();
    report.sourceMeshes = scene->mNumMeshes;
    report.sourceMaterials = scene->mNumMaterials;
    const double angle = options.yawDegrees * 3.141592653589793 / 180;
    auto rotate = [&](aiVector3D v) {
        return aiVector3D(float(v.x*std::cos(angle)+v.z*std::sin(angle)), v.y,
                          float(-v.x*std::sin(angle)+v.z*std::cos(angle)));
    };
    aiVector3D low(1e20f, 1e20f, 1e20f), high(-1e20f, -1e20f, -1e20f);
    size_t triangles = 0;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            const auto p = rotate(mesh->mVertices[i]);
            low.x = std::min(low.x, p.x); low.y = std::min(low.y, p.y); low.z = std::min(low.z, p.z);
            high.x = std::max(high.x, p.x); high.y = std::max(high.y, p.y); high.z = std::max(high.z, p.z);
        }
        triangles += mesh->mNumFaces;
    }
    if (!triangles || triangles > options.maximumTriangles) throw std::runtime_error("Model triangle budget exceeded");
    const double length = high.z-low.z;
    if (!std::isfinite(length) || length < 1e-6) throw std::runtime_error("Degenerate model length; choose a different orientation");
    report.originalLength = length;
    report.scale = options.lengthMeters / length;
    const aiVector3D origin((low.x+high.x)*.5f, low.y, (low.z+high.z)*.5f);
    std::unordered_map<unsigned, Image> images;
    result.model.vertices.reserve(triangles*3);
    result.model.gear.reserve(triangles);

    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        if(!mesh->HasNormals())throw std::runtime_error("Model has no valid normals");
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D base(1, 1, 1, 1);
        if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &base) != AI_SUCCESS) {
            aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &base);
        }
        aiString texture;
        bool hasTexture = material->GetTexture(aiTextureType_BASE_COLOR, 0, &texture) == AI_SUCCESS;
        if (!hasTexture) hasTexture = material->GetTexture(aiTextureType_DIFFUSE, 0, &texture) == AI_SUCCESS;
        if (options.bakeTextures && hasTexture && !images.count(mesh->mMaterialIndex)) {
            auto image = decode(scene, texture, path.parent_path());
            if (!image.pixels.empty()) ++report.decodedTextures;
            else report.warnings += "Texture could not be decoded; material base color retained. ";
            images.emplace(mesh->mMaterialIndex, std::move(image));
        }
        const auto image = images.find(mesh->mMaterialIndex);
        const bool gear = gearName(mesh->mName.C_Str());
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const auto& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) throw std::runtime_error("Importer returned a non-triangle face");
            std::array<Vertex, 3> triangle{};
            bool opaque = false;
            for (unsigned corner = 0; corner < 3; ++corner) {
                const unsigned i = face.mIndices[corner];
                auto p = (rotate(mesh->mVertices[i])-origin)*float(report.scale);
                p.y -= float(options.groundOffset);
                auto n = rotate(mesh->mNormals[i]); if(n.SquareLength()<1e-12)throw std::runtime_error("Degenerate surface normal");n.Normalize();
                aiColor4D color = base;
                if (mesh->HasVertexColors(0)) color = multiplyColor(color,mesh->mColors[0][i]);
                if (image != images.end() && mesh->HasTextureCoords(0)) color = multiplyColor(color,image->second.sample(mesh->mTextureCoords[0][i]));
                if(!std::isfinite(color.r)||!std::isfinite(color.g)||!std::isfinite(color.b)||!std::isfinite(color.a))throw std::runtime_error("Invalid material color");
                opaque = opaque || color.a > .1f;
                triangle[corner] = {p.x, p.y, p.z, std::clamp(color.r, 0.f, 1.f),
                                    std::clamp(color.g, 0.f, 1.f), std::clamp(color.b, 0.f, 1.f),
                                    0, 0, 0, n.x, n.y, n.z};
            }
            if (!opaque) continue;
            for (const auto& vertex : triangle) {
                if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z) ||
                    !std::isfinite(vertex.nx) || !std::isfinite(vertex.ny) || !std::isfinite(vertex.nz)) {
                    throw std::runtime_error("Nonfinite imported vertex");
                }
                result.model.vertices.push_back(vertex);
            }
            result.model.gear.push_back(gear ? 1 : 0);
            if (gear) ++report.gearTriangles;
        }
    }
    report.importedTriangles = result.model.gear.size();
    if (!report.importedTriangles) throw std::runtime_error("No visible triangles after importing");
    if (!report.gearTriangles) report.warnings += "No separate landing-gear nodes recognized; game may use a supplemental gear model. ";
    report.warnings += "Textures baked to vertex colors; animation, cockpit systems and flight data are not imported. ";
    return result;
}

void saveAircraftModel(const ImportedModel& model, const std::filesystem::path& path) {
    if (model.vertices.empty() || model.vertices.size()%3 || model.gear.size()!=model.vertices.size()/3) {
        throw std::runtime_error("Cannot save inconsistent model");
    }
    std::ofstream out(path, std::ios::binary);
    const bool atlas=std::all_of(model.vertices.begin(),model.vertices.end(),[](const Vertex& vertex){return vertex.textured==1;});
    const bool colored=std::all_of(model.vertices.begin(),model.vertices.end(),[](const Vertex& vertex){return vertex.textured==0;});
    if(!atlas&&!colored)throw std::runtime_error("Mixed unsupported model materials");
    out.write(atlas?"FCMESH06":"FCMESH07", 8);
    putU32(out, uint32_t(model.vertices.size())); putU32(out, 12); putU32(out, uint32_t(model.gear.size()));
    out.write(reinterpret_cast<const char*>(model.vertices.data()), std::streamsize(model.vertices.size()*sizeof(Vertex)));
    out.write(reinterpret_cast<const char*>(model.gear.data()), std::streamsize(model.gear.size()));
    out.flush();
    if (!out) throw std::runtime_error("Model output write failed");
}

void writeImportReport(const AircraftImportReport& report, const std::filesystem::path& path) {
    std::ofstream out(path);
    out << std::setprecision(12) << "source=" << report.source << '\n' << "format=" << report.format << '\n'
        << "meshes=" << report.sourceMeshes << '\n' << "materials=" << report.sourceMaterials << '\n'
        << "triangles=" << report.importedTriangles << '\n' << "textures_decoded=" << report.decodedTextures << '\n'
        << "gear_triangles=" << report.gearTriangles << '\n' << "original_length=" << report.originalLength << '\n'
        << "scale=" << report.scale << '\n' << "warnings=" << report.warnings << '\n';
    out.flush();
    if (!out) throw std::runtime_error("Import report write failed");
}

std::string aircraftImporterFormats() {
    Assimp::Importer importer;
    aiString formats; importer.GetExtensionList(formats);
    return formats.C_Str();
}
