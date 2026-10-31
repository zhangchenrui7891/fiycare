#include "src/terrain_stream.h"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

using Stream=presentation::TerrainStream<std::vector<int>>;
using Key=presentation::TerrainKey;
using Update=presentation::TerrainUpdate;
void check(bool value,const char* message) { if(!value)throw std::runtime_error(message); }
template<class Predicate> void until(Predicate predicate) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!predicate()) {
        if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Worker timeout");
        std::this_thread::yield();
    }
}
struct Gate {
    std::promise<void> release;
    std::shared_future<void> ready=release.get_future().share();
    bool open=false;
    void unlock() { if(!open){open=true;release.set_value();} }
    ~Gate() { unlock(); }
};
struct UnlockOnExit { Gate& gate; ~UnlockOnExit(){gate.unlock();} };
void staleCompletion() {
    Gate gate;
    std::atomic<int> running{0},peak{0},calls{0};
    Stream stream; UnlockOnExit unlock{gate};
    auto build=[&](Key k) {
        const int workers=++running;peak.store(std::max(peak.load(),workers));++calls;
        if(k.x==1)gate.ready.wait();
        --running;
        return std::vector<int>{k.x,k.z,k.season,k.mode};
    };
    const Key initial{0,0,0,1},old{1,0,0,1},latest{-9,8,3,0};
    check(stream.update(initial,build)==Update::Applied,"Bootstrap publishes ground");
    stream.update(old,build);
    until([&]{return calls.load()==2;});
    for(int n=0;n<100;n++) {
        stream.update(latest,build);
        check(*stream.key()==initial && stream.mesh()[0]==0,"In-flight request preserves usable mesh");
    }
    gate.unlock();
    bool discarded=false;
    until([&] {
        auto result=stream.update(latest,build);
        if(result==Update::Discarded)discarded=true;
        check(!stream.key() || !(*stream.key()==old),"Obsolete geometry is never published");
        return stream.key() && *stream.key()==latest;
    });
    check(discarded && stream.mesh()==std::vector<int>({-9,8,3,0}),"Newest tile, season and mode win");
    check(peak==1 && calls==3,"Rapid changes keep a single worker and coalesce requests");
    for(int n=0;n<100;n++)stream.update(latest,build);
    check(calls==3,"Identical requests reuse cached terrain");
}
void returnToPublished() {
    Gate gate;std::atomic<int> calls{0};Stream stream;UnlockOnExit unlock{gate};
    auto build=[&](Key k) {++calls;if(k.x==1)gate.ready.wait();return std::vector<int>{k.x};};
    Key origin{},outbound{1,0,0,0};
    stream.update(origin,build);stream.update(outbound,build);gate.unlock();
    until([&]{return stream.update(origin,build)==Update::Discarded;});
    check(*stream.key()==origin && !stream.busy() && calls==2,"Returning to cached tile skips unnecessary rebuild");
}
void failureRecovery() {
    Stream stream;std::atomic<int> calls{0};
    auto build=[&](Key k) {
        ++calls;if(k.mode==9)throw std::runtime_error("injected terrain failure");
        return std::vector<int>{k.x,k.mode};
    };
    const Key origin{},bad{1,0,0,9},good{2,0,0,1};
    stream.update(origin,build);stream.update(bad,build);
    until([&]{return stream.update(bad,build)==Update::Failed;});
    check(*stream.key()==origin && stream.mesh()[0]==0,"Failure retains previous geometry");
    check(stream.lastError()=="injected terrain failure","Failure reaches caller diagnostics");
    for(int n=0;n<100;n++)stream.update(bad,build);
    check(calls==2 && !stream.busy(),"Failing tile cannot trigger a per-frame retry storm");
    until([&]{stream.update(good,build);return *stream.key()==good;});
    check(stream.lastError().empty() && calls==3,"Later valid request recovers");
    Stream firstFailure;
    check(firstFailure.update(bad,build)==Update::Failed && firstFailure.mesh().empty(),"Startup error is contained");
    check(firstFailure.update(origin,build)==Update::Applied,"Startup failure can recover on another key");
}
int main() {
    try {
        staleCompletion();returnToPublished();failureRecovery();
        std::cout<<"PASS: stale terrain rejection, latest request, single worker, cached return, failure retention/recovery\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n';return 1; }
}
