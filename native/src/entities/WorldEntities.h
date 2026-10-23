#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <vector>
namespace isla {
struct Prop {float x,y,halfWidth,halfDepth,height;bool tree;std::string id;int uid{-1},owner{-1};float health{100};std::string kind{"house"};};
struct Crop {int cell{},stage{},cycles{},worker{-1};float growth{},ripeAge{};};
struct Animal {float x{},y{},timer{},respawn{};int kind{4};bool alive{true};};
struct SeaFish {float x{},y{},heading{},timer{};bool alive{true};};
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
}
