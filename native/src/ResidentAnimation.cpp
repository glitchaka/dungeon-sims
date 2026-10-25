#include "ResidentAnimation.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
int ResidentAnimation::phase(double time,double framesPerSecond) {
    if(!std::isfinite(time)||!std::isfinite(framesPerSecond)||framesPerSecond<=0)return 0;
    return static_cast<int>(std::floor(std::fmod(std::max(0.0,time),framesPerAction/framesPerSecond)*framesPerSecond))%framesPerAction;
}
ResidentAnimation ResidentAnimation::select(const Resident& r,double elapsed,double animationTime) {
    const double visualTime=animationTime<0?elapsed:animationTime;
    ResidentAnimation a;
    a.moving=r.next<r.path.size();a.child=r.ageDays<World::adultAgeDays;
    if(a.moving) {
        const int next=r.path[r.next];
        const float dx=next%mapWidth+.5f-r.x,dy=next/mapWidth+.5f-r.y;
        a.direction=std::abs(dx)>std::abs(dy)?(dx>0?1:3):(dy>0?0:2);
    }
    a.height=r.ageDays<World::childhoodDays?.48f:a.child?.72f:r.ageDays>=World::elderAgeDays?.93f:1;
    a.frame=phase(visualTime+r.home*.037,a.moving?8.0:4.0);
    if(a.child){a.pose=a.moving?ResidentPose::Walk:r.task==Task::Rest?ResidentPose::Rest:ResidentPose::Idle;return a;}
    if(r.female&&r.pregnancyDue>=0) {
        const double progress=1-std::clamp((r.pregnancyDue-elapsed)/World::gestationSeconds,0.0,1.0);
        if(progress>=1.0/3)a.pregnancyStage=progress>=2.0/3?2:1;
    }
    if(a.moving){if(a.pregnancyStage)a.pose=ResidentPose::Pregnant;return a;}
    if(r.familyCourtship>0){a.pose=ResidentPose::Courtship;return a;}
    switch(r.task) {
    case Task::Till:a.pose=ResidentPose::Till;break;
    case Task::Gather:a.pose=ResidentPose::Chop;break;
    case Task::Sow:a.pose=ResidentPose::Sow;break;
    case Task::Harvest:a.pose=ResidentPose::Harvest;break;
    case Task::Project:a.pose=ResidentPose::Build;break;
    case Task::Expedition:case Task::Hunt:a.pose=ResidentPose::Hunt;break;
    case Task::Eat:a.pose=ResidentPose::Eat;break;
    case Task::Drink:case Task::DrinkHome:a.pose=ResidentPose::Drink;break;
    case Task::Rest:a.pose=ResidentPose::Rest;break;
    case Task::Bathe:a.pose=ResidentPose::Bathe;break;
    case Task::Social:a.pose=ResidentPose::Social;break;
    case Task::Deposit:a.pose=ResidentPose::Carry;break;
    case Task::Fetch:a.pose=ResidentPose::Fetch;break;
    case Task::Trade:case Task::SellLoot:a.pose=ResidentPose::Trade;break;
    case Task::Tend:a.pose=ResidentPose::Tend;break;
    case Task::Camp:a.pose=ResidentPose::Camp;break;
    default:a.pose=ResidentPose::Idle;break;
    }
    if(a.pose==ResidentPose::Till||a.pose==ResidentPose::Chop||a.pose==ResidentPose::Build)
        a.frame=phase(animationTime<0?r.actionTimer:animationTime,6);
    return a;
}
}
