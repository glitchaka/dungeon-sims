#include "World.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
namespace isla {
using nlohmann::json;
std::string World::sendVisit(int who,const std::string& kind){
    if(night())return "Las visitas embarcan al amanecer";
    if(peerId.empty()||peerId==worldId)return "Conecta otra isla para enviar visitas";
    if(kind!="diplomacy"&&kind!="raid")return "Mision desconocida";
    if(who<0||who>=static_cast<int>(residents.size()))return "Habitante inexistente";auto& r=residents[who];
    if(!r.alive||r.visitor||r.away||r.inBoat||r.inDungeon||r.mission>=0||r.ageDays<World::adultAgeDays||r.health<cnc::maxHealth*.7f||r.energy<50||r.hunger<50||r.thirst<50)return "El habitante debe prepararse antes de viajar";
    auto committed=[&](int who){return std::any_of(visits.begin(),visits.end(),[&](const Visit& v){return !v.incoming&&v.resident==who&&v.state!="complete"&&v.state!="cancelled";});};
    if(committed(who))return "El habitante ya tiene un viaje asignado";
    int present=0;for(std::size_t i=0;i<residents.size();++i){const auto& p=residents[i];if(p.alive&&!p.away&&!p.visitor&&!p.inBoat&&!p.inDungeon&&!committed(static_cast<int>(i)))++present;}if(present<3)return "Deben quedar dos habitantes en la isla";
    Ship* transport=nullptr;
    for(auto& boat:ships)if(boat.passenger<0&&boat.remaining<=0&&boat.state=="docked"&&boat.seaHome>=0&&begin(r,Task::Move,boat.cell)){transport=&boat;break;}
    if(!transport)return "La visita necesita un barco libre y un muelle accesible";
    const std::string id=worldId+":"+std::to_string(++metrics["visit_sequence"]);
    visits.push_back({id,worldId,kind,"boarding",who,who,0,false,peerId});
    transport->passenger=who;transport->state="visit_boarding";transport->path.clear();transport->next=0;
    r.inspiration.clear();r.hasOrder=true;r.orderedTask=Task::Move;r.orderedCell=transport->cell;
    for(auto& c:crops)if(c.worker==who)c.worker=-1;for(auto& j:jobs)if(j.worker==who)j.worker=-1;
    record(r.name+" partio hacia otra isla: "+kind);return "El habitante camina al muelle para embarcar";
}
std::string World::receivePeer(const std::string& raw,const std::string& peer){try{
    if(raw.size()>16384||peer.empty()||peer==worldId)return "Mensaje de red rechazado";
    auto m=json::parse(raw);if(m.at("protocol")!=1||m.at("from")!=peer||m.at("to")!=worldId)return "Destino de red incorrecto";
    const auto type=m.at("type").get<std::string>(),id=m.at("id").get<std::string>();if(id.empty()||id.size()>100)return "Identificador invalido";
    if(type=="chat"){
        auto text=m.at("text").get<std::string>(),name=m.at("name").get<std::string>();if(id.rfind(peer+":chat:",0)!=0||text.empty()||text.size()>256||name.empty()||name.size()>32||std::any_of(text.begin(),text.end(),[](unsigned char c){return c<32;}))return "Chat invalido";
        if(std::find(chatSeen.begin(),chatSeen.end(),id)==chatSeen.end()){chatSeen.push_back(id);if(chatSeen.size()>256)chatSeen.erase(chatSeen.begin());chatLog.push_back(name+": "+text);if(chatLog.size()>64)chatLog.erase(chatLog.begin());}
        outbox["ack:"+id]=json{{"protocol",1},{"type","chat_ack"},{"id",id},{"from",worldId},{"to",peer}}.dump();return "Mensaje recibido";
    }
    if(type=="chat_ack"){auto pending=outbox.find(id);if(pending!=outbox.end()){auto original=json::parse(pending->second);if(original.at("type")=="chat"&&original.at("to")==peer)outbox.erase(pending);}return "Chat entregado";}
    auto trip=std::find_if(visits.begin(),visits.end(),[&](const Visit& v){return v.id==id;});
    if(type=="visit"){
        if(trip!=visits.end())return "Visita ya registrada";
        if(id.rfind(peer+":",0)!=0||visits.size()>4096||residents.size()>1000)return "No se puede recibir la visita";
        const auto kind=m.at("kind").get<std::string>();if(kind!="diplomacy"&&kind!="raid")return "Mision invalida";
        const auto name=m.at("name").get<std::string>();if(name.size()>48)return "Nombre invalido";
        int spawn=-1;float distance=1e9f;
        for(int k=0;k<mapWidth*mapHeight;++k)if(walkable(k%mapWidth,k/mapWidth)){
            bool occupied=false;for(const auto& r:residents)if(r.alive&&!r.away&&!r.inBoat&&!r.inDungeon&&cellId(static_cast<int>(r.x),static_cast<int>(r.y))==k)occupied=true;
            for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;
                        bool coast=false;for(int step:{1,-1,mapWidth,-mapWidth}){int n=k+step;if(n>=0&&n<mapWidth*mapHeight&&std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)==1&&terrain[n]==Terrain::Sea)coast=true;}
            float d=std::hypot(static_cast<float>(k%mapWidth-market%mapWidth),static_cast<float>(k/mapWidth-market/mapWidth));if(!occupied&&coast&&d<distance&&!route(k%mapWidth,k/mapWidth,[&](int x,int y){return cellId(x,y)==market;}).empty()){distance=d;spawn=k;}
        }
        if(spawn<0)return "No hay lugar para desembarcar";
        float health=m.at("health").get<float>();if(!std::isfinite(health)||health<=0||health>cnc::maxHealth)return "Estado de visitante invalido";
        float visitorAge=m.value("ageDays",World::adultAgeDays),visitorLifespan=m.value("naturalDeathAge",90*360.0f);if(!std::isfinite(visitorAge)||visitorAge<World::adultAgeDays||!std::isfinite(visitorLifespan)||visitorLifespan<World::adultAgeDays)return "Edad de visitante invalida";
        Resident guest;guest.ageDays=visitorAge;guest.naturalDeathAge=visitorLifespan;guest.female=m.value("female",false);guest.name=name;guest.trait="Visitante";guest.visitor=true;guest.seeds=guest.money=0;guest.chosen=m.value("chosen",false);guest.health=health;guest.home=guest.goal=spawn;guest.x=spawn%mapWidth+.5f;guest.y=spawn/mapWidth+.5f;guest.relationships.resize(residents.size()+1);
        int index=static_cast<int>(residents.size());residents.push_back(guest);for(auto& r:residents)r.relationships.resize(residents.size());
        visits.push_back({id,peer,kind,"arrived",index,m.at("who").get<int>(),0,true,peer});record(name+" llego desde otra isla: "+kind);return "Visitante recibido";
    }
    if(trip==visits.end()||trip->partner!=peer)return "Viaje desconocido";
    if(type=="result"&&!trip->incoming){
        if(trip->state!="travel"&&trip->state!="returning"&&trip->state!="complete")return "El viaje todavia no llego a destino";
        if(trip->origin!=worldId||trip->resident<0||trip->resident>=static_cast<int>(residents.size()))return "Retorno invalido";
        if(trip->state!="complete"&&trip->state!="returning"){
            int gold=m.at("gold").get<int>(),food=m.at("food").get<int>();float health=m.at("health").get<float>();if(gold<0||gold>12||food<0||food>8||!std::isfinite(health)||health<0||health>cnc::maxHealth)return "Resultado invalido";
            auto& r=residents[trip->resident];double returnedAge=m.value("ageDays",r.ageDays);if(!std::isfinite(returnedAge)||returnedAge<World::adultAgeDays)return "Edad de retorno invalida";r.ageDays=std::max(r.ageDays,returnedAge);
            auto boat=std::find_if(ships.begin(),ships.end(),[&](const Ship& s){return s.passenger==trip->resident&&s.state=="visit_waiting";});
            if(boat!=ships.end()){
                r.health=health;r.alive=health>0;r.money+=gold;r.grain+=food;trip->state="returning";outbox.erase(id);
                relations[peer]=std::clamp(relations[peer]+(trip->kind=="diplomacy"?10:-20),-100,100);
                std::reverse(boat->path.begin(),boat->path.end());boat->next=0;boat->state="visit_returning";
                outbox["ack:"+id]=json{{"protocol",1},{"type","confirmed"},{"id",id},{"from",worldId},{"to",peer}}.dump();return "El barco inicio el regreso a la isla";
            }
            int spawn=-1,best=100000;
            for(int k=0;k<mapWidth*mapHeight;++k)if(walkable(k%mapWidth,k/mapWidth)){
                bool occupied=false;for(const auto& p:residents)if(p.alive&&!p.away&&!p.inBoat&&!p.inDungeon&&cellId(static_cast<int>(p.x),static_cast<int>(p.y))==k)occupied=true;
                for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;
                int distance=std::abs(k%mapWidth-static_cast<int>(r.x))+std::abs(k/mapWidth-static_cast<int>(r.y));if(!occupied&&distance<best){best=distance;spawn=k;}
            }
            if(spawn<0&&health>0)return "El retorno espera un desembarco libre";
            if(spawn>=0){r.x=spawn%mapWidth+.5f;r.y=spawn/mapWidth+.5f;if(!walkable(r.home%mapWidth,r.home/mapWidth))r.home=spawn;}
            r.away=false;r.health=health;r.alive=health>0;r.money+=gold;r.grain+=food;r.task=Task::Idle;r.decisionTimer=0;
            relations[peer]=std::clamp(relations[peer]+(trip->kind=="diplomacy"?10:-20),-100,100);trip->state="complete";outbox.erase(id);++metrics["visits_returned"];record(r.name+(r.alive?" regreso de su visita":" murio durante su visita"));
        }
        outbox["ack:"+id]=json{{"protocol",1},{"type","confirmed"},{"id",id},{"from",worldId},{"to",peer}}.dump();return "Retorno registrado";
    }
    if(type=="confirmed"&&trip->incoming&&trip->origin==peer){outbox.erase("result:"+id);return "Resultado confirmado";}
    return "Mensaje no aplicable";
}catch(...){return "Mensaje de red invalido";}}
void World::updateVisits(float dt){
    for(auto& trip:visits)if(!trip.incoming&&trip.state=="travel"&&!outbox.count(trip.id)){
        const auto& r=residents[trip.resident];
        outbox[trip.id]=json{{"protocol",1},{"type","visit"},{"id",trip.id},{"from",worldId},{"to",trip.partner},{"who",trip.originResident},{"kind",trip.kind},{"name",r.name},{"trait",r.trait},{"health",r.health},{"chosen",r.chosen},{"ageDays",r.ageDays},{"female",r.female},{"naturalDeathAge",r.naturalDeathAge}}.dump();
    }
    for(auto& trip:visits)if(trip.incoming&&trip.state!="complete"){
        auto& r=residents[trip.resident];trip.clock+=dt;
        // Guests walk using local collision and priorities; the receiving island resolves outcomes.
        if(r.alive&&trip.clock<45)continue;
        int gold=0,food=0;
        if(r.alive&&trip.kind=="diplomacy"){metrics["diplomacy"]+=1;relations[trip.origin]=std::min(100,relations[trip.origin]+10);gold=std::min(3,treasury);treasury-=gold;record(r.name+" establecio relaciones diplomaticas");}
        if(r.alive&&trip.kind=="raid"){
            int defenders=0;for(const auto& p:residents)if(p.alive&&!p.away&&!p.visitor&&!p.inBoat&&!p.inDungeon&&p.health>cnc::maxHealth*.5f&&std::hypot(p.x-r.x,p.y-r.y)<12)++defenders;
            r.health=std::max(0.0f,r.health-defenders*25.0f);if(r.health>0){gold=std::min(12,treasury);food=std::min(8,grain);treasury-=gold;grain-=food;}relations[trip.origin]=std::max(-100,relations[trip.origin]-20);++metrics["raids"];alarm(r.x,r.y);record("Una incursion saqueo "+std::to_string(gold)+" monedas y "+std::to_string(food)+" de grano");
        }
        outbox["result:"+trip.id]=json{{"protocol",1},{"type","result"},{"id",trip.id},{"from",worldId},{"to",trip.origin},{"gold",gold},{"food",food},{"health",r.health},{"ageDays",r.ageDays}}.dump();
        trip.state="complete";r.away=true;r.alive=false;r.path.clear();r.next=0;
    }
    if(peerId.empty())return;
    for(std::size_t i=0;i<residents.size();++i){const auto& r=residents[i];if(r.inspiration=="diplomacy"||r.inspiration=="raid"){if(!r.away&&r.hunger>65&&r.thirst>65&&r.energy>60&&r.health>cnc::maxHealth*.7f){sendVisit(static_cast<int>(i),r.inspiration);break;}}}
}
std::string World::sendChat(const std::string& text){
    if(peerId.empty())return "Conecta otra isla para conversar";
    if(text.empty()||text.size()>256||std::any_of(text.begin(),text.end(),[](unsigned char c){return c<32;}))return "Mensaje vacio o demasiado largo";
    if(outbox.size()>128)return "Espera a que se entreguen los mensajes pendientes";
    std::string id=worldId+":chat:"+std::to_string(++metrics["chat_sequence"]);
    outbox[id]=json{{"protocol",1},{"type","chat"},{"id",id},{"from",worldId},{"to",peerId},{"name",playerName},{"text",text}}.dump();chatLog.push_back(playerName+": "+text);if(chatLog.size()>64)chatLog.erase(chatLog.begin());return "Mensaje enviado";
}
}





