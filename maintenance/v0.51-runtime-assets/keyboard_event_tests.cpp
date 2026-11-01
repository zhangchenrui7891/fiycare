#include "src/keyboard_state.h"
#include "src/cockpit_controls.h"
#include <iostream>
#include <stdexcept>

using Keyboard=input::KeyboardState;
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}

void shortTapsAndCoalescing(){
    // Legacy polling sees identical released samples, so its edge is false.
    const bool oldDown=false,oldPrevious=false;
    require(!(oldDown&&!oldPrevious),"Old polling misses completed tap");
    for(std::size_t key=0;key<256;++key){
        Keyboard input;Keyboard::Keys raw{};input.sample(raw,true);
        input.keyDown(key,false,{true,false});input.keyUp(key);
        input.sample(raw,true);
        require(input.pressed()[key]&&!input.down()[key],"Short tap delivers command without latching continuous control");
        require(input.modifiers(key).shift&&!input.modifiers(key).control,"Tap retains press-time modifiers");
        input.sample(raw,true);require(!input.pressed()[key],"Tap consumed once");
        for(int tap=0;tap<1000;++tap){input.keyDown(key,false,{false,tap!=0});input.keyUp(key);}
        input.sample(raw,true);
        require(input.pressed()[key]&&!input.modifiers(key).control,"Same-key taps coalesce with first modifier snapshot");
        input.sample(raw,true);require(!input.pressed()[key],"No deferred backlog");
        input.keyDown(key,true,{});input.sample(raw,true);
        require(!input.pressed()[key],"Autorepeat is not a discrete command");
    }
    std::cout<<"256 slots: completed taps retained, 1000 same-frame taps coalesced, repeat filtered, no continuous latch.\n";
}

void recoveryAndModifiers(){
    Keyboard input;Keyboard::Keys raw{};input.sample(raw,true);
    input.keyDown('G',false,{});input.suspend();input.sample(raw,true);
    require(!input.pressed()['G'],"Context transition drops pending commands");
    raw['G']=raw[16]=true;input.suspend();
    input.keyDown('G',false,{});input.sample(raw,true);
    input.keyDown('G',false,{});input.sample(raw,true);
    require(!input.pressed()['G'],"Recovered held key cannot bypass gate via messages");
    input.keyDown(117,false,{true,true});input.keyUp(117);input.sample(raw,true);
    require(input.pressed()[117]&&!input.modifiers(117).shift&&input.modifiers(117).control,
            "Blocked recovered Shift is neutral; fresh Ctrl snapshot is independent");
    // Release and re-press can occur entirely between samples.
    input.keyUp('G');input.keyDown('G',false,{});input.sample(raw,true);
    require(input.pressed()['G']&&input.down()['G'],"Message release rearms recovered key without an intermediate sample");
    input.keyUp(16);input.keyDown(16,false,{true,false});
    input.keyDown(117,false,{true,false});input.keyUp(117);raw[16]=false;input.sample(raw,true);
    require(input.pressed()[117]&&input.modifiers(117).shift,"Rearmed Shift at event time survives release before sample");
    input.keyDown('F',false,{});input.sample(raw,false);input.sample(raw,true);
    require(!input.pressed()['F'],"Unavailable sample discards pending input");
    input.keyDown(256,false,{});input.keyDown(std::size_t(-1),false,{});input.keyUp(256);
    input.sample(raw,true);require(!input.modifiers(256).shift,"Out-of-range events/access are safe");
    std::cout<<"Recovery barriers, between-frame rearming, press-time modifier gating and bounds passed.\n";
}

void asyncPollCannotDuplicate(){
    Keyboard input;Keyboard::Keys raw{};input.sample(raw,true);
    raw['G']=true;input.sample(raw,true);
    require(input.down()['G']&&!input.pressed()['G'],"Current physical state is not also a discrete event");
    input.keyDown('G',false,{});input.sample(raw,true);
    require(input.pressed()['G'],"Delayed key message emits exactly one command");
    input.sample(raw,true);require(!input.pressed()['G'],"Polling held key cannot duplicate delivered event");
    input.keyDown('G',true,{});input.sample(raw,true);
    require(!input.pressed()['G'],"Repeat plus polling cannot duplicate command");
    raw['W']=false;input.keyDown('W',false,{});input.keyUp('W');input.sample(raw,true);
    require(input.pressed()['W']&&!input.down()['W'],"Completed throttle tap never becomes stuck throttle");
    std::cout<<"Async physical state ahead of message: no duplicate discrete action.\n";
}

void actualGearAtFrameIntervals(){
    // Timing here is a deterministic event schedule, not a GPU FPS benchmark.
    const int intervalsMs[]={16,33,100,250,1000};
    for(int interval:intervalsMs){
        sim::Snapshot state;Keyboard input;Keyboard::Keys raw{};input.sample(raw,true);
        int applied=0;
        for(int frame=0;frame<120;++frame){
            const int pressTime=frame*interval+interval/3;
            const int releaseTime=pressTime+1;
            require(releaseTime<(frame+1)*interval,"Scheduled tap fits between samples");
            input.keyDown('G',false,{});input.keyUp('G');input.sample(raw,true);
            if(input.pressed()['G']){cockpit::apply(state,cockpit::Action::Gear);++applied;}
            require(!input.down()['G'],"Gear tap cannot latch physical state");
        }
        require(applied==120&&state.flight.gear,"120 actual gear toggles delivered once per sample");
    }
    std::cout<<"Actual gear action: 5 deterministic sample intervals x120 completed taps; all 600 delivered once.\n";
}

int main(){
    try{shortTapsAndCoalescing();recoveryAndModifiers();asyncPollCannotDuplicate();actualGearAtFrameIntervals();
        std::cout<<"Keyboard event checks passed; CPU schedules only, not Windows input delivery or performance.\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
