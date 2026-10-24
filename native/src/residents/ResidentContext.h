#pragma once
#include "ResidentDecision.h"

namespace isla {
class World;
// Read-only host facts for the current legacy policy, not an MSPA observation.
ResidentDecisionContext residentDecisionContext(const World& world, const Resident& resident);
}

