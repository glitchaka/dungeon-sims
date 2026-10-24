#pragma once
#include "entities/Resident.h"
#include <string>

namespace isla {
class World;
// Host execution: paths, reservations, progress and real resource changes.
class ResidentActions {
public:
    explicit ResidentActions(World& world) : world_(world) {}
    bool begin(Resident& resident, Task task, int goal);
    void finish(Resident& resident, const std::string& metric, const std::string& note);
    void act(Resident& resident, float dt);
    bool leaveShelter(Resident& resident);

private:
    World& world_;
};
}

