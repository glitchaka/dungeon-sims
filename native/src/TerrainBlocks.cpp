#include "TerrainBlocks.h"
#include "TerrainProjection.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace isla {
void TerrainBlocks::assign(std::vector<TerrainBlockImage> sheets,std::vector<TerrainBlock> blocks) {
    if(sheets.empty()||blocks.empty())throw std::invalid_argument("Empty modular terrain catalog");
    for(const auto& image:sheets)if(image.width<1||image.height<1||image.rgba.size()!=static_cast<std::size_t>(image.width)*image.height*4)
        throw std::invalid_argument("Invalid terrain block image");
    for(const auto& block:blocks) {
        if(block.sheet<0||block.sheet>=static_cast<int>(sheets.size())||block.footprint.empty())throw std::invalid_argument("Invalid terrain block definition");
        const auto& image=sheets[block.sheet];const auto& r=block.source;
        if(r[0]<0||r[1]<0||r[2]<=0||r[3]<=0||r[0]+r[2]>image.width||r[1]+r[3]>image.height)throw std::invalid_argument("Terrain block rectangle outside atlas");
        for(const auto& cell:block.footprint)if(cell[0]<0||cell[1]<0||cell[0]>16||cell[1]>16)throw std::invalid_argument("Invalid block footprint");
    }
    sheets_=std::move(sheets);blocks_=std::move(blocks);
}

std::vector<TerrainBlockPlacement> TerrainBlocks::layout(const World& world) const {
    if(empty())return {};
    if(!world.terrainModules.empty()){
        std::vector<TerrainBlockPlacement> placed;
        for(const auto& module:world.terrainModules){const auto definition=std::find_if(blocks_.begin(),blocks_.end(),[&](const TerrainBlock& b){return b.id==module.block;});if(definition==blocks_.end())throw std::runtime_error("Saved island refers to an unknown terrain block");placed.push_back({&*definition,module.x,module.y,module.step});}
        std::stable_sort(placed.begin(),placed.end(),[](const TerrainBlockPlacement& a,const TerrainBlockPlacement& b){return a.y<b.y;});return placed;
    }
    constexpr int stride=12;
    const int columns=(mapWidth+stride-1)/stride,rows=(mapHeight+stride-1)/stride;
    std::vector<bool> used(columns*rows);
    const auto land=[&](int x,int y){return x>=0&&y>=0&&x<mapWidth&&y<mapHeight&&world.terrainAt(x,y)!=Terrain::Sea;};
    const auto mask=[&](int x,int y,int step){return (land(x,y)?1:0)|(land(x+step,y)?2:0)|(land(x+step,y+step)?4:0)|(land(x,y+step)?8:0);};
    std::vector<TerrainBlockPlacement> placements;
    const auto coast=[&](int x,int y,int step,int m){
        for(const auto& block:blocks_)if(block.category=="coast"&&block.cornerMask==m){placements.push_back({&block,x,y,step});return;}
    };
    for(int y=0;y<rows;++y)for(int x=0;x<columns;++x) {
        if(used[y*columns+x])continue;
        const int wx=x*stride,wy=y*stride,m=mask(wx,wy,stride);
        if(m==0&&!land(wx+stride/2,wy+stride/2))continue;
        if(m!=15) {
            // Diagonal contacts are two smaller pieces, never a wrong facing.
            if(m==5||m==10||m==0)for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){const int sx=wx+dx*stride/2,sy=wy+dy*stride/2;coast(sx,sy,stride/2,mask(sx,sy,stride/2));}
            else coast(wx,wy,stride,m);
            used[y*columns+x]=true;continue;
        }
        const float elevation=world.heightAt(wx+stride*.5f,wy+stride*.5f);
        bool river=false;for(int yy=wy;yy<std::min(mapHeight,wy+stride);++yy)for(int xx=wx;xx<std::min(mapWidth,wx+stride);++xx)
            if(world.terrainAt(xx,yy)==Terrain::FreshWater)river=true;
        const TerrainBlock* chosen=nullptr;float best=0;
        std::uint32_t variant=world.islandSeed^(static_cast<std::uint32_t>(wx)*73856093u)^(static_cast<std::uint32_t>(wy)*19349663u);
        for(const auto& block:blocks_) {
            if(block.category=="coast")continue;
            if(block.category=="waterway"&&!river)continue;
            if(block.category=="relief"&&(river||elevation<4))continue;
            if(block.category=="interior"&&(river||elevation>=7))continue;
            if(block.id=="massif-large"&&elevation<12)continue;
            if(block.id=="plateau-ramp"&&elevation<3)continue;
            bool fits=true;
            for(const auto& cell:block.footprint){const int tx=x+cell[0],ty=y+cell[1];if(tx>=columns||ty>=rows||used[ty*columns+tx]||mask(tx*stride,ty*stride,stride)!=15){fits=false;break;}}
            variant^=variant>>13;variant=variant*1664525u+1013904223u;
            const float score=static_cast<float>(block.footprint.size())*(.6f+(variant&65535u)/65535.0f);
            if(fits&&score>best){chosen=&block;best=score;}
        }
        if(!chosen)for(const auto& block:blocks_)if(block.id=="meadow-small"){chosen=&block;break;}
        if(!chosen)continue;
        placements.push_back({chosen,wx,wy,stride});
        for(const auto& cell:chosen->footprint)used[(y+cell[1])*columns+x+cell[0]]=true;
    }
    std::stable_sort(placements.begin(),placements.end(),[](const TerrainBlockPlacement& a,const TerrainBlockPlacement& b){return a.y<b.y;});
    return placements;
}

std::array<float,4> TerrainBlocks::bounds(const World& world,const TerrainBlockPlacement& placement,float scale) {
    const auto& block=*placement.block;
    int columnsWide=1,rowsDeep=1;for(const auto& cell:block.footprint){columnsWide=std::max(columnsWide,cell[0]+1);rowsDeep=std::max(rowsDeep,cell[1]+1);}
    const float groundWidth=static_cast<float>(columnsWide*placement.step),groundDepth=static_cast<float>(rowsDeep*placement.step);
    const float centerX=placement.x+groundWidth*.5f,centerY=placement.y+groundDepth*.5f;
    const float width=groundWidth*scale*1.08f,height=width*block.source[3]/block.source[2];
    const float bottom=(placement.y+groundDepth-TerrainProjection::elevation(world,centerX,centerY))*scale;
    return {centerX*scale-width*.5f,bottom-height,width,height};
}

void TerrainBlocks::compose(const World& world,std::vector<unsigned char>& pixels,int scale) const {
    const int width=mapWidth*scale,height=mapHeight*scale;
    if(pixels.size()!=static_cast<std::size_t>(width)*height*4)throw std::invalid_argument("Invalid terrain composition canvas");
    const auto placements=layout(world);
    for(const auto& placement:placements) {
        const auto& block=*placement.block;const auto& image=sheets_[block.sheet];
        const auto rect=bounds(world,placement,static_cast<float>(scale));
        const float left=rect[0],top=rect[1],drawWidth=rect[2],drawHeight=rect[3],bottom=top+drawHeight;
        for(int py=std::max(0,static_cast<int>(std::floor(top)));py<std::min(height,static_cast<int>(std::ceil(bottom)));++py)
            for(int px=std::max(0,static_cast<int>(std::floor(left)));px<std::min(width,static_cast<int>(std::ceil(left+drawWidth)));++px) {
                const int sx=block.source[0]+std::clamp(static_cast<int>((px+.5f-left)/drawWidth*block.source[2]),0,block.source[2]-1);
                const int sy=block.source[1]+std::clamp(static_cast<int>((py+.5f-top)/drawHeight*block.source[3]),0,block.source[3]-1);
                const std::size_t source=(static_cast<std::size_t>(sy)*image.width+sx)*4,destination=(static_cast<std::size_t>(py)*width+px)*4;
                const float alpha=image.rgba[source+3]/255.0f;
                for(int channel=0;channel<3;++channel)pixels[destination+channel]=static_cast<unsigned char>(pixels[destination+channel]*(1-alpha)+image.rgba[source+channel]*alpha);
            }
    }
}
}
