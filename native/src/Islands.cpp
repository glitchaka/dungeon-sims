#include "SeaLife.h"
#include "World.h"
#include "SettlementLayout.h"
#include "IslandGeography.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <memory>
namespace isla {
void World::generateIsland(std::uint32_t seed){
    auto generated=std::make_unique<World>();auto& fresh=*generated;fresh.playerName=playerName;fresh.islandSeed=seed;fresh.procedural=true;fresh.randomState_=seed;fresh.elapsed=93;
    fresh.props.clear();fresh.crops.clear();fresh.animals.clear();fresh.ships.clear();fresh.jobs.clear();fresh.gifts.clear();fresh.visits.clear();fresh.outbox.clear();fresh.forest.fill(false);fresh.pasture.fill(false);fresh.protectedLand.fill(false);fresh.fallow.fill(false);fresh.traffic.fill(0);
    IslandGeography::generate(fresh,seed);
    fresh.rebuildNavigation();
    SettlementLayout::placeResidents(fresh,seed);
    IslandGeography::seedVegetation(fresh,seed);
    fresh.rebuildNavigation();
    SettlementLayout::placeHomes(fresh,seed);
    for(int y=102;y<=104;++y)for(int x=128;x<=132;++x){int k=cellId(x,y);if(fresh.walkable(x,y))fresh.crops.push_back({k,3,0,-1,120,0});}
    for(int i=0;i<12;++i){for(int attempt=0;attempt<500;++attempt){int k=static_cast<int>(fresh.random()%(mapWidth*mapHeight));if(!fresh.walkable(k%mapWidth,k/mapWidth)||std::hypot(static_cast<float>(k%mapWidth-fresh.market%mapWidth),static_cast<float>(k/mapWidth-fresh.market/mapWidth))<10)continue;fresh.pasture[k]=i<4;fresh.animals.push_back({k%mapWidth+.5f,k/mapWidth+.5f,2,0,i<4?i:4,true});break;}}
    fresh.rebuildNavigation();
    for(auto& dungeon:fresh.dungeons){int best=-1,distance=100000;for(int k=0;k<mapWidth*mapHeight;++k)if(fresh.walkable(k%mapWidth,k/mapWidth)){int d=std::abs(k%mapWidth-dungeon.entrance%mapWidth)+std::abs(k/mapWidth-dungeon.entrance/mapWidth);if(d<distance){distance=d;best=k;}}dungeon.entrance=best;}
    SeaLife::seed(fresh);fresh.terrainRevision=terrainRevision+1;fresh.events.clear();fresh.record("Isla nueva generada con semilla "+std::to_string(seed));*this=std::move(fresh);
}
std::string World::growIsland(int center,int radius){
    if(center<0||center>=mapWidth*mapHeight)return "Elige una costa dentro del mapa";radius=std::clamp(radius,1,6);std::vector<int> changed;
    for(int y=std::max(1,center/mapWidth-radius);y<std::min(mapHeight-1,center/mapWidth+radius+1);++y)for(int x=std::max(1,center%mapWidth-radius);x<std::min(mapWidth-1,center%mapWidth+radius+1);++x){if(std::hypot(static_cast<float>(x-center%mapWidth),static_cast<float>(y-center/mapWidth))>radius)continue;int k=cellId(x,y);if(terrain[k]!=Terrain::Sea)continue;bool attached=false;for(int d:{1,-1,mapWidth,-mapWidth})if(terrain[k+d]==Terrain::Ground||terrain[k+d]==Terrain::Road)attached=true;for(const auto& boat:ships)if(boat.seaHome==k||cellId(static_cast<int>(boat.x),static_cast<int>(boat.y))==k||std::find(boat.path.begin(),boat.path.end(),k)!=boat.path.end())attached=false;if(attached)changed.push_back(k);}
    if(changed.empty())return "El crecimiento debe tocar tierra existente";
    if(!procedural){procedural=true;islandSeed=1749;}
    for(int k:changed){float sum=0;int count=0;for(int d:{1,-1,mapWidth,-mapWidth})if(terrain[k+d]==Terrain::Ground||terrain[k+d]==Terrain::Road){sum+=elevation[k+d];++count;}terrain[k]=Terrain::Ground;elevation[k]=count?sum/count:.5f;}++terrainRevision;rebuildNavigation();record("La deidad amplio la costa");return "Nueva tierra disponible para habitantes y construcciones";
}
}








