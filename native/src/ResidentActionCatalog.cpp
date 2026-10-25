#include "ResidentActionCatalog.h"
#include "World.h"
#include <algorithm>
#include <cmath>
namespace isla {
std::vector<ResidentAction> ResidentActionCatalog::available(const World& w,const Resident& r){
    const int at=cellId(static_cast<int>(r.x),static_cast<int>(r.y)),id=w.residents.indexOf(r);
    std::vector<ResidentAction> actions;
    auto add=[&](Task task,int goal,int target=-1,int work=-1,int job=-1,int boat=-1,float risk=0){
        if(goal<0||goal>=mapWidth*mapHeight)return;
        const bool field=task==Task::Sow||task==Task::Harvest||task==Task::Till;
        auto path=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return field?std::abs(x-goal%mapWidth)+std::abs(y-goal/mapWidth)==1:cellId(x,y)==goal;});
        if(path.empty())return;
        actions.push_back({task,goal,target,work,job,boat,false,std::min(.35f,static_cast<float>(path.size())*.002f),risk});
    };
    add(Task::Idle,at);add(Task::Rest,r.home);if(actions.size()==1)add(Task::Rest,at);
    if(r.ageDays<World::adultAgeDays)return actions;
    if(r.homeWater>0)add(Task::DrinkHome,r.home);
    auto shore=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return w.besideFreshWater(x,y);});
    if(!shore.empty()){add(Task::Drink,shore.back());add(Task::Bathe,shore.back());}
    if(r.grain+r.meat+r.eggs+r.fish>0)add(Task::Eat,at);
    if(w.grain>0)add(Task::Fetch,w.granary);
    if(r.hasOrder){add(r.orderedTask,r.orderedCell,r.target,r.workCell,r.job);return actions;}
    for(std::size_t i=0;i<w.residents.size();++i){const auto& other=w.residents[i];if(static_cast<int>(i)==id||!other.alive||other.away||other.inBoat||other.inDungeon||other.insideHouse>=0)continue;
        auto path=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return std::hypot(x+.5f-other.x,y+.5f-other.y)<=1.5f;});
        if(!path.empty()){add(Task::Social,path.back(),static_cast<int>(i));break;}}
    auto explore=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return std::abs(x-static_cast<int>(r.x))+std::abs(y-static_cast<int>(r.y))>=4;});
    if(!explore.empty())add(Task::Wander,explore.back());
    if(w.alarmSeconds>0){auto safe=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return std::hypot(x+.5f-w.alarmX,y+.5f-w.alarmY)>=cnc::fleeRadius;});if(!safe.empty())add(Task::Flee,safe.back());}
    if(r.visitor||w.night())return actions;
    if(w.familyOpportunity(r)[0]>=0){ResidentAction family;family.task=Task::Social;family.goal=at;family.family=true;family.cost=.12f;actions.push_back(family);}
    if(r.grain>=6)add(Task::Deposit,w.granary);
    if(r.wood>0)add(Task::Trade,w.market);
    if(!r.loot.empty())add(Task::SellLoot,w.market);
    for(const auto& crop:w.crops){if(crop.worker>=0&&crop.worker!=id)continue;if(w.reserved(crop.cell))continue;
        if(crop.stage==3)add(Task::Harvest,crop.cell);else if(crop.stage==0&&r.seeds>0)add(Task::Sow,crop.cell);
        if(actions.size()>18)break;}
    if(w.crops.size()<128&&r.seeds>2)for(int dy=-4;dy<=4;++dy){bool found=false;for(int dx=-4;dx<=4;++dx){const int x=static_cast<int>(r.x)+dx,y=static_cast<int>(r.y)+dy;
        if(x<0||y<0||x>=mapWidth||y>=mapHeight)continue;const int k=cellId(x,y);
        if(w.walkable(x,y)&&w.terrain[k]==Terrain::Ground&&!w.fallow[k]&&!w.protectedLand[k]&&!w.reserved(k)&&std::none_of(w.crops.begin(),w.crops.end(),[&](const Crop& c){return c.cell==k;})){add(Task::Till,k);found=true;break;}}if(found)break;}
    for(const auto& tree:w.props)if(tree.tree){const int k=cellId(static_cast<int>(tree.x),static_cast<int>(tree.y));if(w.protectedLand[k])continue;
        auto path=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return std::abs(x-k%mapWidth)+std::abs(y-k/mapWidth)==1;});
        if(!path.empty()){add(Task::Gather,path.back(),-1,k);break;}}
    for(const auto& job:w.jobs)if(job.worker<0||job.worker==id)add(Task::Project,job.goal,-1,-1,job.id);
    for(std::size_t i=0;i<w.animals.size();++i){const auto& animal=w.animals[i];if(animal.alive&&animal.kind==4){const int k=cellId(static_cast<int>(animal.x),static_cast<int>(animal.y));if(!w.protectedLand[k])add(Task::Hunt,k,static_cast<int>(i),-1,-1,-1,.12f);break;}}
    if(w.livestockFood>0){auto path=w.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return w.pasture[cellId(x,y)];});if(!path.empty())add(Task::Tend,path.back());}
    if(r.wood>=3&&!w.reserved(at))add(Task::Camp,at);
    if(r.hunger>45&&r.thirst>45&&r.energy>45)for(std::size_t i=0;i<w.ships.size();++i){const auto& boat=w.ships[i];if(boat.passenger<0&&boat.state=="docked"&&boat.seaHome>=0)add(Task::Fish,boat.cell,-1,-1,-1,static_cast<int>(i),.25f);}
    if(r.mission>=0&&r.mission<static_cast<int>(w.dungeons.size()))add(Task::Expedition,w.dungeons[r.mission].entrance);
    return actions;
}
}
