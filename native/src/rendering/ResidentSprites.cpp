#include "ResidentSprites.h"
#include "ResidentAnimation.h"
#include "ResidentEffects.h"
#include "TerrainProjection.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace isla {
void ResidentSprites::load(SDL_Renderer* renderer,const std::filesystem::path& assets) {
    renderer_=renderer;
    women_.load(renderer,(assets/"characters/residents-women-v6.png").string(),8);
    men_.load(renderer,(assets/"characters/residents-men-v6.png").string(),8);
    children_.load(renderer,(assets/"characters/residents-children-v6.png").string(),8);
    workWomen_.load(renderer,(assets/"characters/residents-work-women-v7.png").string(),8);
    workMen_.load(renderer,(assets/"characters/residents-work-men-v7.png").string(),8);
    pregnancy_.load(renderer,(assets/"characters/residents-pregnancy-v7.png").string(),8);
    rowingWomen_.load(renderer,(assets/"boats/boats-women-v7.png").string(),8);
    rowingMen_.load(renderer,(assets/"boats/boats-men-v7.png").string(),8);
    cloud_.load(renderer,(assets/"effects/family-cloud-v7.png").string(),8);
    emptyBoat_.load(renderer,(assets/"boats/boats-empty-v7.png").string(),4);
}
void ResidentSprites::reset() {
    women_.reset();men_.reset();children_.reset();workWomen_.reset();workMen_.reset();
    pregnancy_.reset();rowingWomen_.reset();rowingMen_.reset();cloud_.reset();emptyBoat_.reset();hidden_.clear();launching_.clear();facing_.clear();
}
void ResidentSprites::prepare(const World& world) {
    facing_.resize(world.residents.size(),0);
    hidden_.assign(world.residents.size(),false);launching_.assign(world.residents.size(),false);
    for(std::size_t i=0;i<world.residents.size();++i) {
        const auto& mother=world.residents[i];const int father=mother.pregnancyFather;
        if(!mother.alive||!mother.female||mother.ageDays<World::adultAgeDays||mother.pregnancyDue<0
            ||mother.familyMomentRemaining<=0||father<0||father>=static_cast<int>(hidden_.size()))continue;
        const auto& partner=world.residents[father];
        if(!partner.alive||partner.ageDays<World::adultAgeDays||partner.away||partner.inBoat||partner.inDungeon)continue;
        hidden_[i]=hidden_[father]=true;
    }
    for(const auto& boat:world.ships) {
        if((boat.state!="boarding"&&boat.state!="visit_boarding")||boat.passenger<0||boat.passenger>=static_cast<int>(launching_.size()))continue;
        const auto& r=world.residents[boat.passenger];
        launching_[boat.passenger]=r.next>=r.path.size()&&cellId(static_cast<int>(r.x),static_cast<int>(r.y))==boat.cell;
    }
}
void ResidentSprites::draw(const Resident& r,double elapsed,float footX,float footY,float unit,int index,double animationTime) const {
    const double visualTime=animationTime<0?elapsed:animationTime;
    if(index>=0&&index<static_cast<int>(hidden_.size())&&hidden_[index])return;
    auto a=ResidentAnimation::select(r,elapsed,animationTime);
    if(index>=0&&index<static_cast<int>(facing_.size())) {
        if(a.moving)facing_[index]=a.direction;
        else if(a.pose==ResidentPose::Walk||a.pose==ResidentPose::Pregnant){
            a.direction=facing_[index];a.frame=a.frame/4*4+a.direction;a.nextFrame=a.frame;
        }
    }
    if(!a.child&&index>=0&&index<static_cast<int>(launching_.size())&&launching_[index]) {
        a.pose=ResidentPose::PushBoat;a.frame=4+static_cast<int>(visualTime*3)%2;a.nextFrame=a.frame;a.frameBlend=0;
    }
    const SceneTexture* atlas=a.child?&children_:r.female?&women_:&men_;
    bool work=false;
    if(a.pose==ResidentPose::Pregnant)atlas=&pregnancy_;
    else if(a.pose!=ResidentPose::Walk) {atlas=r.female?&workWomen_:&workMen_;work=true;}
    const auto source=atlas->sprites[a.frame];
    // Keep scale and feet stable across the two authored poses.
    const float referenceHeight=work?atlas->sprites[7].h:
        a.child?source.h:std::max(atlas->sprites[a.direction].h,atlas->sprites[a.direction+4].h);
    const float scale=a.height*unit/std::max(1.0f,referenceHeight);
    const float height=source.h*scale,width=source.w*scale;
    const float bob=a.moving?std::sin(static_cast<float>(visualTime)*10)*unit*.012f:0;
    const auto drawFrame=[&](int frame,float opacity){const auto crop=atlas->sprites[frame];atlas->draw(crop,{footX-crop.w*scale*.5f,footY-crop.h*scale-bob,crop.w*scale,crop.h*scale},opacity);};
    drawFrame(a.frame,1-a.frameBlend);
    if(a.frameBlend>0)drawFrame(a.nextFrame,a.frameBlend);
    if(a.pose==ResidentPose::Courtship) {
        if(!r.female&&r.familyCourtship<3)ResidentEffects::surprise(renderer_,footX,footY-height-unit*.22f,unit*.25f);
        else ResidentEffects::heart(renderer_,footX,footY-height-unit*.2f,unit*.23f);
    } else if(!a.child&&r.familyPartner>=0)
        ResidentEffects::heart(renderer_,footX,footY-height-unit*.2f,unit*.18f);
    if(a.pose==ResidentPose::Till||a.pose==ResidentPose::Chop)
        ResidentEffects::impact(renderer_,footX+width*.35f,footY,unit,animationTime<0?r.actionTimer:animationTime,a.pose==ResidentPose::Chop);
}
void ResidentSprites::drawBoat(const Ship& boat,const Resident& rower,double elapsed,float x,float y,float unit) const {
    int direction=0;
    const bool moving=boat.state=="outbound"||boat.state=="returning"||boat.state=="visit_outbound"||boat.state=="visit_returning";
    if(moving&&boat.next<boat.path.size()) {
        const int next=boat.path[boat.next];const float dx=next%mapWidth+.5f-boat.x,dy=next/mapWidth+.5f-boat.y;
        direction=std::abs(dx)>std::abs(dy)?(dx>0?1:3):(dy>0?0:2);
    }
    const auto& atlas=rower.female?rowingWomen_:rowingMen_;
    const auto source=atlas.sprites[direction+4*(moving?static_cast<int>(elapsed*2.5)%2:0)];
    const float scale=unit*2.5f/std::max(source.w,source.h);
    const float bob=std::sin(static_cast<float>(elapsed)*2.3f)*unit*.035f;
    atlas.draw(source,{x-source.w*scale*.5f,y-source.h*scale*.5f+bob,source.w*scale,source.h*scale});
    if(moving) {
        SDL_SetRenderDrawBlendMode(renderer_,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(renderer_,189,226,221,110);
        for(int i=0;i<3;++i) {
            const float t=std::fmod(static_cast<float>(elapsed)*.8f+i*.33f,1.0f);
            const float dx=direction==1?-1.0f:direction==3?1.0f:0;
            const float dy=direction==0?-1.0f:direction==2?1.0f:0;
            const float cx=x+dx*unit*(.6f+t),cy=y+dy*unit*(.6f+t);
            SDL_RenderLine(renderer_,cx-unit*.15f-dy*unit*t*.2f,cy-dx*unit*t*.2f,cx+unit*.15f+dy*unit*t*.2f,cy+dx*unit*t*.2f);
        }
    }
}
void ResidentSprites::drawEmptyBoat(const Ship& boat,double elapsed,float x,float y,float unit) const {
    const float dx=boat.x-(boat.cell%mapWidth+.5f),dy=boat.y-(boat.cell/mapWidth+.5f);
    const int direction=std::abs(dx)>std::abs(dy)?(dx>0?1:3):(dy>0?0:2);
    const auto source=emptyBoat_.sprites[direction];
    const float scale=unit*1.8f/std::max(source.w,source.h);
    const float bob=std::sin(static_cast<float>(elapsed)*2.3f)*unit*.025f;
    emptyBoat_.draw(source,{x-source.w*scale*.5f,y-source.h*scale*.5f+bob,source.w*scale,source.h*scale});
}
void ResidentSprites::drawFamilyEffects(const World& world,float mapX,float mapY,float unit,double animationTime) const {
    const double visualTime=animationTime<0?world.elapsed:animationTime;
    for(std::size_t i=0;i<world.residents.size();++i) {
        const auto& mother=world.residents[i];
        if(i>=hidden_.size()||!hidden_[i]||!mother.female||mother.pregnancyFather<0)continue;
        const auto& father=world.residents[mother.pregnancyFather];
        float x=(mother.x+father.x)*.5f,y=(mother.y+father.y)*.5f;
        if(mother.insideHouse>=0)for(const auto& house:world.props)if(house.uid==mother.insideHouse){x=house.x;y=house.y;break;}
        if(!TerrainProjection::visible(world,x,y))continue;
        const float width=std::max(1.3f,std::abs(mother.x-father.x)*.5f+.7f);
        const float cx=mapX+x*unit,cy=mapY+(y-TerrainProjection::elevation(world,x,y)-.5f)*unit;
        const auto source=cloud_.sprites[static_cast<int>(visualTime*3)%8];
        const float screenWidth=width*unit*2,screenHeight=screenWidth*source.h/source.w;
        cloud_.draw(source,{cx-screenWidth*.5f,cy-screenHeight*.5f,screenWidth,screenHeight});
        for(int heart=0;heart<3;++heart){
            const float t=std::fmod(static_cast<float>(visualTime)*.3f+heart*.33f,1.0f);
            ResidentEffects::heart(renderer_,cx+(heart-1)*unit*.5f,cy-screenHeight*.5f-t*unit*.6f,unit*.18f);
        }
    }
}
}



