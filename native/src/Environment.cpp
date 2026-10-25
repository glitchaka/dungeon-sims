#include "World.h"
#include <algorithm>
#include <cmath>
#include <memory>
namespace isla {
namespace {bool valid(int k){return k>=0&&k<mapWidth*mapHeight;}int pos(const Resident& r){return cellId(static_cast<int>(r.x),static_cast<int>(r.y));}bool touches(const Prop& p,int k){return valid(k)&&k%mapWidth>=static_cast<int>(std::floor(p.x-p.halfWidth))&&k%mapWidth<=static_cast<int>(std::floor(p.x+p.halfWidth))&&k/mapWidth>=static_cast<int>(std::floor(p.y-p.halfDepth))&&k/mapWidth<=static_cast<int>(std::floor(p.y+p.halfDepth));}}
Prop* World::propAt(int k){for(auto& p:props)if(touches(p,k))return &p;return nullptr;}
bool World::reserved(int k) const {for(const auto& j:jobs)if(j.cell==k||j.source==k)return true;return false;}
bool World::placeable(int k,int ignoreSource) const {
    if(!valid(k)||k%mapWidth>=mapWidth-1||k/mapWidth>=mapHeight-1)return false;
    float low=elevation[k],high=low;for(int c:{k,k+1,k+mapWidth,k+mapWidth+1}){low=std::min(low,elevation[c]);high=std::max(high,elevation[c]);}if(high-low>.35f)return false;
    for(int c:{k,k+1,k+mapWidth,k+mapWidth+1}){
        if(terrain[c]!=Terrain::Ground||protectedLand[c]||pasture[c]||forest[c]||reserved(c))return false;
        for(const auto& crop:crops)if(crop.cell==c)return false;
        for(const auto& p:props)if(touches(p,c)&&!touches(p,ignoreSource))return false;
        for(const auto& r:residents)if(r.alive&&!r.inDungeon&&!r.away&&(pos(r)==c||r.home==c||r.goal==c))return false;
        for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==c)return false;
    }
    auto candidate=std::make_unique<World>(*this);auto& trial=*candidate;
    if(ignoreSource>=0)trial.props.erase(std::remove_if(trial.props.begin(),trial.props.end(),[&](const Prop& p){return touches(p,ignoreSource);}),trial.props.end());
    trial.props.push_back({k%mapWidth+1.0f,k/mapWidth+1.0f,.9f,.9f,3,false,"Casa"});trial.rebuildNavigation();
    for(const auto& r:trial.residents)if(r.alive&&!r.inDungeon&&!r.away){
        auto water=trial.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return trial.besideFreshWater(x,y);});
        auto store=trial.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return cellId(x,y)==granary;});
        if(water.empty()||store.empty())return false;
    }
    return true;
}
void World::seedEnvironment(){
    const std::array<int,6> centers{{cellId(55,46),cellId(29,40),cellId(32,25),cellId(94,32),cellId(93,64),cellId(65,67)}};
    for(int center:centers)for(int y=std::max(0,center/mapWidth-5);y<std::min(mapHeight,center/mapWidth+6);++y)for(int x=std::max(0,center%mapWidth-5);x<std::min(mapWidth,center%mapWidth+6);++x)if(terrain[cellId(x,y)]==Terrain::Ground)forest[cellId(x,y)]=false;
    const std::array<int,12> sites{{cellId(53,42),cellId(57,42),cellId(61,42),cellId(54,49),cellId(58,49),cellId(62,49),cellId(28,39),cellId(31,24),cellId(92,30),cellId(94,62),cellId(64,64),cellId(88,60)}};
    std::vector<int> suggestions(sites.begin(),sites.end());for(int center:centers)for(int dy:{-4,0,4})for(int dx:{-4,0,4})suggestions.push_back(center+dy*mapWidth+dx);
    int owner=0;for(int k:suggestions)if(placeable(k)){
        const auto& def=cnc::buildings[static_cast<std::size_t>(nextProp)%cnc::buildings.size()];
        props.push_back({k%mapWidth+1.0f,k/mapWidth+1.0f,.9f,.9f,def.height,false,"Casa "+std::to_string(nextProp+1),nextProp++,owner<static_cast<int>(residents.size())?owner:-1,100});
        rebuildNavigation();if(owner<static_cast<int>(residents.size())){int door=k+2*mapWidth;if(walkable(door%mapWidth,door/mapWidth))residents[owner].home=door;++owner;}
    }
    for(int k=0;k<mapWidth*mapHeight;++k){if(!forest[k]||terrain[k]!=Terrain::Ground||(k*19)%5>1||!walkable(k%mapWidth,k/mapWidth))continue;bool occupied=false;for(const auto& r:residents)if(pos(r)==k||r.home==k||r.goal==k)occupied=true;for(const auto& a:animals)if(cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;for(const auto& c:crops)if(c.cell==k)occupied=true;if(occupied)continue;
        props.push_back({k%mapWidth+.5f,k/mapWidth+.5f,.32f,.32f,cnc::treeHeight,true,"Arbol",nextProp++,-1,60});}
    rebuildNavigation();
    for(auto& a:animals)if(!walkable(static_cast<int>(a.x),static_cast<int>(a.y))){for(int k=0;k<mapWidth*mapHeight;++k)if(walkable(k%mapWidth,k/mapWidth)&&(a.kind==4?forest[k]:pasture[k])){a.x=k%mapWidth+.5f;a.y=k/mapWidth+.5f;break;}}
}
std::string World::designate(const std::string& requested,int k){
    if(!valid(k))return "Fuera del mapa";std::string kind=requested;Prop* object=propAt(k);
    if(kind=="protect"){if(!forest[k]&&!(object&&object->tree))return "Elige un bosque";bool remove=protectedLand[k];for(int c=0;c<mapWidth*mapHeight;++c)if(std::hypot(static_cast<float>(c%mapWidth-k%mapWidth),static_cast<float>(c/mapWidth-k/mapWidth))<=5)protectedLand[c]=!remove;return remove?"Proteccion retirada":"Bosque protegido";}
    if(kind=="demolish"&&!object){if(cropAt(k))kind="fallow";else if(terrain[k]==Terrain::Road||terrain[k]==Terrain::Bridge)kind="remove_road";else return "No hay un objeto que retirar";}
    if(kind=="clear"){if(!object||!object->tree)return "Elige un arbol";kind="demolish";}
    const bool civic=kind=="dock"||kind=="townhall"||kind=="altar";
    if(!civic&&kind!="house"&&kind!="farm"&&kind!="road"&&kind!="fallow"&&kind!="remove_road"&&kind!="demolish")return "Encargo desconocido";
    if(reserved(k))return "Ya hay un encargo para este lugar";
    if(protectedLand[k])return "Retira primero la proteccion";
    if((kind=="house"||civic)&&!placeable(k))return "La casa necesita 2x2 celdas libres y pasos accesibles";
    if(kind=="farm"&&(terrain[k]!=Terrain::Ground||!walkable(k%mapWidth,k/mapWidth)||cropAt(k)||pasture[k]||forest[k]))return "Necesita suelo despejado y libre";
    if(kind=="road"&&(object||cropAt(k)||(terrain[k]!=Terrain::Ground&&terrain[k]!=Terrain::FreshWater)))return "El camino requiere suelo libre o una orilla para puente";
    if(kind=="fallow"&&!cropAt(k))return "Elige un campo";
    if(kind=="remove_road"&&terrain[k]!=Terrain::Road&&terrain[k]!=Terrain::Bridge)return "No hay camino";
    if(kind=="demolish"&&!object)return "No hay objeto";
    if(kind=="dock"){bool coast=false;for(int y=std::max(0,k/mapWidth-2);y<std::min(mapHeight,k/mapWidth+4);++y)for(int x=std::max(0,k%mapWidth-2);x<std::min(mapWidth,k%mapWidth+4);++x)if(terrain[cellId(x,y)]==Terrain::Sea)coast=true;if(!coast)return "El muelle debe estar junto al mar";}
    int wood=kind=="dock"?16:kind=="townhall"?24:kind=="altar"?8:kind=="house"?12:kind=="road"?(terrain[k]==Terrain::FreshWater?3:1):0;
    int money=kind=="dock"?12:kind=="townhall"?25:kind=="altar"?10:kind=="house"?15:kind=="road"?1:0;
    if(communityWood<wood||treasury<money)return "Recursos insuficientes";
    auto available=std::find_if(residents.begin(),residents.end(),[](const Resident& r){return r.alive&&!r.away&&!r.visitor&&!r.inBoat&&!r.inDungeon&&r.ageDays>=World::adultAgeDays;});if(available==residents.end())return "No hay trabajadores disponibles";int goal=nearest(*available,[&](int x,int y){int at=cellId(x,y);if(kind=="house"||civic)return std::abs(x-k%mapWidth)<=2&&std::abs(y-k/mapWidth)<=2&&at!=k&&at!=k+1&&at!=k+mapWidth&&at!=k+mapWidth+1;return std::abs(x-k%mapWidth)+std::abs(y-k/mapWidth)==1;});
    if(goal<0)return "No existe un lugar accesible desde donde trabajar";
    communityWood-=wood;treasury-=money;jobs.push_back({nextJob++,k,goal,-1,kind,-1,0,wood,money});record("Encargo: "+kind);return "Encargo pendiente: "+kind;
}
std::string World::relocate(int source,int destination){
    if(!valid(source)||!valid(destination)||source==destination)return "Destino no valido";
    if(reserved(source)||reserved(destination))return "Hay un encargo pendiente en origen o destino";
    auto* object=propAt(source);const bool house=object&&!object->tree;const bool crop=cropAt(source)!=nullptr;const bool road=terrain[source]==Terrain::Road;
    if(!house&&!crop&&!road)return "Elige una casa, un campo o un camino";
    if(protectedLand[source]||protectedLand[destination])return "El terreno esta protegido";
    if(house&&!placeable(destination,source))return "El destino no admite una casa de 2x2";
    if(!house&&(terrain[destination]!=Terrain::Ground||!walkable(destination%mapWidth,destination/mapWidth)||forest[destination]||pasture[destination]||cropAt(destination)))return "Destino ocupado o no apto";
    int wood=house?4:1,money=house?4:1;if(communityWood<wood||treasury<money)return "Faltan recursos para reubicar";
    int goal=nearest(residents[0],[&](int x,int y){return std::abs(x-source%mapWidth)+std::abs(y-source/mapWidth)==1;});if(goal<0)return "El origen no es accesible";
    communityWood-=wood;treasury-=money;jobs.push_back({nextJob++,destination,goal,-1,house?"move_house":crop?"move_farm":"move_road",source,0,wood,money});return "Reubicacion encargada; requiere trabajar en origen y destino";
}
void World::completeJob(Resident& r){
    auto it=std::find_if(jobs.begin(),jobs.end(),[&](const Job& j){return j.id==r.job;});if(it==jobs.end()){r.hasOrder=false;r.task=Task::Idle;return;}
    if(it->kind.find("move_")==0&&it->phase==0){int k=it->cell;int goal=nearest(r,[&](int x,int y){return std::abs(x-k%mapWidth)+std::abs(y-k/mapWidth)==1;});if(goal>=0){it->phase=1;it->goal=goal;r.orderedCell=goal;begin(r,Task::Project,goal);return;}}
    const int previousHome=r.home;const Job job=*it;const int k=job.cell;bool success=true;std::string note;
    auto freeCell=[&](){if(!walkable(k%mapWidth,k/mapWidth)||terrain[k]!=Terrain::Ground||cropAt(k)||forest[k]||protectedLand[k])return false;return true;};
    if(job.kind=="house"||job.kind=="move_house"||job.kind=="dock"||job.kind=="townhall"||job.kind=="altar"){
        World trial=*this;trial.jobs.erase(std::remove_if(trial.jobs.begin(),trial.jobs.end(),[&](const Job& j){return j.id==job.id;}),trial.jobs.end());
        success=trial.placeable(k,job.source);
        if(success){if(job.kind=="move_house"){auto* old=propAt(job.source);if(!old||old->tree)success=false;else{int owner=old->owner;old->x=k%mapWidth+1.0f;old->y=k/mapWidth+1.0f;if(owner>=0&&owner<static_cast<int>(residents.size())){residents[owner].home=k+2*mapWidth;for(auto& child:residents)if(child.ageDays<World::adultAgeDays&&(child.mother==owner||child.father==owner))child.home=k+2*mapWidth;}for(auto& occupant:residents)if(occupant.insideHouse==old->uid){occupant.home=k+2*mapWidth;occupant.x=k%mapWidth+.5f;occupant.y=k/mapWidth+2.5f;}}}else{int owner=-1;if(job.kind=="house")for(std::size_t person=0;person<residents.size();++person){const auto& candidate=residents[person];if(!candidate.alive||candidate.away||candidate.visitor||candidate.ageDays<World::adultAgeDays)continue;bool housed=std::any_of(props.begin(),props.end(),[&](const Prop& p){return !p.tree&&p.kind=="house"&&p.owner==static_cast<int>(person);});if(!housed){owner=static_cast<int>(person);break;}}props.push_back({k%mapWidth+1.0f,k/mapWidth+1.0f,.9f,.9f,3,false,"Casa",nextProp++,owner,100});if(owner>=0){residents[owner].home=k+2*mapWidth;for(auto& child:residents)if(child.ageDays<World::adultAgeDays&&(child.mother==owner||child.father==owner))child.home=k+2*mapWidth;}
            if(job.kind!="house"){
                auto& building=props.back();building.kind=job.kind;building.id=job.kind;building.owner=-1;
                r.home=previousHome;
                if(job.kind=="dock"){Ship boat;boat.cell=k+2*mapWidth;ships.push_back(boat);updateFishing(0);}
                if(job.kind=="townhall"){market=k+2*mapWidth;granary=market;r.home=previousHome;}
            }
        }}
    }else if(job.kind=="farm"){success=freeCell();if(success)crops.push_back({k});}
    else if(job.kind=="fallow"){success=cropAt(k)!=nullptr;if(success){crops.erase(std::remove_if(crops.begin(),crops.end(),[=](const Crop& c){return c.cell==k;}),crops.end());fallow[k]=true;}}
    else if(job.kind=="road"){success=!propAt(k)&&!cropAt(k)&&(terrain[k]==Terrain::Ground||terrain[k]==Terrain::FreshWater);if(success){terrain[k]=terrain[k]==Terrain::FreshWater?Terrain::Bridge:Terrain::Road;forest[k]=false;traffic[k]=12;}}
    else if(job.kind=="remove_road"){World trial=*this;trial.terrain[k]=terrain[k]==Terrain::Bridge?Terrain::FreshWater:Terrain::Ground;for(const auto& a:residents)if(a.alive&&!a.inDungeon&&!a.away&&(pos(a)==k||trial.route(static_cast<int>(a.x),static_cast<int>(a.y),[&](int x,int y){return trial.besideFreshWater(x,y);}).empty()))success=false;for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)success=false;if(success){terrain[k]=trial.terrain[k];++communityWood;}}
    else if(job.kind=="demolish"){auto* old=propAt(k);if(!old)success=false;else{const int id=old->uid;const bool tree=old->tree;const int owner=old->owner;props.erase(std::remove_if(props.begin(),props.end(),[=](const Prop& p){return p.uid==id;}),props.end());for(auto& occupant:residents)if(occupant.insideHouse==id)leaveShelter(occupant);communityWood+=tree?4:6;if(tree)forest[k]=false;if(owner>=0&&owner<static_cast<int>(residents.size()))residents[owner].home=market;}}
    else if(job.kind=="move_farm"){auto* old=cropAt(job.source);success=old&&freeCell();if(success){old->cell=k;old->worker=-1;fallow[job.source]=true;}}
    else if(job.kind=="move_road"){success=terrain[job.source]==Terrain::Road&&freeCell();if(success){terrain[job.source]=Terrain::Ground;terrain[k]=Terrain::Road;}}
    if(!success){communityWood+=job.woodCost;treasury+=job.moneyCost;note="cancelo un encargo cuyo terreno cambio; recursos devueltos";}else note="termino "+job.kind;
    jobs.erase(std::remove_if(jobs.begin(),jobs.end(),[&](const Job& j){return j.id==job.id;}),jobs.end());rebuildNavigation();r.job=-1;finish(r,"project",note);
}
std::string World::orderUnit(int who,const std::string& command,int target){
    if(who<0||who>=static_cast<int>(residents.size()))return "Selecciona un habitante";auto& r=residents[who];if(!r.alive||r.inDungeon||r.inBoat)return "La unidad no esta disponible";
    if(command=="stop"){for(auto& boat:ships)if(boat.passenger==who&&boat.state=="boarding"){boat.passenger=-1;boat.state="docked";}for(auto& j:jobs)if(j.worker==who)j.worker=-1;for(auto& c:crops)if(c.worker==who)c.worker=-1;r.hasOrder=false;r.task=Task::Idle;r.path.clear();r.next=0;r.job=-1;r.decisionTimer=cnc::wanderInterval;return "Orden cancelada; vuelve a sus tareas";}
    if(r.mission>=0)return "La unidad tiene una expedicion en curso";
    if(command=="job"){auto it=std::find_if(jobs.begin(),jobs.end(),[=](const Job& j){return j.id==target;});if(it==jobs.end()||(it->worker>=0&&it->worker!=who))return "Encargo no disponible";r.job=it->id;if(!begin(r,Task::Project,it->goal))return "Sin ruta al encargo";it->worker=who;r.hasOrder=true;r.orderedTask=Task::Project;r.orderedCell=it->goal;return "Unidad asignada al encargo";}
    if(!valid(target))return "Fuera del mapa";Task task=Task::Move;int goal=target;
    if(command=="work"){if(auto* c=cropAt(target)){if(c->worker>=0&&c->worker!=who)return "Campo reservado por otro trabajador";if(c->stage==3)task=Task::Harvest;else if(c->stage==0&&r.seeds>0)task=Task::Sow;else return "El cultivo aun esta creciendo o faltan semillas";}
        else if(auto* p=propAt(target)){if(!p->tree)return "La casa bloquea el paso; usa demoler o reubicar";if(protectedLand[target])return "Arbol protegido";task=Task::Gather;goal=nearest(r,[&](int x,int y){return std::abs(x-target%mapWidth)+std::abs(y-target/mapWidth)==1;});}
        else if(terrain[target]==Terrain::FreshWater){task=Task::Drink;goal=nearest(r,[&](int x,int y){return besideFreshWater(x,y)&&std::abs(x-target%mapWidth)+std::abs(y-target/mapWidth)<=2;});}}
    if(!begin(r,task,goal))return "Destino bloqueado o inaccesible";r.hasOrder=true;r.orderedTask=task;r.orderedCell=target;if(task==Task::Gather)r.workCell=target;return "Orden dada a "+r.name;
}
bool World::planCivicWork(){
    if(night()||jobs.size()>=8)return false;
    auto exists=[&](const std::string& kind){return std::any_of(props.begin(),props.end(),[&](const Prop& p){return !p.tree&&p.kind==kind;})||std::any_of(jobs.begin(),jobs.end(),[&](const Job& j){return j.kind==kind;});};
    int adults=0;float social=0;bool inspired=false,voyageNeeded=false;
    for(const auto& r:residents)if(r.alive&&!r.away&&!r.visitor&&!r.inBoat&&!r.inDungeon&&r.ageDays>=World::adultAgeDays){++adults;social+=r.social;inspired|=r.inspiration=="build"&&r.inspirationUntil>=elapsed;voyageNeeded|=(r.inspiration=="diplomacy"||r.inspiration=="raid")&&r.inspirationUntil>=elapsed;}
    if(adults==0)return false;
    std::string kind;
    if((grain<reserve||voyageNeeded)&&!exists("dock")&&communityWood>=20&&treasury>=16)kind="dock";
    else if(social/adults<45&&!exists("altar")&&grain>=reserve&&communityWood>=12&&treasury>=14)kind="altar";
    else if((adults>=8||inspired)&&!exists("townhall")&&grain>=reserve&&communityWood>=28&&treasury>=29)kind="townhall";
    else if(grain>=std::max(reserve,static_cast<int>(std::count_if(residents.begin(),residents.end(),[](const Resident& r){return r.alive&&!r.visitor;}))*2)+2&&communityWood>=12&&treasury>=23&&residents.size()<1000){
        bool vacant=false;for(const auto& p:props)if(!p.tree&&p.kind=="house"&&(p.owner<0||p.owner>=static_cast<int>(residents.size())||!residents[p.owner].alive))vacant=true;
        if(!vacant&&!std::any_of(jobs.begin(),jobs.end(),[](const Job& j){return j.kind=="house";}))kind="house";
    }
    if(kind.empty())return false;
    std::vector<std::pair<int,int>> sites;
    for(int y=1;y<mapHeight-2;++y)for(int x=1;x<mapWidth-2;++x){int k=cellId(x,y),distance=std::abs(x-market%mapWidth)+std::abs(y-market/mapWidth);
        if(distance>(kind=="dock"?100:12)||terrain[k]!=Terrain::Ground||protectedLand[k]||forest[k]||pasture[k]||reserved(k))continue;
        if(kind=="dock"){bool coastal=false;for(int yy=std::max(0,y-2);yy<std::min(mapHeight,y+4);++yy)for(int xx=std::max(0,x-2);xx<std::min(mapWidth,x+4);++xx)if(terrain[cellId(xx,yy)]==Terrain::Sea)coastal=true;if(!coastal)continue;}
        sites.push_back({distance,k});
    }
    std::sort(sites.begin(),sites.end());int attempts=0;
    for(const auto& site:sites){if(++attempts>64)break;auto count=jobs.size();designate(kind,site.second);if(jobs.size()>count){++metrics["autonomous_"+kind];record("Los habitantes decidieron construir "+kind+" por una necesidad de la comunidad");return true;}}
    return false;
}
void World::updateEcology(float dt){
    ecologyClock+=dt;townClock+=dt;
    if(ecologyClock>=30){ecologyClock-=30;for(int attempt=0;attempt<60;++attempt){int k=static_cast<int>(random()%(mapWidth*mapHeight));if(terrain[k]!=Terrain::Ground||protectedLand[k]||pasture[k]||fallow[k]||reserved(k)||cropAt(k)||!walkable(k%mapWidth,k/mapWidth))continue;bool adjacent=false;for(int d:{1,-1,mapWidth,-mapWidth})if(valid(k+d)&&forest[k+d])adjacent=true;if(!adjacent)continue;bool occupied=false;for(const auto& r:residents)if(r.alive&&(pos(r)==k||r.home==k||r.goal==k))occupied=true;for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;if(occupied)continue;World trial=*this;trial.props.push_back({k%mapWidth+.5f,k/mapWidth+.5f,.32f,.32f,2.7f,true,"Arbol joven",nextProp,-1,35});trial.rebuildNavigation();bool safe=true;for(const auto& r:residents)if(r.alive&&!r.inDungeon&&!r.away&&trial.route(static_cast<int>(r.x),static_cast<int>(r.y),[&](int x,int y){return trial.besideFreshWater(x,y);}).empty())safe=false;if(safe){props.push_back(trial.props.back());++nextProp;forest[k]=true;rebuildNavigation();++metrics["regrow"];}break;}}
    if(townClock<20)return;townClock-=20;if(jobs.size()>=8||night())return;
    for(std::size_t i=0;i<residents.size();++i){const auto& r=residents[i];if(!r.alive||r.away||r.visitor||r.ageDays<World::adultAgeDays||r.inDungeon)continue;bool housed=false;for(const auto& p:props)if(!p.tree&&p.kind=="house"&&p.owner==static_cast<int>(i))housed=true;if(!housed&&communityWood>=12&&treasury>=15){for(int dy=-6;dy<=6;++dy)for(int dx=-6;dx<=6;++dx){int x=market%mapWidth+dx,y=market/mapWidth+dy;if(x<0||y<0||x>=mapWidth||y>=mapHeight)continue;int k=cellId(x,y);if(placeable(k)){designate("house",k);return;}}}}
    // Residents condemn unsafe buildings and bring repeatedly worked, remote plots closer to supplies.
    for(const auto& p:props)if(!p.tree&&p.health<30){int k=cellId(static_cast<int>(p.x),static_cast<int>(p.y));if(!protectedLand[k]&&!reserved(k)){designate("demolish",k);return;}}
    if(communityWood>=4&&treasury>=4)for(const auto& p:props)if(!p.tree&&p.kind=="house"&&p.owner>=0&&p.owner<static_cast<int>(residents.size())){
        const auto& owner=residents[p.owner];int source=cellId(static_cast<int>(std::floor(p.x-p.halfWidth)),static_cast<int>(std::floor(p.y-p.halfDepth)));
        if(!owner.alive||owner.away||owner.visitor||owner.energy>=45||reserved(source)||protectedLand[source]||std::abs(source%mapWidth-granary%mapWidth)+std::abs(source/mapWidth-granary/mapWidth)<25)continue;
        for(int dy=-5;dy<=5;++dy)for(int dx=-5;dx<=5;++dx){int x=granary%mapWidth+dx,y=granary/mapWidth+dy;if(x<0||y<0||x>=mapWidth||y>=mapHeight)continue;int destination=cellId(x,y);if(!placeable(destination,source))continue;auto count=jobs.size();relocate(source,destination);if(jobs.size()>count)return;}
    }
    if(grain<reserve&&communityWood>4&&treasury>4)for(const auto& c:crops)if(c.cycles>=4&&!reserved(c.cell)&&std::abs(c.cell%mapWidth-granary%mapWidth)+std::abs(c.cell/mapWidth-granary/mapWidth)>15){
        for(int dy=-5;dy<=5;++dy)for(int dx=-5;dx<=5;++dx){int x=granary%mapWidth+dx,y=granary/mapWidth+dy;if(x<0||y<0||x>=mapWidth||y>=mapHeight)continue;int k=cellId(x,y);auto count=jobs.size();relocate(c.cell,k);if(jobs.size()>count)return;}
    }
    // Recent use drives construction; obsolete roads are removed by a worker.
    int busiest=-1;
    for(int k=0;k<mapWidth*mapHeight;++k){
        if(traffic[k]>0)--traffic[k];
        if(terrain[k]==Terrain::Road&&traffic[k]==0&&!protectedLand[k]&&!reserved(k)){
            bool occupied=false;for(const auto& r:residents)if(r.alive&&(pos(r)==k||r.home==k||r.goal==k))occupied=true;
            for(const auto& a:animals)if(a.alive&&cellId(static_cast<int>(a.x),static_cast<int>(a.y))==k)occupied=true;
            if(!occupied){designate("remove_road",k);return;}
        }
        if(traffic[k]>=6&&terrain[k]==Terrain::Ground&&!forest[k]&&!pasture[k]&&!protectedLand[k]&&!reserved(k)&&!cropAt(k)
           &&(busiest<0||traffic[k]>traffic[busiest]))busiest=k;
    }
    if(busiest>=0&&communityWood>6&&treasury>6){designate("road",busiest);return;}
    planCivicWork();
}
}













