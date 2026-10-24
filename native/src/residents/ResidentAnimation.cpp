#include "ResidentAnimation.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
ResidentAnimation ResidentAnimation::select(const Resident& r,double elapsed,double animationTime) {
    const double visualTime=animationTime<0?elapsed:animationTime;
    ResidentAnimation a;
    a.moving=r.next<r.path.size()&&r.speed>.001f;a.child=r.ageDays<World::adultAgeDays;
    if(a.moving) {
        const int next=r.path[r.next];
        const float dx=next%mapWidth+.5f-r.x,dy=next/mapWidth+.5f-r.y;
        a.direction=std::abs(dx)>std::abs(dy)?(dx>0?1:3):(dy>0?0:2);
    }
    a.height=r.ageDays<World::childhoodDays?.48f:a.child?.72f:r.ageDays>=World::elderAgeDays?.93f:1;
    const double cycle=visualTime*5+r.home*.37;
    const int step=a.moving?static_cast<int>(cycle)%2:0;
    a.frame=a.direction+4*(a.child?(r.female?0:1):step);
    a.nextFrame=a.frame;
    const auto blend=[](double phase) {
        const float t=std::clamp(static_cast<float>((phase-std::floor(phase)-.65)/.35),0.0f,1.0f);
        return t*t*(3-2*t);
    };
    if(a.moving&&!a.child){a.nextFrame=a.direction+4*(1-step);a.frameBlend=blend(cycle);}
    if(a.child)return a;
    if(r.familyCourtship>0) {a.pose=ResidentPose::Courtship;a.frame=r.familyCourtship<3?6:7;a.nextFrame=a.frame;a.frameBlend=0;return a;}
    if(r.female&&r.pregnancyDue>=0) {
        const double progress=1-std::clamp((r.pregnancyDue-elapsed)/World::gestationSeconds,0.0,1.0);
        if(progress>=1.0/3) {a.pose=ResidentPose::Pregnant;a.frame=a.direction+(progress>=2.0/3?4:0);a.nextFrame=a.frame;a.frameBlend=0;return a;}
    }
    if(!a.moving) {
        const int swing=static_cast<int>((animationTime<0?r.actionTimer:animationTime)*3)%2;
        if(r.task==Task::Till) {a.pose=ResidentPose::Till;a.frame=swing;a.nextFrame=1-swing;a.frameBlend=blend((animationTime<0?r.actionTimer:animationTime)*3);}
        else if(r.task==Task::Gather) {a.pose=ResidentPose::Chop;a.frame=2+swing;a.nextFrame=2+1-swing;a.frameBlend=blend((animationTime<0?r.actionTimer:animationTime)*3);}
    }
    return a;
}
}



