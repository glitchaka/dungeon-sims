#include "ResidentDecision.h"
#include <algorithm>

namespace isla {
ResidentIntent ResidentDecision::next(const Resident& r,
                                     const ResidentDecisionContext& c,
                                     ResidentIntent previous) {
    const auto eligible = [previous](ResidentIntent intent) { return intent > previous; };
    if (r.insideHouse >= 0) {
        if (c.night && r.hunger >= 18 && r.thirst >= 20 && !c.alarmActive
            && eligible(ResidentIntent::StaySheltered)) return ResidentIntent::StaySheltered;
        if (eligible(ResidentIntent::LeaveShelter)) return ResidentIntent::LeaveShelter;
    }
    if (!c.adult && eligible(ResidentIntent::Child)) return ResidentIntent::Child;
    if (c.threatened && eligible(ResidentIntent::Flee)) return ResidentIntent::Flee;
    if (r.thirst < 20 && eligible(ResidentIntent::CriticalDrink)) return ResidentIntent::CriticalDrink;
    if (r.hunger < 18 && eligible(ResidentIntent::CriticalEat)) return ResidentIntent::CriticalEat;
    if (c.night && eligible(ResidentIntent::NightRest)) return ResidentIntent::NightRest;
    if (r.visitor) {
        if (r.thirst < 50 && eligible(ResidentIntent::VisitorDrink)) return ResidentIntent::VisitorDrink;
        if (r.hunger < 45 && eligible(ResidentIntent::VisitorEat)) return ResidentIntent::VisitorEat;
        if (r.energy < 30 && eligible(ResidentIntent::VisitorRest)) return ResidentIntent::VisitorRest;
        if (eligible(ResidentIntent::VisitorWander)) return ResidentIntent::VisitorWander;
    }
    if (r.hasOrder && eligible(ResidentIntent::Order)) return ResidentIntent::Order;
    if (r.mission >= 0 && r.energy > 15 && r.hunger > 22 && r.thirst > 25
        && eligible(ResidentIntent::Expedition)) return ResidentIntent::Expedition;
    if (!r.loot.empty() && r.energy > 25 && r.hunger > 28 && r.thirst > 30
        && eligible(ResidentIntent::SellLoot)) return ResidentIntent::SellLoot;
    if (!r.visitor && eligible(ResidentIntent::Family)) return ResidentIntent::Family;
    if (!r.visitor && r.hunger > 45 && r.thirst > 45 && r.energy > 45) {
        if (eligible(ResidentIntent::Gift)) return ResidentIntent::Gift;
        if ((r.inspiration == "sustain" || c.communityGrain < c.reserve) && r.fish < 3
            && eligible(ResidentIntent::Fishing)) return ResidentIntent::Fishing;
    }
    if (!r.visitor && r.energy > 35 && r.hunger > 40 && r.thirst > 40
        && (!c.night || r.inspiration == "build")
        && eligible(ResidentIntent::Project)) return ResidentIntent::Project;
    if (r.thirst < 65 && r.homeWater > 1 && c.nearHome
        && eligible(ResidentIntent::DrinkHome)) return ResidentIntent::DrinkHome;
    if (r.resumeRest && eligible(ResidentIntent::ResumeRest)) return ResidentIntent::ResumeRest;
    if (r.grain >= 6 && r.hunger > 30 && r.thirst > 30 && r.energy > 30
        && eligible(ResidentIntent::Deposit)) return ResidentIntent::Deposit;
    if (eligible(ResidentIntent::Needs)) return ResidentIntent::Needs;
    if (eligible(ResidentIntent::Explore)) return ResidentIntent::Explore;
    return ResidentIntent::Idle;
}

bool ResidentDecision::shouldReconsider(const Resident& r, const ResidentDecisionContext& c) {
    const bool urgent = (r.thirst < 16 && r.task != Task::Drink && r.task != Task::DrinkHome)
        || (r.hunger < 14 && r.task != Task::Eat && r.task != Task::Hunt
            && r.task != Task::Fetch && r.task != Task::Drink
            && r.task != Task::Harvest && r.task != Task::Tend);
    const bool threat = c.threatened && r.task != Task::Flee;
    const bool bedtime = c.night && r.task != Task::Rest && r.task != Task::Drink
        && r.task != Task::DrinkHome && r.task != Task::Eat
        && r.task != Task::Fetch && r.task != Task::Hunt;
    return urgent || threat || bedtime;
}

ResidentPreference ResidentDecision::evaluate(const Resident& r,
                                             const ResidentDecisionContext& c) {
    ResidentPreference result;
    result.scores = {{
        (100-r.thirst)*1.28f+(r.thirst<30?60:0),
        (100-r.hunger)*1.1f+(r.hunger<27?55:0)+(r.trait=="Cazadora"?6:0),
        (100-r.energy)*1.05f+(c.night?28:0)+(r.energy<25?60:0),
        (100-r.social)*.8f+(r.trait=="Sociable"?8:0),
        (100-r.fun)*.86f+(r.trait.find("Explorador")==0?8:0),
        (100-r.hygiene)*.82f,
        (c.hour>8&&c.hour<17?38.0f:0.0f)+(c.priority==2?18:0)
            +(r.trait=="Trabajador"&&r.money<80?9:0)
    }};
    const auto best = std::max_element(result.scores.begin(), result.scores.end());
    result.need = static_cast<ResidentNeed>(best-result.scores.begin());
    result.farmScore = (r.trait=="Agricultora"||r.trait=="Trabajador"?54.0f:31.0f)
        +(c.priority==1?20:0)+(c.communityGrain<12?13:0);
    result.preferFarming = !r.visitor && !c.night
        && result.farmScore+(r.inspiration=="sustain"?35:0)>*best;
    return result;
}
}


