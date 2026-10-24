#include "World.h"
#include "ResidentActions.h"
#include "ResidentSimulation.h"
#include "SeaLife.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace isla {
namespace { int position(const Resident& r) { return cellId(static_cast<int>(r.x),static_cast<int>(r.y)); } }
void World::updateCrops(float dt){for(auto& c:crops){if(c.worker>=0){const auto& r=residents[c.worker];if(r.workCell!=c.cell||(r.task!=Task::Sow&&r.task!=Task::Harvest)||!r.alive)c.worker=-1;}if(c.stage==1||c.stage==2){c.growth+=dt*(night()?.3f:1.0f);if(c.growth>=120){c.stage=3;c.ripeAge=0;}else if(c.growth>=40)c.stage=2;}else if(c.stage==3){c.ripeAge+=dt;if(c.ripeAge>300){c.stage=0;c.growth=0;rebuildNavigation();++metrics["spoil"];}}}}
void World::updateAnimals(float dt){for(auto& a:animals){if(!a.alive){a.respawn-=dt;if(a.respawn<=0){bool placed=false;for(int k=0;k<mapWidth*mapHeight;++k){if(!walkable(k%mapWidth,k/mapWidth)||!(a.kind==4?forest[k]:pasture[k]))continue;bool occupied=false;for(const auto& other:animals)if(&other!=&a&&other.alive&&cellId(static_cast<int>(other.x),static_cast<int>(other.y))==k)occupied=true;for(const auto& person:residents)if(person.alive&&person.insideHouse<0&&!person.inDungeon&&!person.away&&!person.inBoat&&position(person)==k)occupied=true;if(!occupied){a.x=k%mapWidth+.5f;a.y=k/mapWidth+.5f;a.alive=true;a.timer=3;placed=true;break;}}if(!placed)a.respawn=10;}continue;}a.timer-=dt;if(a.timer>0)continue;a.timer=2+static_cast<float>(random()%5);bool hunted=false;for(const auto& r:residents)if(r.task==Task::Hunt&&r.target==static_cast<int>(&a-animals.data())&&std::hypot(r.x-a.x,r.y-a.y)<2)hunted=true;if(hunted)continue;int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));int next=k+std::array<int,4>{{1,-1,mapWidth,-mapWidth}}[random()%4];if(next<0||next>=mapWidth*mapHeight||!canStep(k%mapWidth,k/mapWidth,next%mapWidth,next/mapWidth))continue;bool occupied=false;for(const auto& other:animals)if(&other!=&a&&other.alive&&cellId(static_cast<int>(other.x),static_cast<int>(other.y))==next)occupied=true;for(const auto& person:residents)if(person.alive&&person.insideHouse<0&&!person.inDungeon&&!person.away&&!person.inBoat&&position(person)==next)occupied=true;if(!occupied&&(a.kind==4?forest[next]:pasture[next])){a.x=next%mapWidth+.5f;a.y=next/mapWidth+.5f;}}}
void World::tick(float dt){
    if(dt<=0||dt>.25f||!std::isfinite(dt))throw std::invalid_argument("Invalid fixed simulation step");
    elapsed+=dt;harvestClock=std::fmod(harvestClock+dt,120.0f);alarmSeconds=std::max(0.0f,alarmSeconds-dt);
    SeaLife::update(*this,dt);
    updateCrops(dt);updateAnimals(dt);updateRealm(dt);updateDungeons(dt);updateEcology(dt);updateFishing(dt);updateDivinity(dt);updateLifecycle(dt);updateFamilies(dt);updateVisits(dt);
    ResidentSimulation(*this).update(dt);
}
bool World::begin(Resident& r,Task task,int goal) { return ResidentActions(*this).begin(r,task,goal); }
void World::finish(Resident& r,const std::string& metric,const std::string& note) { ResidentActions(*this).finish(r,metric,note); }
bool World::leaveShelter(Resident& r) { return ResidentActions(*this).leaveShelter(r); }
const char* World::taskName(Task task) { return isla::taskName(task); }
}













