#include "ResidentController.h"
#include "ResidentContext.h"
#include "World.h"
#include <algorithm>
#include <cmath>
namespace isla {
namespace {
int position(const Resident& r) { return cellId(static_cast<int>(r.x),static_cast<int>(r.y)); }
float dist(int a,int b) { return std::hypot(static_cast<float>(a%mapWidth-b%mapWidth),static_cast<float>(a/mapWidth-b/mapWidth)); }
}
void ResidentController::feed(Resident& r){
    if(r.meat+r.grain+r.eggs+r.fish>0){world_.begin(r,Task::Eat,position(r));return;}
    // Urgent hunger takes priority over occupational preferences and livestock.
    if(world_.grain>0&&(r.hunger<18||r.trait!="Cazadora")&&world_.begin(r,Task::Fetch,world_.granary))return;
    if(world_.livestockFood>0&&r.trait!="Cazadora"){int k=world_.nearest(r,[&](int x,int y){return world_.pasture[cellId(x,y)];});if(k>=0&&world_.begin(r,Task::Tend,k))return;}
    float best=1e9f;int chosen=-1;
    for(std::size_t i=0;i<world_.animals.size();++i){const auto& a=world_.animals[i];const int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));if(!a.alive||a.kind!=4||world_.protectedLand[k])continue;bool reserved=false;for(const auto& other:world_.residents)if(&other!=&r&&other.task==Task::Hunt&&other.target==static_cast<int>(i))reserved=true;if(!reserved&&dist(position(r),k)<best){best=dist(position(r),k);chosen=static_cast<int>(i);}}
    if(chosen>=0){r.target=chosen;const auto& a=world_.animals[chosen];if(world_.begin(r,Task::Hunt,cellId(static_cast<int>(a.x),static_cast<int>(a.y))))return;}
    if(!farmTask(r))world_.begin(r,Task::Wander,position(r));
}

bool ResidentController::gatherTask(Resident& r){
    std::vector<int> targets(mapWidth*mapHeight, -1);
    for(const auto& tree:world_.props)if(tree.tree){int source=cellId(static_cast<int>(tree.x),static_cast<int>(tree.y));if(world_.protectedLand[source])continue;
        for(int offset:{1,-1,mapWidth,-mapWidth}){int k=source+offset;if(k<0||k>=mapWidth*mapHeight||std::abs(k%mapWidth-source%mapWidth)+std::abs(k/mapWidth-source/mapWidth)!=1||!world_.walkable(k%mapWidth,k/mapWidth))continue;targets[k]=source;}
    }
    int approach=world_.nearest(r,[&](int x,int y){return targets[cellId(x,y)]>=0;});
    if(approach<0||!world_.begin(r,Task::Gather,approach))return false;r.workCell=targets[approach];return true;
}

void ResidentController::rest(Resident& r){
    if(dist(position(r),r.home)<=11){world_.begin(r,Task::Rest,r.home);return;}
    for(const auto& camp:world_.camps)if(dist(position(r),camp.cell)<8){bool occupied=false;for(const auto& o:world_.residents)if(&o!=&r&&o.task==Task::Rest&&o.goal==camp.cell)occupied=true;if(!occupied){world_.begin(r,Task::Rest,camp.cell);return;}}
    if(r.wood<3){r.resumeRest=true;if(gatherTask(r))return;}
    if(r.wood>=3&&world_.terrain[position(r)]==Terrain::Ground)world_.begin(r,Task::Camp,position(r));else world_.begin(r,Task::Rest,r.home);
}

bool ResidentController::farmTask(Resident& r){
    float best=1e9f;int chosen=-1;Task task=Task::Idle;const int id=world_.residents.indexOf(r);
    for(const auto& c:world_.crops){if((c.worker>=0&&c.worker!=id)||world_.reserved(c.cell))continue;Task action=Task::Idle;if(c.stage==3&&(world_.grain<160||r.seeds<6))action=Task::Harvest;else if(c.stage==0&&r.seeds>0&&world_.grain<130)action=Task::Sow;if(action==Task::Idle)continue;float score=dist(position(r),c.cell)+(action==Task::Harvest?0:8);if(score<best){best=score;chosen=c.cell;task=action;}}
    if(chosen>=0&&world_.begin(r,task,chosen))return true;
    if(world_.crops.size()<128&&r.seeds>2&&world_.harvests>0&&r.trait=="Agricultora")for(const auto& c:world_.crops){for(int d:{1,-1,mapWidth,-mapWidth}){int k=c.cell+d;if(k<0||k>=mapWidth*mapHeight||!world_.walkable(k%mapWidth,k/mapWidth)||world_.terrain[k]!=Terrain::Ground||world_.forest[k]||world_.pasture[k]||world_.fallow[k]||world_.protectedLand[k]||world_.cropAt(k)||world_.reserved(k))continue;bool reserved=false;for(const auto& other:world_.residents)if(other.task==Task::Till&&other.goal==k)reserved=true;if(!reserved&&world_.begin(r,Task::Till,k))return true;}}
    return false;
}


void ResidentController::plan(Resident& resident) {
    ResidentIntent previous = ResidentIntent::None;
    for (;;) {
        const auto context = residentDecisionContext(world_, resident);
        const auto intent = ResidentDecision::next(resident, context, previous);
        if (execute(resident, intent)) return;
        previous = intent;
    }
}

bool ResidentController::execute(Resident& r, ResidentIntent intent) {
    const int at = position(r);
    const int id = world_.residents.indexOf(r);
    switch (intent) {
    case ResidentIntent::StaySheltered:
        r.task = Task::Rest;
        return true;
    case ResidentIntent::LeaveShelter:
        return !world_.leaveShelter(r);
    case ResidentIntent::Child:
        if (world_.night()) {
            if (!world_.begin(r, Task::Rest, r.home)) world_.begin(r, Task::Rest, at);
        } else {
            r.task = Task::Idle;
            r.decisionTimer = 30;
        }
        return true;
    case ResidentIntent::Flee: {
        const int goal = world_.nearest(r, [&](int x, int y) {
            return std::hypot(x+.5f-world_.alarmX,y+.5f-world_.alarmY) >= cnc::fleeRadius;
        });
        return world_.begin(r, Task::Flee, goal);
    }
    case ResidentIntent::CriticalDrink:
    case ResidentIntent::VisitorDrink:
        world_.begin(r, Task::Drink, world_.nearest(r, [&](int x, int y) {
            return world_.besideFreshWater(x, y);
        }));
        return true;
    case ResidentIntent::CriticalEat:
    case ResidentIntent::VisitorEat:
        feed(r);
        return true;
    case ResidentIntent::NightRest:
        for (auto& job : world_.jobs) if (job.worker == id) job.worker = -1;
        r.job = -1;
        r.hasOrder = false;
        if (!world_.begin(r, Task::Rest, r.home)) world_.begin(r, Task::Rest, at);
        return true;
    case ResidentIntent::VisitorRest:
        rest(r);
        return true;
    case ResidentIntent::VisitorWander: {
        const int goal = world_.nearest(r, [&](int x, int y) {
            const int distance = std::abs(x-world_.market%mapWidth)
                + std::abs(y-world_.market/mapWidth);
            return distance >= 2 && distance <= 5;
        });
        if (goal >= 0) world_.begin(r, Task::Wander, goal);
        return true;
    }
    case ResidentIntent::Order:
        if (r.orderedTask == Task::Project) {
            auto job = std::find_if(world_.jobs.begin(), world_.jobs.end(),
                [&](const Job& candidate) { return candidate.id == r.job; });
            if (job != world_.jobs.end() && (job->worker < 0 || job->worker == id)) {
                job->worker = id;
                if (world_.begin(r, Task::Project, job->goal)) return true;
            }
        } else if (r.orderedTask == Task::Move || r.orderedTask == Task::Fish) {
            if (world_.begin(r, r.orderedTask, r.orderedCell)) return true;
        } else {
            const int target = r.orderedCell;
            r.hasOrder = false;
            world_.orderUnit(id, "work", target);
            if (r.hasOrder) return true;
        }
        r.hasOrder = false;
        return false;
    case ResidentIntent::Expedition:
        world_.begin(r, Task::Expedition, world_.dungeons[r.mission].entrance);
        return true;
    case ResidentIntent::SellLoot:
        world_.begin(r, Task::SellLoot, world_.market);
        return true;
    case ResidentIntent::Family:
        return world_.seekFamily(r);
    case ResidentIntent::Gift:
        for (const auto& gift : world_.gifts) {
            const int goal = world_.nearest(r, [&](int x, int y) {
                return std::abs(x-gift.cell%mapWidth)+std::abs(y-gift.cell/mapWidth) == 1;
            });
            if (goal >= 0 && world_.begin(r, Task::Move, goal)) return true;
        }
        return false;
    case ResidentIntent::Fishing:
        for (std::size_t boat = 0; boat < world_.ships.size(); ++boat) {
            if (world_.ships[boat].passenger >= 0 || world_.ships[boat].remaining > 0) continue;
            world_.startFishing(id, static_cast<int>(boat));
            if (r.hasOrder) return true;
        }
        return false;
    case ResidentIntent::Project:
        for (auto& job : world_.jobs) {
            if (job.worker >= 0) continue;
            r.job = job.id;
            if (world_.begin(r, Task::Project, job.goal)) {
                job.worker = id;
                return true;
            }
        }
        return false;
    case ResidentIntent::DrinkHome:
        world_.begin(r, Task::DrinkHome, r.home);
        return true;
    case ResidentIntent::ResumeRest:
        r.resumeRest = false;
        rest(r);
        return true;
    case ResidentIntent::Deposit:
        world_.begin(r, Task::Deposit, world_.granary);
        return true;
    case ResidentIntent::Needs:
        return satisfyNeed(r);
    case ResidentIntent::Explore: {
        if (world_.random()%100 >= static_cast<unsigned>(cnc::wanderChance*100)) return false;
        std::vector<int> options;
        for (int cell = 0; cell < mapWidth*mapHeight; ++cell) {
            if (world_.forest[cell] && world_.walkable(cell%mapWidth, cell/mapWidth))
                options.push_back(cell);
        }
        return !options.empty()
            && world_.begin(r, Task::Wander, options[world_.random()%options.size()]);
    }
    case ResidentIntent::Idle:
        r.task = Task::Idle;
        r.decisionTimer = cnc::wanderInterval;
        r.path.clear();
        r.speed = 0;
        return true;
    case ResidentIntent::None:
        return true;
    }
    return true;
}

bool ResidentController::satisfyNeed(Resident& r) {
    const auto preference = ResidentDecision::evaluate(r, residentDecisionContext(world_, r));
    if (preference.preferFarming && farmTask(r)) return true;
    switch (preference.need) {
    case ResidentNeed::Drink:
        world_.begin(r, Task::Drink, world_.nearest(r, [&](int x, int y) {
            return world_.besideFreshWater(x, y);
        }));
        return true;
    case ResidentNeed::Eat:
        feed(r);
        return true;
    case ResidentNeed::Rest:
        rest(r);
        return true;
    case ResidentNeed::Bathe:
        world_.begin(r, Task::Bathe, world_.nearest(r, [&](int x, int y) {
            return world_.besideFreshWater(x, y);
        }));
        return true;
    case ResidentNeed::Social: {
        const int id = world_.residents.indexOf(r);
        float best = 1e9f;
        int who = -1;
        for (std::size_t i = 0; i < world_.residents.size(); ++i) {
            const auto& other = world_.residents[i];
            if (static_cast<int>(i) == id || !other.alive || other.inDungeon
                || other.mission >= 0 || other.task == Task::Rest
                || other.task == Task::Drink || other.task == Task::Hunt) continue;
            const float distance = dist(position(r), position(other));
            if (distance < best) {
                who = static_cast<int>(i);
                best = distance;
            }
        }
        if (who < 0) return false;
        r.target = who;
        return world_.begin(r, Task::Social, world_.nearest(r, [&](int x, int y) {
            const auto& other = world_.residents[who];
            return std::abs(x-static_cast<int>(other.x))
                + std::abs(y-static_cast<int>(other.y)) == 1;
        }));
    }
    case ResidentNeed::Work:
        if (r.wood >= 4) {
            world_.begin(r, Task::Trade, world_.market);
            return true;
        }
        return gatherTask(r);
    case ResidentNeed::Explore:
        return false;
    }
    return false;
}
}

