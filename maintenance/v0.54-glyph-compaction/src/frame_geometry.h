#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace presentation {

struct IndexedScene {
    std::vector<std::uint32_t> indices;
    std::size_t firstVertex=0, vertexCount=0;

    void clear() {
        indices.clear();
        firstVertex=vertexCount=0;
    }

    template<class Vertex, class Model, class Transform>
    void append(std::vector<Vertex>& scene, const Model& model, bool gearDown, Transform transform) {
        firstVertex=scene.size();
        vertexCount=model.uniqueVertices.size();
        for (const auto& vertex: model.uniqueVertices) scene.push_back(transform(vertex));
        indices.reserve(model.indices.size());
        for (std::size_t triangle=0; triangle<model.gear.size(); ++triangle) {
            if (!gearDown && model.gear[triangle]) continue;
            const auto first=model.indices.begin()+triangle*3;
            indices.insert(indices.end(),first,first+3);
        }
    }
};

// The render thread owns these buffers. Clearing a frame retains capacity;
// the asynchronous terrain mesh remains a separate, immutable source.
template<class Vertex> struct FrameGeometry {
    std::vector<Vertex> scene, cabin, hud;
    IndexedScene indexedScene;

    void beginFrame(const std::vector<Vertex>& terrain) {
        scene.assign(terrain.begin(), terrain.end());
        cabin.clear();
        hud.clear();
        indexedScene.clear();
    }

    std::size_t retainedBytes() const {
        return (scene.capacity()+cabin.capacity()+hud.capacity())*sizeof(Vertex)
            +indexedScene.indices.capacity()*sizeof(std::uint32_t);
    }
};

template<class Vertex> struct VertexSpan {
    const Vertex* data=nullptr;
    std::size_t size=0;
    VertexSpan()=default;
    VertexSpan(const Vertex* pointer, std::size_t count):data(pointer),size(count) {}
    VertexSpan(const std::vector<Vertex>& vertices):data(vertices.data()),size(vertices.size()) {}
    template<std::size_t N> VertexSpan(const Vertex (&vertices)[N]):data(vertices),size(N) {}
};

struct VertexBatchLayout {
    std::size_t scene=0, cabin=0, hud=0, total=0;
};

// Four borrowed ranges, no intermediate concatenation. Construct only after
// building the frame and consume before changing any source vector. The byte
// bound matches the UINT size used by both DirectX vertex-buffer paths.
template<class Vertex> class VertexBatch {
    static_assert(std::is_trivially_copyable<Vertex>::value, "Vertex upload requires plain data");
    std::array<VertexSpan<Vertex>,4> spans;
public:
    VertexBatchLayout layout;

    VertexBatch(VertexSpan<Vertex> sky, VertexSpan<Vertex> scene,
                VertexSpan<Vertex> cabin, VertexSpan<Vertex> hud)
        :spans{{sky,scene,cabin,hud}} {
        constexpr auto maximum=std::numeric_limits<std::uint32_t>::max()/sizeof(Vertex);
        for (const auto& span:spans) {
            if (span.size && !span.data) throw std::invalid_argument("Null vertex source");
            if (span.size>maximum-layout.total)
                throw std::length_error("Vertex batch exceeds DirectX byte range");
            layout.total+=span.size;
        }
        layout.scene=sky.size;
        layout.cabin=layout.scene+scene.size;
        layout.hud=layout.cabin+cabin.size;
    }

    std::size_t bytes() const { return layout.total*sizeof(Vertex); }

    void copyTo(void* destination, std::size_t capacityVertices) const {
        if (capacityVertices<layout.total)
            throw std::length_error("Vertex upload destination is too small");
        if (!layout.total) return;
        if (!destination) throw std::invalid_argument("Null vertex upload destination");
        auto* next=static_cast<unsigned char*>(destination);
        for (const auto& span:spans) if (span.size) {
            const auto count=span.size*sizeof(Vertex);
            std::memcpy(next,span.data,count);
            next+=count;
        }
    }
};

} // namespace presentation
