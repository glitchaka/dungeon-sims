#pragma once
#include "entities/Resident.h"

namespace isla {
enum class ResidentPose { Walk, Till, Chop, PushBoat, Courtship, Pregnant, Idle, Eat, Drink, Rest, Bathe, Social, Sow, Harvest, Build, Hunt, Carry, Fetch, Trade, Tend, Camp };
struct ResidentAnimation {
    ResidentPose pose{ResidentPose::Walk};
    static constexpr int framesPerAction=6;
    int direction{},frame{},pregnancyStage{};
    float height{1};
    bool moving{},child{};
    static ResidentAnimation select(const Resident& resident,double elapsed,double animationTime=-1);
    static int phase(double time,double framesPerSecond=6.0);
};
}
