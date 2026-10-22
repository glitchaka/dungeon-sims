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
void village(isla::World& w){flat(w);for(std::size_t i=0;i<w.residents.size();++i){w.props.push_back({20.0f+3*i,30,.9f,.9f,3,false,"Home",w.nextProp++,static_cast<int>(i),100});w.residents[i].home=isla::cellId(20+3*static_cast<int>(i),32);w.residents[i].inspiration.clear();}w.rebuildNavigation();w.townClock=19.9f;}
void autonomousFinish(isla::World& w){w.townClock=-100000;w.residents[0].decisionTimer=0;for(int i=0;i<5000&&!w.jobs.empty();++i){w.elapsed=100;w.tick(.1f);}check(w.jobs.empty(),"Autonomous job was not performed without a player order");}
int main(){try{
    {
        auto legacy=std::make_unique<isla::World>(),loaded=std::make_unique<isla::World>();flat(*legacy);
        legacy->terrain[isla::cellId(120,80)]=isla::Terrain::Road;
        legacy->gifts.push_back({isla::cellId(120,82),12,8});
        legacy->residents[0].path={isla::cellId(5,5),isla::cellId(6,5)};
        check(legacy->save("legacy-size-test.json"),"Cannot write migration fixture");
        json old;{std::ifstream input("legacy-size-test.json");input>>old;}
        old["version"]=8;old.erase("mapWidth");old.erase("mapHeight");
        auto oldCell=[](json& v){int k=v.get<int>();if(k>=0)v=k/isla::mapWidth*128+k%isla::mapWidth;};
        for(const char* key:{"terrain","forest","pasture","protected","fallow","traffic","elevation"}){json grid=json::array();for(int y=0;y<96;++y)for(int x=0;x<128;++x)grid.push_back(old[key][isla::cellId(x,y)]);old[key]=std::move(grid);}
        for(auto& r:old["residents"]){for(const char* key:{"goal","home","orderedCell","workCell"})oldCell(r[key]);for(auto& k:r["path"])oldCell(k);}
        for(auto& d:old["dungeons"])oldCell(d["entrance"]);
        for(auto& gift:old["gifts"])oldCell(gift["cell"]);
        oldCell(old["market"]);oldCell(old["granary"]);
        {std::ofstream output("legacy-size-test.json");output<<old.dump();}
        check(loaded->load("legacy-size-test.json"),"128x96 legacy island cannot load into larger world");
        check(loaded->terrain[isla::cellId(120,80)]==isla::Terrain::Road&&loaded->terrain[isla::cellId(180,80)]==isla::Terrain::Sea,"Legacy island cells or new sea were corrupted");
        check(loaded->gifts[0].cell==isla::cellId(120,82)&&loaded->residents[0].path==legacy->residents[0].path&&loaded->market==legacy->market,"Legacy cell references were not remapped");
        check(loaded->save("large-migrated-test.json")&&legacy->load("large-migrated-test.json")&&legacy->terrain==loaded->terrain,"Migrated large island does not persist");
    }
    auto w=std::make_unique<isla::World>(),other=std::make_unique<isla::World>();w->generateIsland(42);other->generateIsland(42);check(w->terrain==other->terrain,"Island seed is not reproducible");check(isla::mapWidth==256&&isla::mapHeight==192&&std::count(w->terrain.begin(),w->terrain.end(),isla::Terrain::Ground)>15000,"Generated island is not substantially larger");other->generateIsland(43);check(w->terrain!=other->terrain,"Distinct seeds produce the same island");
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
    village(*w);w->grain=0;w->updateEcology(.2f);check(w->jobs.size()==1&&w->jobs[0].kind=="dock","Food shortage did not trigger autonomous dock planning");auto dockCost=w->communityWood;w->townClock=19.9f;w->updateEcology(.2f);check(std::count_if(w->jobs.begin(),w->jobs.end(),[](const isla::Job& j){return j.kind=="dock";})==1&&w->communityWood==dockCost,"Autonomous planner duplicated dock or spent resources twice");
    village(*w);w->grain=200;w->residents[0].inspiration="diplomacy";w->residents[0].inspirationUntil=w->elapsed+100;w->updateEcology(.2f);check(w->jobs.size()==1&&w->jobs[0].kind=="dock","Diplomatic inspiration did not create autonomous harbor demand");
    village(*w);w->grain=200;for(auto& r:w->residents)r.social=20;w->updateEcology(.2f);check(w->jobs.size()==1&&w->jobs[0].kind=="altar","Social need did not trigger autonomous altar");autonomousFinish(*w);check(std::any_of(w->props.begin(),w->props.end(),[](const isla::Prop& p){return p.kind=="altar";}),"Autonomous altar did not become a collidable building");
    village(*w);w->grain=200;w->residents[0].inspiration="build";w->residents[0].inspirationUntil=w->elapsed+100;w->updateEcology(.2f);check(w->jobs.size()==1&&w->jobs[0].kind=="townhall","Building inspiration did not trigger autonomous townhall");
    village(*w);w->grain=200;int priorHome=w->residents[0].home;w->updateEcology(.2f);check(w->jobs.size()==1&&w->jobs[0].kind=="house","Prosperous village did not prepare spare housing");autonomousFinish(*w);check(w->residents[0].home==priorHome&&std::any_of(w->props.begin(),w->props.end(),[](const isla::Prop& p){return p.kind=="house"&&p.owner<0;}),"Spare housing was incorrectly assigned to an already housed builder");w->populationClock=179.95f;w->updateFishing(.1f);check(w->residents.size()==7,"Autonomously built spare housing did not permit population growth");
    village(*w);w->elapsed=isla::World::daySeconds*21/24;w->grain=0;w->updateEcology(.2f);check(w->jobs.empty(),"Civic planner initiated construction at night");
    village(*w);w->communityWood=19;w->grain=0;w->updateEcology(.2f);check(w->jobs.empty(),"Autonomous planner spent emergency resource reserve");
    std::cout<<"PASS: random seeded islands, navigability, growth/save, civic work, functional docks, night sleep and one-time road migration\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

