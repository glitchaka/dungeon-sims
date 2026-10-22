#include "World.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cmath>
#define NOMINMAX
#include <windows.h>
namespace isla {
using nlohmann::json;
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Artifact,name,bonus,value,rare)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Resident,name,trait,x,y,hunger,thirst,energy,health,social,fun,hygiene,speed,decisionTimer,actionTimer,homeWater,task,path,next,goal,target,home,wood,meat,grain,seeds,money,eggs,job,mission,alive,inDungeon,inBoat,resumeRest,relationships,artifacts,loot,fish,hasOrder,orderedTask,orderedCell,workCell,away,visitor,chosen,ageDays,inspiration,inspirationUntil)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Prop,x,y,halfWidth,halfDepth,height,tree,id,uid,owner,health,kind)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Crop,cell,stage,cycles,worker,growth,ripeAge)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Animal,x,y,timer,respawn,kind,alive)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camp,cell,owner,created)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Job,id,cell,goal,worker,kind,source,phase,woodCost,moneyCost)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Ship,cell,voyages,remaining,passenger,seaHome,x,y,fishingClock,state,path,next,riskResolved)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dungeon,name,state,entrance,level,room,runs,kills,party,x,y,enemies,opened,clock,cooldown,log)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Landform,kind,x,y,radiusX,radiusY,height,drawWidth,sprite)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Gift,cell,grain,wood)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Visit,id,origin,kind,state,resident,originResident,clock,incoming,partner)
bool World::save(const std::string& filename) const{try{
    json data={{"version",8},{"playerName",playerName},{"chatLog",chatLog},{"chatSeen",chatSeen},{"procedural",procedural},{"islandSeed",islandSeed},{"terrainRevision",terrainRevision},{"elevation",elevation},{"landforms",landforms},{"riverCourse",riverCourse},{"roadPolicy",roadPolicy},{"worldId",worldId},{"relations",relations},{"raining",raining},{"chosenBirthAt",chosenBirthAt},{"chosenPatron",chosenPatron},{"gifts",gifts},{"visits",visits},{"outbox",outbox},{"terrain",terrain},{"forest",forest},{"pasture",pasture},{"protected",protectedLand},{"fallow",fallow},{"residents",residents},{"props",props},{"crops",crops},{"animals",animals},{"camps",camps},{"jobs",jobs},{"ships",ships},{"dungeons",dungeons},{"events",events},{"metrics",metrics},{"elapsed",elapsed},{"harvestClock",harvestClock},{"alarmSeconds",alarmSeconds},{"alarmX",alarmX},{"alarmY",alarmY},{"livestockClock",livestockClock},{"lastDispatch",lastDispatch},{"grain",grain},{"harvests",harvests},{"wood",communityWood},{"treasury",treasury},{"herbs",herbs},{"reserve",reserve},{"livestockFood",livestockFood},{"priority",priority},{"nextJob",nextJob},{"autoTrade",autoTrade},{"randomState",randomState_},{"ecologyClock",ecologyClock},{"townClock",townClock},{"populationClock",populationClock},{"nextProp",nextProp},{"traffic",traffic}};
    std::filesystem::path path(filename);if(path.has_parent_path())std::filesystem::create_directories(path.parent_path());auto temp=path;temp+=".tmp";{std::ofstream out(temp,std::ios::binary|std::ios::trunc);out<<data.dump();out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}catch(...){return false;}}
bool World::load(const std::string& filename){try{
    std::ifstream input(filename,std::ios::binary);if(!input)return false;auto data=json::parse(input);int version=data.at("version").get<int>();if(version!=6&&version!=7&&version!=8)return false;World next;
    if(version==6)for(auto& r:data["residents"]){r["away"]=false;r["visitor"]=false;r["chosen"]=false;r["ageDays"]=6480;r["inspiration"]="";r["inspirationUntil"]=0;}
    if(version>=7){next.worldId=data.at("worldId");data.at("relations").get_to(next.relations);next.raining=data.at("raining");next.chosenBirthAt=data.at("chosenBirthAt");next.chosenPatron=data.at("chosenPatron");data.at("gifts").get_to(next.gifts);data.at("visits").get_to(next.visits);data.at("outbox").get_to(next.outbox);
        if(next.worldId.size()!=32||!std::isfinite(next.chosenBirthAt)||next.visits.size()>4096||next.outbox.size()>8192)return false;
        for(const auto& gift:next.gifts)if(gift.cell<0||gift.cell>=mapWidth*mapHeight||gift.grain<0||gift.wood<0)return false;
    }
    if(data.contains("riverCourse"))data.at("riverCourse").get_to(next.riverCourse);
    if(data.contains("landforms")){data.at("landforms").get_to(next.landforms);for(const auto& f:next.landforms)if((f.sprite<0&&f.kind!="plateau")||f.sprite>=4||f.radiusX<=0||f.radiusY<=0||f.height<0||(f.drawWidth<=0&&f.kind!="plateau"))return false;}
    if(data.contains("elevation")){data.at("elevation").get_to(next.elevation);for(float h:next.elevation)if(!std::isfinite(h)||h<0||h>32)return false;}
    next.playerName=data.value("playerName",std::string("Deidad"));next.procedural=data.value("procedural",false);next.islandSeed=data.value("islandSeed",0u);next.terrainRevision=data.value("terrainRevision",0);next.roadPolicy=2;
    if(version>=8){data.at("chatLog").get_to(next.chatLog);data.at("chatSeen").get_to(next.chatSeen);if(next.chatLog.size()>64||next.chatSeen.size()>256)return false;}
    for(auto& p:data["props"])if(!p.contains("kind"))p["kind"]="house";
    data.at("terrain").get_to(next.terrain);data.at("forest").get_to(next.forest);data.at("pasture").get_to(next.pasture);data.at("protected").get_to(next.protectedLand);data.at("fallow").get_to(next.fallow);data.at("residents").get_to(next.residents);data.at("props").get_to(next.props);data.at("crops").get_to(next.crops);data.at("animals").get_to(next.animals);data.at("camps").get_to(next.camps);data.at("jobs").get_to(next.jobs);data.at("ships").get_to(next.ships);data.at("dungeons").get_to(next.dungeons);data.at("events").get_to(next.events);data.at("metrics").get_to(next.metrics);
    next.elapsed=data.at("elapsed");next.harvestClock=data.at("harvestClock");next.alarmSeconds=data.at("alarmSeconds");next.alarmX=data.at("alarmX");next.alarmY=data.at("alarmY");next.livestockClock=data.at("livestockClock");next.lastDispatch=data.at("lastDispatch");next.grain=data.at("grain");next.harvests=data.at("harvests");next.communityWood=data.at("wood");next.treasury=data.at("treasury");next.herbs=data.at("herbs");next.reserve=data.at("reserve");next.livestockFood=data.at("livestockFood");next.priority=data.at("priority");next.nextJob=data.at("nextJob");next.autoTrade=data.at("autoTrade");next.randomState_=data.at("randomState");next.ecologyClock=data.at("ecologyClock");next.townClock=data.at("townClock");next.populationClock=data.at("populationClock");next.nextProp=data.at("nextProp");data.at("traffic").get_to(next.traffic);
    if(version<8){
        for(auto& t:next.terrain)if(t==Terrain::Road)t=Terrain::Ground;next.traffic.fill(0);
        next.jobs.erase(std::remove_if(next.jobs.begin(),next.jobs.end(),[&](const Job& j){bool obsolete=j.kind=="road"||j.kind=="remove_road"||j.kind=="move_road";if(obsolete){next.communityWood+=j.woodCost;next.treasury+=j.moneyCost;for(auto& r:next.residents)if(r.job==j.id){r.job=-1;r.hasOrder=false;r.task=Task::Idle;r.path.clear();r.next=0;}}return obsolete;}),next.jobs.end());
        next.record("Migracion: retirada la red de caminos antigua; las nuevas rutas dependen del transito");
    }
    if((next.residents.empty()||next.residents.size()>1000)||next.dungeons.size()!=3||next.ships.size()>128||!std::isfinite(next.elapsed)||next.grain<0||next.communityWood<0||next.treasury<0)return false;
    for(const auto& v:next.visits)if(v.resident<0||v.resident>=static_cast<int>(next.residents.size())||!std::isfinite(v.clock))return false;
    next.rebuildNavigation();for(const auto& r:next.residents){if(!std::isfinite(r.x)||!std::isfinite(r.y)||r.x<0||r.y<0||r.x>=mapWidth||r.y>=mapHeight||r.next>r.path.size()||r.mission<-1||r.mission>=3)return false;for(int k:r.path)if(k<0||k>=mapWidth*mapHeight)return false;if(r.alive&&!r.away&&!r.inDungeon&&!r.inBoat&&!next.walkable(static_cast<int>(r.x),static_cast<int>(r.y)))return false;}
    for(const auto& d:next.dungeons){if(d.room<1||d.room>3||(!d.party.empty()&&d.party.size()!=2))return false;for(int who:d.party)if(who<0||who>=static_cast<int>(next.residents.size()))return false;}
    for(const auto& c:next.crops)if(c.cell<0||c.cell>=mapWidth*mapHeight||c.worker<-1||c.worker>=static_cast<int>(next.residents.size())||c.stage<0||c.stage>3)return false;
    *this=std::move(next);return true;
}catch(...){return false;}}
}











