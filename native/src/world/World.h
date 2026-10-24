#pragma once
#include "ResidentRegistry.h"
#include "entities/WorldEntities.h"
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
namespace isla {
constexpr int mapWidth=256,mapHeight=192;
constexpr int cellId(int x,int y){return y*mapWidth+x;}
enum class Terrain {Sea,Ground,FreshWater,Road,Bridge,Mountain};
class World {
public:
    World();void tick(float seconds);
    void generateIsland(std::uint32_t seed);std::string growIsland(int cell,int radius=3);
    std::vector<std::array<float,3>> riverCourse;
    std::vector<Landform> landforms;
    std::uint32_t islandSeed{};bool procedural{};int terrainRevision{},roadPolicy{2};
    std::string playerName{"Deidad"},peerName;std::vector<std::string> chatLog,chatSeen;
    std::string sendChat(const std::string& text);
    static constexpr float daySeconds=300,monthDays=30,gestationSeconds=9*monthDays*daySeconds,adultAgeDays=18*12*monthDays,childhoodDays=6*12*monthDays,elderAgeDays=65*12*monthDays;
    std::string miracle(const std::string& kind,int who,int cell=-1);
    void updateLifecycle(float dt);static const char* lifeStageName(const Resident& resident);void updateFamilies(float dt);bool leaveShelter(Resident& resident);void updateDivinity(float dt);void updateVisits(float dt);
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
    ResidentRegistry residents;std::vector<Prop> props;std::vector<Crop> crops;
    std::vector<SeaFish> seaFish;
    double mysteriousReadyAt{};
    std::vector<Animal> animals;std::vector<Camp> camps;std::vector<Job> jobs;
    std::vector<Ship> ships;std::vector<Dungeon> dungeons;std::vector<std::string> events;
    std::map<std::string,int> metrics;
    double elapsed{93};float harvestClock{},alarmSeconds{},alarmX{},alarmY{},livestockClock{},lastDispatch{};
    int grain{12},harvests{},communityWood{24},treasury{60},herbs{},reserve{30},livestockFood{6},priority{},nextJob{};
    int market{cellId(59,46)},granary{cellId(58,47)};bool autoTrade{};float ecologyClock{},townClock{},populationClock{};int nextProp{};std::array<unsigned short,mapWidth*mapHeight> traffic{};
private:
    friend class ResidentController;
    friend class ResidentActions;
    friend class ResidentSimulation;
    std::array<bool,mapWidth*mapHeight> blocked_{};std::uint32_t randomState_{1749};
    std::uint32_t random();bool planCivicWork();bool seekFamily(Resident& resident);void addProp(Prop p);
    bool begin(Resident& r,Task task,int goal);void finish(Resident& r,const std::string& metric,const std::string& note);
    void updateRealm(float dt);void updateDungeons(float dt);
    void updateCrops(float dt);void updateAnimals(float dt);void completeJob(Resident& r);
    int nearest(const Resident& r,const std::function<bool(int,int)>& predicate) const;
};
}












