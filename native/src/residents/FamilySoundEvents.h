#pragma once
#include <cstddef>
#include <vector>
namespace isla {
class World;
enum class FamilySound { WomanStart, ManStart, ManEnd, Cancel };
struct FamilySoundEvent {FamilySound sound;int woman,man;};
// Observes transitions only; has no playback or platform dependencies.
class FamilySoundEvents {
public:
    void prime(const World& world);
    std::vector<FamilySoundEvent> observe(const World& world);
private:
    struct Pair {bool active{};int man{-1};};
    std::vector<Pair> previous_;
    bool primed_{};
    static Pair pair(const World& world,std::size_t woman);
};
}


