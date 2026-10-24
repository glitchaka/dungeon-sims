#pragma once
#include "FamilySoundEvents.h"
#include <array>
#include <filesystem>
#include <string>

namespace isla {
// Windows host adapter. MP3 decoding and mixing use the system multimedia API.
class FamilyAudio {
public:
    ~FamilyAudio(){reset();}
    FamilyAudio()=default;
    FamilyAudio(const FamilyAudio&)=delete;
    FamilyAudio& operator=(const FamilyAudio&)=delete;
    bool load(const std::filesystem::path& assets,int volume=750);
    void prime(const World& world){events_.prime(world);}
    void update(const World& world,bool paused);
    void reset();
    static unsigned long duration(const std::filesystem::path& mp3);
    std::size_t playing()const{return channels_.size();}
private:
    struct Channel {std::wstring alias;int woman;};
    FamilySoundEvents events_;
    std::array<std::filesystem::path,3> clips_;
    std::array<bool,3> ready_{};
    std::vector<Channel> channels_;
    int volume_{750};bool paused_{};
    void play(const FamilySoundEvent& event);
    void stopPair(int woman);
};
}

