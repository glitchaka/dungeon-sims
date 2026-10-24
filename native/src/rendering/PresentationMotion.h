#pragma once
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
class PresentationClock {
public:
    void advance(float delta,bool paused) {
        if(!paused&&std::isfinite(delta)&&delta>0)seconds_+=std::min(delta,.25f);
    }
    double seconds()const{return seconds_;}
private:
    double seconds_{};
};
struct ScreenPosition {float x,y;};
// Render interpolation only. The fixed-step world and saved positions stay exact.
class PresentationMotion {
public:
    void capture(const World& world);
    ScreenPosition resident(const World& world,std::size_t index,float alpha)const;
    ScreenPosition boat(const Ship& ship,std::size_t index,float alpha)const;
private:
    struct PreviousResident {float x,y;bool inBoat,away;int house;};
    struct PreviousBoat {float x,y;int passenger;std::string state;};
    std::vector<PreviousResident> residents_;
    std::vector<PreviousBoat> boats_;
};
}


