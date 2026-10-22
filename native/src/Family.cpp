#include "World.h"
#include <algorithm>
#include <cmath>
namespace isla {
void World::updateFamilies(float){
    const std::size_t initial=residents.size();std::vector<Resident> newborns;
    auto available=[](const Resident& r){return r.alive&&!r.away&&!r.visitor&&!r.inBoat&&!r.inDungeon;};
    for(std::size_t i=0;i<initial;++i){auto& mother=residents[i];if(!available(mother)||!mother.female||mother.ageDays<World::adultAgeDays)continue;
        if(mother.pregnancyDue>=0){if(elapsed<mother.pregnancyDue||residents.size()+newborns.size()>=1000)continue;
            int spawn=-1;for(int radius=0;radius<8&&spawn<0;++radius)for(int y=std::max(0,mother.home/mapWidth-radius);y<std::min(mapHeight,mother.home/mapWidth+radius+1)&&spawn<0;++y)for(int x=std::max(0,mother.home%mapWidth-radius);x<std::min(mapWidth,mother.home%mapWidth+radius+1);++x){if(!walkable(x,y))continue;bool busy=false;for(const auto& p:residents)if(available(p)&&p.insideHouse<0&&static_cast<int>(p.x)==x&&static_cast<int>(p.y)==y)busy=true;for(const auto& a:animals)if(a.alive&&static_cast<int>(a.x)==x&&static_cast<int>(a.y)==y)busy=true;for(const auto& child:newborns)if(static_cast<int>(child.x)==x&&static_cast<int>(child.y)==y)busy=true;if(!busy){spawn=cellId(x,y);break;}}
            if(spawn<0)continue;Resident child;child.female=random()%2==0;child.name="Infante "+std::to_string(++metrics["family_births"]);child.trait="Infante";child.ageDays=0;child.naturalDeathAge=(75+random()%21)*360.0f;child.mother=static_cast<int>(i);child.father=mother.pregnancyFather;child.home=mother.home;child.goal=spawn;child.x=spawn%mapWidth+.5f;child.y=spawn/mapWidth+.5f;child.insideHouse=mother.insideHouse;child.seeds=child.money=0;newborns.push_back(child);mother.pregnancyDue=-1;mother.pregnancyFather=-1;mother.postpartumUntil=elapsed+30*daySeconds;record(mother.name+" tuvo un bebe tras nueve meses de gestacion");continue;
        }
        if(elapsed<mother.postpartumUntil)continue;
        for(std::size_t j=0;j<initial;++j){const auto& father=residents[j];if(!available(father)||father.female||father.ageDays<World::adultAgeDays||father.insideHouse!=mother.insideHouse)continue;
            if(mother.mother==static_cast<int>(j)||mother.father==static_cast<int>(j)||father.mother==static_cast<int>(i)||father.father==static_cast<int>(i)||(mother.mother>=0&&mother.mother==father.mother)||(mother.father>=0&&mother.father==father.father))continue;
            if(mother.insideHouse<0&&std::hypot(mother.x-father.x,mother.y-father.y)>2)continue;
            bool alone=true;for(std::size_t k=0;k<initial;++k){if(k==i||k==j)continue;const auto& other=residents[k];if(!available(other)||other.insideHouse!=mother.insideHouse)continue;if(mother.insideHouse>=0||std::hypot(other.x-mother.x,other.y-mother.y)<4||std::hypot(other.x-father.x,other.y-father.y)<4){alone=false;break;}}
            if(alone){mother.pregnancyDue=elapsed+gestationSeconds;mother.pregnancyFather=static_cast<int>(j);mother.familyPartner=-1;residents[j].familyPartner=-1;++metrics["pregnancies"];record(mother.name+" inicio un embarazo; padre: "+father.name+". Nacimiento en nueve meses");break;}
        }
    }
    for(auto& child:newborns)residents.push_back(std::move(child));if(!newborns.empty())for(auto& r:residents)r.relationships.resize(residents.size());
}
}

namespace isla {
const char* World::lifeStageName(const Resident& r){return !r.alive?"FALLECIDO":r.ageDays<childhoodDays?"INFANCIA":r.ageDays<adultAgeDays?"JUVENTUD":r.ageDays<elderAgeDays?"ADULTEZ":"VEJEZ";}
void World::updateLifecycle(float){
    for(auto& r:residents)if(r.familyPartner>=0&&(r.familyPartner>=static_cast<int>(residents.size())||!residents[r.familyPartner].alive))r.familyPartner=-1;
    for(std::size_t i=0;i<residents.size();++i){auto& r=residents[i];if(!r.alive){r.pregnancyDue=-1;r.pregnancyFather=-1;r.familyPartner=-1;continue;}if(r.away||r.visitor||r.ageDays<r.naturalDeathAge)continue;r.alive=false;r.health=0;r.insideHouse=-1;r.pregnancyDue=-1;r.pregnancyFather=-1;r.hasOrder=false;r.path.clear();r.next=0;
        for(auto& crop:crops)if(crop.worker==static_cast<int>(i))crop.worker=-1;for(auto& job:jobs)if(job.worker==static_cast<int>(i))job.worker=-1;
        for(auto& trip:visits)if(!trip.incoming&&trip.resident==static_cast<int>(i)&&trip.state!="complete"){trip.state="cancelled";outbox.erase(trip.id);}
        for(auto& boat:ships)if(boat.passenger==static_cast<int>(i)){boat.passenger=-1;boat.state="lost";}r.inBoat=false;++metrics["natural_deaths"];record(r.name+" fallecio por vejez a los "+std::to_string(static_cast<int>(r.ageDays/360))+" anos");
    }
}
bool World::seekFamily(Resident& r){
    if(night()||elapsed<r.familySeekAfter||r.ageDays<adultAgeDays||r.hunger<35||r.thirst<35||r.energy<25||r.away||r.inBoat||r.inDungeon||r.visitor)return false;
    const int id=static_cast<int>(&r-residents.data());r.familySeekAfter=elapsed+30;
    for(std::size_t i=0;i<residents.size();++i){auto& partner=residents[i];if(!partner.alive||partner.visitor||partner.away||partner.inBoat||partner.inDungeon||partner.female==r.female||partner.ageDays<adultAgeDays||partner.hunger<35||partner.thirst<35||partner.energy<25||partner.hasOrder||partner.mission>=0)continue;
        auto& woman=r.female?r:partner;if(woman.pregnancyDue>=0||woman.postpartumUntil>elapsed)continue;
        if(r.mother==static_cast<int>(i)||r.father==static_cast<int>(i)||partner.mother==id||partner.father==id||(r.mother>=0&&r.mother==partner.mother)||(r.father>=0&&r.father==partner.father))continue;
        if((partner.familyPartner>=0&&partner.familyPartner!=id)||(r.familyPartner>=0&&r.familyPartner!=static_cast<int>(i)))continue;
        std::vector<std::pair<float,int>> sites;const int cx=static_cast<int>((r.x+partner.x)*.5f),cy=static_cast<int>((r.y+partner.y)*.5f);
        for(int y=std::max(1,cy-14);y<std::min(mapHeight-1,cy+15);++y)for(int x=std::max(1,cx-14);x<std::min(mapWidth-2,cx+15);++x){if(!walkable(x,y)||!canStep(x,y,x+1,y))continue;bool privatePlace=true;for(std::size_t k=0;k<residents.size();++k){if(k==i||k==static_cast<std::size_t>(id))continue;const auto& other=residents[k];if(other.alive&&!other.away&&!other.inBoat&&!other.inDungeon&&other.insideHouse<0&&std::hypot(other.x-x-.5f,other.y-y-.5f)<5){privatePlace=false;break;}}if(privatePlace)sites.push_back({std::hypot(r.x-x-.5f,r.y-y-.5f)+std::hypot(partner.x-x-1.5f,partner.y-y-.5f),cellId(x,y)});}
        std::sort(sites.begin(),sites.end());int attempts=0;for(const auto& site:sites){if(++attempts>16)break;int goal=site.second;if(route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return cellId(x,y)==goal;}).empty()||route(static_cast<int>(partner.x),static_cast<int>(partner.y),[&](int x,int y){return cellId(x,y)==goal+1;}).empty())continue;
            if(!begin(r,Task::Social,goal)||!begin(partner,Task::Social,goal+1))continue;r.target=static_cast<int>(i);partner.target=id;r.familyPartner=static_cast<int>(i);partner.familyPartner=id;r.familySeekAfter=partner.familySeekAfter=elapsed+60;++metrics["family_meetings"];record(r.name+" y "+partner.name+" buscan formar una familia");return true;
        }
    }
    return false;
}
}
