#include "BlockIslandGenerator.h"
#include "TerrainBlockData.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace isla {
namespace {
std::uint32_t hash(int x,int y,std::uint32_t seed){auto n=static_cast<std::uint32_t>(x)*73856093u^static_cast<std::uint32_t>(y)*19349663u^seed;n^=n>>16;n*=0x7feb352du;return n^(n>>15);}
}
void BlockIslandGenerator::generate(World& w,std::uint32_t seed){
    constexpr int step=8,cols=mapWidth/step,rows=mapHeight/step;
    // The geographic outline is a blueprint; modules below own the final land,
    // height, slopes and water. The renderer consumes these exact placements.
    const auto blueprint=w.terrain;const auto blueprintHeight=w.elevation;
    std::vector<bool> vertices((cols+1)*(rows+1));
    for(int y=1;y<rows;++y)for(int x=1;x<cols;++x)vertices[y*(cols+1)+x]=blueprint[cellId(x*step,y*step)]!=Terrain::Sea;
    auto mask=[&](int x,int y){const int k=y*(cols+1)+x;return (vertices[k]?1:0)|(vertices[k+1]?2:0)|(vertices[k+cols+2]?4:0)|(vertices[k+cols+1]?8:0);};
    // Shared vertices resolve diagonal contacts consistently on both sides.
    for(int pass=0;pass<cols+rows;++pass){bool changed=false;for(int y=0;y<rows;++y)for(int x=0;x<cols;++x){const int m=mask(x,y);if(m==5||m==10){vertices[y*(cols+1)+x]=vertices[y*(cols+1)+x+1];changed=true;}}if(!changed)break;}
    w.terrain.fill(Terrain::Sea);w.elevation.fill(0);w.terrainModules.clear();
    std::vector<bool> used(cols*rows);
    for(int y=0;y<rows;++y)for(int x=0;x<cols;++x){
        if(used[y*cols+x])continue;const int m=mask(x,y);if(!m)continue;
        const blockdata::Definition* chosen=nullptr;float best=-1;
        const float originalHeight=blueprintHeight[cellId(x*step+step/2,y*step+step/2)];
        const bool settlement=std::hypot(static_cast<float>(x*step-w.market%mapWidth),static_cast<float>(y*step-w.market/mapWidth))<24;
        for(const auto& definition:blockdata::definitions){
            if(m!=15){if(definition.mask==m){chosen=&definition;break;}continue;}
            if(definition.mask>=0||std::strcmp(definition.category,"waterway")==0)continue;
            if((settlement||originalHeight<3)&&definition.relief>0)continue;
            if(originalHeight<12&&std::strcmp(definition.id,"massif-large")==0)continue;
            if(originalHeight>=12&&definition.relief<3)continue;
            bool fits=true;for(int i=0;i<definition.count;++i){const int xx=x+definition.cells[i][0],yy=y+definition.cells[i][1];if(xx>=cols||yy>=rows||used[yy*cols+xx]||mask(xx,yy)!=15){fits=false;break;}}
            const auto variant=hash(x+definition.count,y+static_cast<int>(definition.relief),seed);
            const float score=definition.count*(.6f+(variant&65535u)/65535.0f);
            if(fits&&score>best){chosen=&definition;best=score;}
        }
        if(!chosen)for(const auto& definition:blockdata::definitions)if(std::strcmp(definition.id,"meadow-small")==0){chosen=&definition;break;}
        if(!chosen)throw std::runtime_error("Modular terrain catalog has no interior ground");
        w.terrainModules.push_back({chosen->id,x*step,y*step,step});
        int width=1,depth=1;for(int i=0;i<chosen->count;++i){width=std::max(width,chosen->cells[i][0]+1);depth=std::max(depth,chosen->cells[i][1]+1);}
        for(int i=0;i<chosen->count;++i){const int gx=x+chosen->cells[i][0],gy=y+chosen->cells[i][1];used[gy*cols+gx]=true;
            for(int dy=0;dy<step;++dy)for(int dx=0;dx<step;++dx){const int xx=gx*step+dx,yy=gy*step+dy,k=cellId(xx,yy);const float u=(dx+.5f)/step,v=(dy+.5f)/step;
                if(chosen->mask>=0){const float top=(m&1?1-u:0)+(m&2?u:0),bottom=(m&8?1-u:0)+(m&4?u:0);if(top*(1-v)+bottom*v<.5f)continue;}
                const float localX=(xx-x*step+.5f)/(width*step),localY=(yy-y*step+.5f)/(depth*step);
                const float radial=std::max(0.0f,1-std::hypot((localX-.5f)*2,(localY-.5f)*2));
                float relief=chosen->relief*std::pow(radial,1.2f);
                if(std::strcmp(chosen->id,"plateau-ramp")==0)relief=chosen->relief*std::clamp(std::min(localX,localY)*4,0.0f,1.0f);
                if(std::strcmp(chosen->id,"massif-large")==0)relief*=.78f+.22f*std::abs(std::sin(localX*19+localY*11));
                w.terrain[k]=relief>11?Terrain::Mountain:Terrain::Ground;w.elevation[k]=chosen->height+relief;
            }
        }
    }
    // Water modules carve a connected channel into the physical modules, rather
    // than painting an unwalkable-looking river over traversable grass.
    const auto course=w.riverCourse;w.riverCourse.clear();
    auto riverX=[&](int y){float result=-1,nearest=1e9f;for(const auto& point:course){const float d=std::abs(point[1]-y);if(d<nearest){nearest=d;result=point[0];}}return result;};
    if(!course.empty())for(int y=std::max(step,static_cast<int>(course.front()[1])/step*step);y<std::min(mapHeight-step,static_cast<int>(course.back()[1]));y+=step*2){
        const float x0=riverX(y),x1=riverX(y+step*2);if(x0<0||x1<0)continue;
        const int originX=std::clamp(static_cast<int>(std::min(x0,x1))/step*step-step/2,0,mapWidth-step*2);
        const float topHeight=w.heightAt(x0,y),bottomHeight=w.heightAt(x1,y+step*2);
        const char* block=topHeight-bottomHeight>2?"waterfall-gorge":std::abs(x1-x0)>step*.5f?"river-bend":"river-straight";
        w.terrainModules.push_back({block,originX,y,step});
        for(int dy=0;dy<step*2&&y+dy<mapHeight;++dy){const float f=dy/static_cast<float>(step*2),cx=x0+(x1-x0)*f;const float level=std::max(.1f,(topHeight+(bottomHeight-topHeight)*f)-.25f);
            w.riverCourse.push_back({cx,static_cast<float>(y+dy),level});
            for(int dx=-2;dx<=2;++dx){const int xx=static_cast<int>(cx)+dx;if(xx<0||xx>=mapWidth)continue;const int k=cellId(xx,y+dy);if(w.terrain[k]==Terrain::Sea)continue;
                if(std::abs(xx+.5f-cx)<.85f){w.terrain[k]=Terrain::FreshWater;w.elevation[k]=level;}
                else w.elevation[k]=std::min(w.elevation[k],level+.3f*std::abs(dx));
            }
        }
    }
    // Keep the settlement on the module's actual surface and near fresh water.
    if(w.terrain[w.market]!=Terrain::Ground||w.terrain[w.granary]!=Terrain::Ground)throw std::runtime_error("Generated modules cover the settlement site with water");
    w.landforms.clear();
}
}
