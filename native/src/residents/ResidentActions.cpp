#include "ResidentActions.h"
#include "World.h"
#include <algorithm>
#include <cmath>
namespace isla {
namespace {
int position(const Resident& r) { return cellId(static_cast<int>(r.x),static_cast<int>(r.y)); }
float dist(int a,int b) { return std::hypot(static_cast<float>(a%mapWidth-b%mapWidth),static_cast<float>(a/mapWidth-b/mapWidth)); }
}
bool ResidentActions::begin(Resident& r,Task task,int goal){
    if(goal<0||goal>=mapWidth*mapHeight)return false;
    if(task!=Task::Rest&&r.insideHouse>=0&&!leaveShelter(r))return false;
    const int workTarget=goal;
    if(task==Task::Hunt||task==Task::Tend||task==Task::Fetch||task==Task::Deposit){
        // Work beside an animal or shared store instead of waiting forever to
        // enter its occupied cell. Other residents can use another free side.
        goal=world_.nearest(r,[&](int x,int y){
            if(std::abs(x-workTarget%mapWidth)+std::abs(y-workTarget/mapWidth)>1)return false;
            for(const auto& other:world_.residents)
                if(&other!=&r&&other.alive&&other.insideHouse<0&&!other.inBoat&&!other.inDungeon&&!other.away&&position(other)==cellId(x,y))return false;
            for(const auto& animal:world_.animals)
                if(animal.alive&&static_cast<int>(animal.x)==x&&static_cast<int>(animal.y)==y)return false;
            return true;
        });
    }
    if(task==Task::Sow||task==Task::Harvest||task==Task::Till) goal=world_.nearest(r,[&](int x,int y){return std::abs(x-workTarget%mapWidth)+std::abs(y-workTarget/mapWidth)==1;});
    if(goal<0)return false;
    auto path=world_.route(static_cast<int>(r.x),static_cast<int>(r.y),[=](int x,int y){return cellId(x,y)==goal;});if(path.empty())return false;
    const int id=world_.residents.indexOf(r);for(auto& c:world_.crops)if(c.worker==id)c.worker=-1;
    for(auto& j:world_.jobs)if(j.worker==id&&task!=Task::Project)j.worker=-1;
    r.workCell=(task==Task::Sow||task==Task::Harvest||task==Task::Till)?workTarget:-1;r.task=task;r.path=std::move(path);r.next=0;r.goal=goal;r.actionTimer=0;r.decisionTimer=cnc::wanderInterval;
    if(task==Task::Sow||task==Task::Harvest){auto* crop=world_.cropAt(workTarget);if(crop)crop->worker=id;}
    return true;
}

void ResidentActions::finish(Resident& r,const std::string& metric,const std::string& note){if(r.hasOrder&&r.task==r.orderedTask)r.hasOrder=false;r.workCell=-1;++world_.metrics[metric];world_.record(r.name+" "+note);for(auto& c:world_.crops)if(c.worker==world_.residents.indexOf(r))c.worker=-1;r.task=Task::Idle;r.path.clear();r.next=0;r.actionTimer=0;r.decisionTimer=0;}

bool ResidentActions::leaveShelter(Resident& r){
    if(r.insideHouse<0)return true;
    int best=-1;float distance=1e9f;
    for(int y=std::max(0,static_cast<int>(r.y)-5);y<std::min(mapHeight,static_cast<int>(r.y)+6);++y)for(int x=std::max(0,static_cast<int>(r.x)-5);x<std::min(mapWidth,static_cast<int>(r.x)+6);++x){if(!world_.walkable(x,y))continue;bool busy=false;for(const auto& p:world_.residents)if(&p!=&r&&p.alive&&p.insideHouse<0&&!p.away&&!p.inBoat&&!p.inDungeon&&static_cast<int>(p.x)==x&&static_cast<int>(p.y)==y)busy=true;for(const auto& a:world_.animals)if(a.alive&&static_cast<int>(a.x)==x&&static_cast<int>(a.y)==y)busy=true;float d=std::hypot(x+.5f-r.x,y+.5f-r.y);if(!busy&&d<distance){best=cellId(x,y);distance=d;}}
    if(best<0)return false;r.x=best%mapWidth+.5f;r.y=best/mapWidth+.5f;r.insideHouse=-1;return true;
}

void ResidentActions::act(Resident& r,float dt){
    r.actionTimer+=dt;const int at=position(r);auto done=[&](const std::string& key,const std::string& note){finish(r,key,note);};
    switch(r.task){
    case Task::Drink:if(!world_.besideFreshWater(at%mapWidth,at/mapWidth)){r.task=Task::Idle;break;}r.thirst=std::min(100.0f,r.thirst+9*dt);if(r.thirst>=98){r.homeWater=4;done("drink","bebio agua dulce");}break;
    case Task::DrinkHome:r.thirst=std::min(100.0f,r.thirst+9*dt);r.homeWater=std::max(0.0f,r.homeWater-.3f*dt);if(r.thirst>=98||r.homeWater<=0)done("drink","uso su reserva de agua");break;
    case Task::Bathe:r.hygiene=std::min(100.0f,r.hygiene+7*dt);if(r.hygiene>=98)done("bathe","se lavo en el rio");break;
    case Task::Eat:if(r.actionTimer>=4){if(r.eggs>0){--r.eggs;r.hunger=std::min(100.0f,r.hunger+36);}else if(r.grain>0){--r.grain;++world_.metrics["grain_eaten"];r.hunger=std::min(100.0f,r.hunger+42);}else if(r.fish>0){--r.fish;r.hunger=std::min(100.0f,r.hunger+46);}else if(r.meat>0){--r.meat;r.hunger=std::min(100.0f,r.hunger+54);}done("eat","comio una racion");}break;
    case Task::Fetch:if(r.actionTimer>=2){int amount=std::min(3,world_.grain);world_.grain-=amount;r.grain+=amount;done("fetch","retiro grano del almacen");}break;
    case Task::Deposit:if(r.actionTimer>=3){int amount=std::max(0,r.grain-2);r.grain-=amount;world_.grain+=amount;r.money+=amount;done("deposit","deposito su cosecha");}break;
    case Task::Rest:if(world_.night()&&r.insideHouse<0&&at!=r.home&&r.actionTimer>3){if(begin(r,Task::Rest,r.home))break;r.actionTimer=0;}if(r.insideHouse<0&&at==r.home){int id=world_.residents.indexOf(r);for(const auto& p:world_.props)if(p.kind=="house"&&p.uid>=0&&(p.owner==id||(r.mother>=0&&p.owner==r.mother)||(r.father>=0&&p.owner==r.father))&&std::hypot(p.x-r.x,p.y-r.y)<4){r.insideHouse=p.uid;r.path.clear();r.next=0;break;}}r.energy=std::min(100.0f,r.energy+1.3f*(world_.hasBonus(r,"rest")?1.25f:1)*dt);if(!world_.night()&&r.actionTimer>18&&r.energy>78&&leaveShelter(r))done("sleep","desperto descansado");break;
    case Task::Social:if(r.target>=0&&r.target<static_cast<int>(world_.residents.size())){auto& o=world_.residents[r.target];if(o.alive&&!o.inDungeon&&!o.away&&dist(at,position(o))<=2){r.social=std::min(100.0f,r.social+5*dt);o.social=std::min(100.0f,o.social+3*dt);if(r.actionTimer>=7){int id=world_.residents.indexOf(r);r.relationships[r.target]=std::min(100,r.relationships[r.target]+8);o.relationships[id]=std::min(100,o.relationships[id]+8);done("social","converso con "+o.name);}}else if(r.actionTimer>2)r.task=Task::Idle;}break;
    case Task::Gather:{int k=r.workCell>=0?r.workCell:at;if(!world_.forest[k]||world_.protectedLand[k]){r.hasOrder=false;r.task=Task::Idle;r.resumeRest=false;}else if(r.actionTimer>=8){if(auto* tree=world_.propAt(k);tree&&tree->tree){int uid=tree->uid;world_.props.erase(std::remove_if(world_.props.begin(),world_.props.end(),[=](const Prop& p){return p.uid==uid;}),world_.props.end());}world_.forest[k]=false;world_.rebuildNavigation();r.wood+=4;done("gather","talo y obtuvo 4 de madera");}break;}
    case Task::Camp:if(r.actionTimer>=7&&r.wood>=3){r.wood-=3;int id=world_.residents.indexOf(r);world_.camps.erase(std::remove_if(world_.camps.begin(),world_.camps.end(),[&](const Camp& c){return c.owner==id;}),world_.camps.end());world_.camps.push_back({at,id,static_cast<float>(world_.elapsed)});done("camp","construyo un refugio");begin(r,Task::Rest,at);}break;
    case Task::Trade:if(r.actionTimer>=10){int amount=std::min(4,r.wood);r.wood-=amount;r.money+=amount*3;world_.communityWood+=amount;done("work","vendio madera en el mercado");}break;
    case Task::Tend:if(r.actionTimer>=5){int amount=std::min(3,world_.livestockFood);world_.livestockFood-=amount;r.eggs+=amount;done("tend","recogio huevos y leche");}break;
    case Task::Hunt:if(r.target<0||r.target>=static_cast<int>(world_.animals.size())){r.task=Task::Idle;break;}else{auto& a=world_.animals[r.target];int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));if(!a.alive||a.kind!=4||world_.protectedLand[k]){r.task=Task::Idle;break;}if(std::hypot(a.x-r.x,a.y-r.y)>1.4f){begin(r,Task::Hunt,k);break;}if(r.actionTimer>=2.6f){a.alive=false;a.respawn=180;r.meat+=3;r.energy=std::max(0.0f,r.energy-3);r.fun=std::min(100.0f,r.fun+9);done("hunt","cazo y obtuvo 3 raciones");}}break;
    case Task::Sow:case Task::Harvest:if(r.actionTimer>=4){auto* c=world_.cropAt(r.workCell);if(c&&r.task==Task::Sow&&c->stage==0&&r.seeds>0){bool occupied=false;for(const auto& person:world_.residents)if(person.alive&&!person.inDungeon&&!person.away&&position(person)==c->cell)occupied=true;for(const auto& animal:world_.animals)if(animal.alive&&cellId(static_cast<int>(animal.x),static_cast<int>(animal.y))==c->cell)occupied=true;if(occupied){if(r.actionTimer>20)done("idle","aplazo una siembra ocupada");break;}--r.seeds;c->stage=1;c->growth=0;world_.rebuildNavigation();done("sow","sembro una semilla");}else if(c&&r.task==Task::Harvest&&c->stage==3){r.grain+=world_.hasBonus(r,"harvest")?4:3;r.seeds+=2;c->stage=0;c->growth=0;++c->cycles;++world_.harvests;world_.rebuildNavigation();done("harvest","cosecho grano y semillas");}else done("idle","termino de revisar el cultivo");}break;
    case Task::Till:if(r.actionTimer>=6){int k=r.workCell;if(k>=0&&!world_.cropAt(k)&&world_.crops.size()<128&&!world_.fallow[k]&&!world_.protectedLand[k]&&!world_.reserved(k))world_.crops.push_back({k});done("till","preparo una parcela");}break;
    case Task::Project:if(r.actionTimer>=8)world_.completeJob(r);break;
    case Task::SellLoot:if(r.actionTimer>=4){int value=0;for(const auto& item:r.loot)value+=item.value;int tax=value/5;r.money+=value-tax;world_.treasury+=tax;r.loot.clear();done("sell_loot","vendio sus artefactos");}break;
    case Task::Move:done("move","llego al destino indicado");break;
    case Task::Fish:break;
    case Task::Expedition:break;
    case Task::Wander:r.fun=std::min(100.0f,r.fun+3*dt);if(r.actionTimer>=12)done("explore","exploro el bosque");break;
    case Task::Flee:done("flee","se alejo de la alerta");break;
    default:r.task=Task::Idle;break;
    }
}
}


