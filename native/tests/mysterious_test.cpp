#include "MysteriousPower.h"
#include "MysteriousGesture.h"
#include "SeaLife.h"
#include "World.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void clear(isla::World& w){
    w.props.clear();w.crops.clear();w.animals.clear();w.seaFish.clear();w.ships.clear();w.jobs.clear();w.gifts.clear();w.camps.clear();
    w.terrain.fill(isla::Terrain::Ground);w.elevation.fill(0);w.forest.fill(false);w.protectedLand.fill(false);w.mysteriousReadyAt=0;
    for(auto& r:w.residents){r=isla::Resident{};r.alive=false;r.home=r.goal=isla::cellId(10,10);r.x=r.y=10.5f;}
    w.residents[0].alive=true;
    for(int y=10;y<40;++y)for(int x=40;x<100;++x)w.terrain[isla::cellId(x,y)]=isla::Terrain::Sea;
    w.rebuildNavigation();
}
int main(){try{
    using namespace isla;auto w=std::make_unique<World>(),loaded=std::make_unique<World>();clear(*w);
    const MysteryTarget person{MysteryKind::Resident,0};auto& r=w->residents[0];r.task=Task::Till;r.path={cellId(11,10)};r.hasOrder=true;
    check(MysteriousPower::pick(*w,10.5f,10.3f).kind==MysteryKind::Resident,"Cannot pick a resident");
    check(MysteriousPower::move(*w,person,cellId(12,12),1000),"Resident not moved");
    check(r.x==12.5f&&r.y==12.5f&&r.path.empty()&&!r.hasOrder&&r.task==Task::Idle,"Resident kept stale movement or work after teleport");
    check(w->mysteriousReadyAt==1180,"Cooldown is not 180 real seconds");
    check(!MysteriousPower::move(*w,person,cellId(14,14),1179)&&r.x==12.5f,"Cooldown can be bypassed");
    check(w->save("mystery-test.json")&&loaded->load("mystery-test.json")&&loaded->mysteriousReadyAt==1180,"Cooldown lost on save/load");
    check(!MysteriousPower::move(*loaded,person,cellId(14,14),1179)&&MysteriousPower::move(*loaded,person,cellId(14,14),1180),"Restored cooldown boundary is wrong");
    w->mysteriousReadyAt=0;check(!MysteriousPower::move(*w,person,cellId(50,20),1200)&&w->mysteriousReadyAt==0,"Invalid water drop consumed cooldown or drowned a resident");
    r.ageDays=0;check(MysteriousPower::move(*w,person,cellId(14,14),1200),"A child cannot be carried");
    clear(*w);w->animals.push_back({15.5f,15.5f,2,0,0,true});
    check(MysteriousPower::move(*w,{MysteryKind::Animal,0},cellId(17,17),1000)&&w->animals[0].x==17.5f,"Land animal not moved");
    clear(*w);w->props.push_back({20,30,.9f,.9f,3,false,"Casa",777,0});r.home=cellId(20,32);r.insideHouse=777;r.x=20.5f;r.y=30.5f;w->rebuildNavigation();
    check(MysteriousPower::move(*w,{MysteryKind::Prop,777},cellId(25,35),1000),"House not moved");
    check(r.home==cellId(25,37)&&r.x==25.5f&&r.y==35.5f&&r.insideHouse==777,"Indoor family or home shifted twice or detached from house");
    w->mysteriousReadyAt=0;check(!MysteriousPower::move(*w,{MysteryKind::Prop,777},cellId(25,mapHeight-1),1000),"House moved its residents' homes beyond the map edge");
    w->mysteriousReadyAt=0;w->animals.push_back({29.5f,39.5f,2,0,0,true});
    check(!MysteriousPower::move(*w,{MysteryKind::Prop,777},cellId(30,40),1000)&&w->props[0].x==25,"House overlapped an animal or failed nonatomically");
    clear(*w);w->seaFish.push_back({50.5f,20.5f,0,2,true});
    check(MysteriousPower::pick(*w,50.5f,20.5f).kind==MysteryKind::Fish,"Fish cannot be picked");
    check(!MysteriousPower::move(*w,{MysteryKind::Fish,0},cellId(20,20),1000),"Fish was dropped on dry land");
    check(MysteriousPower::move(*w,{MysteryKind::Fish,0},cellId(60,20),1000),"Fish not moved through water");
    SeaLife::update(*w,.1f);check(w->seaFish[0].x>60.5f,"Fish stopped swimming after drop");
    check(w->save("mystery-fish-test.json")&&loaded->load("mystery-fish-test.json")&&loaded->seaFish.size()==1&&loaded->seaFish[0].x==w->seaFish[0].x,"Moved fish does not persist");
    clear(*w);const int sea=cellId(55,25),pool=cellId(20,20);
    check(MysteriousPower::move(*w,{MysteryKind::Water,sea},pool,1000)&&w->terrain[sea]==Terrain::Ground&&w->terrain[pool]==Terrain::Sea,"Sea water did not actually change geography");
    check(w->terrainRevision>0&&!w->walkable(20,20),"Water movement did not update navigation and terrain");
    clear(*w);w->crops.push_back({cellId(15,15),3});
    check(MysteriousPower::move(*w,{MysteryKind::Crop,cellId(15,15)},cellId(16,16),1000)&&w->crops[0].stage==3&&w->crops[0].cell==cellId(16,16),"Crop state lost during move");
    clear(*w);MysteriousGesture gesture;
    check(!gesture.update(*w,{true,true,true,100,100,10.5f,10.3f},1000),"Press alone triggered power");
    check(!gesture.update(*w,{false,false,true,100,100,10.5f,10.3f},1000.1)&&w->mysteriousReadyAt==0,"Inspection click consumed power");
    gesture.update(*w,{true,true,true,100,100,10.5f,10.3f},1001);
    gesture.update(*w,{false,true,true,140,140,14.5f,14.5f},1001.3);check(gesture.active(),"Held drag did not lift target");
    check(gesture.update(*w,{false,false,true,140,140,14.5f,14.5f},1001.4)&&r.x==14.5f,"Drag release did not commit move");
    check(!gesture.update(*w,{true,true,true,140,140,14.5f,14.5f},1002)&&!gesture.active(),"Cooldown gesture was not silent/inactive");
    clear(*w);w->camps.push_back({cellId(20,20),3,0});
    check(MysteriousPower::move(*w,{MysteryKind::Camp,3},cellId(21,21),1000)&&w->camps[0].cell==cellId(21,21),"Camp did not use its stable owner identity");
    check(w->save("mystery-old-save.json"),"Cannot save migration fixture");
    nlohmann::json old;{std::ifstream input("mystery-old-save.json");input>>old;}old.erase("seaFish");old.erase("mysteriousReadyAt");
    {std::ofstream output("mystery-old-save.json");output<<old;}
    check(loaded->load("mystery-old-save.json")&&loaded->mysteriousReadyAt==0&&!loaded->seaFish.empty(),"Existing v9 island cannot gain the hidden power and sea life");
    std::cout<<"PASS: draggable residents/animals/houses/crops/fish/water, valid destinations, atomic cancellation, 180-second saved cooldown and hidden mouse gesture\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

