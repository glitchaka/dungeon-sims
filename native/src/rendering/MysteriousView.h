#pragma once
#include "MysteriousPower.h"
#include "SceneTexture.h"
#include "ResidentSprites.h"
#include <filesystem>
namespace isla {
struct MysteryArt {
    const SceneTexture &nature,&buildings,&civic;
    const std::array<ra::Rect,8> &natureRegions,&buildingRegions;
    const std::array<ra::Rect,4>& civicRegions;
    const ResidentSprites& people;
    ra::Texture animals;
};
class MysteriousView {
public:
    void load(SDL_Renderer*,const std::filesystem::path& assets);
    void reset(){fish_.reset();wordArt_.reset();}
    void flash(int cell,double realSeconds){effectCell_=cell;effectAt_=realSeconds;}
    void drawFish(const World&,float mapX,float mapY,float unit,double visualTime,MysteryTarget held={})const;
    void drawHeld(ra::Engine&,const World&,MysteryTarget,float x,float y,float unit,double time,const MysteryArt&,bool validDestination=true)const;
    void drawEffect(SDL_Renderer*,const World&,ra::Rect viewport,float mapX,float mapY,float unit,double now)const;
private:
    SceneTexture fish_,wordArt_;
    SDL_Renderer* renderer_{};
    int effectCell_{-1};double effectAt_{-100};
    void buildWordArt(SDL_Renderer*);
};
}


