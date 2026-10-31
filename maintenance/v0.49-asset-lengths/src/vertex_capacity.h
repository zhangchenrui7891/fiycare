#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

inline size_t growVertexCapacity(size_t current, size_t required, size_t stride) {
    if (!stride) throw std::runtime_error("Invalid vertex stride");
    const size_t maximum = std::numeric_limits<uint32_t>::max() / stride;
    if (required > maximum) throw std::runtime_error("Vertex buffer exceeds DirectX byte range");
    if (required <= current) return current;
    size_t grown = std::max(size_t(4096), current);
    while (grown < required) {
        const size_t increment = std::max(size_t(4096), grown/2);
        grown = increment > maximum-grown ? maximum : grown+increment;
    }
    return grown;
}
