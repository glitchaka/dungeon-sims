#pragma once
#include "entities/Resident.h"
#include "ResidentDecision.h"

namespace isla {
class World;
// Resolves the legacy policy against host resources and starts feasible actions.
// It owns neither resident state nor psychological state.
class ResidentController {
public:
    explicit ResidentController(World& world) : world_(world) {}
    void plan(Resident& resident);

private:
    World& world_;
    bool execute(Resident& resident, ResidentIntent intent);
    bool satisfyNeed(Resident& resident);
    void feed(Resident& resident);
    void rest(Resident& resident);
    bool farmTask(Resident& resident);
    bool gatherTask(Resident& resident);
};
}

