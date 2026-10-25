#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
void World::updatePopulation(float dt){
    populationClock+=dt;
    if(populationClock<180||night())return;
    // Retry a blocked doorway next tick instead of discarding a whole arrival period.
    populationClock=std::min(populationClock,180.0f);
    if(residents.size()>=1000)return;
    int present=0,women=0,men=0;
    for(const auto& r:residents)if(r.alive&&!r.visitor){++present;if(r.ageDays>=adultAgeDays)(r.female?women:men)++;}
    if(grain<std::max(reserve,present*2)+2||treasury<8)return;
    for(auto& house:props){
        if(house.tree||house.kind!="house"||(house.owner>=0&&house.owner<static_cast<int>(residents.size())&&residents[house.owner].alive))continue;
        int home=-1;
        const int hx=static_cast<int>(house.x),hy=static_cast<int>(house.y);
        for(int radius=1;radius<=4&&home<0;++radius)for(int y=hy-radius;y<=hy+radius&&home<0;++y)for(int x=hx-radius;x<=hx+radius;++x){
            if(!walkable(x,y)||reserved(cellId(x,y)))continue;
            bool occupied=false;
            for(const auto& r:residents)if(r.alive&&!r.away&&!r.inBoat&&!r.inDungeon&&r.insideHouse<0&&cellId(static_cast<int>(r.x),static_cast<int>(r.y))==cellId(x,y))occupied=true;
            for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==cellId(x,y))occupied=true;
            if(occupied||route(x,y,[&](int xx,int yy){return besideFreshWater(xx,yy);}).empty()||route(x,y,[&](int xx,int yy){return cellId(xx,yy)==market;}).empty())continue;
            home=cellId(x,y);break;
        }
        if(home<0)continue;
        Resident newcomer;newcomer.female=women==men?random()%2==0:women<men;
        newcomer.name="Habitante "+std::to_string(residents.size()+1);newcomer.trait=newcomer.female?"Agricultora":"Trabajador";
        newcomer.home=newcomer.goal=home;newcomer.x=home%mapWidth+.5f;newcomer.y=home/mapWidth+.5f;newcomer.grain=2;
        const int index=residents.add(std::move(newcomer));house.owner=index;grain-=2;treasury-=8;populationClock=0;
        ++metrics["arrivals"];record("Llego un nuevo habitante: vivienda, provisiones y acceso al agua disponibles");return;
    }
}
}
