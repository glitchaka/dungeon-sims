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
    auto& r=residents[who];auto& s=ships[boat];if(!r.alive||r.away||r.visitor||r.ageDays<World::adultAgeDays||r.inDungeon||r.inBoat||r.mission>=0)return "El habitante no esta disponible";
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
        if(s.state.rfind("visit_",0)==0){
            if(s.passenger<0||s.passenger>=static_cast<int>(residents.size())){s.state="docked";s.passenger=-1;continue;}
            auto& traveler=residents[s.passenger];auto trip=std::find_if(visits.begin(),visits.end(),[&](const Visit& v){return !v.incoming&&v.resident==s.passenger&&v.state!="complete"&&v.state!="cancelled";});
            if(trip==visits.end()){traveler.inBoat=traveler.away=false;s.state="docked";s.passenger=-1;continue;}
            if(s.state=="visit_boarding"){
                if(night())continue;
                if(!traveler.alive){trip->state="cancelled";traveler.hasOrder=false;s.state="docked";s.passenger=-1;continue;}
                if(at(traveler)!=s.cell||traveler.next<traveler.path.size()){
                    if(!night()&&traveler.hunger>45&&traveler.thirst>45&&traveler.energy>45&&!traveler.hasOrder&&begin(traveler,Task::Move,s.cell)){traveler.hasOrder=true;traveler.orderedTask=Task::Move;traveler.orderedCell=s.cell;}continue;
                }
                s.fishingClock+=dt;if(s.fishingClock<3)continue;
                const int home=s.seaHome;s.path=seaRoute(*this,home,[=](int k){return std::abs(k%mapWidth-home%mapWidth)+std::abs(k/mapWidth-home/mapWidth)>=12;});
                if(s.path.empty()){trip->state="cancelled";traveler.hasOrder=false;traveler.task=Task::Idle;s.state="docked";s.passenger=-1;record("Visita cancelada: no existe salida navegable al mar");continue;}
                s.next=0;s.state="visit_outbound";trip->state="sailing";traveler.inBoat=true;traveler.hasOrder=false;traveler.path.clear();traveler.next=0;++metrics["naval_departures"];
            }
            if(s.state=="visit_outbound"||s.state=="visit_returning"){
                if(s.next<s.path.size()){int k=s.path[s.next];if(terrain[k]!=Terrain::Sea){const int destination=s.state=="visit_returning"?s.seaHome:s.path.back();auto rerouted=seaRoute(*this,cellId(static_cast<int>(s.x),static_cast<int>(s.y)),[=](int c){return c==destination;});if(rerouted.empty())continue;s.path=std::move(rerouted);s.next=0;k=s.path[0];}float tx=k%mapWidth+.5f,ty=k/mapWidth+.5f,len=std::hypot(tx-s.x,ty-s.y),step=std::min(len,1.25f*dt);if(len>.0001f){s.x+=(tx-s.x)/len*step;s.y+=(ty-s.y)/len*step;}if(len<=step+.0001f){s.x=tx;s.y=ty;++s.next;}continue;}
                if(s.state=="visit_outbound"){s.state="visit_waiting";trip->state="travel";traveler.away=true;traveler.inBoat=true;record(traveler.name+" alcanzo alta mar rumbo a otra isla");}
                else {
                    int landing=s.cell;
                    auto freeLanding=[&](int k){if(!walkable(k%mapWidth,k/mapWidth))return false;for(const auto& p:residents)if(&p!=&traveler&&p.alive&&!p.away&&!p.inBoat&&!p.inDungeon&&at(p)==k)return false;for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)return false;return true;};
                    if(!freeLanding(landing)){
                        auto routeToShore=seaRoute(*this,cellId(static_cast<int>(s.x),static_cast<int>(s.y)),[&](int k){for(int step:{1,-1,mapWidth,-mapWidth}){int n=k+step;if(n>=0&&n<mapWidth*mapHeight&&std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)==1&&freeLanding(n))return true;}return false;});
                        if(routeToShore.empty())continue;s.path=std::move(routeToShore);s.next=0;int end=s.path.back();for(int step:{1,-1,mapWidth,-mapWidth}){int n=end+step;if(n>=0&&n<mapWidth*mapHeight&&std::abs(n%mapWidth-end%mapWidth)+std::abs(n/mapWidth-end/mapWidth)==1&&freeLanding(n)){s.cell=n;break;}}continue;
                    }

                    traveler.x=landing%mapWidth+.5f;traveler.y=landing/mapWidth+.5f;traveler.away=traveler.inBoat=false;traveler.task=Task::Idle;traveler.decisionTimer=0;if(!walkable(traveler.home%mapWidth,traveler.home/mapWidth))traveler.home=landing;
                    trip->state="complete";++metrics["visits_returned"];++metrics["naval_returns"];s.state="docked";s.passenger=-1;record(traveler.name+(traveler.alive?" desembarco de regreso de su visita":" no sobrevivio a su visita"));
                }
            }
            continue;
        }
        if(s.passenger<0||s.passenger>=static_cast<int>(residents.size()))continue;auto& r=residents[s.passenger];
        if(!r.alive){r.inBoat=false;s.passenger=-1;s.state="lost";continue;}
        if(s.state=="boarding"){if(at(r)!=s.cell||r.next<r.path.size())continue;s.fishingClock+=dt;if(s.fishingClock<3)continue;int home=s.seaHome;s.path=seaRoute(*this,home,[=](int k){return std::abs(k%mapWidth-home%mapWidth)+std::abs(k/mapWidth-home/mapWidth)>=12;});if(s.path.empty()){r.hasOrder=false;r.task=Task::Idle;s.passenger=-1;s.state="docked";record("Pesca cancelada: ruta maritima no disponible");continue;}s.next=0;s.state="outbound";r.inBoat=true;r.path.clear();r.next=0;}
        r.hunger=std::max(0.0f,r.hunger-.03f*dt);r.thirst=std::max(0.0f,r.thirst-.04f*dt);r.energy=std::max(0.0f,r.energy-.06f*dt);
        if(s.state=="outbound"||s.state=="returning"){
            if(s.next<s.path.size()){int k=s.path[s.next];float tx=k%mapWidth+.5f,ty=k/mapWidth+.5f,len=std::hypot(tx-s.x,ty-s.y),step=std::min(len,1.25f*dt);if(len>.0001f){s.x+=(tx-s.x)/len*step;s.y+=(ty-s.y)/len*step;}if(len<=step+.0001f){s.x=tx;s.y=ty;++s.next;}continue;}
            if(s.state=="outbound"){s.state="fishing";s.fishingClock=0;}
            else {int fish=4+static_cast<int>(random()%5);r.fish+=fish;r.inBoat=false;r.hasOrder=false;r.x=s.cell%mapWidth+.5f;r.y=s.cell/mapWidth+.5f;r.task=Task::Idle;r.path.clear();r.next=0;++s.voyages;++metrics["fishing_returns"];minds.complete(*this,residents.indexOf(r),true,"catch");record(r.name+" regreso con "+std::to_string(fish)+" pescados");s.state="docked";s.passenger=-1;}
        }
        if(s.state=="fishing"){s.fishingClock+=dt;if(s.fishingClock<30)continue;if(!s.riskResolved){s.riskResolved=true;const float roll=static_cast<float>(random()>>8)/16777216.0f;if(fishingLoss(roll)){minds.complete(*this,residents.indexOf(r),false,"lost_at_sea");r.alive=false;r.health=0;r.inBoat=false;r.hasOrder=false;r.task=Task::Idle;s.state="lost";s.passenger=-1;++metrics["lost_at_sea"];record(r.name+" y su barco se perdieron en alta mar");continue;}}
            const int now=cellId(static_cast<int>(s.x),static_cast<int>(s.y)),home=s.seaHome;s.path=seaRoute(*this,now,[=](int k){return k==home;});s.next=0;s.state="returning";}
    }
}
}




