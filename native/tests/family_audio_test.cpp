#include "FamilyAudio.h"
#include "World.h"
#include <iostream>
#include <memory>
#include <stdexcept>

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv){try{
    using namespace isla;check(argc==2,"Expected asset directory");
    auto w=std::make_unique<World>();for(auto& r:w->residents)r.alive=false;
    auto& woman=w->residents[0];auto& man=w->residents[1];
    woman=Resident{};man=Resident{};woman.female=true;woman.pregnancyDue=1000;woman.pregnancyFather=1;
    FamilySoundEvents events;events.prime(*w);woman.familyMomentRemaining=6;
    const auto start=events.observe(*w);
    check(start.size()==2&&start[0].sound==FamilySound::WomanStart&&start[1].sound==FamilySound::ManStart,"Initial voices missing or reversed");
    check(events.observe(*w).empty(),"Initial voices repeated every tick");
    FamilySoundEvents restored;restored.prime(*w);check(restored.observe(*w).empty(),"Loading replays start voices");
    woman.familyMomentRemaining=0;const auto end=events.observe(*w);
    check(end.size()==1&&end[0].sound==FamilySound::ManEnd,"Final male voice missing");
    check(events.observe(*w).empty(),"Final voice repeated every tick");
    woman.ageDays=World::adultAgeDays-1;woman.familyMomentRemaining=6;
    check(events.observe(*w).empty(),"Minor triggered adult voices");
    woman.ageDays=World::adultAgeDays;events.observe(*w);man.alive=false;
    const auto cancel=events.observe(*w);check(cancel.size()==1&&cancel[0].sound==FamilySound::Cancel,"Cancelled encounter played a completion voice");man.alive=true;
    FamilyAudio audio;check(audio.load(argv[1],0),"One of the supplied MP3 files cannot be decoded");
    woman.familyMomentRemaining=0;audio.prime(*w);woman.familyMomentRemaining=6;audio.update(*w,false);
    check(audio.playing()==2,"MP3 start playback failed");
    audio.update(*w,true);check(audio.playing()==2,"Pause closed active voice clips");audio.update(*w,false);
    woman.familyMomentRemaining=0;audio.update(*w,false);check(audio.playing()==1,"Final MP3 did not replace initial voices");
    audio.reset();check(audio.playing()==0,"Audio resources leaked after shutdown");
    for(const char* file:{"ara-ara-girl.mp3","yameteh-kodasai.mp3","kyaaaaaaaa.mp3"})std::cout<<file<<": "<<FamilyAudio::duration(std::filesystem::path(argv[1])/file)<<" ms\n";
    std::cout<<"PASS: MP3 decoding/playback, one-shot transitions, pause, restore and adult-only events\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
