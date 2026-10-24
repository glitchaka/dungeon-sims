#pragma once
#include <utility>
namespace isla {
class World;
enum class MysteryKind {None,Resident,Animal,Prop,Crop,Boat,Fish,Water,Road,Gift,Camp};
struct MysteryTarget {MysteryKind kind{MysteryKind::None};int id{-1};explicit operator bool()const{return kind!=MysteryKind::None;}};
class MysteriousPower {
public:
    static constexpr double cooldownSeconds=180;
    static MysteryTarget pick(const World&,float x,float y);
    static std::pair<float,float> position(const World&,MysteryTarget);
    static bool canDrop(const World&,MysteryTarget,int destination);
    static bool move(World&,MysteryTarget,int destination,double realSeconds);
};
}

