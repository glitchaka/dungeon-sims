#pragma once
#include "CncData.h"
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
namespace isla {
constexpr int mapWidth=128,mapHeight=96;
constexpr int cellId(int x,int y){return y*mapWidth+x;}
enum class Terrain {Sea,Ground,FreshWater,Road,Bridge,Mountain};
enum class Task {Idle,Wander,Drink,Eat,Rest,Flee,Bathe,Social,Gather,Camp,Trade,Till,Sow,Harvest,Deposit,Fetch,Hunt,Tend,DrinkHome,Project,Expedition,SellLoot,Move,Fish};
struct Artifact {std::string name,bonus;int value{};bool rare{};};
struct Resident {
    std::string name,trait;
    float x{},y{},hunger{75},thirst{80},energy{85},health{cnc::maxHealth},social{65},fun{65},hygiene{80};
    float speed{},decisionTimer{},actionTimer{},homeWater{4};
    Task task{Task::Idle};std::vector<int> path;std::size_t next{};
    int goal{},target{-1},home{},wood{},meat{},grain{},seeds{12},money{12},eggs{},fish{},job{-1},mission{-1};
    bool alive{true},inDungeon{},inBoat{},resumeRest{},hasOrder{}; Task orderedTask{Task::Idle}; int orderedCell{-1},workCell{-1};
    bool away{},visitor{},chosen{};float ageDays{6480};std::string inspiration;float inspirationUntil{};
    std::vector<int> relationships=std::vector<int>(6,0);std::vector<Artifact> artifacts,loot;
};
struct Prop {float x,y,halfWidth,halfDepth,height;bool tree;std::string id;int uid{-1},owner{-1};float health{100};std::string kind{"house"};};
struct Crop {int cell{},stage{},cycles{},worker{-1};float growth{},ripeAge{};};
struct Animal {float x{},y{},timer{},respawn{};int kind{4};bool alive{true};};
struct Camp {int cell{},owner{};float created{};};
struct Job {int id{},cell{},goal{},worker{-1};std::string kind;int source{-1},phase{},woodCost{},moneyCost{};};
struct Ship {int cell{},voyages{};float remaining{};int passenger{-1},seaHome{-1};float x{},y{},fishingClock{};std::string state{"docked"};std::vector<int> path;std::size_t next{};bool riskResolved{};};
struct Dungeon {
    std::string name,state{"idle"};int entrance{},level{1},room{1},runs{},kills{};
    std::vector<int> party;std::array<float,2> x{{6.5f,6.5f}},y{{5.5f,5.5f}};
    std::array<float,3> enemies{};std::array<bool,3> opened{};
    float clock{},cooldown{};std::vector<std::string> log;
};
struct Landform {std::string kind;float x{},y{},radiusX{},radiusY{},height{},drawWidth{};int sprite{};};
struct Gift {int cell{},grain{12},wood{8};};
struct Visit {std::string id,origin,kind,state;int resident{-1},originResident{-1};float clock{};bool incoming{};std::string partner;};
class World {
public:
    World();void tick(float seconds);
    void generateIsland(std::uint32_t seed);std::string growIsland(int cell,int radius=3);
    std::vector<std::array<float,3>> riverCourse;
    std::vector<Landform> landforms;
    std::uint32_t islandSeed{};bool procedural{};int terrainRevision{},roadPolicy{2};
    std::string playerName{"Deidad"},peerName;std::vector<std::string> chatLog,chatSeen;
    std::string sendChat(const std::string& text);
    static constexpr float daySeconds=300,monthDays=30,gestationSeconds=9*monthDays*daySeconds;
    std::string miracle(const std::string& kind,int who,int cell=-1);
    void updateDivinity(float dt);void updateVisits(float dt);
    std::string sendVisit(int who,const std::string& kind);
    std::string receivePeer(const std::string& message,const std::string& peer);
    std::string worldId,peerId;bool raining{};float chosenBirthAt{-1};int chosenPatron{-1};
    std::map<std::string,int> relations;std::vector<Gift> gifts;std::vector<Visit> visits;std::map<std::string,std::string> outbox;
    float heightAt(float x,float y) const;bool canStep(int x,int y,int nx,int ny) const;
    std::array<float,mapWidth*mapHeight> elevation{};
    bool walkable(int x,int y) const;Terrain terrainAt(int x,int y) const;bool besideFreshWater(int x,int y) const;
    std::vector<int> route(int sx,int sy,const std::function<bool(int,int)>& goal) const;
    void alarm(float x,float y);void record(const std::string& message);
    std::string designate(const std::string& kind,int cell);
    std::string relocate(int source,int destination);
    std::string orderUnit(int who,const std::string& command,int target);
    Prop* propAt(int cell); bool reserved(int cell) const; bool placeable(int cell,int ignoreSource=-1) const;
    void updateEcology(float dt); void seedEnvironment();
    std::string startFishing(int who,int boat);void updateFishing(float dt);
    static bool fishingLoss(float roll){return roll<.25f;}
    std::string dispatch(int index);std::string requestExpedition(int index);
    bool save(const std::string& path) const;bool load(const std::string& path);
    bool night() const;bool hasBonus(const Resident& r,const std::string& bonus) const;
    Artifact makeArtifact(float roll,int variant,int level) const;
    Crop* cropAt(int cell);void rebuildNavigation();
    static const char* taskName(Task task);
    std::array<Terrain,mapWidth*mapHeight> terrain{};
    std::array<bool,mapWidth*mapHeight> forest{},pasture{},protectedLand{},fallow{};
    std::vector<Resident> residents;std::vector<Prop> props;std::vector<Crop> crops;
    std::vector<Animal> animals;std::vector<Camp> camps;std::vector<Job> jobs;
    std::vector<Ship> ships;std::vector<Dungeon> dungeons;std::vector<std::string> events;
    std::map<std::string,int> metrics;
    float elapsed{93},harvestClock{},alarmSeconds{},alarmX{},alarmY{},livestockClock{},lastDispatch{};
    int grain{12},harvests{},communityWood{24},treasury{60},herbs{},reserve{30},livestockFood{6},priority{},nextJob{};
    int market{cellId(59,46)},granary{cellId(58,47)};bool autoTrade{};float ecologyClock{},townClock{},populationClock{};int nextProp{};std::array<unsigned short,mapWidth*mapHeight> traffic{};
private:
    std::array<bool,mapWidth*mapHeight> blocked_{};std::uint32_t randomState_{1749};
    std::uint32_t random();void plan(Resident& r);void addProp(Prop p);
    bool begin(Resident& r,Task task,int goal);void finish(Resident& r,const std::string& metric,const std::string& note);
    void act(Resident& r,float dt);void updateRealm(float dt);void updateDungeons(float dt);
    void updateCrops(float dt);void updateAnimals(float dt);void completeJob(Resident& r);
    int nearest(const Resident& r,const std::function<bool(int,int)>& predicate) const;
    void feed(Resident& r);void rest(Resident& r);bool farmTask(Resident& r);bool gatherTask(Resident& r);
};
}










