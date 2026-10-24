#include "World.h"
#include "ResidentActions.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
int cell(const isla::Resident& r){return isla::cellId(static_cast<int>(r.x),static_cast<int>(r.y));}
void fixture(isla::World& w){
    w.props.clear();w.crops.clear();w.animals.clear();w.camps.clear();w.jobs.clear();w.events.clear();w.metrics.clear();
    w.forest.fill(false);w.pasture.fill(false);w.protectedLand.fill(false);w.fallow.fill(false);w.traffic.fill(0);
    for(int y=0;y<isla::mapHeight;++y)for(int x=0;x<isla::mapWidth;++x)w.terrain[isla::cellId(x,y)]=x==0||y==0||x==isla::mapWidth-1||y==isla::mapHeight-1?isla::Terrain::Sea:isla::Terrain::Ground;
    w.terrain[isla::cellId(8,8)]=isla::Terrain::FreshWater;w.terrain[isla::cellId(8,9)]=isla::Terrain::FreshWater;
    w.market=isla::cellId(5,20);w.granary=isla::cellId(6,20);w.grain=200;w.communityWood=200;w.treasury=200;w.livestockFood=0;
    w.ecologyClock=w.townClock=w.populationClock=-100000;w.nextProp=2000;w.elapsed=100;
    for(std::size_t i=0;i<w.residents.size();++i){auto& r=w.residents[i];r.x=5.5f+static_cast<float>(i*2);r.y=5.5f;r.home=cell(r);r.goal=r.home;r.hunger=r.thirst=r.energy=r.social=r.fun=r.hygiene=99;r.health=isla::cnc::maxHealth;r.task=isla::Task::Idle;r.path.clear();r.next=0;r.decisionTimer=100000;r.hasOrder=false;r.job=r.mission=-1;r.inDungeon=r.inBoat=false;r.alive=true;r.relationships.resize(w.residents.size());}
    for(auto& d:w.dungeons){d.state="idle";d.party.clear();}
    for(auto& s:w.ships){s.passenger=-1;s.seaHome=-1;s.remaining=0;s.state="docked";s.path.clear();s.next=0;}
    w.rebuildNavigation();w.updateFishing(0);
}
void advance(isla::World& w,int ticks){for(int i=0;i<ticks;++i){w.tick(.1f);for(const auto& r:w.residents)if(r.alive&&!r.inDungeon&&!r.inBoat)check(w.walkable(static_cast<int>(r.x),static_cast<int>(r.y)),"Resident intersects an obstacle");for(const auto& a:w.animals)if(a.alive)check(w.walkable(static_cast<int>(a.x),static_cast<int>(a.y)),"Animal intersects an obstacle");}}
void finishJob(isla::World& w){check(!w.jobs.empty(),"Expected a queued job");int id=w.jobs.back().id;w.orderUnit(0,"job",id);for(int t=0;t<2500&&!w.jobs.empty();++t)advance(w,1);check(w.jobs.empty(),"Assigned job never completed");}
int main(){try{
    auto world=std::make_unique<isla::World>();
    int mountain=0,water=0;for(auto t:world->terrain){mountain+=t==isla::Terrain::Mountain;water+=t==isla::Terrain::FreshWater;}
    check(mountain>100&&water>30,"Missing hard geology or river");
    check(std::none_of(world->terrain.begin(),world->terrain.end(),[](isla::Terrain t){return t==isla::Terrain::Road||t==isla::Terrain::Bridge;}),"New islands must not contain predefined roads");
    for(const auto& r:world->residents){check(world->walkable(static_cast<int>(r.x),static_cast<int>(r.y)),"Invalid authored spawn");auto path=world->route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return world->besideFreshWater(x,y);});check(!path.empty(),"Authored map has no route to fresh water");}
    for(const auto& p:world->props)check(!world->walkable(static_cast<int>(p.x),static_cast<int>(p.y)),"Visible object lacks collision");
    fixture(*world);
    // Livestock occupies its pasture cell: collect from a free neighbour,
    // then eat, rather than endlessly attempting to enter the animal's cell.
    for(std::size_t i=1;i<world->residents.size();++i)world->residents[i].alive=false;
    auto& hungry=world->residents[0];hungry.hunger=1;
    world->grain=0;world->livestockFood=3;
    const int pasture=isla::cellId(6,5);world->pasture[pasture]=true;
    world->animals.push_back({6.5f,5.5f,100000,0,0,true});
    check(isla::ResidentActions(*world).begin(hungry,isla::Task::Tend,pasture),"No free approach to livestock");
    advance(*world,150);
    check(hungry.alive&&hungry.hunger>20&&world->metrics["eat"]>0,"Occupied pasture prevented a hungry resident from eating");
    check(world->livestockFood==0&&hungry.eggs==2,"Livestock food was not conserved");
    // A starving hunter must use stored food even when the preferred hunt
    // has no prey. Occupation preferences must not override survival.
    fixture(*world);
    for(std::size_t i=1;i<world->residents.size();++i)world->residents[i].alive=false;
    world->residents[0].trait="Cazadora";world->residents[0].hunger=1;
    world->granary=isla::cellId(6,5);world->grain=3;
    advance(*world,150);
    check(world->residents[0].alive&&world->residents[0].hunger>20&&world->metrics["grain_eaten"]>0,"Starving hunter ignored available stored grain");
    fixture(*world);
    // Demand must produce actual worker jobs, and abandoned roads must be reclaimed.
    for(std::size_t i=1;i<world->residents.size();++i)world->residents[i].alive=false;
    world->props.push_back({40.5f,40.5f,.3f,.3f,3,false,"Casa",9000,0,100});world->rebuildNavigation();
    int usedRoad=isla::cellId(14,8),unusedGround=isla::cellId(16,8);
    world->traffic[usedRoad]=8;world->townClock=0;world->updateEcology(20);
    check(world->jobs.size()==1&&world->jobs[0].kind=="road"&&world->jobs[0].cell==usedRoad,"NPC road demand did not follow traffic");
    check(world->terrain[usedRoad]==isla::Terrain::Ground,"Road appeared before worker construction");
    finishJob(*world);
    check(world->terrain[usedRoad]==isla::Terrain::Road&&world->terrain[unusedGround]==isla::Terrain::Ground,"Worker did not build only the needed road");
    for(int i=0;i<12&&world->jobs.empty();++i){world->townClock=0;world->updateEcology(20);}
    check(!world->jobs.empty()&&world->jobs[0].kind=="remove_road","Disused road did not create a removal job");
    finishJob(*world);check(world->terrain[usedRoad]==isla::Terrain::Ground,"Worker did not reclaim an obsolete road");
    fixture(*world);
    const int house=isla::cellId(12,12),destination=isla::cellId(18,12);
    world->designate("house",house);finishJob(*world);
    check(world->propAt(house)&&!world->walkable(12,12)&&!world->walkable(13,13),"House footprint is not solid");
    world->animals.push_back({11.5f,12.5f,0,0,0,true});world->pasture.fill(true);advance(*world,100);world->pasture.fill(false);world->animals.clear();
    const int uid=world->propAt(house)->uid;world->relocate(house,destination);finishJob(*world);
    check(!world->propAt(house)&&world->propAt(destination)&&world->propAt(destination)->uid==uid&&world->walkable(12,12),"House relocation did not preserve the entity and update collision");
    world->designate("demolish",destination);finishJob(*world);
    check(!world->propAt(destination)&&world->walkable(18,12),"Demolition left a visible or solid house");
    fixture(*world);
    const int farm=isla::cellId(12,10);world->crops.push_back({farm,3,0,-1,120,0});world->rebuildNavigation();
    check(!world->walkable(12,10),"Growing crops must block entry");int oldGrain=world->residents[0].grain,oldSeeds=world->residents[0].seeds;
    world->orderUnit(0,"work",farm);for(int t=0;t<1200&&world->harvests==0;++t)advance(*world,1);
    check(world->harvests==1&&world->residents[0].grain==oldGrain+3&&world->residents[0].seeds==oldSeeds+2,"Harvest failed from outside the crop footprint");
    world->orderUnit(0,"work",farm);for(int t=0;t<600&&world->cropAt(farm)->stage==0;++t)advance(*world,1);
    check(world->cropAt(farm)->stage>0&&!world->walkable(12,10),"Sowing did not consume seed and restore crop collision");
    const int movedFarm=isla::cellId(16,10);world->relocate(farm,movedFarm);finishJob(*world);
    check(!world->cropAt(farm)&&world->cropAt(movedFarm)&&world->cropAt(movedFarm)->cycles==1,"Relocating a farm lost its real state");
    world->designate("fallow",movedFarm);finishJob(*world);check(!world->cropAt(movedFarm)&&world->fallow[movedFarm],"Cleared farm can reappear automatically");
    fixture(*world);
    int tree=isla::cellId(10,10);world->forest[tree]=true;world->props.push_back({10.5f,10.5f,.32f,.32f,2.7f,true,"Tree",55,-1,60});world->rebuildNavigation();
    world->orderUnit(0,"work",tree);for(int t=0;t<1000&&world->propAt(tree);++t)advance(*world,1);
    check(!world->propAt(tree)&&world->walkable(10,10)&&world->residents[0].wood==4,"Cut tree did not disappear or produce wood");
    world->designate("road",isla::cellId(14,8));finishJob(*world);check(world->terrain[isla::cellId(14,8)]==isla::Terrain::Road,"Road work did not change the environment");
    world->relocate(isla::cellId(14,8),isla::cellId(16,8));finishJob(*world);check(world->terrain[isla::cellId(14,8)]==isla::Terrain::Ground&&world->terrain[isla::cellId(16,8)]==isla::Terrain::Road,"Road relocation failed");
    fixture(*world);world->forest.fill(true);world->ecologyClock=29.95f;world->updateEcology(.1f);check(world->metrics["regrow"]>0,"Forest cannot regrow on new free soil");
    fixture(*world);world->terrain[isla::cellId(10,5)]=isla::Terrain::Mountain;auto reply=world->orderUnit(0,"move",isla::cellId(10,5));check(!world->residents[0].hasOrder,"Move order accepted a mountain destination");
    world->orderUnit(0,"move",isla::cellId(9,7));for(int t=0;t<600&&world->residents[0].hasOrder;++t)advance(*world,1);check(cell(world->residents[0])==isla::cellId(9,7),"Direct movement order did not arrive");
    check(isla::World::fishingLoss(.249999f)&&!isla::World::fishingLoss(.25f),"Fishing loss threshold is not exactly 25 percent");
    fixture(*world);world->startFishing(0,0);check(world->ships[0].state=="boarding","Fishing lacks a physical boarding phase");
    bool sailed=false;for(int t=0;t<2500&&world->ships[0].passenger>=0;++t){advance(*world,1);const auto& s=world->ships[0];if(world->residents[0].inBoat){sailed=true;check(world->terrain[isla::cellId(static_cast<int>(s.x),static_cast<int>(s.y))]==isla::Terrain::Sea,"Boat sailed across land");}}
    check(sailed&&world->ships[0].passenger<0,"Boat never completed its voyage");
    check((world->residents[0].alive&&world->residents[0].fish>=4)||(!world->residents[0].alive&&world->metrics["lost_at_sea"]==1),"Voyage did not yield fish or a real single population loss");
    fixture(*world);world->props.push_back({21,11,.9f,.9f,3,false,"Vacant",999,-1,100});world->rebuildNavigation();world->populationClock=179.95f;advance(*world,1);check(world->residents.size()==7,"Population cannot grow when housing and supplies exist");
    const std::string save="environment-test-save.json";check(world->save(save),"Cannot save modified environment");auto loaded=std::make_unique<isla::World>();check(loaded->load(save)&&loaded->residents.size()==7&&loaded->props.size()==world->props.size(),"Cannot restore modified environment and population");
    std::cout<<"PASS: object and animal collision, construction, demolition, relocation, fields, tree cutting/regrowth, direct orders, fishing risk, population and save/load\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}


