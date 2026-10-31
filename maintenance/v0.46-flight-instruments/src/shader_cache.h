#pragma once
#include <cstddef>
#include <cstdint>
#include <list>
#include <stdexcept>
#include <string>

namespace compilation {

// Full source text is part of identity: no hash collisions or pointer-identity
// assumptions. Defines/includes are not supported by the current app compiler.
struct ShaderRequest {
    std::string source, entry, profile;
    std::uint32_t flags1=0, flags2=0;
    bool operator==(const ShaderRequest& other) const {
        return source==other.source && entry==other.entry && profile==other.profile &&
               flags1==other.flags1 && flags2==other.flags2;
    }
};

struct ShaderCacheStats {
    std::size_t requests=0, hits=0, compilations=0, failures=0, evictions=0;
};

// Render-thread-only, process-local cache of successful compiler-owned handles.
// These are CPU bytecode blobs, never device-specific shaders or GPU resources.
// Return by value so callers retain ownership even after LRU eviction/clear.
template<class Bytecode> class ShaderCache {
    struct Entry {ShaderRequest request; Bytecode bytecode;};
    std::list<Entry> entries;
    std::size_t limit;
    ShaderCacheStats counters;
public:
    explicit ShaderCache(std::size_t capacity=8):limit(capacity) {
        if(!capacity)throw std::invalid_argument("Shader cache capacity must be positive");
    }
    const ShaderCacheStats& stats() const {return counters;}
    std::size_t size() const {return entries.size();}
    void clear() {entries.clear();} // Existing returned handles remain valid.

    template<class Compiler> Bytecode get(const ShaderRequest& request, Compiler compile) {
        if(request.source.empty()||request.entry.empty()||request.profile.empty())
            throw std::invalid_argument("Incomplete shader compilation request");
        ++counters.requests;
        for(auto it=entries.begin();it!=entries.end();++it) if(it->request==request) {
            auto bytecode=it->bytecode;
            entries.splice(entries.end(),entries,it);
            ++counters.hits;
            return bytecode;
        }
        ++counters.compilations;
        try {
            auto bytecode=compile();
            if(!bytecode)throw std::runtime_error("Compiler returned no shader bytecode");
            entries.push_back({request,bytecode});
            if(entries.size()>limit) {entries.pop_front();++counters.evictions;}
            return bytecode;
        } catch(...) {++counters.failures;throw;}
    }
};

} // namespace compilation
