#include "MysteriousPower.h"
#include "World.h"
#include "SeaLife.h"
#include <algorithm>
#include <cmath>

namespace isla {
namespace {
bool valid(int k){return k>=0&&k<mapWidth*mapHeight;}
int cell(float x,float y){return cellId(static_cast<int>(std::floor(x)),static_cast<int>(std::floor(y)));}
bool water(Terrain t){return t==Terrain::Sea||t==Terrain::FreshWater;}
bool present(const Resident& r){return r.alive&&!r.away&&!r.inBoat&&!r.inDungeon&&r.insideHouse<0;}
bool touches(const Prop& p,int k){return k%mapWidth>=std::floor(p.x-p.halfWidth)&&k%mapWidth<=std::floor(p.x+p.halfWidth)&&k/mapWidth>=std::floor(p.y-p.halfDepth)&&k/mapWidth<=std::floor(p.y+p.halfDepth);}
const Prop* prop(const World& w,int uid){for(const auto& p:w.props)if(p.uid==uid)return &p;return nullptr;}
bool openLand(const World& w,int k,int resident=-1,int animal=-1,int propUid=-1,int crop=-1,int gift=-1) {
    if(!valid(k)||water(w.terrain[k])||w.terrain[k]==Terrain::Mountain||w.reserved(k))return false;
    for(const auto& p:w.props)if(p.uid!=propUid&&touches(p,k))return false;
    for(const auto& c:w.crops)if(c.cell==k&&c.cell!=crop)return false;
    for(const auto& g:w.gifts)if(g.cell==k&&g.cell!=gift)return false;
    for(std::size_t i=0;i<w.residents.size();++i)if(static_cast<int>(i)!=resident&&present(w.residents[i])&&cell(w.residents[i].x,w.residents[i].y)==k)return false;
    for(std::size_t i=0;i<w.animals.size();++i)if(static_cast<int>(i)!=animal&&w.animals[i].alive&&cell(w.animals[i].x,w.animals[i].y)==k)return false;
    return true;
}
void release(World& w,int id) {
    auto& r=w.residents[id];
    if(r.familyPartner>=0&&r.familyPartner<static_cast<int>(w.residents.size())){auto& partner=w.residents[r.familyPartner];partner.familyPartner=-1;partner.familyCourtship=0;}
    r.familyPartner=-1;r.familyCourtship=0;r.familyMomentRemaining=0;r.hasOrder=false;r.job=-1;r.workCell=-1;r.target=-1;
    r.task=Task::Idle;r.path.clear();r.next=0;r.speed=r.actionTimer=r.decisionTimer=0;
    for(auto& j:w.jobs)if(j.worker==id)j.worker=-1;
    for(auto& c:w.crops)if(c.worker==id)c.worker=-1;
    for(auto& s:w.ships)if(s.passenger==id){s.passenger=-1;s.state="docked";s.path.clear();s.next=0;}
    for(auto& v:w.visits)if(v.resident==id&&!v.incoming&&v.state!="complete"){v.state="cancelled";w.outbox.erase(v.id);}
    for(auto& other:w.residents)if(other.pregnancyFather==id&&other.familyMomentRemaining>0)other.familyMomentRemaining=0;
    if(r.pregnancyFather>=0&&r.pregnancyFather<static_cast<int>(w.residents.size()))w.residents[r.pregnancyFather].familyMomentRemaining=0;
}
void invalidateRoutes(World& w) {
    w.rebuildNavigation();++w.terrainRevision;
    for(auto& r:w.residents)if(present(r)){r.path.clear();r.next=0;r.task=Task::Idle;r.decisionTimer=0;}
}
}
MysteryTarget MysteriousPower::pick(const World& w,float x,float y) {
    if(!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=mapWidth||y>=mapHeight)return {};
    float best=.85f;MysteryTarget result;
    for(std::size_t i=0;i<w.residents.size();++i){const auto& r=w.residents[i];const float d=std::hypot(r.x-x,r.y-.35f-y);if(present(r)&&d<best){best=d;result={MysteryKind::Resident,static_cast<int>(i)};}}
    if(result)return result;
    for(std::size_t i=0;i<w.animals.size();++i){const auto& a=w.animals[i];const float d=std::hypot(a.x-x,a.y-y);if(a.alive&&d<best){best=d;result={MysteryKind::Animal,static_cast<int>(i)};}}
    if(result)return result;
    for(std::size_t i=0;i<w.seaFish.size();++i){const auto& f=w.seaFish[i];const float d=std::hypot(f.x-x,f.y-y);if(f.alive&&d<best){best=d;result={MysteryKind::Fish,static_cast<int>(i)};}}
    if(result)return result;
    for(std::size_t i=0;i<w.ships.size();++i){const auto& s=w.ships[i];if(s.state!="lost"&&s.seaHome>=0&&std::hypot(s.x-x,s.y-y)<1.1f)return {MysteryKind::Boat,static_cast<int>(i)};}
    const int k=cell(x,y);
    for(const auto& p:w.props)if(touches(p,k))return {MysteryKind::Prop,p.uid};
    const Prop* raised=nullptr;
    for(const auto& p:w.props)if(std::abs(x-p.x)<(p.tree?1.35f:1.45f)&&y>=p.y-(p.tree?2.7f:1.8f)&&y<=p.y+.7f&&(!raised||p.y>raised->y))raised=&p;
    if(raised)return {MysteryKind::Prop,raised->uid};
    for(const auto& c:w.crops)if(c.cell==k)return {MysteryKind::Crop,k};
    for(const auto& g:w.gifts)if(g.cell==k)return {MysteryKind::Gift,k};
    for(const auto& camp:w.camps)if(camp.cell==k)return {MysteryKind::Camp,camp.owner};
    if(water(w.terrain[k]))return {MysteryKind::Water,k};
    if(w.terrain[k]==Terrain::Road||w.terrain[k]==Terrain::Bridge)return {MysteryKind::Road,k};
    return {};
}
std::pair<float,float> MysteriousPower::position(const World& w,MysteryTarget t) {
    if(t.id<0)return {-1,-1};
    switch(t.kind){
    case MysteryKind::Resident:if(t.id<static_cast<int>(w.residents.size())&&present(w.residents[t.id]))return {w.residents[t.id].x,w.residents[t.id].y};break;
    case MysteryKind::Animal:if(t.id<static_cast<int>(w.animals.size())&&w.animals[t.id].alive)return {w.animals[t.id].x,w.animals[t.id].y};break;
    case MysteryKind::Fish:if(t.id<static_cast<int>(w.seaFish.size())&&w.seaFish[t.id].alive)return {w.seaFish[t.id].x,w.seaFish[t.id].y};break;
    case MysteryKind::Boat:if(t.id<static_cast<int>(w.ships.size())&&w.ships[t.id].state!="lost")return {w.ships[t.id].x,w.ships[t.id].y};break;
    case MysteryKind::Prop:if(const auto* p=prop(w,t.id))return {p->x,p->y};break;
    case MysteryKind::Camp:for(const auto& camp:w.camps)if(camp.owner==t.id)return {camp.cell%mapWidth+.5f,camp.cell/mapWidth+.5f};break;
    case MysteryKind::Crop:for(const auto& c:w.crops)if(c.cell==t.id)return {t.id%mapWidth+.5f,t.id/mapWidth+.5f};break;
    case MysteryKind::Gift:for(const auto& g:w.gifts)if(g.cell==t.id)return {t.id%mapWidth+.5f,t.id/mapWidth+.5f};break;
    case MysteryKind::Water:if(valid(t.id)&&water(w.terrain[t.id]))return {t.id%mapWidth+.5f,t.id/mapWidth+.5f};break;
    case MysteryKind::Road:if(valid(t.id)&&(w.terrain[t.id]==Terrain::Road||w.terrain[t.id]==Terrain::Bridge))return {t.id%mapWidth+.5f,t.id/mapWidth+.5f};break;
    default:break;
    }return {-1,-1};
}
bool MysteriousPower::canDrop(const World& w,MysteryTarget t,int destination) {
    const auto source=position(w,t);if(!valid(destination)||source.first<0||cell(source.first,source.second)==destination||w.reserved(destination))return false;
    switch(t.kind){
    case MysteryKind::Fish:return water(w.terrain[destination]);
    case MysteryKind::Boat:{
        if(w.terrain[destination]!=Terrain::Sea)return false;const auto& s=w.ships[t.id];
        if(s.passenger<0){for(int d:{1,-1,mapWidth,-mapWidth})if(valid(destination+d)&&std::abs((destination+d)%mapWidth-destination%mapWidth)+std::abs((destination+d)/mapWidth-destination/mapWidth)==1&&openLand(w,destination+d))return true;return false;}
        if(s.passenger>=static_cast<int>(w.residents.size())||!w.residents[s.passenger].inBoat||w.residents[s.passenger].away)return false;
        return !SeaLife::route(w,destination,s.path.empty()?s.seaHome:s.path.back()).empty();
    }
    case MysteryKind::Resident:return openLand(w,destination,t.id);
    case MysteryKind::Animal:return openLand(w,destination,-1,t.id);
    case MysteryKind::Prop:{
        const auto* p=prop(w,t.id);if(!p||w.reserved(cell(p->x,p->y)))return false;
        Prop moved=*p;moved.x=destination%mapWidth+(p->x-std::floor(p->x));moved.y=destination/mapWidth+(p->y-std::floor(p->y));
        float low=32,high=0;
        for(int y=static_cast<int>(std::floor(moved.y-moved.halfDepth));y<=std::floor(moved.y+moved.halfDepth);++y)
        for(int x=static_cast<int>(std::floor(moved.x-moved.halfWidth));x<=std::floor(moved.x+moved.halfWidth);++x){
            if(x<0||y<0||x>=mapWidth||y>=mapHeight)return false;const int k=cellId(x,y);
            if(!openLand(w,k,-1,-1,p->uid))return false;low=std::min(low,w.elevation[k]);high=std::max(high,w.elevation[k]);
        }
        const int dx=destination%mapWidth-static_cast<int>(p->x),dy=destination/mapWidth-static_cast<int>(p->y);
        for(std::size_t i=0;i<w.residents.size();++i){const auto& r=w.residents[i];
            if(r.insideHouse!=p->uid&&!(p->owner>=0&&(static_cast<int>(i)==p->owner||(r.ageDays<World::adultAgeDays&&(r.mother==p->owner||r.father==p->owner)))))continue;
            const int hx=r.home%mapWidth+dx,hy=r.home/mapWidth+dy;
            if(hx<0||hy<0||hx>=mapWidth||hy>=mapHeight||!openLand(w,cellId(hx,hy),static_cast<int>(i),-1,p->uid))return false;
        }
        return p->tree||high-low<.8f;
    }
    case MysteryKind::Water:{
        if(!openLand(w,destination)||w.reserved(t.id))return false;
        for(const auto& s:w.ships)if(s.seaHome==t.id||cell(s.x,s.y)==t.id)return false;
        for(const auto& f:w.seaFish)if(f.alive&&cell(f.x,f.y)==t.id)return false;
        for(const auto& r:w.residents)if(r.home==destination||r.goal==destination)return false;
        return true;
    }
    case MysteryKind::Crop:case MysteryKind::Road:case MysteryKind::Gift:case MysteryKind::Camp:return openLand(w,destination,-1,-1,-1,t.kind==MysteryKind::Crop?t.id:-1,t.kind==MysteryKind::Gift?t.id:-1);
    default:return false;
    }
}
bool MysteriousPower::move(World& w,MysteryTarget t,int destination,double now) {
    if(!std::isfinite(now)||now<w.mysteriousReadyAt||!canDrop(w,t,destination))return false;
    const auto source=position(w,t);const int oldCell=cell(source.first,source.second);
    const float x=destination%mapWidth+.5f,y=destination/mapWidth+.5f;
    switch(t.kind){
    case MysteryKind::Resident:{release(w,t.id);auto& r=w.residents[t.id];r.x=x;r.y=y;r.goal=destination;break;}
    case MysteryKind::Animal:w.animals[t.id].x=x;w.animals[t.id].y=y;w.animals[t.id].timer=2;break;
    case MysteryKind::Fish:w.seaFish[t.id].x=x;w.seaFish[t.id].y=y;w.seaFish[t.id].timer=2;break;
    case MysteryKind::Boat:{auto& s=w.ships[t.id];s.x=x;s.y=y;
        if(s.passenger>=0){s.path=SeaLife::route(w,destination,s.path.empty()?s.seaHome:s.path.back());s.next=0;}
        else{s.seaHome=destination;for(int d:{1,-1,mapWidth,-mapWidth})if(valid(destination+d)&&openLand(w,destination+d)){s.cell=destination+d;break;}}
        break;}
    case MysteryKind::Prop:{
        auto it=std::find_if(w.props.begin(),w.props.end(),[&](const Prop& p){return p.uid==t.id;});
        const int dx=destination%mapWidth-oldCell%mapWidth,dy=destination/mapWidth-oldCell/mapWidth;
        it->x+=dx;it->y+=dy;
        if(it->tree){w.forest[oldCell]=false;w.forest[destination]=true;}
        for(std::size_t i=0;i<w.residents.size();++i){auto& r=w.residents[i];const bool indoors=r.insideHouse==it->uid;
            if(indoors){r.x+=dx;r.y+=dy;}
            if(indoors||(it->owner>=0&&(static_cast<int>(i)==it->owner||(r.ageDays<World::adultAgeDays&&(r.mother==it->owner||r.father==it->owner)))))r.home=std::clamp(r.home+dx+dy*mapWidth,0,mapWidth*mapHeight-1);
        }
        if(w.market==oldCell)w.market=destination;if(w.granary==oldCell)w.granary=destination;
        invalidateRoutes(w);break;
    }
    case MysteryKind::Crop:for(auto& c:w.crops)if(c.cell==t.id){if(c.worker>=0)release(w,c.worker);c.cell=destination;c.worker=-1;break;}invalidateRoutes(w);break;
    case MysteryKind::Gift:for(auto& g:w.gifts)if(g.cell==t.id){g.cell=destination;break;}invalidateRoutes(w);break;
    case MysteryKind::Camp:for(auto& camp:w.camps)if(camp.owner==t.id){camp.cell=destination;break;}break;
    case MysteryKind::Road:w.terrain[destination]=Terrain::Road;w.terrain[t.id]=w.terrain[t.id]==Terrain::Bridge?Terrain::FreshWater:Terrain::Ground;invalidateRoutes(w);break;
    case MysteryKind::Water:w.terrain[destination]=w.terrain[t.id];w.elevation[destination]=0;w.terrain[t.id]=Terrain::Ground;w.elevation[t.id]=.3f;invalidateRoutes(w);break;
    default:return false;
    }
    w.mysteriousReadyAt=now+cooldownSeconds;++w.metrics["mysterious_interventions"];
    return true;
}
}


