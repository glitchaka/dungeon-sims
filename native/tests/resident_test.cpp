#include "ResidentDecision.h"
#include "ResidentRegistry.h"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void registryInvariants() {
    isla::ResidentRegistry registry;
    isla::Resident mother;
    mother.name = "Mara";
    check(registry.add(mother) == 0, "First resident has wrong identity");
    registry[0].relationships[0] = 17;
    isla::Resident child;
    child.name = "Infante";
    child.mother = 0;
    child.ageDays = 0;
    check(registry.add(child) == 1, "Birth changed resident ordering");
    check(registry[1].mother == 0 && registry[1].ageDays == 0, "Registration changed family state");
    check(registry[0].relationships.size() == 2 && registry[1].relationships.size() == 2,
          "Population growth left incomplete relationship rows");
    check(registry[0].relationships[0] == 17 && registry[0].relationships[1] == 0,
          "Population growth destroyed existing relationships");
    registry[0].alive = false;
    for (int i = 0; i < 80; ++i) registry.add(isla::Resident{});
    check(registry[1].mother == 0 && registry.indexOf(registry[1]) == 1,
          "Reallocation or death changed resident identity");
    isla::ResidentRegistry restored;
    restored.restore(registry.values());
    check(restored.size() == registry.size() && restored[0].relationships[0] == 17,
          "Restore lost resident state");
    restored[0].name = "Independent copy";
    check(registry[0].name == "Mara", "Restored world aliases the original population");
    bool rejected = false;
    try { restored.restore(std::vector<isla::Resident>(1001)); }
    catch (const std::length_error&) { rejected = true; }
    check(rejected && restored.size() == registry.size(), "Invalid restore changed population");
    isla::ResidentRegistry full;
    full.restore(std::vector<isla::Resident>(1000));
    rejected = false;
    try { full.add(isla::Resident{}); }
    catch (const std::length_error&) { rejected = true; }
    check(rejected && full.size() == 1000, "Population limit is not enforced on insertion");
}

void decisionPriorities() {
    using isla::ResidentIntent;
    isla::Resident r;
    isla::ResidentDecisionContext context;
    r.thirst = 1;
    r.hunger = 1;
    r.hasOrder = true;
    context.threatened = true;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::Flee,
          "Orders or hunger displaced an immediate threat");
    check(isla::ResidentDecision::next(r, context, ResidentIntent::Flee) == ResidentIntent::CriticalDrink,
          "Unreachable escape did not fall back to urgent drinking");
    r.thirst = 90;
    check(isla::ResidentDecision::next(r, context, ResidentIntent::Flee) == ResidentIntent::CriticalEat,
          "Hunger did not precede assigned work");
    context.adult = false;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::Child,
          "Child entered adult work priorities");
    context.adult = true;
    context.threatened = false;
    context.night = true;
    context.alarmActive = false;
    r.hunger = 90;
    r.insideHouse = 42;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::StaySheltered,
          "Resting resident left shelter without a need");
    r.thirst = 10;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::LeaveShelter,
          "Shelter suppressed a critical need");
    r.insideHouse = -1;
    r.thirst = 90;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::NightRest,
          "Night work displaced sleep");
    context.night = false;
    r.visitor = true;
    r.thirst = 49;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::VisitorDrink,
          "Visitor acquired resident work before satisfying needs");
    r.visitor = false;
    r.thirst = 90;
    check(isla::ResidentDecision::next(r, context) == ResidentIntent::Order,
          "Explicit order lost its priority");
    r.hasOrder = false;
    r.fish = 0;
    r.inspiration = "sustain";
    check(isla::ResidentDecision::next(r, context, ResidentIntent::Gift) == ResidentIntent::Fishing,
          "Sustenance inspiration no longer proposes fishing");
}

void preferenceAndScheduling() {
    isla::Resident r;
    r.hunger = r.thirst = r.energy = r.social = r.fun = r.hygiene = 100;
    isla::ResidentDecisionContext context;
    context.communityGrain = 100;
    r.energy = 0;
    const auto preference = isla::ResidentDecision::evaluate(r, context);
    check(preference.need == isla::ResidentNeed::Rest, "Exhaustion did not select rest");
    check(r.energy == 0 && r.task == isla::Task::Idle && r.path.empty(),
          "Decision executed an action or modified physical state");
    const auto repeated = isla::ResidentDecision::evaluate(r, context);
    check(repeated.scores == preference.scores, "Decision is not reproducible");
    r.energy = 100;
    r.trait = "Agricultora";
    context.priority = 1;
    check(isla::ResidentDecision::evaluate(r, context).preferFarming,
          "Agricultural priority no longer prefers farming");
    context.night = true;
    check(!isla::ResidentDecision::evaluate(r, context).preferFarming,
          "Farming preference ignored night");
    check(isla::ResidentDecision::shouldReconsider(r, context), "Night did not request reevaluation");
    r.task = isla::Task::Rest;
    check(!isla::ResidentDecision::shouldReconsider(r, context), "Sleep repeatedly interrupted itself");
    r.thirst = 0;
    check(isla::ResidentDecision::shouldReconsider(r, context), "Critical thirst did not interrupt sleep");
}
}

int main() {
    try {
        registryInvariants();
        decisionPriorities();
        preferenceAndScheduling();
        std::cout << "PASS: resident ownership, relationship invariants, priorities and pure decisions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

