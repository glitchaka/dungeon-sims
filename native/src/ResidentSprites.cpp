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
    women_.loadGrid(renderer,(assets/"residents-women-walk-v9.png").string(),6,4);
    men_.loadGrid(renderer,(assets/"residents-men-walk-v9.png").string(),6,4);
    children_.loadGrid(renderer,(assets/"residents-girl-walk-v9.png").string(),6,4);
    boys_.loadGrid(renderer,(assets/"residents-boy-walk-v9.png").string(),6,4);
    workWomen_.loadGrid(renderer,(assets/"residents-women-work-v9.png").string(),6,6);
    workMen_.loadGrid(renderer,(assets/"residents-men-work-v9.png").string(),6,6);
    lifeWomen_.loadGrid(renderer,(assets/"residents-women-needs-v9.png").string(),6,3);
    lifeMen_.loadGrid(renderer,(assets/"residents-men-needs-v9.png").string(),6,3);
    socialWomen_.loadGrid(renderer,(assets/"residents-women-rest-social-v9.png").string(),6,3);
    socialMen_.loadGrid(renderer,(assets/"residents-men-rest-social-v9.png").string(),6,3);
    errandsWomen_.loadGrid(renderer,(assets/"residents-women-errands-v9.png").string(),6,6);
    errandsMen_.loadGrid(renderer,(assets/"residents-men-errands-v9.png").string(),6,6);
    pregnancy_.loadGrid(renderer,(assets/"residents-pregnancy-walk-v9.png").string(),6,8);
    pregnancyLife_.loadGrid(renderer,(assets/"residents-pregnancy-life-v9.png").string(),6,4);
    childrenLife_.loadGrid(renderer,(assets/"residents-children-life-v9.png").string(),6,4);
    rowingWomen_.loadGrid(renderer,(assets/"boats-women-row-v9.png").string(),6,4);
    rowingMen_.loadGrid(renderer,(assets/"boats-men-row-v9.png").string(),6,4);
    fishingWomen_.loadGrid(renderer,(assets/"boats-women-fish-v9.png").string(),6,4);
    fishingMen_.loadGrid(renderer,(assets/"boats-men-fish-v9.png").string(),6,4);
    cloud_.load(renderer,(assets/"family-cloud-v7.png").string(),8);
    emptyBoat_.load(renderer,(assets/"boats-empty-v7.png").string(),4);
}
void ResidentSprites::reset() {
    women_.reset();men_.reset();children_.reset();boys_.reset();workWomen_.reset();workMen_.reset();
    pregnancy_.reset();rowingWomen_.reset();rowingMen_.reset();cloud_.reset();emptyBoat_.reset();hidden_.clear();launching_.clear();
    lifeWomen_.reset();lifeMen_.reset();errandsWomen_.reset();errandsMen_.reset();childrenLife_.reset();pregnancyLife_.reset();
    fishingWomen_.reset();fishingMen_.reset();
    socialWomen_.reset();socialMen_.reset();
}
void ResidentSprites::prepare(const World& world) {
    hitBoxes_.clear();
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
    if(!a.child&&index>=0&&index<static_cast<int>(launching_.size())&&launching_[index]) {
        a.pose=ResidentPose::PushBoat;a.frame=ResidentAnimation::phase(visualTime,6);
    }
    const SceneTexture* atlas=r.female?&women_:&men_;
    int row=a.direction;float bodyFraction=.88f;
    if(a.child){
        if(a.moving){atlas=r.female?&children_:&boys_;row=a.direction;}
        else {atlas=&childrenLife_;row=(r.female?0:2)+(a.pose==ResidentPose::Rest?1:0);}
    }else if(a.pose==ResidentPose::Pregnant){atlas=&pregnancy_;row=(a.pregnancyStage-1)*4+a.direction;}
    else if(a.pregnancyStage&&(a.pose==ResidentPose::Idle||a.pose==ResidentPose::Rest)){
        atlas=&pregnancyLife_;row=(a.pose==ResidentPose::Rest?2:0)+a.pregnancyStage-1;
    }else if(a.pose!=ResidentPose::Walk){
        atlas=r.female?&lifeWomen_:&lifeMen_;bodyFraction=.78f;
        switch(a.pose){
        case ResidentPose::Till:case ResidentPose::Chop:case ResidentPose::Sow:case ResidentPose::Harvest:case ResidentPose::Build:case ResidentPose::Hunt:
            atlas=r.female?&workWomen_:&workMen_;
            row=a.pose==ResidentPose::Till?0:a.pose==ResidentPose::Chop?1:a.pose==ResidentPose::Sow?2:a.pose==ResidentPose::Harvest?3:a.pose==ResidentPose::Build?4:5;break;
        case ResidentPose::Carry:case ResidentPose::Fetch:case ResidentPose::Trade:case ResidentPose::Tend:case ResidentPose::Camp:case ResidentPose::PushBoat:
            atlas=r.female?&errandsWomen_:&errandsMen_;
            row=a.pose==ResidentPose::Carry?0:a.pose==ResidentPose::Fetch?1:a.pose==ResidentPose::Trade?2:a.pose==ResidentPose::Tend?3:a.pose==ResidentPose::Camp?4:5;break;
        case ResidentPose::Rest:case ResidentPose::Bathe:case ResidentPose::Social:case ResidentPose::Courtship:
            atlas=r.female?&socialWomen_:&socialMen_;
            row=a.pose==ResidentPose::Rest?0:a.pose==ResidentPose::Bathe?1:2;break;
        default:row=a.pose==ResidentPose::Eat?1:a.pose==ResidentPose::Drink?2:0;break;
        }
    }
    const auto source=atlas->gridFrame(row*ResidentAnimation::framesPerAction+a.frame);
    // Grid cells keep a stable pivot/scale instead of zooming each cropped pose.
    const float scale=a.height*unit/(source.h*bodyFraction);
    const float height=source.h*scale,width=source.w*scale;
    const float bob=a.moving?std::sin(static_cast<float>(visualTime)*10)*unit*.012f:0;
    const ra::Rect destination{footX-width*.5f,footY-height*.96f-bob,width,height};
    atlas->draw(source,destination);
    if(index>=0)hitBoxes_.push_back({index,destination});
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
    const auto& atlas=boat.state=="fishing"?(rower.female?fishingWomen_:fishingMen_):(rower.female?rowingWomen_:rowingMen_);
    const auto source=atlas.gridFrame(direction*ResidentAnimation::framesPerAction+ResidentAnimation::phase(elapsed,moving?7:3));
    const float scale=unit*2.5f/std::max(source.w,source.h);
    const float bob=std::sin(static_cast<float>(elapsed)*2.3f)*unit*.035f;
    const ra::Rect destination{x-source.w*scale*.5f,y-source.h*scale*.5f+bob,source.w*scale,source.h*scale};
    atlas.draw(source,destination);
    if(boat.passenger>=0)hitBoxes_.push_back({boat.passenger,destination});
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
int ResidentSprites::pick(float x,float y) const {
    for(auto it=hitBoxes_.rbegin();it!=hitBoxes_.rend();++it){const auto& r=it->second;
        if(x>=r.x&&y>=r.y&&x<=r.x+r.w&&y<=r.y+r.h)return it->first;}
    return -1;
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
