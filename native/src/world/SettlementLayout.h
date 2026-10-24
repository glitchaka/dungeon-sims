#pragma once
#include <cstdint>
namespace isla {
class World;
class SettlementLayout {
public:
    static void placeResidents(World& world, std::uint32_t seed);
    static void placeHomes(World& world, std::uint32_t seed);
};
}


