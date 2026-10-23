#pragma once
#include "Artifact.h"
#include "Task.h"
#include "CncData.h"
#include <cstddef>
#include <string>
#include <vector>

namespace isla {
// Physical and game state. Psychological state belongs to the MSPA runtime.
struct Resident {
    std::string name, trait;
    bool female{};
    bool alive{true};
    double ageDays{6480};
    float naturalDeathAge{90*360.0f};

    float x{}, y{}, speed{};
    int home{};
    int insideHouse{-1};
    bool inDungeon{}, inBoat{}, away{}, visitor{};

    float hunger{75}, thirst{80}, energy{85}, health{cnc::maxHealth};
    float social{65}, fun{65}, hygiene{80};
    float homeWater{4};

    int wood{}, meat{}, grain{}, seeds{12}, money{12}, eggs{}, fish{};
    std::vector<Artifact> artifacts, loot;

    Task task{Task::Idle};
    float decisionTimer{}, actionTimer{};
    std::vector<int> path;
    std::size_t next{};
    int goal{}, target{-1}, workCell{-1}, job{-1}, mission{-1};
    bool resumeRest{}, hasOrder{};
    Task orderedTask{Task::Idle};
    int orderedCell{-1};

    int mother{-1}, father{-1}, pregnancyFather{-1}, familyPartner{-1};
    float pregnancyDue{-1}, postpartumUntil{}, familySeekAfter{};
    float familyCourtship{};
    float familyMomentRemaining{};
    // Legacy host relationship scores, maintained by ResidentRegistry.
    std::vector<int> relationships;

    bool chosen{};
    std::string inspiration;
    float inspirationUntil{};
};
}
