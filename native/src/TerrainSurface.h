#pragma once
namespace isla {
class World;
class TerrainSurface {
public:
    static constexpr float maximumWalkSlope=.8f;
    static float slope(const World&,int x,int y,bool includeWater=false);
    static void repairResidents(World&);
};
}
