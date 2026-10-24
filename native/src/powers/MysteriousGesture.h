#pragma once
#include "MysteriousPower.h"
#include <string>
namespace isla {
struct MysteryPointer {bool pressed{},down{},overMap{};float screenX{},screenY{},worldX{},worldY{};};
class MysteriousGesture {
public:
    bool update(World&,const MysteryPointer&,double realSeconds,bool enabled=true);
    void cancel(){target_={};dragging_=false;}
    bool active()const{return dragging_;}
    bool holding(MysteryKind kind,int id)const{return dragging_&&target_.kind==kind&&target_.id==id;}
    MysteryTarget target()const{return target_;}
    int destination()const{return destination_;}
private:
    MysteryTarget target_;bool dragging_{};double pressedAt_{};float pressX_{},pressY_{};std::string worldId_;int destination_{-1};
};
}


