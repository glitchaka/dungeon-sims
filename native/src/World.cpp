#include "World.h"
#include "Geography.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
namespace isla {
namespace {
bool inside(int x,int y){return x>=0&&y>=0&&x<mapWidth&&y<mapHeight;}
bool polygon(float x,float y,const std::vector<std::array<float,2>>& p){bool in=false;for(std::size_t i=0,j=p.size()-1;i<p.size();j=i++){if(((p[i][1]>y)!=(p[j][1]>y))&&(x<(p[j][0]-p[i][0])*(y-p[i][1])/(p[j][1]-p[i][1])+p[i][0]))in=!in;}return in;}
float ellipse(float x,float y,float cx,float cy,float rx,float ry){return (x-cx)*(x-cx)/(rx*rx)+(y-cy)*(y-cy)/(ry*ry);}
}
std::uint32_t World::random(){randomState_=randomState_*1664525u+1013904223u;return randomState_;}
Terrain World::terrainAt(int x,int y) const{return inside(x,y)?terrain[cellId(x,y)]:Terrain::Sea;}
bool World::walkable(int x,int y) const{return inside(x,y)&&terrainAt(x,y)!=Terrain::Sea&&terrainAt(x,y)!=Terrain::FreshWater&&terrainAt(x,y)!=Terrain::Mountain&&!blocked_[cellId(x,y)];}
bool World::besideFreshWater(int x,int y) const{return terrainAt(x+1,y)==Terrain::FreshWater||terrainAt(x-1,y)==Terrain::FreshWater||terrainAt(x,y+1)==Terrain::FreshWater||terrainAt(x,y-1)==Terrain::FreshWater;}
void World::rebuildNavigation(){blocked_.fill(false);for(const auto& p:props)for(int y=static_cast<int>(std::floor(p.y-p.halfDepth));y<=static_cast<int>(std::floor(p.y+p.halfDepth));++y)for(int x=static_cast<int>(std::floor(p.x-p.halfWidth));x<=static_cast<int>(std::floor(p.x+p.halfWidth));++x)if(inside(x,y))blocked_[cellId(x,y)]=true;for(const auto& c:crops)if(c.stage>0)blocked_[c.cell]=true;}
void World::addProp(Prop p){props.push_back(std::move(p));rebuildNavigation();}
World::World(){
    // Only hard geology is authored. Roads are built by workers from actual use.
    for(int k=0;k<mapWidth*mapHeight;++k)terrain[k]=static_cast<Terrain>(geologyCells[k]);
    for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){int k=cellId(x,y);if(terrain[k]!=Terrain::Ground)continue;forest[k]=(ellipse(x,y,45,23,11,9)<1||ellipse(x,y,46,55,13,10)<1||ellipse(x,y,68,40,10,9)<1||ellipse(x,y,83,24,9,8)<1||ellipse(x,y,77,61,8,7)<1);pasture[k]=ellipse(x,y,37,50,3,3)<1;}
    auto freeNear=[&](int desired){int best=-1;float score=1e9f;for(int k=0;k<mapWidth*mapHeight;++k)if(walkable(k%mapWidth,k/mapWidth)){bool occupied=false;for(const auto& r:residents)if(cellId(static_cast<int>(r.x),static_cast<int>(r.y))==k)occupied=true;if(occupied)continue;float d=std::hypot(static_cast<float>(k%mapWidth-desired%mapWidth),static_cast<float>(k/mapWidth-desired/mapWidth));if(d<score){score=d;best=k;}}return best;};
    market=freeNear(market);granary=freeNear(granary);
    constexpr std::array<const char*,6> names{{"Luna","Bruno","Alma","Teo","Vera","Nico"}};
    constexpr std::array<const char*,6> traits{{"Exploradora","Sociable","Cazadora","Trabajador","Agricultora","Explorador"}};
    constexpr std::array<int,6> homes{{cellId(55,46),cellId(56,46),cellId(57,46),cellId(59,46),cellId(60,46),cellId(61,46)}};
    for(std::size_t i=0;i<names.size();++i){Resident r;r.name=names[i];r.trait=traits[i];r.home=freeNear(homes[i]);r.goal=r.home;r.x=r.home%mapWidth+.5f;r.y=r.home/mapWidth+.5f;residents.push_back(r);}

    for(int center:std::array<int,5>{{cellId(49,35),cellId(96,38),cellId(88,48),cellId(56,63),cellId(55,80)}})
        for(int dy=-1;dy<=1;++dy)for(int dx=-2;dx<=2;++dx){int x=center%mapWidth+dx,y=center/mapWidth+dy,k=cellId(x,y);if(x<0||y<0||x>=mapWidth||y>=mapHeight||terrain[k]!=Terrain::Ground)continue;forest[k]=false;crops.push_back({k,(x%4==0?0:3),0,-1,(x%4==0?0.0f:120.0f),0});}
    std::vector<int> habitat,domestic;for(int k=0;k<mapWidth*mapHeight;++k){if(forest[k]&&walkable(k%mapWidth,k/mapWidth))habitat.push_back(k);if(pasture[k]&&walkable(k%mapWidth,k/mapWidth))domestic.push_back(k);}
    for(int i=0;i<20;++i){const auto& pool=i<8?domestic:habitat;int k=pool[random()%pool.size()];for(std::size_t attempt=0;attempt<pool.size();++attempt){bool occupied=false;for(const auto& other:animals)if(cellId(static_cast<int>(other.x),static_cast<int>(other.y))==k)occupied=true;if(!occupied)break;k=pool[(attempt+1)%pool.size()];}animals.push_back({k%mapWidth+.5f,k/mapWidth+.5f,2,0,i<8?i%4:4,true});}
    ships={{cellId(23,42),0,0},{cellId(96,65),0,0}};
    for(int i=0;i<3;++i){Dungeon d;d.name=std::array<const char*,3>{{"CRIPTA DEL ROBLE","MINAS DEL ECO","RUINAS DE LA MAREA"}}[i];d.entrance=std::array<int,3>{{cellId(44,23),cellId(78,29),cellId(47,76)}}[i];d.entrance=freeNear(d.entrance);d.level=i+1;d.enemies.fill(22.0f+d.level*7);dungeons.push_back(d);}
    seedEnvironment();
    record("La isla despierta. Sus habitantes deciden por si mismos.");
}
std::vector<int> World::route(int sx,int sy,const std::function<bool(int,int)>& goal) const{
    if(!walkable(sx,sy))return {};std::array<int,mapWidth*mapHeight> previous;previous.fill(-1);std::queue<int> q;const int start=cellId(sx,sy);previous[start]=start;q.push(start);int found=-1;
    while(!q.empty()){int at=q.front();q.pop();int x=at%mapWidth,y=at/mapWidth;if(goal(x,y)){found=at;break;}for(auto d:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int nx=x+d[0],ny=y+d[1];if(!walkable(nx,ny))continue;int k=cellId(nx,ny);if(previous[k]>=0)continue;bool occupied=false;for(const auto& person:residents)if(person.alive&&!person.inDungeon&&!person.inBoat&&cellId(static_cast<int>(person.x),static_cast<int>(person.y))==k)occupied=true;for(const auto& animal:animals)if(animal.alive&&cellId(static_cast<int>(animal.x),static_cast<int>(animal.y))==k)occupied=true;if(occupied&&!goal(nx,ny))continue;previous[k]=at;q.push(k);}}
    if(found<0)return {};std::vector<int> path;for(int at=found;at!=start;at=previous[at])path.push_back(at);path.push_back(start);std::reverse(path.begin(),path.end());return path;
}
int World::nearest(const Resident& r,const std::function<bool(int,int)>& predicate) const{auto p=route(static_cast<int>(r.x),static_cast<int>(r.y),predicate);return p.empty()?-1:p.back();}
void World::alarm(float x,float y){alarmX=x;alarmY=y;alarmSeconds=8;}
bool World::night() const{float h=std::fmod(elapsed/300*24,24.0f);return h>=19||h<6;}
bool World::hasBonus(const Resident& r,const std::string& b) const{return std::any_of(r.artifacts.begin(),r.artifacts.end(),[&](const Artifact& a){return a.bonus==b;});}
Crop* World::cropAt(int k){for(auto& c:crops)if(c.cell==k)return &c;return nullptr;}
void World::record(const std::string& message){events.push_back("D"+std::to_string(static_cast<int>(elapsed/300)+1)+" | "+message);if(events.size()>120)events.erase(events.begin());}
}







