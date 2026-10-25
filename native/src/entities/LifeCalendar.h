#pragma once
namespace isla {
// Biological time is accelerated independently of the 300-second daylight cycle.
// Ages remain in biological days in entities, saves and network messages.
struct LifeCalendar {
    static constexpr int version=2;
    static constexpr float daySeconds=1,monthDays=30,yearDays=12*monthDays;
    static constexpr float gestationDays=9*monthDays,gestationSeconds=gestationDays*daySeconds;
    static constexpr float recoverySeconds=monthDays*daySeconds;
    static constexpr float childhoodDays=6*yearDays,adultAgeDays=18*yearDays,elderAgeDays=65*yearDays;
    static constexpr float defaultDeathAgeDays=90*yearDays;
    static constexpr bool canCarryPregnancy(double ageDays,double deathAgeDays){return deathAgeDays-ageDays>=gestationDays;}
};
}
