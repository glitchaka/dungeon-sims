#include "World.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
using nlohmann::json;
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void flat(isla::World& w){w.props.clear();w.crops.clear();w.animals.clear();w.jobs.clear();w.gifts.clear();w.ships.clear();w.forest.fill(false);w.pasture.fill(false);w.protectedLand.fill(false);w.fallow.fill(false);w.traffic.fill(0);w.elevation.fill(0);w.terrain.fill(isla::Terrain::Ground);for(int y=0;y<isla::mapHeight;++y)w.terrain[isla::cellId(0,y)]=isla::Terrain::Sea;w.terrain[isla::cellId(8,8)]=isla::Terrain::FreshWater;w.market=isla::cellId(5,20);w.granary=isla::cellId(6,20);w.grain=w.communityWood=w.treasury=200;w.ecologyClock=w.townClock=w.populationClock=-100000;w.elapsed=100;for(std::size_t i=0;i<w.residents.size();++i){auto& r=w.residents[i];r.x=5.5f+i*2;r.y=5.5f;r.home=r.goal=isla::cellId(static_cast<int>(r.x),5);r.hunger=r.thirst=r.energy=r.social=r.fun=r.hygiene=99;r.task=isla::Task::Idle;r.path.clear();r.next=0;r.decisionTimer=100000;r.hasOrder=false;}w.rebuildNavigation();}
void finish(isla::World& w){check(!w.jobs.empty(),"No queued civic work");int id=w.jobs.back().id;w.orderUnit(0,"job",id);for(int i=0;i<4000&&!w.jobs.empty();++i){w.elapsed=100;w.tick(.1f);}check(w.jobs.empty(),"Civic work never completed");}
int main(){try{
    auto w=std::make_unique<isla::World>(),other=std::make_unique<isla::World>();w->generateIsland(42);other->generateIsland(42);check(w->terrain==other->terrain,"Island seed is not reproducible");other->generateIsland(43);check(w->terrain!=other->terrain,"Distinct seeds produce the same island");
    check(std::none_of(w->terrain.begin(),w->terrain.end(),[](isla::Terrain t){return t==isla::Terrain::Road||t==isla::Terrain::Bridge;}),"Random island has predefined roads");
    for(unsigned seed:{1u,101u,8881u}){w->generateIsland(seed);for(const auto& r:w->residents){check(w->walkable(static_cast<int>(r.x),static_cast<int>(r.y)),"Generated resident intersects terrain/object");check(!w->route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return w->besideFreshWater(x,y);}).empty(),"Generated settlement cannot reach water");}}
    int coast=-1;for(int k=isla::mapWidth+1;k<(isla::mapHeight-1)*isla::mapWidth-1;++k)if(w->terrain[k]==isla::Terrain::Sea&&w->terrain[k+1]==isla::Terrain::Ground){coast=k;break;}check(coast>=0,"No generated shore");int land=std::count(w->terrain.begin(),w->terrain.end(),isla::Terrain::Ground);w->growIsland(coast);check(std::count(w->terrain.begin(),w->terrain.end(),isla::Terrain::Ground)>land,"Island cannot expand");check(w->save("island-test.json")&&other->load("island-test.json")&&other->terrain==w->terrain&&other->islandSeed==w->islandSeed,"Expanded generated island does not persist");
    flat(*w);w->elevation[isla::cellId(9,5)]=3;check(!w->canStep(8,5,9,5),"Resident can climb a vertical terrain drop");w->elevation[isla::cellId(12,12)]=3;check(!w->placeable(isla::cellId(12,12)),"Construction accepts an uneven cliff foundation");
    flat(*w);w->designate("townhall",isla::cellId(12,12));finish(*w);check(w->propAt(isla::cellId(12,12))->kind=="townhall"&&w->market==isla::cellId(12,14),"Townhall does not become the civic center");
    w->designate("altar",isla::cellId(18,12));finish(*w);check(w->propAt(isla::cellId(18,12))->kind=="altar"&&!w->walkable(18,12),"Altar lacks independent identity or collision");
    w->designate("dock",isla::cellId(1,12));finish(*w);check(w->propAt(isla::cellId(1,12))->kind=="dock"&&w->ships.size()==1&&w->ships[0].seaHome>=0,"Constructing a dock did not provide a functional boat");
    check(w->save("civic-test.json")&&other->load("civic-test.json")&&other->propAt(isla::cellId(12,12))->kind=="townhall","Building types or dynamic ports lost on load");
    flat(*w);w->designate("house",isla::cellId(12,12));w->orderUnit(0,"job",w->jobs[0].id);w->elapsed=isla::World::daySeconds*21/24;
    for(int i=0;i<100;++i)w->tick(.1f);for(const auto& r:w->residents)check(r.task==isla::Task::Rest,"A healthy resident keeps working instead of sleeping at night");check(w->jobs.size()==1&&w->jobs[0].worker<0,"Night interruption lost the construction or kept a sleeping worker reserved");
    flat(*w);int road=isla::cellId(14,8);w->terrain[road]=isla::Terrain::Road;w->save("old-road-test.json");json old;{std::ifstream input("old-road-test.json");input>>old;}old["version"]=7;{std::ofstream output("old-road-test.json");output<<old.dump();}check(other->load("old-road-test.json")&&other->terrain[road]==isla::Terrain::Ground,"Legacy roads were restored instead of migrated");
    other->terrain[road]=isla::Terrain::Road;other->traffic[road]=12;other->save("new-road-test.json");check(w->load("new-road-test.json")&&w->terrain[road]==isla::Terrain::Road,"Migration removed a new legitimate NPC road on every load");
    std::cout<<"PASS: random seeded islands, navigability, growth/save, civic work, functional docks, night sleep and one-time road migration\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

