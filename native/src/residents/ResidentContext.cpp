#include "ResidentContext.h"
#include "World.h"
#include <cmath>

namespace isla {
ResidentDecisionContext residentDecisionContext(const World& world, const Resident& resident) {
    ResidentDecisionContext context;
    context.night = world.night();
    context.hour = static_cast<float>(std::fmod(world.elapsed / World::daySeconds * 24, 24.0f));
    context.priority = world.priority;
    context.communityGrain = world.grain;
    context.reserve = world.reserve;
    context.adult = resident.ageDays >= World::adultAgeDays;
    context.alarmActive = world.alarmSeconds > 0;
    context.threatened = context.alarmActive
        && std::hypot(resident.x-world.alarmX, resident.y-world.alarmY) < cnc::fleeRadius;
    const int x = static_cast<int>(resident.x), y = static_cast<int>(resident.y);
    context.nearHome = std::hypot(static_cast<float>(x-resident.home%mapWidth),
                                  static_cast<float>(y-resident.home/mapWidth)) < 6;
    return context;
}
}

