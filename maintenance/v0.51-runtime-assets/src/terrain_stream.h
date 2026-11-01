#pragma once
#include <chrono>
#include <exception>
#include <future>
#include <optional>
#include <string>
#include <utility>

namespace presentation {
struct TerrainKey {
    int x=0, z=0, season=0, mode=0;
    bool operator==(const TerrainKey& other) const {
        return x==other.x && z==other.z && season==other.season && mode==other.mode;
    }
};
enum class TerrainUpdate { Unchanged, Applied, Discarded, Failed };

// One worker, one published mesh. Only the render thread touches published data.
// A completed job must still match the latest request before it can be shown.
template<class Mesh> class TerrainStream {
    std::future<Mesh> pending;
    std::optional<TerrainKey> pendingKey, publishedKey, failedKey;
    Mesh published;
    std::string error;
public:
    const Mesh& mesh() const { return published; }
    const std::optional<TerrainKey>& key() const { return publishedKey; }
    const std::string& lastError() const { return error; }
    bool busy() const { return pending.valid(); }

    // Bootstrap waits once, as before, so the first frame has ground. Subsequent
    // loads never wait for the worker; failures retain the last usable mesh.
    template<class Builder>
    TerrainUpdate update(TerrainKey wanted, Builder builder) {
        TerrainUpdate result=TerrainUpdate::Unchanged;
        auto receive=[&]() {
            const auto completed=*pendingKey;
            pendingKey.reset();
            try {
                auto next=pending.get();
                if(completed==wanted) {
                    published=std::move(next); publishedKey=completed;
                    failedKey.reset(); error.clear();
                    result=TerrainUpdate::Applied;
                } else result=TerrainUpdate::Discarded;
            } catch(const std::exception& failure) {
                failedKey=completed; error=failure.what();
                result=TerrainUpdate::Failed;
            } catch(...) {
                failedKey=completed; error="Unknown terrain generation failure";
                result=TerrainUpdate::Failed;
            }
        };
        if(pending.valid() && pending.wait_for(std::chrono::seconds(0))==std::future_status::ready)
            receive();
        if(!pending.valid() && (!publishedKey || !(*publishedKey==wanted)) &&
           (!failedKey || !(*failedKey==wanted))) {
            try {
                pending=std::async(std::launch::async,[builder,wanted]() { return builder(wanted); });
                pendingKey=wanted;
                if(!publishedKey) receive();
            } catch(const std::exception& failure) {
                pendingKey.reset(); failedKey=wanted; error=failure.what();
                result=TerrainUpdate::Failed;
            }
        }
        return result;
    }
};
} // namespace presentation
