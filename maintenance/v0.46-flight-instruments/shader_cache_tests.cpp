#include "src/shader_cache.h"
#include <iostream>
#include <memory>
#include <stdexcept>

using Handle=std::shared_ptr<int>;
using Cache=compilation::ShaderCache<Handle>;
using Request=compilation::ShaderRequest;
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
template<class Action> void rejects(Action action,const char* reason){
    bool failed=false;try{action();}catch(const std::exception&){failed=true;}
    require(failed,reason);
}

void rebuildReuse(){
    Cache cache;int calls=0;
    Request requests[]={{"scene","vs","vs_5_0",1,0},{"scene","ps","ps_5_0",1,0},
                        {"post","postVS","vs_5_0",1,0},{"post","postPS","ps_5_0",1,0}};
    Handle first[4];
    for(int rebuild=0;rebuild<100;rebuild++)for(int stage=0;stage<4;stage++){
        auto result=cache.get(requests[stage],[&]{return std::make_shared<int>(++calls);});
        if(!rebuild)first[stage]=result;
        else require(result==first[stage],"Renderer recreation retains the same CPU blob handle");
    }
    require(calls==4&&cache.size()==4&&cache.stats().requests==400&&cache.stats().hits==396,
            "Four programs compile once across 100 simulated rebuild requests");
    require(!cache.stats().failures&&!cache.stats().evictions,"Normal four-stage set does not evict");
    std::cout<<"100 simulated four-stage renderer requests: 4 compiler calls / 396 cache hits\n";
}

void fullRequestIdentity(){
    Cache cache;int calls=0;
    auto compile=[&]{return std::make_shared<int>(++calls);};
    Request original{"float4 main():SV_TARGET{return 1;}","main","ps_5_0",1,2};
    auto first=cache.get(original,compile);
    Request same=original; // Different string storage with identical content.
    require(cache.get(same,compile)==first,"Equal source content, not address, determines identity");
    Request changed=original;changed.source+="// source changed";
    require(cache.get(changed,compile)!=first,"Source change recompiles");
    changed=original;changed.entry="other";require(cache.get(changed,compile)!=first,"Entry change recompiles");
    changed=original;changed.profile="ps_4_0";require(cache.get(changed,compile)!=first,"Profile change recompiles");
    changed=original;changed.flags1=3;require(cache.get(changed,compile)!=first,"Flags1 change recompiles");
    changed=original;changed.flags2=4;require(cache.get(changed,compile)!=first,"Flags2 change recompiles");
    require(calls==6&&cache.size()==6,"Every compiler input participates in identity");
    original.source="modified after insertion";
    require(cache.get(same,compile)==first,"Cached request owns its source and is not externally mutated");
    rejects([&]{cache.get({"","main","ps_5_0",0,0},compile);},"Incomplete requests rejected");
}

void failuresRetry(){
    Cache cache;Request request{"source","main","ps_5_0",0,0};int calls=0;
    auto unstable=[&]()->Handle{if(++calls==1)throw std::runtime_error("injected compiler failure");return std::make_shared<int>(calls);};
    rejects([&]{cache.get(request,unstable);},"Compiler exceptions propagate");
    require(cache.size()==0,"Failure never publishes a partial entry");
    auto result=cache.get(request,unstable);
    require(*result==2&&cache.get(request,unstable)==result&&calls==2,"Failed requests retry and then cache");
    Request empty{"empty-output","main","ps_5_0",0,0};
    rejects([&]{cache.get(empty,[]{return Handle{};});},"Null bytecode is not success");
    require(cache.size()==1&&cache.get(request,unstable)==result,"Other entries survive a failed compile");
    require(cache.stats().failures==2&&cache.stats().compilations==3,"Failed attempts are visible in counters");
}

void boundedLifetime(){
    rejects([]{Cache invalid(0);},"Zero-capacity cache rejected");
    Cache cache(2);int calls=0;
    auto compile=[&]{return std::make_shared<int>(++calls);};
    Request a{"a","main","vs_5_0",0,0},b{"b","main","vs_5_0",0,0},c{"c","main","vs_5_0",0,0};
    auto keepA=cache.get(a,compile),keepB=cache.get(b,compile);
    require(cache.get(a,compile)==keepA,"Touch A to protect it from LRU eviction");
    cache.get(c,compile);
    require(cache.size()==2&&cache.stats().evictions==1,"Cache capacity is bounded");
    require(cache.get(a,compile)==keepA,"Recently used A remains cached");
    auto newB=cache.get(b,compile);
    require(newB!=keepB&&*keepB==2,"Evicted entry recompiles, existing owners remain valid");
    cache.clear();
    require(!cache.size()&&*keepA==1&&*newB==4,"Clearing cache does not invalidate returned owners");
    require(cache.get(a,compile)!=keepA,"Explicit clear permits a fresh compilation");
}

int main(){
    try{rebuildReuse();fullRequestIdentity();failuresRetry();boundedLifetime();
        std::cout<<"Shader cache key, failure/retry, LRU capacity and ownership checks passed. No HLSL/GPU executed.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
