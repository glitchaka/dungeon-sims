#pragma once
#include "entities/Resident.h"

namespace isla {
enum class ResidentPose { Walk, Till, Chop, PushBoat, Courtship, Pregnant };
struct ResidentAnimation {
    ResidentPose pose{ResidentPose::Walk};
    int direction{},frame{},nextFrame{};
    float frameBlend{};
    float height{1};
    bool moving{},child{};
    static ResidentAnimation select(const Resident& resident,double elapsed,double animationTime=-1);
};
}


