#pragma once
#include <vector>
namespace isla {
class World;
class SeaLife {public:static void seed(World& world);static void update(World& world,float dt);static std::vector<int> route(const World&,int start,int end);};
}

