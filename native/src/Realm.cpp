#include "World.h"
#include <algorithm>
#include <cmath>
#include <queue>
namespace isla {
namespace {
int loc(const Resident& r){return cellId(static_cast<int>(r.x),static_cast<int>(r.y));}
std::array<std::array<int,2>,4> centers{{{6,5},{26,5},{26,15},{6,15}}};
bool dungeonFloor(int x,int y){if(x<0||y<0||x>=35||y>=20)return false;bool room=((x>=2&&x<12)||(x>=21&&x<32))&&((y>=2&&y<8)||(y>=12&&y<18));return room||((y==5||y==6||y==15||y==16)&&x>=6&&x<=27)||((x==26||x==27)&&y>=5&&y<=15);}
void moveHero(Dungeon& d,int hero,int tx,int ty,float dt){
    const int sx=static_cast<int>(d.x[hero]),sy=static_cast<int>(d.y[hero]);
    std::array<int,700> prev;prev.fill(-1);std::queue<int> q;int start=sy*35+sx,target=ty*35+tx;if(start<0||start>=700||target<0||target>=700)return;prev[start]=start;q.push(start);
    auto pass=[&](int x,int y){if(!dungeonFloor(x,y))return false;for(int i=0;i<3;++i){const auto c=centers[i+1];if(d.enemies[i]>0&&x==c[0]&&y==c[1])return false;if(x==c[0]+3&&y==c[1]-1)return false;}return true;};
    while(!q.empty()&&prev[target]<0){int at=q.front();q.pop();int x=at%35,y=at/35;for(auto v:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int nx=x+v[0],ny=y+v[1];if(!pass(nx,ny))continue;int k=ny*35+nx;if(prev[k]>=0)continue;prev[k]=at;q.push(k);}}
    if(prev[target]<0)return;int next=target;while(prev[next]!=start&&next!=start)next=prev[next];
    float x=next%35+.5f,y=next/35+.5f,len=std::hypot(x-d.x[hero],y-d.y[hero]),step=std::min(len,1.75f*dt);if(len>.0001f){d.x[hero]+=(x-d.x[hero])/len*step;d.y[hero]+=(y-d.y[hero])/len*step;}
}
}
std::string World::dispatch(int i){if(i<0||i>=static_cast<int>(ships.size()))return "Puerto inexistente";auto& s=ships[i];if(s.remaining>0||s.passenger>=0||s.state=="lost")return "El barco no esta disponible";if(grain<reserve+10)return "Faltan 10 de grano por encima de la reserva";grain-=10;s.remaining=60;lastDispatch=elapsed;record("El puerto exporto 10 de grano");return "Barco enviado: vuelve en 60 segundos";}
void World::updateRealm(float dt){livestockClock+=dt;if(livestockClock>=50){livestockClock-=50;livestockFood=std::min(12,livestockFood+2);}for(auto& s:ships)if(s.remaining>0){s.remaining=std::max(0.0f,s.remaining-dt);if(s.remaining<=.0001f){s.remaining=0;++s.voyages;communityWood+=8;treasury+=15;herbs+=2;++metrics["trade"];record("Barco de vuelta: 8 madera, 15 monedas, 2 hierbas");}}
    if(autoTrade&&elapsed-lastDispatch>90)dispatch(0);
    if(priority==3&&static_cast<int>(elapsed*10)%1500==0)for(std::size_t i=0;i<dungeons.size();++i)if(dungeons[i].state=="idle"){requestExpedition(static_cast<int>(i));break;}
    for(auto& r:residents)if(!r.inDungeon&&!r.away&&r.alive&&r.health<cnc::maxHealth*.7f&&herbs>0&&std::hypot(r.x-market%mapWidth-.5f,r.y-market/mapWidth-.5f)<4){--herbs;r.health=std::min(cnc::maxHealth,r.health+cnc::maxHealth*.25f);record(r.name+" uso una hierba medicinal");}
    for(auto& j:jobs)if(j.worker>=0){const auto& r=residents[j.worker];if(!r.alive||r.task!=Task::Project||r.job!=j.id)j.worker=-1;}
}
Artifact World::makeArtifact(float roll,int v,int level) const{v=std::clamp(v,0,3);if(roll<.04f)return {std::array<const char*,4>{{"BOTAS DEL CAMINANTE","CANTIMPLORA ETERNA","HOZ DEL ALBA","AMULETO DEL REPOSO"}}[v],std::array<const char*,4>{{"speed","thirst","harvest","rest"}}[v],90+level*20,true};return {std::array<const char*,4>{{"IDOLILLO ANTIGUO","CALIZ DE PLATA","SELLO DE BRONCE","GEMA DEL ECO"}}[v],"",12+level*8+v*3,false};}
std::string World::requestExpedition(int i){if(i<0||i>=static_cast<int>(dungeons.size()))return "Mazmorra inexistente";auto& d=dungeons[i];if(d.state!="idle")return "La expedicion ya esta en curso o repoblando";std::vector<int> candidates;int free=0;for(std::size_t k=0;k<residents.size();++k){auto& r=residents[k];if(r.alive&&!r.away&&!r.visitor&&r.ageDays>=4320&&r.mission<0)++free;if(r.alive&&!r.visitor&&r.ageDays>=4320&&!r.inBoat&&!r.inDungeon&&!r.away&&r.mission<0&&r.health>=cnc::maxHealth*.65f&&r.energy>=45&&r.hunger>30&&r.thirst>30)candidates.push_back(static_cast<int>(k));}if(free<4||candidates.size()<2)return "Faltan dos habitantes aptos; deben quedar dos en la isla";
    std::sort(candidates.begin(),candidates.end(),[&](int a,int b){auto score=[&](int k){return residents[k].energy+(residents[k].trait.find("Explorador")==0?40:0);};return score(a)>score(b);});d.party={candidates[0],candidates[1]};
    for(int who:d.party){const auto& r=residents[who];auto p=route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return cellId(x,y)==d.entrance;});if(p.empty()){d.party.clear();return "La entrada no tiene camino accesible";}}
    d.state="approach";d.room=1;d.clock=0;d.opened.fill(false);d.enemies.fill(22.0f+d.level*7);d.x={{6.5f,6.8f}};d.y={{5.5f,5.5f}};d.log={"La expedicion viaja por la isla"};for(int who:d.party){auto& r=residents[who];r.mission=i;begin(r,Task::Expedition,d.entrance);}record("Expedicion enviada a "+d.name);return "Expedicion enviada";
}
void World::updateDungeons(float dt){for(auto& d:dungeons){
    if(d.state=="idle")continue;if(d.state=="cooldown"){if(elapsed>=d.cooldown)d.state="idle";continue;}
    if(d.state=="approach"){bool arrived=true;for(int id:d.party){auto& r=residents[id];if(!r.alive){d.state="idle";for(int who:d.party)residents[who].mission=-1;arrived=false;break;}if(loc(r)!=d.entrance||r.next<r.path.size())arrived=false;}if(arrived){d.state="exploring";for(int id:d.party){residents[id].inDungeon=true;residents[id].path.clear();}d.log.push_back("Han entrado en la mazmorra");}continue;}
    bool retreat=false;for(int id:d.party){auto& r=residents[id];r.hunger=std::max(0.0f,r.hunger-.03f*dt);r.thirst=std::max(0.0f,r.thirst-.04f*dt);r.energy=std::max(0.0f,r.energy-.06f*dt);if(r.health<cnc::maxHealth*.3f||r.thirst<14||r.energy<12)retreat=true;}if(retreat)d.state="retreat";
    if(d.state=="returning"||d.state=="retreat"){bool arrived=true;for(int h=0;h<2;++h){moveHero(d,h,6,5,dt);if(std::hypot(d.x[h]-6.5f,d.y[h]-5.5f)>.5f)arrived=false;}if(arrived){for(int id:d.party){auto& r=residents[id];r.inDungeon=false;r.mission=-1;r.x=d.entrance%mapWidth+.5f;r.y=d.entrance/mapWidth+.5f;r.task=Task::Idle;r.next=0;r.path.clear();}d.state="cooldown";d.cooldown=elapsed+300;++d.runs;++metrics["expeditions"];record("La expedicion regreso de "+d.name);}continue;}
    const int room=d.room-1;const auto center=centers[d.room];bool ready=true;
    for(int h=0;h<2;++h){const int tx=center[0]+(d.enemies[room]>0?(h==0?-2:2):2);const int ty=center[1]+(d.enemies[room]>0?0:h-1);moveHero(d,h,tx,ty,dt);if(std::hypot(d.x[h]-tx-.5f,d.y[h]-ty-.5f)>.05f)ready=false;}
    if(!ready)continue;
    if(d.enemies[room]>0){d.state="combat";d.clock+=dt;if(d.clock>=1){d.clock-=1;for(int id:d.party){(void)id;d.enemies[room]=std::max(0.0f,d.enemies[room]-static_cast<float>(4+random()%5));}if(d.enemies[room]>0){auto& target=residents[d.party[random()%2]];target.health=std::max(1.0f,target.health-2*static_cast<float>(2+random()%static_cast<unsigned>(3+d.level)));}else{++d.kills;++metrics["dungeon_kills"];d.log.push_back("Guardian derrotado en camara "+std::to_string(d.room));}}continue;}
    d.state="looting";if(!d.opened[room]){d.opened[room]=true;auto& owner=residents[d.party[room%2]];auto item=makeArtifact(static_cast<float>(random()%1000000)/1000000,static_cast<int>(random()%4),d.level);if(item.rare&&!hasBonus(owner,item.bonus)){owner.artifacts.push_back(item);++metrics["rare_artifacts"];}else owner.loot.push_back(item);++metrics["artifacts_found"];record(owner.name+" encontro "+item.name);d.log.push_back("Cofre abierto: "+item.name);}
    if(d.room>=3)d.state="returning";else{++d.room;d.state="exploring";}
}}
}




