#pragma once
#include "entities/Resident.h"
namespace isla {
class World;
// Physical needs, movement and scheduling of resident actions.
class ResidentSimulation {
public:
    explicit ResidentSimulation(World& world) : world_(world) {}
    void update(float dt);

private:
    World& world_;
    bool updateNeeds(Resident& resident, float dt);
    bool move(Resident& resident, float dt);
};
}
