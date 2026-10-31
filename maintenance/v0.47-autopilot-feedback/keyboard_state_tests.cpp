#include "src/keyboard_state.h"
#include "src/cockpit_controls.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

using Keyboard=input::KeyboardState;
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
std::string snapshot(const sim::Snapshot& state){std::ostringstream out;state.write(out);return out.str();}

void reproduceLegacyGear(){
    bool previous=false;
    auto oldPoll=[&](bool focus,bool physical){bool down=focus&&physical;bool pressed=down&&!previous;previous=down;return pressed;};
    sim::Snapshot state;state.flight.gear=true;
    require(!oldPoll(false,true),"Old loop suppresses key while away");
    const bool bogusPress=oldPoll(true,true);
    if(bogusPress)cockpit::apply(state,cockpit::Action::Gear);
    require(bogusPress&&!state.flight.gear,"Legacy return-to-focus incorrectly retracts gear");
    std::cout<<"Legacy defect reproduced: held G becomes a new edge on foreground return and retracts gear.\n";
}

void everyKeyRearmsIndependently(){
    for(std::size_t key=0;key<256;key++){
        Keyboard input;Keyboard::Keys raw{};
        input.sample(raw,true);raw[key]=true;input.keyDown(key,false,{});
        input.sample(raw,true);
        require(input.pressed()[key]&&input.down()[key],"Fresh foreground press works");
        input.sample(raw,true);
        require(!input.pressed()[key]&&input.down()[key],"Hold does not repeat one-shot commands");
        input.sample(raw,false);
        require(!input.down()[key]&&!input.pressed()[key],"Inactive input is neutral");
        input.sample(raw,true);
        require(input.resumed()&&input.blockedCount()==1&&!input.down()[key]&&!input.pressed()[key],
                "Recovery gates a held key, including its continuous control");
        const auto other=(key+1)%256;raw[other]=true;input.keyDown(other,false,{});input.sample(raw,true);
        require(input.pressed()[other]&&input.down()[other]&&!input.down()[key],
                "One blocked key cannot freeze unrelated fresh input");
        for(int frame=0;frame<20;frame++){
            input.sample(raw,true);
            require(!input.down()[key]&&!input.pressed()[key],"Held recovery key remains gated");
        }
        raw[key]=false;input.sample(raw,true);
        require(!input.blockedCount()&&!input.pressed()[key],"Release rearms without issuing a press");
        raw[key]=true;input.keyDown(key,false,{});input.sample(raw,true);
        require(input.down()[key]&&input.pressed()[key],"Repress issues exactly one edge");
        input.suspend(); // Focus/menu can leave and return between render samples.
        require(!input.down()[key]&&!input.pressed()[key],"Event suspension immediately neutralizes references");
        input.sample(raw,true);
        require(!input.down()[key]&&!input.pressed()[key]&&input.blockedCount()==2,
                "Explicit suspension catches an event with no inactive polling frame");
    }
    std::cout<<"All 256 key slots: focus recovery, hold, release/repress and independent-key gating passed.\n";
}

void actualCockpitState(){
    struct Binding{int key;cockpit::Action action;};
    const Binding bindings[]={{'G',cockpit::Action::Gear},{'F',cockpit::Action::Flaps},
        {'I',cockpit::Action::Engine},{'Q',cockpit::Action::Battery},{'M',cockpit::Action::Generator},
        {'Y',cockpit::Action::Avionics},{'V',cockpit::Action::ParkingBrake},{'L',cockpit::Action::Assist},
        {'J',cockpit::Action::PanelBrightness},{'O',cockpit::Action::CockpitFlood},{'U',cockpit::Action::Wind}};
    for(const auto& binding:bindings){
        sim::Snapshot state;state.flight.configure(3);Keyboard input;Keyboard::Keys raw{};
        input.sample(raw,true);
        const auto before=snapshot(state);raw[std::size_t(binding.key)]=true;
        input.sample(raw,false);input.sample(raw,true);
        for(int frame=0;frame<120;frame++){
            if(input.pressed()[std::size_t(binding.key)])cockpit::apply(state,binding.action);
            require(snapshot(state)==before,"Recovered held key cannot mutate actual aircraft/cockpit state");
            input.sample(raw,true);
        }
        raw[std::size_t(binding.key)]=false;input.sample(raw,true);
        raw[std::size_t(binding.key)]=true;input.keyDown(std::size_t(binding.key),false,{});input.sample(raw,true);
        require(input.pressed()[std::size_t(binding.key)],"New press is delivered to actual binding");
        cockpit::apply(state,binding.action);
        const auto after=snapshot(state);
        require(after!=before,"Real control still changes state after release/repress");
        input.sample(raw,true);
        if(input.pressed()[std::size_t(binding.key)])cockpit::apply(state,binding.action);
        require(snapshot(state)==after,"Real control is not double-triggered on hold");
    }
    std::cout<<"11 actual cockpit bindings x120 held frames: no false state change; fresh presses still act once.\n";
}

void sequenceInvariants(){
    Keyboard input;Keyboard::Keys raw{};unsigned value=0x12345678u;
    for(int frame=0;frame<10000;frame++){
        value=value*1664525u+1013904223u;
        const bool available=(value&15u)!=0;
        const auto changed=(value>>8)&255u;
        const bool next=((value>>16)&1u)!=0;
        if(next&&!raw[changed])input.keyDown(changed,false,{});
        if(!next&&raw[changed])input.keyUp(changed);
        raw[changed]=next;
        if((value&127u)==1)input.suspend();
        input.sample(raw,available);
        for(std::size_t key=0;key<raw.size();key++){
            require(!input.pressed()[key]||input.down()[key],"Edge implies delivered held state");
            require(!input.down()[key]||(available&&raw[key]),"Delivered input needs availability and physical state");
            if(!available||input.resumed())require(!input.down()[key]&&!input.pressed()[key],
                                                   "Inactive and first recovery samples are neutral");
        }
    }
    std::cout<<"10,000 deterministic mixed availability/key sequences preserve input invariants.\n";
}

int main(){
    try{reproduceLegacyGear();everyKeyRearmsIndependently();actualCockpitState();sequenceInvariants();
        std::cout<<"Keyboard recovery checks passed. CPU state checks only; no Windows messages executed.\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
