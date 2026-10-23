#pragma once
#include <cstdint>
namespace isla {
class World;
class IslandGeography {
public:
    static void generate(World& world,std::uint32_t seed);
    static void seedVegetation(World& world,std::uint32_t seed);
};
}
