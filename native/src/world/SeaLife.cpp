#include "SeaLife.h"
#include "World.h"
#include <cmath>
#include <algorithm>
#include <queue>
namespace isla {
std::vector<int> SeaLife::route(const World& w,int start,int end) {
    if(start<0||end<0||start>=mapWidth*mapHeight||end>=mapWidth*mapHeight||w.terrain[start]!=Terrain::Sea||w.terrain[end]!=Terrain::Sea)return {};
    std::vector<int> previous(mapWidth*mapHeight,-1);std::queue<int> queue;queue.push(start);previous[start]=start;
    while(!queue.empty()&&previous[end]<0){const int k=queue.front();queue.pop();for(int d:{1,-1,mapWidth,-mapWidth}){const int n=k+d;if(n<0||n>=mapWidth*mapHeight||std::abs(n%mapWidth-k%mapWidth)+std::abs(n/mapWidth-k/mapWidth)!=1||previous[n]>=0||w.terrain[n]!=Terrain::Sea)continue;previous[n]=k;queue.push(n);}}
    if(previous[end]<0)return {};std::vector<int> path;for(int k=end;k!=start;k=previous[k])path.push_back(k);path.push_back(start);std::reverse(path.begin(),path.end());return path;
}
void SeaLife::seed(World& w) {
    if(!w.seaFish.empty())return;
    for(int y=3;y<mapHeight-3;y+=3)for(int x=3;x<mapWidth-3;x+=3) {
        if(w.terrainAt(x,y)!=Terrain::Sea)continue;
        bool coast=false;
        for(int d=1;d<=4;++d)for(auto step:{std::pair<int,int>{d,0},{-d,0},{0,d},{0,-d}})
            if(w.terrainAt(x+step.first,y+step.second)==Terrain::Ground)coast=true;
        const unsigned hash=static_cast<unsigned>(x*73856093u)^static_cast<unsigned>(y*19349663u)^w.islandSeed;
        if(coast&&hash%3==0)w.seaFish.push_back({x+.5f,y+.5f,static_cast<float>(hash%628)*.01f,2,true});
        if(w.seaFish.size()>=160)return;
    }
}
void SeaLife::update(World& w,float dt) {
    for(std::size_t i=0;i<w.seaFish.size();++i) {
        auto& f=w.seaFish[i];if(!f.alive)continue;
        f.timer-=dt;if(f.timer<=0){f.heading+=.6f+static_cast<float>(i%5)*.13f;f.timer=2+static_cast<float>(i%4);}
        const float x=f.x+std::cos(f.heading)*dt*.35f,y=f.y+std::sin(f.heading)*dt*.35f;
        const auto water=w.terrainAt(static_cast<int>(std::floor(x)),static_cast<int>(std::floor(y)));
        if(x>=0&&y>=0&&x<mapWidth&&y<mapHeight&&(water==Terrain::Sea||water==Terrain::FreshWater)){f.x=x;f.y=y;}
        else f.heading+=2.4f;
    }
}
}


