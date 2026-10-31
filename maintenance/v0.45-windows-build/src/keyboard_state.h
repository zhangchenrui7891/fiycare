#pragma once
#include <array>
#include <cstddef>

namespace input {

// One render/UI thread owns this state. Never infer a new key press from a
// foreground transition. Keys already held at recovery must be released first;
// each key rearms independently, so unrelated fresh inputs remain usable.
// Non-repeat key messages latch one discrete press per key per sample. Several
// taps of the same key coalesce; this is not an ordered macro/event queue.
class KeyboardState {
public:
    using Keys=std::array<bool,256>;
    struct Modifiers {bool shift=false,control=false;};
private:
    Keys held{},edges{},blocked{},pending{};
    std::array<Modifiers,256> pendingModifiers{},pressModifiers{};
    bool needsSync=true,resumedFlag=false;
public:
    const Keys& down() const {return held;}
    const Keys& pressed() const {return edges;}
    Modifiers modifiers(std::size_t key) const {
        return key<pressModifiers.size()?pressModifiers[key]:Modifiers{};
    }
    // Caller accepts only foreground, non-minimized, non-native-menu messages.
    // VK_SHIFT/VK_CONTROL are the generic Win32 slots 16/17. A modifier held
    // over recovery must not bypass the same release-to-rearm protection.
    void keyDown(std::size_t key,bool repeat,Modifiers modifiers) {
        if(key>=pending.size()||needsSync||blocked[key]||repeat||pending[key])return;
        pending[key]=true;
        pendingModifiers[key]={modifiers.shift&&!blocked[16],modifiers.control&&!blocked[17]};
    }
    void keyUp(std::size_t key) {
        if(key<blocked.size()&&!needsSync)blocked[key]=false;
    }
    bool resumed() const {return resumedFlag;}
    std::size_t blockedCount() const {
        std::size_t count=0;
        for(bool value:blocked)if(value)++count;
        return count;
    }
    void suspend() {
        held.fill(false);edges.fill(false);blocked.fill(false);pending.fill(false);
        pendingModifiers.fill(Modifiers{});pressModifiers.fill(Modifiers{});
        needsSync=true;resumedFlag=false;
    }
    void sample(const Keys& physical,bool available) {
        resumedFlag=false;edges.fill(false);pressModifiers.fill(Modifiers{});
        if(!available){suspend();return;}
        if(needsSync){
            blocked=physical;held.fill(false);
            needsSync=false;resumedFlag=true;
            return;
        }
        for(std::size_t key=0;key<held.size();++key){
            if(blocked[key]){
                if(!physical[key])blocked[key]=false;
                held[key]=false;
            }else{
                // Discrete commands use messages exclusively: combining an
                // async polling edge with a later message can issue it twice.
                edges[key]=pending[key];
                held[key]=physical[key];
                if(edges[key])pressModifiers[key]=pendingModifiers[key];
            }
        }
        pending.fill(false);pendingModifiers.fill(Modifiers{});
    }
};

} // namespace input
