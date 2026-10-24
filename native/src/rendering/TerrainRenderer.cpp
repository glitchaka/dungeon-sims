#include "TerrainRenderer.h"
#include "TerrainProjection.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>

namespace isla {
namespace {
using Color = std::array<float,3>;
float smooth(float value) {
    value = std::clamp(value,0.0f,1.0f);
    return value*value*(3-2*value);
}
Color mix(Color a, Color b, float amount) {
    amount = std::clamp(amount,0.0f,1.0f);
    for (int i=0;i<3;++i) a[i] += (b[i]-a[i])*amount;
    return a;
}
float hash(int x,int y,std::uint32_t seed) {
    std::uint32_t value = static_cast<std::uint32_t>(x)*0x85ebca6bu
        ^ static_cast<std::uint32_t>(y)*0xc2b2ae35u ^ seed;
    value ^= value>>16;
    value *= 0x7feb352du;
    value ^= value>>15;
    return (value&65535u)/65535.0f;
}
float noise(float x,float y,std::uint32_t seed) {
    const int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));
    const float fx=smooth(x-ix),fy=smooth(y-iy);
    const float a=hash(ix,iy,seed)*(1-fx)+hash(ix+1,iy,seed)*fx;
    const float b=hash(ix,iy+1,seed)*(1-fx)+hash(ix+1,iy+1,seed)*fx;
    return a*(1-fy)+b*fy;
}
float sample(const std::vector<float>& grid,float x,float y) {
    x=std::clamp(x-.5f,0.0f,static_cast<float>(mapWidth-1));
    y=std::clamp(y-.5f,0.0f,static_cast<float>(mapHeight-1));
    const int ix=static_cast<int>(x),iy=static_cast<int>(y);
    const int nx=std::min(ix+1,mapWidth-1),ny=std::min(iy+1,mapHeight-1);
    const float fx=x-ix,fy=y-iy;
    return (grid[cellId(ix,iy)]*(1-fx)+grid[cellId(nx,iy)]*fx)*(1-fy)
        +(grid[cellId(ix,ny)]*(1-fx)+grid[cellId(nx,ny)]*fx)*fy;
}
std::vector<int> distanceTo(const World& world,bool sea) {
    std::vector<int> distances(mapWidth*mapHeight,999);
    std::queue<int> pending;
    for (int cell=0;cell<mapWidth*mapHeight;++cell)
        if ((world.terrain[cell]==Terrain::Sea)==sea) { distances[cell]=0;pending.push(cell); }
    while (!pending.empty()) {
        const int cell=pending.front();pending.pop();
        for (int offset : {1,-1,mapWidth,-mapWidth}) {
            const int next=cell+offset;
            if (next<0 || next>=mapWidth*mapHeight
                || std::abs(next%mapWidth-cell%mapWidth)+std::abs(next/mapWidth-cell/mapWidth)!=1
                || distances[next]<=distances[cell]+1) continue;
            distances[next]=distances[cell]+1;pending.push(next);
        }
    }
    return distances;
}
}

void TerrainRenderer::setMaterials(std::vector<unsigned char> pixels,int materialWidth,int materialHeight) {
    if(materialWidth<3 || materialHeight<2 || materialWidth%3 || materialHeight%2
        || pixels.size()!=static_cast<std::size_t>(materialWidth)*materialHeight*4)
        throw std::invalid_argument("Invalid terrain material atlas");
    materials_=std::move(pixels);
    materialWidth_=materialWidth;
    materialHeight_=materialHeight;
}

std::vector<unsigned char> TerrainRenderer::render(const World& world) const {
    const auto material=[&](int tile,float x,float y,Color fallback) {
        if(materials_.empty()) return fallback;
        const int tileW=materialWidth_/3,tileH=materialHeight_/2;
        // Four overlapping, independently oriented material patches. Every
        // anchor has a stable transform, so crossing a patch boundary is
        // continuous and there are no mirrored checkerboard repetitions.
        const float period=tile==0?1.5f:tile==1?3.0f:tile==2?4.0f:tile==3?4.0f:tile==4?1.2f:3.5f;
        const float gx=x/period,gy=y/period;
        const int ix=static_cast<int>(std::floor(gx)),iy=static_cast<int>(std::floor(gy));
        const float fx=smooth(gx-ix),fy=smooth(gy-iy);
        Color color{};
        for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx) {
            const int ax=ix+dx,ay=iy+dy;
            float u=(gx-ax)*.24f,v=(gy-ay)*.24f;
            const float transform=hash(ax,ay,world.islandSeed+tile*97);
            if(transform>.5f)std::swap(u,v);
            if(transform>.25f&&transform<.75f)u=-u;
            if(transform>.75f)v=-v;
            u+=.5f+(hash(ax,ay,world.islandSeed+tile*97+7)-.5f)*.24f;
            v+=.5f+(hash(ax,ay,world.islandSeed+tile*97+11)-.5f)*.24f;
            const int sx=tile%3*tileW+std::clamp(static_cast<int>(u*tileW),0,tileW-1);
            const int sy=tile/3*tileH+std::clamp(static_cast<int>(v*tileH),0,tileH-1);
            const float weight=(dx?fx:1-fx)*(dy?fy:1-fy);
            for(int channel=0;channel<3;++channel)
                color[channel]+=materials_[(static_cast<std::size_t>(sy)*materialWidth_+sx)*4+channel]*weight;
        }
        return color;
    };
    const auto fromSea=distanceTo(world,true),fromLand=distanceTo(world,false);
    std::vector<float> coast(mapWidth*mapHeight),heights(world.elevation.begin(),world.elevation.end());
    std::vector<float> illumination(mapWidth*mapHeight),slope(mapWidth*mapHeight),freshWater(mapWidth*mapHeight);
    for (int y=0;y<mapHeight;++y) for (int x=0;x<mapWidth;++x) {
        const int cell=cellId(x,y);
        freshWater[cell]=world.terrain[cell]==Terrain::FreshWater?1.0f:0.0f;
        coast[cell]=world.terrain[cell]==Terrain::Sea?.5f-fromLand[cell]:fromSea[cell]-.5f;
        const float dx=(sample(heights,x+1.5f,y+.5f)-sample(heights,x-.5f,y+.5f))*.5f;
        const float dy=(sample(heights,x+.5f,y+1.5f)-sample(heights,x+.5f,y-.5f))*.5f;
        slope[cell]=std::hypot(dx,dy);
        const float normal=std::sqrt(1+dx*dx*9+dy*dy*9);
        float lighting=.48f+.64f*std::max(0.0f,(1+dx*2.1f+dy*2.5f)/normal);
        // Cast shadows across valleys, using the same heights used by navigation.
        for (int step=1;step<=18;++step) {
            const int sx=x-step,sy=y-step;
            if(sx<0 || sy<0) break;
            if(heights[cellId(sx,sy)]>heights[cell]+step*.7f) {lighting*=.69f;break;}
        }
        illumination[cell]=lighting;
    }
    // Smooth only the coast contour. Elevation remains the world's real field.
    std::vector<float> shore(coast.size());
    constexpr int weights[]{1,4,6,4,1};
    for(int y=0;y<mapHeight;++y) for(int x=0;x<mapWidth;++x) {
        float sum=0;
        for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx)
            sum+=coast[cellId(std::clamp(x+dx,0,mapWidth-1),std::clamp(y+dy,0,mapHeight-1))]
                *weights[dx+2]*weights[dy+2];
        shore[cellId(x,y)]=sum/256;
    }
    // Resolve the river per map row once rather than searching every segment per pixel.
    std::vector<float> riverX(mapHeight,-1),riverHeight(mapHeight,0);
    for(const auto& point:world.riverCourse) {
        const int row=static_cast<int>(point[1]);
        if(row>=0 && row<mapHeight) {riverX[row]=point[0];riverHeight[row]=point[2];}
    }
    std::vector<unsigned char> pixels(width*height*4);
    for(int py=0;py<height;++py) for(int px=0;px<width;++px) {
        const float x=(px+.5f)/scale,y=(py+.5f)/scale;
        const float d=sample(shore,x,y),height=sample(heights,x,y);
        const float detail=noise(x*.35f,y*.35f,world.islandSeed);
        const float broad=noise(x*.07f,y*.07f,world.islandSeed+31);
        const float waves=(detail-.5f)*1.8f;
        Color color=mix({12+waves,52+waves,91+waves},material(5,x,y,{34,151,161}),smooth((d+6)/6));
        if(d>-.35f) {
            const float incline=sample(slope,x,y);
            Color grass=material(0,x,y,mix({61,111,55},{137,157,72},broad*.7f+detail*.3f));
            grass=mix(grass,{164,151,97},smooth((height-2.5f)/4)*.65f);
            const float rock=smooth((height-11)/8)*.7f+smooth((incline-.3f)/.7f)*.8f;
            Color stone=material(1,x,y,mix({97,105,96},{174,171,148},detail*.6f+broad*.4f));
            stone=mix(stone,material(2,x,y,{119,119,108}),smooth((incline-.7f)/1.4f));
            const float strata=std::sin(height*11+detail*2)*3;
            for(auto& channel:stone) channel+=strata;
            Color land=mix(grass,stone,rock);
            const float snow=smooth((height-12.0f+(broad-.5f)*3)/4)*(1-smooth((incline-1.2f)/2)*.75f);
            land=mix(land,material(3,x,y,{232,239,228}),snow);
            const float lighting=sample(illumination,x,y);
            for(auto& channel:land) channel*=lighting;
            const float cliff=smooth((height-1)/2)*smooth((2.2f-d)/1.8f);
            Color coastRock=material(2,x,y,mix({91,100,93},{175,162,129},detail));
            for(auto& channel:coastRock) channel*=lighting;
            const Color beach=mix(material(4,x,y,{225,208,157}),coastRock,cliff);
            land=mix(beach,land,smooth((d-.4f)/2));
            const int row=std::clamp(static_cast<int>(y),0,mapHeight-1);
            float river=0;
            if(riverX[row]>=0) {
                const int next=std::min(row+1,mapHeight-1);
                const float rx=riverX[next]>=0?riverX[row]+(riverX[next]-riverX[row])*(y-row):riverX[row];
                const float distance=std::abs(x-rx);
                river=1-smooth((distance-.34f)/.36f);
                // Dark moist banks and light shallows give the channel a readable profile.
                land=mix(land,{56,91,61},(1-smooth((distance-.6f)/.8f))*.35f);
                Color water=mix({28,109,143},{66,177,185},smooth(distance/.7f));
                if(row>0 && std::abs(riverHeight[row]-riverHeight[row-1])>.5f)
                    water=mix(water,{218,239,229},.8f);
                land=mix(land,water,river);
            }
            const float waterCoverage=smooth((sample(freshWater,x,y)-.15f)/.55f);
            land=mix(land,material(5,x,y,{38,137,161}),waterCoverage);
            color=mix(color,land,smooth((d+.35f)/.65f));
        }
        const float foam=(1-smooth(std::abs(d+.12f)/.17f))*.45f;
        color=mix(color,{213,239,222},foam);
        const std::size_t at=(static_cast<std::size_t>(py)*width+px)*4;
        for(int channel=0;channel<3;++channel)
            pixels[at+channel]=static_cast<unsigned char>(std::clamp(color[channel],0.0f,255.0f));
        pixels[at+3]=255;
    }
    if(!world.procedural) return pixels;
    // Orthographic oblique heightfield scan conversion. Nearer (southern) strips
    // overwrite farther surfaces, exposing actual cliff faces and hiding back slopes.
    std::vector<unsigned char> projected(pixels.size(),255);
    for(std::size_t i=0;i<projected.size();i+=4) {
        projected[i]=12;projected[i+1]=52;projected[i+2]=91;
    }
    for(int px=0;px<width;++px) {
        const float x=(px+.5f)/scale;
        float previousY=-TerrainProjection::elevation(world,x,0)*scale;
        int previousRow=0;
        for(int sourceRow=0;sourceRow<height;++sourceRow) {
            const float groundY=(sourceRow+.5f)/scale;
            const float projectedY=(groundY-TerrainProjection::elevation(world,x,groundY))*scale;
            if(projectedY>previousY) {
                const int first=std::max(0,static_cast<int>(std::ceil(previousY-.5f)));
                const int last=std::min(height-1,static_cast<int>(std::floor(projectedY-.5f)));
                for(int py=first;py<=last;++py) {
                    const float blend=std::clamp((py+.5f-previousY)/(projectedY-previousY),0.0f,1.0f);
                    const std::size_t out=(static_cast<std::size_t>(py)*width+px)*4;
                    const std::size_t a=(static_cast<std::size_t>(previousRow)*width+px)*4;
                    const std::size_t b=(static_cast<std::size_t>(sourceRow)*width+px)*4;
                    for(int channel=0;channel<3;++channel)
                        projected[out+channel]=static_cast<unsigned char>(pixels[a+channel]*(1-blend)+pixels[b+channel]*blend);
                }
            }
            previousY=projectedY;previousRow=sourceRow;
        }
    }
    return projected;
}
}

