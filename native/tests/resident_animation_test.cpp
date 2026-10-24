#include "ResidentAnimation.h"
#include "PresentationMotion.h"
#include "World.h"
#include <iostream>
#include <memory>
#include <stdexcept>

void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    using namespace isla;Resident r;r.x=r.y=10.5f;r.task=Task::Till;
    PresentationClock visualClock;Resident walker;walker.x=walker.y=10.5f;walker.path={cellId(11,10)};
    int walkFrames=0,workFrames=0;double simulationTime=100;
    for(int frame=0;frame<120;++frame){
        visualClock.advance(1.0f/60,false);simulationTime+=.4;
        walkFrames|=1<<ResidentAnimation::select(walker,simulationTime,visualClock.seconds()).frame;
        workFrames|=1<<ResidentAnimation::select(r,simulationTime,visualClock.seconds()).frame;
    }
    check(walkFrames==((1<<1)|(1<<5))&&workFrames==3,"Animation aliases to a frozen frame at accelerated simulation speed");
    const auto pausedTime=visualClock.seconds();visualClock.advance(.2f,true);
    check(visualClock.seconds()==pausedTime,"Presentation continues while paused");
    check(ResidentAnimation::select(r,100).pose==ResidentPose::Till,"Working resident lacks hoe pose");
    r.actionTimer=.4f;check(ResidentAnimation::select(r,100).frame==1,"Hoe does not swing");
    r.path={cellId(11,10)};check(ResidentAnimation::select(r,100).pose==ResidentPose::Walk,"Tool swings while walking to work");
    r.path.clear();r.task=Task::Gather;check(ResidentAnimation::select(r,100).pose==ResidentPose::Chop,"Chopping lacks axe pose");
    r.ageDays=World::adultAgeDays-1;r.familyCourtship=2;r.female=true;r.pregnancyDue=200;
    check(ResidentAnimation::select(r,100).pose==ResidentPose::Walk,"Minor received adult action pose");
    r.ageDays=World::adultAgeDays;check(ResidentAnimation::select(r,100).pose==ResidentPose::Courtship,"Adult courtship invisible");
    r.familyCourtship=5;check(ResidentAnimation::select(r,100).frame==7,"Surprise never resolves into reciprocal greeting");
    r.familyCourtship=0;r.task=Task::Idle;r.pregnancyDue=100+World::gestationSeconds*.5f;
    check(ResidentAnimation::select(r,100).pose==ResidentPose::Pregnant&&ResidentAnimation::select(r,100).frame==0,"Mid pregnancy missing");
    r.pregnancyDue=100+World::gestationSeconds*.1f;check(ResidentAnimation::select(r,100).frame==4,"Late pregnancy does not change silhouette");
    r.pregnancyDue=-1;check(ResidentAnimation::select(r,100).pose==ResidentPose::Walk,"Pregnancy pose survives birth");
    auto w=std::make_unique<World>(),saved=std::make_unique<World>();
    PresentationMotion motion;const auto originalX=w->residents[0].x;
    motion.capture(*w);w->residents[0].x+=.1f;
    const auto quarter=motion.resident(*w,0,.25f),threeQuarters=motion.resident(*w,0,.75f);
    check(quarter.x<threeQuarters.x&&quarter.x>originalX&&threeQuarters.x<w->residents[0].x,"Resident positions still advance in fixed-step jumps");
    check(w->residents[0].x==originalX+.1f,"Rendering changed authoritative simulation position");
    w->residents[0].x+=10;check(motion.resident(*w,0,.5f).x==w->residents[0].x,"Teleport was interpolated across the island");
    w->props.clear();w->animals.clear();w->ships.clear();w->terrain.fill(Terrain::Ground);w->elevation.fill(0);w->forest.fill(false);
    for(int y=0;y<mapHeight;++y)for(int x=16;x<mapWidth;++x)w->terrain[cellId(x,y)]=Terrain::Sea;
    for(auto& p:w->residents)p.alive=false;
    auto& sailor=w->residents[0];sailor=Resident{};sailor.alive=true;sailor.x=15.5f;sailor.y=10.5f;sailor.home=cellId(15,10);
    Ship boat;boat.cell=cellId(15,10);boat.seaHome=cellId(16,10);boat.x=16.5f;boat.y=10.5f;w->ships.push_back(boat);w->rebuildNavigation();
    w->startFishing(0,0);sailor.path.clear();sailor.next=0;w->updateFishing(1);
    check(!sailor.inBoat&&w->ships[0].state=="boarding"&&w->ships[0].fishingClock==1,"Boat launched without pushing phase");
    check(w->save("launch-animation-test.json")&&saved->load("launch-animation-test.json")&&saved->ships[0].fishingClock==1,"Launching progress lost on save");
    saved->updateFishing(2);check(saved->residents[0].inBoat&&saved->ships[0].state=="outbound","Launching did not transition into rowing");
    std::cout<<"PASS: action selection, age boundaries, pregnancy stages and saved launch-to-row transition\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


