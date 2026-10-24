#include "ActionPreview.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
// Deterministic capture fixture. Called only by --capture --actions-preview.
// Uses the gameplay renderer and never loads or writes a player's save.
void setupActionPreview(World& w,int phase) {
    w.elapsed=100+phase*.4;w.animals.clear();w.ships.clear();w.crops.clear();
    w.props.erase(std::remove_if(w.props.begin(),w.props.end(),[](const Prop& p){return p.x>99&&p.x<121&&p.y>103&&p.y<122;}),w.props.end());
    for(int y=103;y<122;++y)for(int x=100;x<121;++x) {
        const int cell=cellId(x,y);w.terrain[cell]=Terrain::Ground;w.elevation[cell]=1;w.forest[cell]=false;
    }
    for(int y=116;y<121;++y)for(int x=106;x<120;++x) {w.terrain[cellId(x,y)]=Terrain::Sea;w.elevation[cellId(x,y)]=0;}
    std::vector<Resident> people;
    auto add=[&](float x,float y,bool female,const char* name)->Resident& {
        Resident r;r.name=name;r.female=female;r.x=x;r.y=y;r.home=r.goal=cellId(static_cast<int>(x),static_cast<int>(y));
        r.actionTimer=phase==0?0:.4f;r.decisionTimer=100000;people.push_back(r);return people.back();
    };
    people.reserve(12);
    add(103.5f,106.5f,false,"Labrando").task=Task::Till;
    add(108.5f,106.5f,true,"Labrando").task=Task::Till;
    add(113.5f,106.5f,false,"Talando").task=Task::Gather;
    add(118.5f,106.5f,true,"Talando").task=Task::Gather;
    w.props.push_back({114,106,.5f,.5f,3,true,"Tree",1901});
    w.props.push_back({119,106,.5f,.5f,3,true,"Tree",1902});
    add(103.5f,112.5f,true,"Cortejo").familyCourtship=phase==0?1:5;
    add(105.0f,112.5f,false,"Sorpresa").familyCourtship=phase==0?1:5;
    people[4].familyPartner=5;people[5].familyPartner=4;
    add(110.5f,112.5f,true,"Embarazo medio").pregnancyDue=static_cast<float>(w.elapsed+World::gestationSeconds*.5);
    add(115.5f,112.5f,true,"Embarazo avanzado").pregnancyDue=static_cast<float>(w.elapsed+World::gestationSeconds*.15);
    add(105.5f,117.5f,true,"Botando").task=Task::Fish;
    Ship launch;launch.cell=cellId(105,117);launch.seaHome=cellId(106,117);launch.x=106.5f;launch.y=117.5f;
    launch.state="boarding";launch.passenger=8;launch.fishingClock=1.5f;w.ships.push_back(launch);
    for(int i=0;i<2;++i) {
        add(111.5f+i*5,118.5f,i==0,"Remando").inBoat=true;
        Ship boat;boat.cell=cellId(105,117);boat.seaHome=cellId(106,117);boat.x=111.5f+i*5;boat.y=118.5f;
        boat.state="outbound";boat.passenger=9+i;boat.path={cellId(static_cast<int>(boat.x)+1,118)};w.ships.push_back(boat);
    }
    if(phase==2) {
        people[4].familyCourtship=people[5].familyCourtship=0;
        people[4].familyMomentRemaining=people[5].familyMomentRemaining=4;
        people[4].pregnancyFather=5;people[4].pregnancyDue=static_cast<float>(w.elapsed+World::gestationSeconds);
    }
    w.residents.restore(std::move(people));w.rebuildNavigation();++w.terrainRevision;
}
}


