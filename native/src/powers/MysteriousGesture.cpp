#include "MysteriousGesture.h"
#include "World.h"
#include <cmath>
namespace isla {
bool MysteriousGesture::update(World& w,const MysteryPointer& p,double now,bool enabled) {
    if(!enabled||now<w.mysteriousReadyAt){cancel();return false;}
    if(p.pressed&&p.overMap){target_=MysteriousPower::pick(w,p.worldX,p.worldY);pressedAt_=now;pressX_=p.screenX;pressY_=p.screenY;worldId_=w.worldId;dragging_=false;}
    if(!target_)return false;
    if(worldId_!=w.worldId||MysteriousPower::position(w,target_).first<0){cancel();return false;}
    if(p.down&&now-pressedAt_>=.2&&std::hypot(p.screenX-pressX_,p.screenY-pressY_)>=7)dragging_=true;
    if(p.down)return false;
    bool moved=false;
    if(dragging_&&p.overMap&&std::isfinite(p.worldX)&&std::isfinite(p.worldY)&&p.worldX>=0&&p.worldY>=0&&p.worldX<mapWidth&&p.worldY<mapHeight){
        destination_=cellId(static_cast<int>(p.worldX),static_cast<int>(p.worldY));moved=MysteriousPower::move(w,target_,destination_,now);
    }
    cancel();return moved;
}
}

