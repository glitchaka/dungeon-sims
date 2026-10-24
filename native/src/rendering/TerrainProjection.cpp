#include "TerrainProjection.h"
#include <algorithm>
#include <cmath>

namespace isla {
float TerrainProjection::elevation(const World& world,float x,float y) {
    return world.heightAt(x,y)*heightScale;
}

std::pair<float,float> TerrainProjection::groundPoint(const World& world,float x,float projectedY) {
    if(x<0 || x>=mapWidth || !world.procedural) return {x,projectedY};
    float groundY=projectedY;
    // Imported elevations are bounded to 32. Each cell is bilinear in x/y,
    // so at fixed x the intersection is an exact linear solve per row segment.
    const int first=std::max(0,static_cast<int>(std::floor(projectedY-.5f)));
    const int last=std::min(mapHeight,static_cast<int>(std::ceil(projectedY+32*heightScale+1)));
    for(int row=first;row<=last;++row) {
        const float y0=std::max(0.0f,row-.5f),y1=std::min(static_cast<float>(mapHeight),row+.5f);
        if(y1<=y0) continue;
        const float p0=y0-elevation(world,x,y0),p1=y1-elevation(world,x,y1);
        if(p1<=p0 || projectedY<p0 || projectedY>p1) continue;
        groundY=y0+(y1-y0)*(projectedY-p0)/(p1-p0);
    }
    return {x,groundY};
}

bool TerrainProjection::visible(const World& world,float x,float y) {
    const auto point=groundPoint(world,x,y-elevation(world,x,y));
    return std::abs(point.second-y)<.2f;
}
}

