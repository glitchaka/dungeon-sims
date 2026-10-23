#include "PresentationMotion.h"

namespace isla {
void PresentationMotion::capture(const World& world) {
    residents_.clear();residents_.reserve(world.residents.size());
    for(const auto& r:world.residents)residents_.push_back({r.x,r.y,r.inBoat,r.away,r.insideHouse});
    boats_.clear();boats_.reserve(world.ships.size());
    for(const auto& s:world.ships)boats_.push_back({s.x,s.y,s.passenger,s.state});
}
ScreenPosition PresentationMotion::resident(const World& world,std::size_t index,float alpha)const {
    const auto& r=world.residents[index];
    if(index>=residents_.size())return {r.x,r.y};
    const auto& previous=residents_[index];
    if(previous.inBoat!=r.inBoat||previous.away!=r.away||previous.house!=r.insideHouse||std::hypot(r.x-previous.x,r.y-previous.y)>2)return {r.x,r.y};
    alpha=std::clamp(alpha,0.0f,1.0f);
    return {previous.x+(r.x-previous.x)*alpha,previous.y+(r.y-previous.y)*alpha};
}
ScreenPosition PresentationMotion::boat(const Ship& ship,std::size_t index,float alpha)const {
    if(index>=boats_.size())return {ship.x,ship.y};
    const auto& previous=boats_[index];
    if(previous.passenger!=ship.passenger||previous.state!=ship.state||std::hypot(ship.x-previous.x,ship.y-previous.y)>2)return {ship.x,ship.y};
    alpha=std::clamp(alpha,0.0f,1.0f);
    return {previous.x+(ship.x-previous.x)*alpha,previous.y+(ship.y-previous.y)*alpha};
}
}
