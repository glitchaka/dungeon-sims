#include "World.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace isla {
namespace {int position(const Resident& r){return cellId(static_cast<int>(r.x),static_cast<int>(r.y));}float dist(int a,int b){return std::hypot(static_cast<float>(a%mapWidth-b%mapWidth),static_cast<float>(a/mapWidth-b/mapWidth));}}
bool World::begin(Resident& r,Task task,int goal){
    if(goal<0||goal>=mapWidth*mapHeight)return false;
    if(task!=Task::Rest&&r.insideHouse>=0&&!leaveShelter(r))return false;
    const int workTarget=goal;
    if(task==Task::Sow||task==Task::Harvest||task==Task::Till) goal=nearest(r,[&](int x,int y){return std::abs(x-workTarget%mapWidth)+std::abs(y-workTarget/mapWidth)==1;});
    if(goal<0)return false;
    auto path=route(static_cast<int>(r.x),static_cast<int>(r.y),[=](int x,int y){return cellId(x,y)==goal;});if(path.empty())return false;
    const int id=static_cast<int>(&r-residents.data());for(auto& c:crops)if(c.worker==id)c.worker=-1;
    for(auto& j:jobs)if(j.worker==id&&task!=Task::Project)j.worker=-1;
    r.workCell=(task==Task::Sow||task==Task::Harvest||task==Task::Till)?workTarget:-1;r.task=task;r.path=std::move(path);r.next=0;r.goal=goal;r.actionTimer=0;r.decisionTimer=cnc::wanderInterval;
    if(task==Task::Sow||task==Task::Harvest){auto* crop=cropAt(workTarget);if(crop)crop->worker=id;}
    return true;
}
void World::finish(Resident& r,const std::string& metric,const std::string& note){if(r.hasOrder&&r.task==r.orderedTask)r.hasOrder=false;r.workCell=-1;++metrics[metric];record(r.name+" "+note);for(auto& c:crops)if(c.worker==static_cast<int>(&r-residents.data()))c.worker=-1;r.task=Task::Idle;r.path.clear();r.next=0;r.actionTimer=0;r.decisionTimer=0;}
void World::feed(Resident& r){
    if(r.meat+r.grain+r.eggs+r.fish>0){begin(r,Task::Eat,position(r));return;}
    if(livestockFood>0&&r.trait!="Cazadora"){int k=nearest(r,[&](int x,int y){return pasture[cellId(x,y)];});if(k>=0){begin(r,Task::Tend,k);return;}}
    if(grain>0&&r.trait!="Cazadora"){begin(r,Task::Fetch,granary);return;}
    float best=1e9f;int chosen=-1;
    for(std::size_t i=0;i<animals.size();++i){const auto& a=animals[i];const int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));if(!a.alive||a.kind!=4||protectedLand[k])continue;bool reserved=false;for(const auto& other:residents)if(&other!=&r&&other.task==Task::Hunt&&other.target==static_cast<int>(i))reserved=true;if(!reserved&&dist(position(r),k)<best){best=dist(position(r),k);chosen=static_cast<int>(i);}}
    if(chosen>=0){r.target=chosen;const auto& a=animals[chosen];if(begin(r,Task::Hunt,cellId(static_cast<int>(a.x),static_cast<int>(a.y))))return;}
    if(!farmTask(r))begin(r,Task::Wander,position(r));
}
bool World::gatherTask(Resident& r){
    std::array<int,mapWidth*mapHeight> targets;targets.fill(-1);
    for(const auto& tree:props)if(tree.tree){int source=cellId(static_cast<int>(tree.x),static_cast<int>(tree.y));if(protectedLand[source])continue;
        for(int offset:{1,-1,mapWidth,-mapWidth}){int k=source+offset;if(k<0||k>=mapWidth*mapHeight||std::abs(k%mapWidth-source%mapWidth)+std::abs(k/mapWidth-source/mapWidth)!=1||!walkable(k%mapWidth,k/mapWidth))continue;targets[k]=source;}
    }
    int approach=nearest(r,[&](int x,int y){return targets[cellId(x,y)]>=0;});
    if(approach<0||!begin(r,Task::Gather,approach))return false;r.workCell=targets[approach];return true;
}
void World::rest(Resident& r){
    if(dist(position(r),r.home)<=11){begin(r,Task::Rest,r.home);return;}
    for(const auto& camp:camps)if(dist(position(r),camp.cell)<8){bool occupied=false;for(const auto& o:residents)if(&o!=&r&&o.task==Task::Rest&&o.goal==camp.cell)occupied=true;if(!occupied){begin(r,Task::Rest,camp.cell);return;}}
    if(r.wood<3){r.resumeRest=true;if(gatherTask(r))return;}
    if(r.wood>=3&&terrain[position(r)]==Terrain::Ground)begin(r,Task::Camp,position(r));else begin(r,Task::Rest,r.home);
}
bool World::farmTask(Resident& r){
    float best=1e9f;int chosen=-1;Task task=Task::Idle;const int id=static_cast<int>(&r-residents.data());
    for(const auto& c:crops){if((c.worker>=0&&c.worker!=id)||reserved(c.cell))continue;Task action=Task::Idle;if(c.stage==3&&(grain<160||r.seeds<6))action=Task::Harvest;else if(c.stage==0&&r.seeds>0&&grain<130)action=Task::Sow;if(action==Task::Idle)continue;float score=dist(position(r),c.cell)+(action==Task::Harvest?0:8);if(score<best){best=score;chosen=c.cell;task=action;}}
    if(chosen>=0&&begin(r,task,chosen))return true;
    if(crops.size()<128&&r.seeds>2&&harvests>0&&r.trait=="Agricultora")for(const auto& c:crops){for(int d:{1,-1,mapWidth,-mapWidth}){int k=c.cell+d;if(k<0||k>=mapWidth*mapHeight||!walkable(k%mapWidth,k/mapWidth)||terrain[k]!=Terrain::Ground||forest[k]||pasture[k]||fallow[k]||protectedLand[k]||cropAt(k)||reserved(k))continue;bool reserved=false;for(const auto& other:residents)if(other.task==Task::Till&&other.goal==k)reserved=true;if(!reserved&&begin(r,Task::Till,k))return true;}}
    return false;
}
bool World::leaveShelter(Resident& r){
    if(r.insideHouse<0)return true;
    int best=-1;float distance=1e9f;
    for(int y=std::max(0,static_cast<int>(r.y)-5);y<std::min(mapHeight,static_cast<int>(r.y)+6);++y)for(int x=std::max(0,static_cast<int>(r.x)-5);x<std::min(mapWidth,static_cast<int>(r.x)+6);++x){if(!walkable(x,y))continue;bool busy=false;for(const auto& p:residents)if(&p!=&r&&p.alive&&p.insideHouse<0&&!p.away&&!p.inBoat&&!p.inDungeon&&static_cast<int>(p.x)==x&&static_cast<int>(p.y)==y)busy=true;for(const auto& a:animals)if(a.alive&&static_cast<int>(a.x)==x&&static_cast<int>(a.y)==y)busy=true;float d=std::hypot(x+.5f-r.x,y+.5f-r.y);if(!busy&&d<distance){best=cellId(x,y);distance=d;}}
    if(best<0)return false;r.x=best%mapWidth+.5f;r.y=best/mapWidth+.5f;r.insideHouse=-1;return true;
}
void World::plan(Resident& r){
    if(r.insideHouse>=0){if(night()&&r.hunger>=18&&r.thirst>=20&&alarmSeconds<=0){r.task=Task::Rest;return;}if(!leaveShelter(r))return;}
    const int at=position(r),id=static_cast<int>(&r-residents.data());
    if(r.ageDays<World::adultAgeDays){if(night()){if(!begin(r,Task::Rest,r.home))begin(r,Task::Rest,at);}else{r.task=Task::Idle;r.decisionTimer=30;}return;}
    if(alarmSeconds>0&&std::hypot(r.x-alarmX,r.y-alarmY)<cnc::fleeRadius){int k=nearest(r,[&](int x,int y){return std::hypot(x+.5f-alarmX,y+.5f-alarmY)>=cnc::fleeRadius;});if(begin(r,Task::Flee,k))return;}
    if(r.thirst<20){begin(r,Task::Drink,nearest(r,[&](int x,int y){return besideFreshWater(x,y);}));return;}
    if(r.hunger<18){feed(r);return;}
    if(night()){
        for(auto& j:jobs)if(j.worker==id)j.worker=-1;r.job=-1;r.hasOrder=false;
        if(!begin(r,Task::Rest,r.home))begin(r,Task::Rest,at);return;
    }
    if(r.visitor){
        if(r.thirst<50){begin(r,Task::Drink,nearest(r,[&](int x,int y){return besideFreshWater(x,y);}));return;}
        if(r.hunger<45){feed(r);return;}if(r.energy<30){rest(r);return;}
        int goal=nearest(r,[&](int x,int y){int d=std::abs(x-market%mapWidth)+std::abs(y-market/mapWidth);return d>=2&&d<=5;});
        if(goal>=0)begin(r,Task::Wander,goal);return;
    }
    if(r.hasOrder){ if(r.orderedTask==Task::Project){auto it=std::find_if(jobs.begin(),jobs.end(),[&](const Job& j){return j.id==r.job;});if(it!=jobs.end()&&(it->worker<0||it->worker==id)){it->worker=id;if(begin(r,Task::Project,it->goal))return;}}else if(r.orderedTask==Task::Move||r.orderedTask==Task::Fish){if(begin(r,r.orderedTask,r.orderedCell))return;}else {const int target=r.orderedCell;r.hasOrder=false;orderUnit(id,"work",target);if(r.hasOrder)return;}r.hasOrder=false; }
    if(r.mission>=0&&r.energy>15&&r.hunger>22&&r.thirst>25){begin(r,Task::Expedition,dungeons[r.mission].entrance);return;}
    if(!r.loot.empty()&&r.energy>25&&r.hunger>28&&r.thirst>30){begin(r,Task::SellLoot,market);return;}
    if(!r.visitor&&seekFamily(r))return;
    if(!r.visitor&&r.hunger>45&&r.thirst>45&&r.energy>45){
        for(const auto& gift:gifts){int k=nearest(r,[&](int x,int y){return std::abs(x-gift.cell%mapWidth)+std::abs(y-gift.cell/mapWidth)==1;});if(k>=0&&begin(r,Task::Move,k))return;}
        if((r.inspiration=="sustain"||grain<reserve)&&r.fish<3)for(std::size_t boat=0;boat<ships.size();++boat)if(ships[boat].passenger<0&&ships[boat].remaining<=0){startFishing(id,static_cast<int>(boat));if(r.hasOrder)return;}
    }
    if(!r.visitor&&r.energy>35&&r.hunger>40&&r.thirst>40&&(!night()||r.inspiration=="build"))for(auto& j:jobs)if(j.worker<0){r.job=j.id;if(begin(r,Task::Project,j.goal)){j.worker=id;return;}}
    if(r.thirst<65&&r.homeWater>1&&dist(at,r.home)<6){begin(r,Task::DrinkHome,r.home);return;}
    if(r.resumeRest){r.resumeRest=false;rest(r);return;}
    if(r.grain>=6&&r.hunger>30&&r.thirst>30&&r.energy>30){begin(r,Task::Deposit,granary);return;}
    const float hour=std::fmod(elapsed/300*24,24.0f);
    const std::array<float,7> scores{{(100-r.thirst)*1.28f+(r.thirst<30?60:0),(100-r.hunger)*1.1f+(r.hunger<27?55:0)+(r.trait=="Cazadora"?6:0),(100-r.energy)*1.05f+(night()?28:0)+(r.energy<25?60:0),(100-r.social)*.8f+(r.trait=="Sociable"?8:0),(100-r.fun)*.86f+(r.trait.find("Explorador")==0?8:0),(100-r.hygiene)*.82f,(hour>8&&hour<17?38.0f:0.0f)+(priority==2?18:0)+(r.trait=="Trabajador"&&r.money<80?9:0)}};
    int choice=static_cast<int>(std::max_element(scores.begin(),scores.end())-scores.begin());
    float farmScore=(r.trait=="Agricultora"||r.trait=="Trabajador"?54.0f:31.0f)+(priority==1?20:0)+(grain<12?13:0);
    if(!r.visitor&&!night()&&(farmScore+(r.inspiration=="sustain"?35:0))>scores[choice]&&farmTask(r))return;
    if(choice==0){begin(r,Task::Drink,nearest(r,[&](int x,int y){return besideFreshWater(x,y);}));return;}
    if(choice==1){feed(r);return;}if(choice==2){rest(r);return;}
    if(choice==5){begin(r,Task::Bathe,nearest(r,[&](int x,int y){return besideFreshWater(x,y);}));return;}
    if(choice==3){float best=1e9f;int who=-1;for(std::size_t i=0;i<residents.size();++i){const auto& o=residents[i];if(static_cast<int>(i)==id||!o.alive||o.inDungeon||o.mission>=0||o.task==Task::Rest||o.task==Task::Drink||o.task==Task::Hunt)continue;float d=dist(at,position(o));if(d<best){who=static_cast<int>(i);best=d;}}if(who>=0){r.target=who;if(begin(r,Task::Social,nearest(r,[&](int x,int y){return std::abs(x-static_cast<int>(residents[who].x))+std::abs(y-static_cast<int>(residents[who].y))==1;})))return;}}
    if(choice==6){if(r.wood>=4){begin(r,Task::Trade,market);return;}if(gatherTask(r))return;}
    // Original exploration chooses a real forest destination; small local wander uses the imported civilian parameters.
    if(random()%100<static_cast<unsigned>(cnc::wanderChance*100)){std::vector<int> options;for(int k=0;k<mapWidth*mapHeight;++k)if(forest[k]&&walkable(k%mapWidth,k/mapWidth))options.push_back(k);if(!options.empty()&&begin(r,Task::Wander,options[random()%options.size()]))return;}
    r.task=Task::Idle;r.decisionTimer=cnc::wanderInterval;r.path.clear();r.speed=0;
}
void World::act(Resident& r,float dt){
    r.actionTimer+=dt;const int at=position(r);auto done=[&](const std::string& key,const std::string& note){finish(r,key,note);};
    switch(r.task){
    case Task::Drink:if(!besideFreshWater(at%mapWidth,at/mapWidth)){r.task=Task::Idle;break;}r.thirst=std::min(100.0f,r.thirst+9*dt);if(r.thirst>=98){r.homeWater=4;done("drink","bebio agua dulce");}break;
    case Task::DrinkHome:r.thirst=std::min(100.0f,r.thirst+9*dt);r.homeWater=std::max(0.0f,r.homeWater-.3f*dt);if(r.thirst>=98||r.homeWater<=0)done("drink","uso su reserva de agua");break;
    case Task::Bathe:r.hygiene=std::min(100.0f,r.hygiene+7*dt);if(r.hygiene>=98)done("bathe","se lavo en el rio");break;
    case Task::Eat:if(r.actionTimer>=4){if(r.eggs>0){--r.eggs;r.hunger=std::min(100.0f,r.hunger+36);}else if(r.grain>0){--r.grain;++metrics["grain_eaten"];r.hunger=std::min(100.0f,r.hunger+42);}else if(r.fish>0){--r.fish;r.hunger=std::min(100.0f,r.hunger+46);}else if(r.meat>0){--r.meat;r.hunger=std::min(100.0f,r.hunger+54);}done("eat","comio una racion");}break;
    case Task::Fetch:if(r.actionTimer>=2){int amount=std::min(3,grain);grain-=amount;r.grain+=amount;done("fetch","retiro grano del almacen");}break;
    case Task::Deposit:if(r.actionTimer>=3){int amount=std::max(0,r.grain-2);r.grain-=amount;grain+=amount;r.money+=amount;done("deposit","deposito su cosecha");}break;
    case Task::Rest:if(night()&&r.insideHouse<0&&at!=r.home&&r.actionTimer>3){if(begin(r,Task::Rest,r.home))break;r.actionTimer=0;}if(r.insideHouse<0&&at==r.home){int id=static_cast<int>(&r-residents.data());for(const auto& p:props)if(p.kind=="house"&&p.uid>=0&&(p.owner==id||(r.mother>=0&&p.owner==r.mother)||(r.father>=0&&p.owner==r.father))&&std::hypot(p.x-r.x,p.y-r.y)<4){r.insideHouse=p.uid;r.path.clear();r.next=0;break;}}r.energy=std::min(100.0f,r.energy+1.3f*(hasBonus(r,"rest")?1.25f:1)*dt);if(!night()&&r.actionTimer>18&&r.energy>78&&leaveShelter(r))done("sleep","desperto descansado");break;
    case Task::Social:if(r.target>=0&&r.target<static_cast<int>(residents.size())){auto& o=residents[r.target];if(o.alive&&!o.inDungeon&&!o.away&&dist(at,position(o))<=2){r.social=std::min(100.0f,r.social+5*dt);o.social=std::min(100.0f,o.social+3*dt);if(r.actionTimer>=7){int id=static_cast<int>(&r-residents.data());r.relationships[r.target]=std::min(100,r.relationships[r.target]+8);o.relationships[id]=std::min(100,o.relationships[id]+8);done("social","converso con "+o.name);}}else if(r.actionTimer>2)r.task=Task::Idle;}break;
    case Task::Gather:{int k=r.workCell>=0?r.workCell:at;if(!forest[k]||protectedLand[k]){r.hasOrder=false;r.task=Task::Idle;r.resumeRest=false;}else if(r.actionTimer>=8){if(auto* tree=propAt(k);tree&&tree->tree){int uid=tree->uid;props.erase(std::remove_if(props.begin(),props.end(),[=](const Prop& p){return p.uid==uid;}),props.end());}forest[k]=false;rebuildNavigation();r.wood+=4;done("gather","talo y obtuvo 4 de madera");}break;}
    case Task::Camp:if(r.actionTimer>=7&&r.wood>=3){r.wood-=3;int id=static_cast<int>(&r-residents.data());camps.erase(std::remove_if(camps.begin(),camps.end(),[&](const Camp& c){return c.owner==id;}),camps.end());camps.push_back({at,id,elapsed});done("camp","construyo un refugio");begin(r,Task::Rest,at);}break;
    case Task::Trade:if(r.actionTimer>=10){int amount=std::min(4,r.wood);r.wood-=amount;r.money+=amount*3;communityWood+=amount;done("work","vendio madera en el mercado");}break;
    case Task::Tend:if(r.actionTimer>=5){int amount=std::min(3,livestockFood);livestockFood-=amount;r.eggs+=amount;done("tend","recogio huevos y leche");}break;
    case Task::Hunt:if(r.target<0||r.target>=static_cast<int>(animals.size())){r.task=Task::Idle;break;}else{auto& a=animals[r.target];int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));if(!a.alive||a.kind!=4||protectedLand[k]){r.task=Task::Idle;break;}if(std::hypot(a.x-r.x,a.y-r.y)>1.4f){begin(r,Task::Hunt,k);break;}if(r.actionTimer>=2.6f){a.alive=false;a.respawn=180;r.meat+=3;r.energy=std::max(0.0f,r.energy-3);r.fun=std::min(100.0f,r.fun+9);done("hunt","cazo y obtuvo 3 raciones");}}break;
    case Task::Sow:case Task::Harvest:if(r.actionTimer>=4){auto* c=cropAt(r.workCell);if(c&&r.task==Task::Sow&&c->stage==0&&r.seeds>0){bool occupied=false;for(const auto& person:residents)if(person.alive&&!person.inDungeon&&!person.away&&position(person)==c->cell)occupied=true;for(const auto& animal:animals)if(animal.alive&&cellId(static_cast<int>(animal.x),static_cast<int>(animal.y))==c->cell)occupied=true;if(occupied){if(r.actionTimer>20)done("idle","aplazo una siembra ocupada");break;}--r.seeds;c->stage=1;c->growth=0;rebuildNavigation();done("sow","sembro una semilla");}else if(c&&r.task==Task::Harvest&&c->stage==3){r.grain+=hasBonus(r,"harvest")?4:3;r.seeds+=2;c->stage=0;c->growth=0;++c->cycles;++harvests;rebuildNavigation();done("harvest","cosecho grano y semillas");}else done("idle","termino de revisar el cultivo");}break;
    case Task::Till:if(r.actionTimer>=6){int k=r.workCell;if(k>=0&&!cropAt(k)&&crops.size()<128&&!fallow[k]&&!protectedLand[k]&&!reserved(k))crops.push_back({k});done("till","preparo una parcela");}break;
    case Task::Project:if(r.actionTimer>=8)completeJob(r);break;
    case Task::SellLoot:if(r.actionTimer>=4){int value=0;for(const auto& item:r.loot)value+=item.value;int tax=value/5;r.money+=value-tax;treasury+=tax;r.loot.clear();done("sell_loot","vendio sus artefactos");}break;
    case Task::Move:done("move","llego al destino indicado");break;
    case Task::Fish:break;
    case Task::Expedition:break;
    case Task::Wander:r.fun=std::min(100.0f,r.fun+3*dt);if(r.actionTimer>=12)done("explore","exploro el bosque");break;
    case Task::Flee:done("flee","se alejo de la alerta");break;
    default:r.task=Task::Idle;break;
    }
}
void World::updateCrops(float dt){for(auto& c:crops){if(c.worker>=0){const auto& r=residents[c.worker];if(r.workCell!=c.cell||(r.task!=Task::Sow&&r.task!=Task::Harvest)||!r.alive)c.worker=-1;}if(c.stage==1||c.stage==2){c.growth+=dt*(night()?.3f:1.0f);if(c.growth>=120){c.stage=3;c.ripeAge=0;}else if(c.growth>=40)c.stage=2;}else if(c.stage==3){c.ripeAge+=dt;if(c.ripeAge>300){c.stage=0;c.growth=0;rebuildNavigation();++metrics["spoil"];}}}}
void World::updateAnimals(float dt){for(auto& a:animals){if(!a.alive){a.respawn-=dt;if(a.respawn<=0){bool placed=false;for(int k=0;k<mapWidth*mapHeight;++k){if(!walkable(k%mapWidth,k/mapWidth)||!(a.kind==4?forest[k]:pasture[k]))continue;bool occupied=false;for(const auto& other:animals)if(&other!=&a&&other.alive&&cellId(static_cast<int>(other.x),static_cast<int>(other.y))==k)occupied=true;for(const auto& person:residents)if(person.alive&&person.insideHouse<0&&!person.inDungeon&&!person.away&&!person.inBoat&&position(person)==k)occupied=true;if(!occupied){a.x=k%mapWidth+.5f;a.y=k/mapWidth+.5f;a.alive=true;a.timer=3;placed=true;break;}}if(!placed)a.respawn=10;}continue;}a.timer-=dt;if(a.timer>0)continue;a.timer=2+static_cast<float>(random()%5);bool hunted=false;for(const auto& r:residents)if(r.task==Task::Hunt&&r.target==static_cast<int>(&a-animals.data())&&std::hypot(r.x-a.x,r.y-a.y)<2)hunted=true;if(hunted)continue;int k=cellId(static_cast<int>(a.x),static_cast<int>(a.y));int next=k+std::array<int,4>{{1,-1,mapWidth,-mapWidth}}[random()%4];if(next<0||next>=mapWidth*mapHeight||!canStep(k%mapWidth,k/mapWidth,next%mapWidth,next/mapWidth))continue;bool occupied=false;for(const auto& other:animals)if(&other!=&a&&other.alive&&cellId(static_cast<int>(other.x),static_cast<int>(other.y))==next)occupied=true;for(const auto& person:residents)if(person.alive&&person.insideHouse<0&&!person.inDungeon&&!person.away&&!person.inBoat&&position(person)==next)occupied=true;if(!occupied&&(a.kind==4?forest[next]:pasture[next])){a.x=next%mapWidth+.5f;a.y=next/mapWidth+.5f;}}}
void World::tick(float dt){
    if(dt<=0||dt>.25f||!std::isfinite(dt))throw std::invalid_argument("Invalid fixed simulation step");
    elapsed+=dt;harvestClock=std::fmod(harvestClock+dt,120.0f);alarmSeconds=std::max(0.0f,alarmSeconds-dt);
    updateCrops(dt);updateAnimals(dt);updateRealm(dt);updateDungeons(dt);updateEcology(dt);updateFishing(dt);updateDivinity(dt);updateLifecycle(dt);updateFamilies(dt);updateVisits(dt);
    for(auto& r:residents){if(!r.alive||r.away||r.inDungeon||r.inBoat)continue;bool moving=r.next<r.path.size();
        r.hunger=std::max(0.0f,r.hunger-.066f*dt);r.thirst=std::max(0.0f,r.thirst-(moving?.11f:.085f)*(hasBonus(r,"thirst")?.75f:1)*dt);r.energy=std::max(0.0f,r.energy-(moving?.075f:.038f)*dt);r.social=std::max(0.0f,r.social-(r.trait=="Sociable"?.06f:.037f)*dt);r.fun=std::max(0.0f,r.fun-.045f*dt);r.hygiene=std::max(0.0f,r.hygiene-(r.task==Task::Hunt?.07f:.035f)*dt);
        if(r.hunger<2||r.thirst<2)r.health=std::max(0.0f,r.health-.44f*dt);else if(r.hunger>30&&r.thirst>30)r.health=std::min(cnc::maxHealth,r.health+.36f*dt);
        if(r.health<=0){r.alive=false;record(r.name+" no sobrevivio");continue;}r.decisionTimer-=dt;
        bool centered=std::hypot(r.x-std::floor(r.x)-.5f,r.y-std::floor(r.y)-.5f)<.001f;
        bool urgent=(r.thirst<16&&r.task!=Task::Drink&&r.task!=Task::DrinkHome)||(r.hunger<14&&r.task!=Task::Eat&&r.task!=Task::Hunt&&r.task!=Task::Fetch&&r.task!=Task::Drink&&r.task!=Task::Harvest&&r.task!=Task::Tend);
        bool threat=alarmSeconds>0&&r.task!=Task::Flee&&std::hypot(r.x-alarmX,r.y-alarmY)<cnc::fleeRadius;
        bool bedtime=night()&&r.task!=Task::Rest&&r.task!=Task::Drink&&r.task!=Task::DrinkHome&&r.task!=Task::Eat&&r.task!=Task::Fetch&&r.task!=Task::Hunt;
        if(centered&&(urgent||threat||bedtime))plan(r);
        if(r.next<r.path.size()){int k=r.path[r.next];bool occupied=false;if(k!=position(r)){for(const auto& other:residents)if(&other!=&r&&other.alive&&other.insideHouse<0&&!other.inDungeon&&!other.away&&!other.inBoat&&position(other)==k)occupied=true;for(const auto& animal:animals)if(animal.alive&&cellId(static_cast<int>(animal.x),static_cast<int>(animal.y))==k)occupied=true;}if(occupied){r.actionTimer+=dt;if(r.actionTimer>2){Task prior=r.task;int goal=(prior==Task::Sow||prior==Task::Harvest||prior==Task::Till)?r.workCell:r.goal;begin(r,prior,goal);}continue;}r.actionTimer=0;if(!canStep(static_cast<int>(r.x),static_cast<int>(r.y),k%mapWidth,k/mapWidth)){r.path.clear();r.task=Task::Idle;continue;}float tx=k%mapWidth+.5f,ty=k/mapWidth+.5f,len=std::hypot(tx-r.x,ty-r.y);float top=(r.task==Task::Flee?cnc::panicSpeed:cnc::walkSpeed)*(hasBonus(r,"speed")?1.25f:1)*(r.ageDays>=World::elderAgeDays?.65f:1.0f);r.speed=std::min(top,r.speed+top/std::max(.01f,cnc::accelerationSeconds)*dt);float step=std::min(len,r.speed*dt);if(len>.0001f){r.x+=(tx-r.x)/len*step;r.y+=(ty-r.y)/len*step;}if(len<=step+.0001f){r.x=tx;r.y=ty;if((terrain[k]==Terrain::Ground||terrain[k]==Terrain::Road||terrain[k]==Terrain::Bridge)&&traffic[k]<60)++traffic[k];++r.next;}continue;}
        r.speed=0;if(r.ageDays<World::adultAgeDays){if(r.task==Task::Rest)act(r,dt);else r.task=Task::Idle;continue;}if(r.task==Task::Idle){if(r.decisionTimer<=0)plan(r);}else act(r,dt);
    }
}
const char* World::taskName(Task task){switch(task){case Task::Move:return "DESPLAZANDOSE";case Task::Fish:return "PESCANDO";case Task::Wander:return "EXPLORANDO";case Task::Drink:return "BEBER AGUA DULCE";case Task::Eat:return "COMIENDO";case Task::Rest:return "DESCANSANDO";case Task::Flee:return "HUYENDO";case Task::Bathe:return "LAVANDOSE";case Task::Social:return "CONVERSANDO";case Task::Gather:return "RECOLECTANDO MADERA";case Task::Camp:return "CONSTRUYENDO REFUGIO";case Task::Trade:return "COMERCIANDO";case Task::Till:return "PREPARANDO TIERRA";case Task::Sow:return "SEMBRANDO";case Task::Harvest:return "COSECHANDO";case Task::Deposit:return "DEPOSITANDO COSECHA";case Task::Fetch:return "BUSCANDO PROVISIONES";case Task::Hunt:return "CAZANDO";case Task::Tend:return "CUIDANDO GANADO";case Task::DrinkHome:return "BEBIENDO EN CASA";case Task::Project:return "REALIZANDO ENCARGO";case Task::Expedition:return "EN EXPEDICION";case Task::SellLoot:return "VENDIENDO ARTEFACTOS";default:return "OBSERVANDO";}}
}












