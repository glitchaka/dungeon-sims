#include "World.h"
#include <algorithm>
#include <cmath>
namespace isla {
std::string World::miracle(const std::string& kind,int who,int cell){
    if(kind=="rain"){raining=!raining;record(raining?"La deidad envio lluvia":"La deidad despejo el cielo");return raining?"Lluvia enviada":"Lluvia detenida";}
    if(kind=="gift"){
        if(cell<0||cell>=mapWidth*mapHeight||!walkable(cell%mapWidth,cell/mapWidth)||reserved(cell))return "Deja el regalo en suelo libre";
        for(const auto& r:residents)if(r.alive&&!r.away&&!r.inBoat&&!r.inDungeon&&cellId(static_cast<int>(r.x),static_cast<int>(r.y))==cell)return "Elige un lugar sin habitantes";
        for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==cell)return "Elige un lugar sin animales";
        gifts.push_back({cell,12,8});rebuildNavigation();record("Aparecio un regalo divino");return "Regalo dejado; los habitantes lo recogeran";
    }
    if(who<0||who>=static_cast<int>(residents.size()))return "Selecciona un habitante";auto& r=residents[who];
    if(!r.alive||r.away||r.visitor||r.inBoat||r.inDungeon)return "El habitante no esta disponible";
    if(kind=="chosen"){
        if(chosenBirthAt>=0)return "Ya hay un nacimiento elegido pendiente";
        chosenPatron=who;chosenBirthAt=elapsed+gestationSeconds;record("Se anuncio un elegido: nacera dentro de nueve meses");return "Elegido anunciado: nueve meses de 30 dias";
    }
    if(kind=="heal"){r.health=cnc::maxHealth;r.hunger=std::max(r.hunger,60.0f);r.thirst=std::max(r.thirst,60.0f);record(r.name+" recibio un milagro de sanacion");return "Habitante sanado";}
    if(kind!="sustain"&&kind!="build"&&kind!="diplomacy"&&kind!="raid")return "Influencia desconocida";
    if(r.ageDays<World::adultAgeDays)return "El habitante todavia es un menor";
    r.inspiration=kind;r.inspirationUntil=elapsed+daySeconds*30;r.decisionTimer=0;
    record(r.name+" recibio una inspiracion: "+kind);return "Influencia recibida; conserva sus necesidades y decisiones";
}
void World::updateDivinity(float dt){
    for(auto& r:residents)if(r.alive&&!r.away){
        r.ageDays+=static_cast<double>(dt)/daySeconds;
        for(const auto& p:props)if(p.kind=="altar"&&std::hypot(r.x-p.x,r.y-p.y)<4){r.social=std::min(100.0f,r.social+dt*.8f);r.fun=std::min(100.0f,r.fun+dt*.5f);}
        if(r.inspirationUntil<elapsed)r.inspiration.clear();
        if(r.ageDays<World::adultAgeDays&&!r.visitor){if(r.hunger<45&&grain>0){--grain;r.hunger=90;}if(r.thirst<45){
                if(r.homeWater>0){r.homeWater-=.1f;r.thirst=90;}
                else for(auto& carer:residents)if(&carer!=&r&&carer.alive&&!carer.away&&!carer.visitor&&carer.ageDays>=World::adultAgeDays&&carer.homeWater>=.2f&&std::abs(carer.home%mapWidth-r.home%mapWidth)+std::abs(carer.home/mapWidth-r.home/mapWidth)<=10){carer.homeWater-=.2f;r.thirst=90;break;}
            }}
    }
    if(raining)for(auto& c:crops)if(c.stage==1||c.stage==2)c.growth+=dt*.3f;
    for(auto gift=gifts.begin();gift!=gifts.end();){bool collected=false;
        for(auto& r:residents)if(r.alive&&!r.away&&!r.visitor&&!r.inBoat&&!r.inDungeon&&r.ageDays>=World::adultAgeDays&&std::abs(static_cast<int>(r.x)-gift->cell%mapWidth)+std::abs(static_cast<int>(r.y)-gift->cell/mapWidth)==1){grain+=gift->grain;communityWood+=gift->wood;++metrics["divine_gifts"];record(r.name+" recogio el regalo divino");collected=true;break;}
        if(collected){gift=gifts.erase(gift);rebuildNavigation();}else ++gift;
    }
    if(chosenBirthAt<0||elapsed<chosenBirthAt)return;
    int desired=market;if(chosenPatron>=0&&chosenPatron<static_cast<int>(residents.size()))desired=residents[chosenPatron].home;
    int spawn=-1,distance=100000;
    for(int k=0;k<mapWidth*mapHeight;++k)if(walkable(k%mapWidth,k/mapWidth)){
        bool occupied=false;for(const auto& r:residents)if(r.alive&&!r.away&&!r.inBoat&&!r.inDungeon&&cellId(static_cast<int>(r.x),static_cast<int>(r.y))==k)occupied=true;
        for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;
        int d=std::abs(k%mapWidth-desired%mapWidth)+std::abs(k/mapWidth-desired/mapWidth);if(!occupied&&d<distance){spawn=k;distance=d;}
    }
    if(spawn<0)return;
    Resident child;child.female=random()%2==0;child.naturalDeathAge=(75+random()%21)*360.0f;child.name="Elegido "+std::to_string(metrics["chosen_births"]+1);child.trait="Elegido";child.ageDays=0;child.chosen=true;child.home=child.goal=spawn;child.x=spawn%mapWidth+.5f;child.y=spawn/mapWidth+.5f;child.seeds=child.money=0;child.relationships.resize(residents.size()+1);
    if(chosenPatron>=0&&chosenPatron<static_cast<int>(residents.size())&&residents[chosenPatron].ageDays>=World::adultAgeDays){if(residents[chosenPatron].female)child.mother=chosenPatron;else child.father=chosenPatron;child.home=residents[chosenPatron].home;child.insideHouse=residents[chosenPatron].insideHouse;}
    residents.push_back(std::move(child));for(auto& r:residents)r.relationships.resize(residents.size());chosenBirthAt=-1;chosenPatron=-1;++metrics["chosen_births"];record("Nacio el elegido despues de nueve meses. Es un recien nacido, no un trabajador adulto.");
}
}


