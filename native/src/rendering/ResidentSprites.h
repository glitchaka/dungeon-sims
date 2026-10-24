#pragma once
#include "SceneTexture.h"
#include "entities/Resident.h"
#include <filesystem>

namespace isla {
class World;
struct Ship;
// Presentation only: owns character art, facing, gait and physical screen size.
class ResidentSprites {
public:
    void load(SDL_Renderer* renderer,const std::filesystem::path& assets);
    void reset();
    void prepare(const World& world);
    void draw(const Resident& resident,double elapsed,float footX,float footY,float unit,int index=-1,double animationTime=-1) const;
    void drawBoat(const Ship& boat,const Resident& rower,double elapsed,float x,float y,float unit) const;
    void drawEmptyBoat(const Ship& boat,double elapsed,float x,float y,float unit) const;
    void drawFamilyEffects(const World& world,float mapX,float mapY,float unit,double animationTime=-1) const;
private:
    SceneTexture women_,men_,children_;
    SceneTexture workWomen_,workMen_,pregnancy_,rowingWomen_,rowingMen_;
    SceneTexture cloud_,emptyBoat_;
    SDL_Renderer* renderer_{};
    std::vector<bool> hidden_,launching_;
    mutable std::vector<int> facing_;
};
}


