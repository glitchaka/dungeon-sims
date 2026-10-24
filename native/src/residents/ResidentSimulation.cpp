#include "ResidentSimulation.h"
#include "ResidentController.h"
#include "ResidentActions.h"
#include "ResidentContext.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
namespace {
int position(const Resident& r) {
    return cellId(static_cast<int>(r.x), static_cast<int>(r.y));
}
}

void ResidentSimulation::update(float dt) {
    ResidentController controller(world_);
    ResidentActions actions(world_);
    for (auto& resident : world_.residents) {
        if (!resident.alive || resident.away || resident.inDungeon || resident.inBoat) continue;
        if (!updateNeeds(resident, dt)) continue;
        if (resident.familyCourtship > 0 || resident.familyMomentRemaining > 0) continue;
        resident.decisionTimer -= dt;

        const bool centered = std::hypot(resident.x-std::floor(resident.x)-.5f,
                                         resident.y-std::floor(resident.y)-.5f) < .001f;
        if (centered && ResidentDecision::shouldReconsider(
                resident, residentDecisionContext(world_, resident))) controller.plan(resident);
        if (move(resident, dt)) continue;

        resident.speed = 0;
        if (resident.ageDays < World::adultAgeDays) {
            if (resident.task == Task::Rest) actions.act(resident, dt);
            else resident.task = Task::Idle;
            continue;
        }
        if (resident.task == Task::Idle) {
            if (resident.decisionTimer <= 0) controller.plan(resident);
        } else actions.act(resident, dt);
    }
}

bool ResidentSimulation::updateNeeds(Resident& r, float dt) {
    const bool moving = r.next < r.path.size();
    r.hunger = std::max(0.0f, r.hunger-.066f*dt);
    r.thirst = std::max(0.0f, r.thirst-(moving?.11f:.085f)
        *(world_.hasBonus(r,"thirst")?.75f:1)*dt);
    r.energy = std::max(0.0f, r.energy-(moving?.075f:.038f)*dt);
    r.social = std::max(0.0f, r.social-(r.trait=="Sociable"?.06f:.037f)*dt);
    r.fun = std::max(0.0f, r.fun-.045f*dt);
    r.hygiene = std::max(0.0f, r.hygiene-(r.task==Task::Hunt?.07f:.035f)*dt);
    if (r.hunger < 2 || r.thirst < 2) r.health = std::max(0.0f, r.health-.44f*dt);
    else if (r.hunger > 30 && r.thirst > 30) r.health = std::min(cnc::maxHealth, r.health+.36f*dt);
    if (r.health > 0) return true;
    r.alive = false;
    world_.record(r.name+" no sobrevivio");
    return false;
}

bool ResidentSimulation::move(Resident& r, float dt) {
    if (r.next >= r.path.size()) return false;
    const float top = (r.task==Task::Flee?cnc::panicSpeed:cnc::walkSpeed)
        *(world_.hasBonus(r,"speed")?1.25f:1)*(r.ageDays>=World::elderAgeDays?.65f:1.0f);
    r.speed=std::min(top,r.speed+top/std::max(.01f,cnc::accelerationSeconds)*dt);
    float remaining=dt;
    // Consume zero-length waypoints immediately and carry unused movement time
    // into the next segment instead of stopping for one fixed tick per cell.
    while(r.next<r.path.size()&&remaining>0) {
        const int cell=r.path[r.next];
        const float tx=cell%mapWidth+.5f,ty=cell/mapWidth+.5f;
        const float distance=std::hypot(tx-r.x,ty-r.y);
        if(distance<.0001f){r.x=tx;r.y=ty;++r.next;continue;}
        bool occupied=false;
        if(cell!=position(r)) {
            for(const auto& other:world_.residents)
                if(&other!=&r&&other.alive&&other.insideHouse<0&&!other.inDungeon&&!other.away&&!other.inBoat&&position(other)==cell)occupied=true;
            for(const auto& animal:world_.animals)
                if(animal.alive&&cellId(static_cast<int>(animal.x),static_cast<int>(animal.y))==cell)occupied=true;
        }
        if(occupied) {
            r.speed=0;r.actionTimer+=dt;
            if(r.actionTimer>2){r.path.clear();r.next=0;r.actionTimer=0;r.task=Task::Idle;ResidentController(world_).plan(r);}
            return true;
        }
        r.actionTimer=0;
        if(!world_.canStep(static_cast<int>(r.x),static_cast<int>(r.y),cell%mapWidth,cell/mapWidth)) {
            r.path.clear();r.task=Task::Idle;r.speed=0;return true;
        }
        const float step=std::min(distance,r.speed*remaining);
        if(step<=0)break;
        r.x+=(tx-r.x)/distance*step;r.y+=(ty-r.y)/distance*step;
        remaining=std::max(0.0f,remaining-step/r.speed);
        if(distance<=step+.0001f) {
            r.x=tx;r.y=ty;
            if((world_.terrain[cell]==Terrain::Ground||world_.terrain[cell]==Terrain::Road||world_.terrain[cell]==Terrain::Bridge)&&world_.traffic[cell]<60)++world_.traffic[cell];
            ++r.next;
        }
    }
    return true;
}
}


