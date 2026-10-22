#include "World.h"
#include <algorithm>
#include <cmath>
#include <queue>
namespace isla {
void World::generateIsland(std::uint32_t seed){
    World fresh;fresh.playerName=playerName;fresh.islandSeed=seed;fresh.procedural=true;fresh.randomState_=seed;fresh.elapsed=93;
    fresh.props.clear();fresh.crops.clear();fresh.animals.clear();fresh.ships.clear();fresh.jobs.clear();fresh.gifts.clear();fresh.visits.clear();fresh.outbox.clear();fresh.forest.fill(false);fresh.pasture.fill(false);fresh.protectedLand.fill(false);fresh.fallow.fill(false);fresh.traffic.fill(0);
    const float phase=static_cast<float>((seed*2654435761u)>>8)/16777216.0f*6.2831853f;
    for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){float nx=(x-mapWidth*.5f)/(mapWidth*.5f-2),ny=(y-mapHeight*.5f)/(mapHeight*.5f-2);float angle=std::atan2(ny,nx),radius=std::hypot(nx,ny);float coast=.75f+.13f*std::sin(angle*3+phase)+.075f*std::sin(angle*7-phase*.7f)+.025f*std::sin(x*.2f+y*.115f+phase);int k=cellId(x,y);fresh.terrain[k]=radius<coast?Terrain::Ground:Terrain::Sea;}
    float shift=3*std::sin(phase);
    fresh.landforms={{"mountain",63+shift,20,13,10,17,23,0},{"mountain",78-shift,31,11,8,14,20,0},{"hill",48+shift,31,7,5,6,12,1},{"hill",93-shift,53,6,4,5,10,1},{"mount",33+shift,59,3,2.5f,3,5.5f,3},{"ramp",42+shift,64,4.5f,6,2.4f,9,2},{"plateau",45+shift,59,5,4,2.4f,0,-1}};
    for(auto& form:fresh.landforms){form.x*=2;form.y*=2;form.radiusX*=2;form.radiusY*=2;form.drawWidth*=2;}
    for(const auto& form:fresh.landforms)for(int y=std::max(1,static_cast<int>(form.y-form.radiusY));y<std::min(mapHeight-1,static_cast<int>(form.y+form.radiusY+1));++y)for(int x=std::max(1,static_cast<int>(form.x-form.radiusX));x<std::min(mapWidth-1,static_cast<int>(form.x+form.radiusX+1));++x){int k=cellId(x,y);float r=std::hypot((x-form.x)/form.radiusX,(y-form.y)/form.radiusY);if(fresh.terrain[k]!=Terrain::Ground)continue;if(form.kind=="mountain"&&r<1)fresh.terrain[k]=Terrain::Mountain;else if((form.kind=="hill"||form.kind=="mount")&&r<.38f)fresh.terrain[k]=Terrain::Mountain;}
    // The southern river ends at the sea without splitting the settlement.
    for(int y=46;y<mapHeight-1;++y){float x=148+10*std::sin(y*.045f+phase)+4*std::sin(y*.115f-phase);int center=cellId(static_cast<int>(x),y);if(fresh.terrain[center]==Terrain::Sea)break;float level=y<66?5.0f:y<88?2.4f:y<114?1.2f:.2f;fresh.riverCourse.push_back({x,y+.5f,level});for(int xx=static_cast<int>(x-1);xx<=static_cast<int>(x+1);++xx)if(std::abs(xx+.5f-x)<.85f&&fresh.terrain[cellId(xx,y)]!=Terrain::Sea)fresh.terrain[cellId(xx,y)]=Terrain::FreshWater;}
    // Elevation is shared by rendering, navigation and building foundations.
    for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){int k=cellId(x,y);if(fresh.terrain[k]==Terrain::Sea){fresh.elevation[k]=0;continue;}
        float hills=1.45f+.65f*std::sin(x*.06f+phase)*std::cos(y*.055f-phase)+.35f*std::sin(x*.105f+y*.07f+phase);
        float coastBand=.5f+.5f*std::sin(x*.09f-y*.14f+phase);bool shore=false;for(int d:{1,-1,mapWidth,-mapWidth})if(k+d>=0&&k+d<mapWidth*mapHeight&&fresh.terrain[k+d]==Terrain::Sea)shore=true;
        if(shore)hills=coastBand>.47f?2.4f+coastBand:.12f;
        float settlement=std::clamp((std::hypot(x-114.0f,y-96.0f)-7)/10,0.0f,1.0f);hills=.8f+(hills-.8f)*settlement;
        for(const auto& form:fresh.landforms){float r=std::hypot((x-form.x)/form.radiusX,(y-form.y)/form.radiusY);if(r>=1)continue;if(form.kind=="plateau"){hills=std::max(hills,.8f+form.height);}else if(form.kind=="ramp"){float t=std::clamp(.5f+(x-form.x)/(form.radiusX*2)-(y-form.y)/(form.radiusY*2),0.0f,1.0f);hills=std::max(hills,.8f+form.height*t);}else hills+=form.height*(1-r)*(1-r);}
        if(fresh.terrain[k]==Terrain::FreshWater)hills=(y<66?5.0f:y<88?2.4f:y<114?1.2f:.2f);
        // Gentle, traversable riverbanks form the valley; mountain stream drops remain waterfalls.
        if(fresh.terrain[k]==Terrain::Ground){int nearest=99;for(int yy=std::max(0,y-4);yy<std::min(mapHeight,y+5);++yy)for(int xx=std::max(0,x-4);xx<std::min(mapWidth,x+5);++xx)if(fresh.terrain[cellId(xx,yy)]==Terrain::FreshWater)nearest=std::min(nearest,std::abs(xx-x)+std::abs(yy-y));if(nearest<5){float valley=(y<66?5.0f:y<88?2.4f:y<114?1.2f:.2f);hills=valley+(hills-valley)*nearest/5.0f;}}
        fresh.elevation[k]=std::max(0.0f,hills);
    }
    fresh.market=cellId(114,96);fresh.granary=cellId(115,96);
    fresh.rebuildNavigation();
    for(std::size_t i=0;i<fresh.residents.size();++i){auto& r=fresh.residents[i];r.x=104.5f+static_cast<float>(i);r.y=96.5f;r.home=r.goal=cellId(static_cast<int>(r.x),96);r.task=Task::Idle;r.path.clear();r.next=0;}
    for(int y=3;y<mapHeight-3;++y)for(int x=3;x<mapWidth-3;++x){int k=cellId(x,y);if(fresh.terrain[k]!=Terrain::Ground)continue;float cluster=std::sin(x*.065f+phase)+std::cos(y*.095f-phase)+std::sin((x+y)*.045f);if(cluster>.6f&&std::hypot(x-114.0f,y-96.0f)>12&&fresh.random()%4==0){fresh.forest[k]=true;fresh.props.push_back({x+.5f,y+.5f,.32f,.32f,3,true,"Arbol",fresh.nextProp++,-1,60});}}
    fresh.rebuildNavigation();
    // Six homes, placed by validity rather than a fixed village illustration.
    for(std::size_t i=0;i<fresh.residents.size();++i){bool placed=false;for(int y=87;y<106&&!placed;y+=3)for(int x=101;x<121&&!placed;x+=3){int k=cellId(x,y);if(!fresh.placeable(k))continue;fresh.props.push_back({x+1.0f,y+1.0f,.9f,.9f,3,false,"Casa",fresh.nextProp++,static_cast<int>(i),100});fresh.residents[i].home=k+2*mapWidth;fresh.rebuildNavigation();placed=true;}}
    for(int y=102;y<=104;++y)for(int x=128;x<=132;++x){int k=cellId(x,y);if(fresh.walkable(x,y))fresh.crops.push_back({k,3,0,-1,120,0});}
    for(int i=0;i<12;++i){for(int attempt=0;attempt<500;++attempt){int k=static_cast<int>(fresh.random()%(mapWidth*mapHeight));if(!fresh.walkable(k%mapWidth,k/mapWidth)||std::hypot(k%mapWidth-114.0f,k/mapWidth-96.0f)<10)continue;fresh.pasture[k]=i<4;fresh.animals.push_back({k%mapWidth+.5f,k/mapWidth+.5f,2,0,i<4?i:4,true});break;}}
    fresh.rebuildNavigation();
    for(auto& dungeon:fresh.dungeons){int best=-1,distance=100000;for(int k=0;k<mapWidth*mapHeight;++k)if(fresh.walkable(k%mapWidth,k/mapWidth)){int d=std::abs(k%mapWidth-dungeon.entrance%mapWidth)+std::abs(k/mapWidth-dungeon.entrance/mapWidth);if(d<distance){distance=d;best=k;}}dungeon.entrance=best;}
    fresh.terrainRevision=terrainRevision+1;fresh.events.clear();fresh.record("Isla nueva generada con semilla "+std::to_string(seed));*this=std::move(fresh);
}
std::string World::growIsland(int center,int radius){
    if(center<0||center>=mapWidth*mapHeight)return "Elige una costa dentro del mapa";radius=std::clamp(radius,1,6);std::vector<int> changed;
    for(int y=std::max(1,center/mapWidth-radius);y<std::min(mapHeight-1,center/mapWidth+radius+1);++y)for(int x=std::max(1,center%mapWidth-radius);x<std::min(mapWidth-1,center%mapWidth+radius+1);++x){if(std::hypot(static_cast<float>(x-center%mapWidth),static_cast<float>(y-center/mapWidth))>radius)continue;int k=cellId(x,y);if(terrain[k]!=Terrain::Sea)continue;bool attached=false;for(int d:{1,-1,mapWidth,-mapWidth})if(terrain[k+d]==Terrain::Ground||terrain[k+d]==Terrain::Road)attached=true;for(const auto& boat:ships)if(boat.seaHome==k||cellId(static_cast<int>(boat.x),static_cast<int>(boat.y))==k||std::find(boat.path.begin(),boat.path.end(),k)!=boat.path.end())attached=false;if(attached)changed.push_back(k);}
    if(changed.empty())return "El crecimiento debe tocar tierra existente";
    if(!procedural){procedural=true;islandSeed=1749;}
    for(int k:changed){float sum=0;int count=0;for(int d:{1,-1,mapWidth,-mapWidth})if(terrain[k+d]==Terrain::Ground||terrain[k+d]==Terrain::Road){sum+=elevation[k+d];++count;}terrain[k]=Terrain::Ground;elevation[k]=count?sum/count:.5f;}++terrainRevision;rebuildNavigation();record("La deidad amplio la costa");return "Nueva tierra disponible para habitantes y construcciones";
}
}








