#include "FamilyAudio.h"
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <atomic>
#include <iostream>

namespace isla {
namespace {
std::atomic<unsigned long> nextAlias{0};
std::wstring alias(){return L"islaFamily"+std::to_wstring(++nextAlias);}
bool command(const std::wstring& text){return mciSendStringW(text.c_str(),nullptr,0,nullptr)==0;}
bool open(const std::filesystem::path& file,const std::wstring& name) {
    return command(L"open \""+std::filesystem::absolute(file).wstring()+L"\" type mpegvideo alias "+name);
}
}
unsigned long FamilyAudio::duration(const std::filesystem::path& file) {
    const auto name=alias();if(!open(file,name))return 0;
    command(L"set "+name+L" time format milliseconds");wchar_t length[64]{};
    const bool ok=mciSendStringW((L"status "+name+L" length").c_str(),length,64,nullptr)==0;
    command(L"close "+name);return ok?wcstoul(length,nullptr,10):0;
}
bool FamilyAudio::load(const std::filesystem::path& assets,int volume) {
    reset();volume_=std::clamp(volume,0,1000);
    clips_={assets/"audio/ara-ara-girl.mp3",assets/"audio/yameteh-kodasai.mp3",assets/"audio/kyaaaaaaaa.mp3"};
    bool all=true;
    for(std::size_t i=0;i<clips_.size();++i){ready_[i]=duration(clips_[i])>0;all=all&&ready_[i];}
    if(!all)std::cerr<<"Some family voice clips could not be opened by the audio device\n";
    return all;
}
void FamilyAudio::reset() {
    for(const auto& channel:channels_)command(L"close "+channel.alias);
    channels_.clear();ready_.fill(false);paused_=false;events_=FamilySoundEvents{};
}
void FamilyAudio::stopPair(int woman) {
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const Channel& channel){
        if(channel.woman!=woman)return false;command(L"close "+channel.alias);return true;
    }),channels_.end());
}
void FamilyAudio::play(const FamilySoundEvent& event) {
    if(event.sound==FamilySound::Cancel||event.sound==FamilySound::ManEnd)stopPair(event.woman);
    if(event.sound==FamilySound::Cancel)return;
    const auto clip=static_cast<std::size_t>(event.sound);if(!ready_[clip])return;
    const auto name=alias();if(!open(clips_[clip],name))return;
    if(!command(L"setaudio "+name+L" volume to "+std::to_wstring(volume_))&&volume_==0){command(L"close "+name);return;}
    if(command(L"play "+name+L" from 0"))channels_.push_back({name,event.woman});
    else command(L"close "+name);
}
void FamilyAudio::update(const World& world,bool paused) {
    if(paused!=paused_) {
        for(const auto& channel:channels_)command((paused?L"pause ":L"resume ")+channel.alias);
        paused_=paused;
    }
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[](const Channel& channel){
        wchar_t mode[32]{};const bool ok=mciSendStringW((L"status "+channel.alias+L" mode").c_str(),mode,32,nullptr)==0;
        if(ok&&std::wstring(mode)!=L"stopped")return false;command(L"close "+channel.alias);return true;
    }),channels_.end());
    if(!paused)for(const auto& event:events_.observe(world))play(event);
}
}

