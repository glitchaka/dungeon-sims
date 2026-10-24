#include "IslandGeography.h"
#include "World.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace isla {
namespace {
struct Point {float x,y;};
struct Peak {float x,y,height;};
float smooth(float value) {
    value=std::clamp(value,0.0f,1.0f);
    return value*value*(3-2*value);
}
float hash(int x,int y,std::uint32_t seed) {
    std::uint32_t value=static_cast<std::uint32_t>(x)*0x85ebca6bu
        ^static_cast<std::uint32_t>(y)*0xc2b2ae35u^seed;
    value^=value>>16;value*=0x7feb352du;value^=value>>15;
    return (value&65535u)/65535.0f;
}
float noise(float x,float y,std::uint32_t seed) {
    const int ix=static_cast<int>(std::floor(x)),iy=static_cast<int>(std::floor(y));
    const float fx=smooth(x-ix),fy=smooth(y-iy);
    return (hash(ix,iy,seed)*(1-fx)+hash(ix+1,iy,seed)*fx)*(1-fy)
        +(hash(ix,iy+1,seed)*(1-fx)+hash(ix+1,iy+1,seed)*fx)*fy;
}
float segmentDistance(Point p,Point a,Point b,float& along) {
    const float dx=b.x-a.x,dy=b.y-a.y;
    along=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/std::max(.00001f,dx*dx+dy*dy),0.0f,1.0f);
    return std::hypot(p.x-a.x-dx*along,p.y-a.y-dy*along);
}
template<std::size_t N>
float signedDistance(Point point,const std::array<Point,N>& outline) {
    bool inside=false;
    float distance=std::numeric_limits<float>::max();
    for(std::size_t i=0,j=N-1;i<N;j=i++) {
        const auto a=outline[i],b=outline[j];
        if((a.y>point.y)!=(b.y>point.y)
            &&point.x<(b.x-a.x)*(point.y-a.y)/(b.y-a.y)+a.x) inside=!inside;
        float along;
        distance=std::min(distance,segmentDistance(point,a,b,along));
    }
    return inside?distance:-distance;
}
float riverLevel(float y) {
    // Headwater lake -> mountain falls -> gorge -> lowland floodplain -> sea.
    if(y<.32f) return 8.5f;
    if(y<.36f) return 5.5f-(y-.32f)*10;
    if(y<.44f) return 3.7f-(y-.36f)*12;
    if(y<.56f) return 2.4f-(y-.44f)*9;
    return std::max(.05f,1.3f-(y-.56f)*5);
}
}

void IslandGeography::seedVegetation(World& world,std::uint32_t seed) {
    for(int y=3;y<mapHeight-3;++y) for(int x=3;x<mapWidth-3;++x) {
        const int cell=cellId(x,y);
        if(world.terrain[cell]!=Terrain::Ground || world.elevation[cell]>10) continue;
        if(std::hypot(static_cast<float>(x-world.market%mapWidth),static_cast<float>(y-world.market/mapWidth))<12) continue;
        const float incline=std::abs(world.elevation[cell+1]-world.elevation[cell-1])
            +std::abs(world.elevation[cell+mapWidth]-world.elevation[cell-mapWidth]);
        if(incline>1.3f) continue;
        float moisture=noise(x*.028f,y*.028f,seed+41)*.65f+noise(x*.09f,y*.09f,seed+43)*.35f;
        if(x<mapWidth*.43f&&y<mapHeight*.6f) moisture+=.14f;
        if(moisture<.47f || hash(x,y,seed+47)>.29f+moisture*.10f) continue;
        const float jitterX=(hash(x,y,seed+51)-.5f)*.5f;
        const float jitterY=(hash(x,y,seed+53)-.5f)*.5f;
        world.forest[cell]=true;
        world.props.push_back({x+.5f+jitterX,y+.5f+jitterY,.28f,.28f,3,true,"Arbol",world.nextProp++,-1,60});
    }
    for(int i=0;i<5;++i) {
        const int x=188+i*2,y=56+i*2;
        if(world.terrain[cellId(x,y)]==Terrain::Sea)continue;
        world.props.push_back({x+.5f,y+.5f,.35f,.35f,4,false,"Cristales",world.nextProp++,-1,100,"crystal"});
    }
}

void IslandGeography::generate(World& world,std::uint32_t seed) {
    // Authored geographic organization from the user's reference; seed changes
    // erosion, shore inlets, ridge detail and islet shapes, not the art direction.
    constexpr std::array<Point,34> outline{{
        {.16f,.075f},{.30f,.045f},{.35f,.11f},{.33f,.18f},{.42f,.19f},
        {.49f,.085f},{.55f,.045f},{.69f,.06f},{.77f,.14f},{.83f,.20f},
        {.88f,.29f},{.92f,.38f},{.90f,.48f},{.86f,.57f},{.91f,.70f},
        {.87f,.83f},{.79f,.86f},{.72f,.80f},{.64f,.89f},{.55f,.85f},
        {.50f,.79f},{.45f,.91f},{.36f,.91f},{.29f,.82f},{.21f,.84f},
        {.16f,.76f},{.145f,.64f},{.19f,.56f},{.20f,.51f},{.13f,.46f},
        {.14f,.34f},{.09f,.25f},{.12f,.18f},{.14f,.13f}
    }};
    constexpr std::array<Peak,6> spine{{
        {.49f,.17f,14},{.56f,.17f,25},{.62f,.23f,28},
        {.67f,.30f,23},{.75f,.32f,16},{.82f,.28f,9}
    }};
    constexpr std::array<Point,11> river{{
        {.58f,.275f},{.55f,.32f},{.49f,.38f},{.465f,.44f},{.43f,.49f},
        {.40f,.53f},{.37f,.59f},{.32f,.65f},{.30f,.71f},{.28f,.79f},{.27f,.87f}
    }};
    constexpr std::array<Point,20> tableland{{
        {.54f,.37f},{.59f,.38f},{.62f,.43f},{.69f,.425f},{.72f,.46f},
        {.735f,.50f},{.78f,.52f},{.745f,.56f},{.77f,.60f},{.72f,.635f},
        {.67f,.61f},{.64f,.65f},{.59f,.64f},{.60f,.585f},{.55f,.57f},
        {.525f,.535f},{.55f,.49f},{.515f,.455f},{.54f,.42f},{.51f,.39f}
    }};
    constexpr std::array<Peak,9> islets{{
        {.065f,.09f,3.2f},{.075f,.21f,2.2f},{.045f,.43f,2},
        {.105f,.54f,2.4f},{.19f,.89f,2},{.34f,.96f,2.3f},
        {.83f,.91f,2.1f},{.92f,.81f,1.8f},{.945f,.65f,2.1f}
    }};
    std::vector<float> shore(mapWidth*mapHeight),riverCenters(mapHeight,-1);
    world.market=cellId(112,111);
    world.granary=cellId(113,111);
    world.landforms={
        {"mountain",153,44,37,28,28,60,0},
        {"hill",57,43,28,24,6,48,1},
        {"plateau",164,101,35,24,7,0,-1},
        {"plateau",147,140,21,14,4,0,-1},
        {"ramp",132,112,10,10,5,20,2}
    };
    world.riverCourse.clear();
    for(int y=0;y<mapHeight;++y) {
        const float v=(y+.5f)/mapHeight;
        for(std::size_t i=1;i<river.size();++i) {
            if(v<river[i-1].y || v>river[i].y) continue;
            const float t=smooth((v-river[i-1].y)/(river[i].y-river[i-1].y));
            riverCenters[y]=(river[i-1].x+(river[i].x-river[i-1].x)*t)*mapWidth
                +(noise(y*.13f,7,seed)-.5f)*2;
            break;
        }
    }
    for(int y=0;y<mapHeight;++y) for(int x=0;x<mapWidth;++x) {
        const int cell=cellId(x,y);
        const float u=(x+.5f)/mapWidth,v=(y+.5f)/mapHeight;
        const float coast=signedDistance(Point{u,v},outline)*mapHeight
            +(noise(x*.16f,y*.16f,seed)-.5f)*3+(noise(x*.47f,y*.47f,seed+1)-.5f)*.65f;
        float coastDistance=coast;
        const float lagoon=std::hypot((u-.815f)/.105f,(v-.755f)/.09f);
        coastDistance=std::min(coastDistance,(lagoon-1)*mapHeight*.09f);
        const float harbor=std::hypot((u-.275f)/.045f,(v-.795f)/.05f);
        coastDistance=std::min(coastDistance,(harbor-1)*mapHeight*.05f);
        for(const auto& island:islets) {
            const float r=std::hypot((u-island.x)*mapWidth,(v-island.y)*mapHeight);
            coastDistance=std::max(coastDistance,island.height-r+(noise(x*.4f,y*.4f,seed)-.5f));
        }
        shore[cell]=coastDistance;
        if(coastDistance<0) {world.terrain[cell]=Terrain::Sea;world.elevation[cell]=0;continue;}

        const float broad=noise(x*.035f,y*.035f,seed);
        float altitude=.65f+broad*1.8f+noise(x*.10f,y*.10f,seed+2)*.65f;
        // A connected, broken alpine spine: valleys separate individual peaks.
        for(std::size_t i=1;i<spine.size();++i) {
            float along;
            const float distance=segmentDistance({x+.5f,y+.5f},
                {spine[i-1].x*mapWidth,spine[i-1].y*mapHeight},
                {spine[i].x*mapWidth,spine[i].y*mapHeight},along);
            const float radius=18+(1-along)*5;
            const float crest=spine[i-1].height+(spine[i].height-spine[i-1].height)*along;
            const float folds=.77f+.23f*(1-std::abs(noise(x*.23f,y*.23f,seed+5)*2-1));
            const float ridge=std::pow(std::max(0.0f,1-distance/radius),1.35f);
            altitude=std::max(altitude,1.4f+crest*ridge*folds);
        }
        // Weathered western uplands and two broad tablelands with actual cliff edges.
        const float northwest=std::hypot((u-.23f)/.20f,(v-.24f)/.22f);
        altitude+=std::max(0.0f,1-northwest)*4*(.7f+broad*.3f);
        const float plateau=signedDistance(Point{u,v},tableland)*mapHeight
            +(noise(x*.13f,y*.13f,seed+11)-.5f)*1.4f;
        const float terrace=smooth(plateau/.65f);
        altitude+=terrace*(5.8f+noise(x*.06f,y*.06f,seed+13)*.8f);
        const float southern=std::hypot((u-.56f)/.105f,(v-.73f)/.085f)
            +(noise(x*.08f,y*.08f,seed+19)-.5f)*.35f
            +.09f*std::sin(u*180+v*92);
        altitude+=smooth((1-southern)/.06f)*3.6f;
        // Wet western basin and low river delta; still real navigable terrain.
        const float basin=std::hypot((u-.31f)/.085f,(v-.43f)/.15f);
        if(basin<1) altitude=altitude*(smooth(basin)*.8f+.2f);
        if(coastDistance<2.5f) {
            const bool beach=noise(x*.035f,y*.035f,seed+17)>.57f;
            const float edge=beach?.12f:2.8f+broad*2.5f;
            altitude=edge+(altitude-edge)*smooth(coastDistance/2.5f);
        }
        // A settlement occupies the low valley, not a row painted into the map.
        const float settlement=std::hypot(x-112.0f,y-111.0f);
        altitude=1.05f+(altitude-1.05f)*smooth((settlement-7)/9);
        world.elevation[cell]=std::clamp(altitude,0.0f,31.5f);
        world.terrain[cell]=altitude>12?Terrain::Mountain:Terrain::Ground;
        if(basin<.8f&&altitude<1&&noise(x*.19f,y*.19f,seed+23)>.62f) {
            world.terrain[cell]=Terrain::FreshWater;world.elevation[cell]=.35f;
        }
    }
    // Carve the valley and channel after the massif, so the river is a drainage
    // route through terrain rather than a blue line drawn over unrelated heights.
    bool entered=false;
    for(int y=0;y<mapHeight;++y) {
        if(riverCenters[y]<0) continue;
        const float rx=riverCenters[y],v=(y+.5f)/mapHeight,level=riverLevel(v);
        const int center=cellId(std::clamp(static_cast<int>(rx),0,mapWidth-1),y);
        if(world.terrain[center]==Terrain::Sea) {if(entered) break;continue;}
        entered=true;
        world.riverCourse.push_back({rx,y+.5f,level});
        for(int x=std::max(0,static_cast<int>(rx)-7);x<std::min(mapWidth,static_cast<int>(rx)+8);++x) {
            const int cell=cellId(x,y);
            if(world.terrain[cell]==Terrain::Sea) continue;
            const float distance=std::abs(x+.5f-rx);
            const float bank=level+std::max(0.0f,distance-1)*.18f;
            world.elevation[cell]=bank+(world.elevation[cell]-bank)*smooth((distance-1)/6);
            if(distance<.85f) {world.terrain[cell]=Terrain::FreshWater;world.elevation[cell]=level;}
            else if(distance<4) world.terrain[cell]=Terrain::Ground;
        }
    }
    // A headwater lake inside the massif, with a spillway to the river.
    for(int y=47;y<58;++y) for(int x=143;x<154;++x) {
        const float radius=std::hypot((x-148.0f)/4.1f,(y-52.0f)/3.3f);
        if(radius<1) {world.terrain[cellId(x,y)]=Terrain::FreshWater;world.elevation[cellId(x,y)]=8.5f;}
    }
    // A second drainage cuts the eastern tableland and falls into the lagoon.
    for(int y=89;y<153;++y) {
        const float v=(y+.5f)/mapHeight;
        const float rx=(.645f+(v-.46f)*.33f)*mapWidth+std::sin(y*.16f)*1.6f;
        const float level=v<.54f?6.8f:v<.61f?3.2f:.2f;
        for(int x=static_cast<int>(rx)-3;x<=static_cast<int>(rx)+3;++x) {
            const int cell=cellId(x,y);
            if(shore[cell]<0) continue;
            const float distance=std::abs(x+.5f-rx);
            const float bank=level+std::max(0.0f,distance-.75f)*.2f;
            world.elevation[cell]=std::min(world.elevation[cell],bank+(world.elevation[cell]-bank)*smooth(distance/3));
            if(distance<.8f) {world.terrain[cell]=Terrain::FreshWater;world.elevation[cell]=level;}
        }
    }
}
}


