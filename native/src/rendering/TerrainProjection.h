#pragma once
#include "World.h"
#include <utility>

namespace isla {
class TerrainProjection {
public:
    static constexpr float heightScale=.65f;
    static float elevation(const World& world,float x,float y);
    static std::pair<float,float> groundPoint(const World& world,float x,float projectedY);
    static bool visible(const World& world,float x,float y);
};
}


