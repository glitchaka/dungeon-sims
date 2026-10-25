#include "TerrainSurface.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace isla {
float TerrainSurface::slope(const World& w,int x,int y,bool includeWater){
    if(!w.procedural||x<0||y<0||x>=mapWidth||y>=mapHeight)return 0;
    const float center=w.elevation[cellId(x,y)];float result=0;
    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){
        const int xx=x+dx,yy=y+dy;if((dx==0&&dy==0)||xx<0||yy<0||xx>=mapWidth||yy>=mapHeight)continue;
        const auto terrain=w.terrainAt(xx,yy);if(!includeWater&&(terrain==Terrain::Sea||terrain==Terrain::FreshWater))continue;
        result=std::max(result,std::abs(w.elevation[cellId(xx,yy)]-center)/(dx&&dy?1.41421356f:1.0f));
    }
    return result;
}
void TerrainSurface::repairResidents(World& w){
    for(std::size_t i=0;i<w.residents.size();++i){auto& r=w.residents[i];
        if(!r.alive||r.away||r.inBoat||r.inDungeon||r.insideHouse>=0||!std::isfinite(r.x)||!std::isfinite(r.y)||r.x<0||r.y<0||r.x>=mapWidth||r.y>=mapHeight)continue;
        const int x=static_cast<int>(r.x),y=static_cast<int>(r.y);
        if(slope(w,x,y)<=maximumWalkSlope||w.terrainAt(x,y)==Terrain::Bridge)continue;
        int chosen=-1,best=std::numeric_limits<int>::max();
        for(int yy=0;yy<mapHeight;++yy)for(int xx=0;xx<mapWidth;++xx){
            const int distance=std::abs(xx-x)+std::abs(yy-y);if(distance>=best||!w.walkable(xx,yy))continue;
            bool occupied=false;
            for(std::size_t j=0;j<w.residents.size();++j){const auto& other=w.residents[j];if(j!=i&&other.alive&&!other.away&&!other.inBoat&&!other.inDungeon&&other.insideHouse<0&&std::isfinite(other.x)&&std::isfinite(other.y)&&other.x>=0&&other.y>=0&&other.x<mapWidth&&other.y<mapHeight&&static_cast<int>(other.x)==xx&&static_cast<int>(other.y)==yy)occupied=true;}
            for(const auto& animal:w.animals)if(animal.alive&&static_cast<int>(animal.x)==xx&&static_cast<int>(animal.y)==yy)occupied=true;
            if(!occupied){best=distance;chosen=cellId(xx,yy);}
        }
        if(chosen<0)continue;
        r.x=chosen%mapWidth+.5f;r.y=chosen/mapWidth+.5f;r.goal=chosen;
        if(r.home>=0&&r.home<mapWidth*mapHeight&&slope(w,r.home%mapWidth,r.home/mapWidth)>maximumWalkSlope)r.home=chosen;
        r.path.clear();r.next=0;r.task=Task::Idle;r.hasOrder=false;r.job=r.workCell=-1;r.decisionTimer=0;
        if(r.familyPartner>=0&&r.familyPartner<static_cast<int>(w.residents.size())){auto& partner=w.residents[r.familyPartner];partner.familyPartner=-1;partner.familyCourtship=0;}
        r.familyPartner=-1;r.familyCourtship=0;
        for(auto& job:w.jobs)if(job.worker==static_cast<int>(i))job.worker=-1;
        for(auto& crop:w.crops)if(crop.worker==static_cast<int>(i))crop.worker=-1;
        ++w.metrics["cliff_positions_recovered"];
    }
}
}
