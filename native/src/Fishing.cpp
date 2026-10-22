#include "World.h"
#include <algorithm>
#include <cmath>
#include <queue>
namespace isla {
namespace {
int at(const Resident& r){return cellId(static_cast<int>(r.x),static_cast<int>(r.y));}
std::vector<int> seaRoute(const World& w,int start,const std::function<bool(int)>& goal){
    if(start<0||start>=mapWidth*mapHeight||w.terrain[start]!=Terrain::Sea)return {};
    std::array<int,mapWidth*mapHeight> previous;previous.fill(-1);std::queue<int> queue;queue.push(start);previous[start]=start;int found=-1;
    while(!queue.empty()){int k=queue.front();queue.pop();if(goal(k)){found=k;break;}for(int d:{1,-1,mapWidth,-mapWidth}){int n=k+d;if(n<0||n>=mapWidth*mapHeight||std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)!=1||w.terrain[n]!=Terrain::Sea||previous[n]>=0)continue;previous[n]=k;queue.push(n);}}
    if(found<0)return {};std::vector<int> path;for(int k=found;k!=start;k=previous[k])path.push_back(k);path.push_back(start);std::reverse(path.begin(),path.end());return path;
}
}
std::string World::startFishing(int who,int boat){
    if(who<0||who>=static_cast<int>(residents.size())||boat<0||boat>=static_cast<int>(ships.size()))return "Selecciona habitante y barco";
    auto& r=residents[who];auto& s=ships[boat];if(!r.alive||r.inDungeon||r.inBoat||r.mission>=0)return "El habitante no esta disponible";
    for(const auto& boat:ships)if(boat.passenger==who)return "El habitante ya esta asignado a un barco";
    if(s.passenger>=0||s.remaining>0)return "Barco ocupado";
    if(s.state=="lost"){if(communityWood<8||treasury<5)return "Reponer barco: 8 madera y 5 monedas";communityWood-=8;treasury-=5;s.state="docked";}
    if(s.seaHome<0)return "El muelle aun no esta listo";
    if(!begin(r,Task::Fish,s.cell))return "No hay ruta terrestre al muelle";
    s.passenger=who;s.state="boarding";s.riskResolved=false;s.fishingClock=0;s.path.clear();s.next=0;
    r.hasOrder=true;r.orderedTask=Task::Fish;r.orderedCell=s.cell;
    record(r.name+" va al muelle para pescar; riesgo de perdida: 25%");return "Pesca ordenada: riesgo por viaje 25%";
}
void World::updateFishing(float dt){
    for(auto& s:ships){
        if(s.seaHome<0){float best=1e9f;int dock=-1,sea=-1;for(int k=0;k<mapWidth*mapHeight;++k){if(!walkable(k%mapWidth,k/mapWidth)||reserved(k))continue;for(int d:{1,-1,mapWidth,-mapWidth}){int n=k+d;if(n<0||n>=mapWidth*mapHeight||std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)!=1||terrain[n]!=Terrain::Sea)continue;float dist=std::hypot(static_cast<float>(k%mapWidth-s.cell%mapWidth),static_cast<float>(k/mapWidth-s.cell/mapWidth));if(dist<best){best=dist;dock=k;sea=n;}}}if(dock>=0){s.cell=dock;s.seaHome=sea;s.x=sea%mapWidth+.5f;s.y=sea/mapWidth+.5f;}}
        if(s.passenger<0||s.passenger>=static_cast<int>(residents.size()))continue;auto& r=residents[s.passenger];
        if(!r.alive){r.inBoat=false;s.passenger=-1;s.state="lost";continue;}
        if(s.state=="boarding"){if(at(r)!=s.cell||r.next<r.path.size())continue;int home=s.seaHome;s.path=seaRoute(*this,home,[=](int k){return std::abs(k%mapWidth-home%mapWidth)+std::abs(k/mapWidth-home/mapWidth)>=12;});if(s.path.empty()){r.hasOrder=false;r.task=Task::Idle;s.passenger=-1;s.state="docked";record("Pesca cancelada: ruta maritima no disponible");continue;}s.next=0;s.state="outbound";r.inBoat=true;r.path.clear();r.next=0;}
        r.hunger=std::max(0.0f,r.hunger-.03f*dt);r.thirst=std::max(0.0f,r.thirst-.04f*dt);r.energy=std::max(0.0f,r.energy-.06f*dt);
        if(s.state=="outbound"||s.state=="returning"){
            if(s.next<s.path.size()){int k=s.path[s.next];float tx=k%mapWidth+.5f,ty=k/mapWidth+.5f,len=std::hypot(tx-s.x,ty-s.y),step=std::min(len,1.25f*dt);if(len>.0001f){s.x+=(tx-s.x)/len*step;s.y+=(ty-s.y)/len*step;}if(len<=step+.0001f){s.x=tx;s.y=ty;++s.next;}continue;}
            if(s.state=="outbound"){s.state="fishing";s.fishingClock=0;}
            else {int fish=4+static_cast<int>(random()%5);r.fish+=fish;r.inBoat=false;r.hasOrder=false;r.x=s.cell%mapWidth+.5f;r.y=s.cell/mapWidth+.5f;r.task=Task::Idle;r.path.clear();r.next=0;++s.voyages;++metrics["fishing_returns"];record(r.name+" regreso con "+std::to_string(fish)+" pescados");s.state="docked";s.passenger=-1;}
        }
        if(s.state=="fishing"){s.fishingClock+=dt;if(s.fishingClock<30)continue;if(!s.riskResolved){s.riskResolved=true;const float roll=static_cast<float>(random()>>8)/16777216.0f;if(fishingLoss(roll)){r.alive=false;r.health=0;r.inBoat=false;r.hasOrder=false;r.task=Task::Idle;s.state="lost";s.passenger=-1;++metrics["lost_at_sea"];record(r.name+" y su barco se perdieron en alta mar");continue;}}
            const int now=cellId(static_cast<int>(s.x),static_cast<int>(s.y)),home=s.seaHome;s.path=seaRoute(*this,now,[=](int k){return k==home;});s.next=0;s.state="returning";}
    }
    populationClock+=dt;if(populationClock<180)return;populationClock-=180;
    int alive=static_cast<int>(std::count_if(residents.begin(),residents.end(),[](const Resident& r){return r.alive;}));if(alive>=24||grain<reserve+8||treasury<8)return;
    auto house=std::find_if(props.begin(),props.end(),[&](const Prop& p){return !p.tree&&(p.owner<0||p.owner>=static_cast<int>(residents.size())||!residents[p.owner].alive);});if(house==props.end())return;
    int home=cellId(static_cast<int>(house->x),static_cast<int>(house->y+1));if(!walkable(home%mapWidth,home/mapWidth))return;
    bool occupied=false;for(const auto& r:residents)if(r.alive&&!r.inBoat&&!r.inDungeon&&at(r)==home)occupied=true;if(occupied)return;
    Resident newcomer;newcomer.name="Habitante "+std::to_string(residents.size()+1);newcomer.trait=alive%2?"Trabajador":"Agricultora";newcomer.home=home;newcomer.goal=home;newcomer.x=home%mapWidth+.5f;newcomer.y=home/mapWidth+.5f;newcomer.grain=2;newcomer.relationships.resize(residents.size()+1);house->owner=static_cast<int>(residents.size());grain-=2;treasury-=8;residents.push_back(std::move(newcomer));for(auto& r:residents)r.relationships.resize(residents.size());++metrics["arrivals"];record("Llego un nuevo habitante: hay vivienda y provisiones");
}
}

