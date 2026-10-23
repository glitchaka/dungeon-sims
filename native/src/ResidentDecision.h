#pragma once
#include "entities/Resident.h"
#include <array>

namespace isla {
enum class ResidentNeed { Drink, Eat, Rest, Social, Explore, Bathe, Work };

// Ordered host intentions. Resolution may fail (for example, no route), in
// which case the controller asks for the next intention using fresh facts.
enum class ResidentIntent {
    None, StaySheltered, LeaveShelter, Child, Flee, CriticalDrink, CriticalEat,
    NightRest, VisitorDrink, VisitorEat, VisitorRest, VisitorWander,
    Order, Expedition, SellLoot, Family, Gift, Fishing,
    Project, DrinkHome, ResumeRest, Deposit, Needs, Explore, Idle
};

// Host facts only. The policy cannot navigate, assign work or spend resources.
struct ResidentDecisionContext {
    bool night{};
    float hour{};
    int priority{};
    int communityGrain{};
    int reserve{};
    bool adult{true};
    bool alarmActive{};
    bool threatened{};
    bool nearHome{};
};

struct ResidentPreference {
    ResidentNeed need{ResidentNeed::Drink};
    std::array<float, 7> scores{};
    float farmScore{};
    bool preferFarming{};
};

class ResidentDecision {
public:
    static ResidentIntent next(const Resident& resident,
                               const ResidentDecisionContext& context,
                               ResidentIntent previous = ResidentIntent::None);
    static bool shouldReconsider(const Resident& resident,
                                 const ResidentDecisionContext& context);
    static ResidentPreference evaluate(const Resident& resident,
                                       const ResidentDecisionContext& context);
};
}
