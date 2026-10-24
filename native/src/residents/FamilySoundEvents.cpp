#include "FamilySoundEvents.h"
#include "World.h"

namespace isla {
FamilySoundEvents::Pair FamilySoundEvents::pair(const World& w,std::size_t index) {
    const auto& r=w.residents[index];const int man=r.pregnancyFather;
    if(!r.alive||!r.female||r.ageDays<World::adultAgeDays||r.away||r.inBoat||r.inDungeon||r.pregnancyDue<0
        ||r.familyMomentRemaining<=0||man<0||man>=static_cast<int>(w.residents.size()))return {};
    const auto& partner=w.residents[man];
    if(!partner.alive||partner.female||partner.ageDays<World::adultAgeDays||partner.away||partner.inBoat||partner.inDungeon)return {};
    return {true,man};
}
void FamilySoundEvents::prime(const World& w) {
    previous_.resize(w.residents.size());
    for(std::size_t i=0;i<previous_.size();++i)previous_[i]=pair(w,i);
    primed_=true;
}
std::vector<FamilySoundEvent> FamilySoundEvents::observe(const World& w) {
    if(!primed_){prime(w);return {};}
    std::vector<FamilySoundEvent> events;previous_.resize(w.residents.size());
    for(std::size_t i=0;i<previous_.size();++i) {
        const auto now=pair(w,i),before=previous_[i];
        if(before.active&&(!now.active||now.man!=before.man)) {
            const auto& woman=w.residents[i];
            const bool completed=woman.alive&&woman.familyMomentRemaining<=0&&woman.pregnancyDue>=0
                &&before.man>=0&&before.man<static_cast<int>(w.residents.size())&&w.residents[before.man].alive;
            events.push_back({completed?FamilySound::ManEnd:FamilySound::Cancel,static_cast<int>(i),before.man});
        }
        if(now.active&&(!before.active||now.man!=before.man)) {
            events.push_back({FamilySound::WomanStart,static_cast<int>(i),now.man});
            events.push_back({FamilySound::ManStart,static_cast<int>(i),now.man});
        }
        previous_[i]=now;
    }
    return events;
}
}


