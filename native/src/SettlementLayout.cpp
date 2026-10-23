#include "SettlementLayout.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace isla {
namespace {
std::vector<int> candidates(const World& world, int radius, std::uint32_t seed) {
    std::vector<int> cells;
    const int cx = world.market%mapWidth, cy = world.market/mapWidth;
    for (int y = cy-radius; y <= cy+radius; ++y)
        for (int x = cx-radius; x <= cx+radius; ++x) {
            if (x < 2 || y < 2 || x >= mapWidth-2 || y >= mapHeight-3) continue;
            if (std::hypot(static_cast<float>(x-cx),static_cast<float>(y-cy)) > radius) continue;
            const int cell = cellId(x,y);
            if (cell != world.market && cell != world.granary && world.walkable(x,y)) cells.push_back(cell);
        }
    std::mt19937 random(seed);
    for (std::size_t i = cells.size(); i > 1; --i) std::swap(cells[i-1],cells[random()%i]);
    return cells;
}
}

void SettlementLayout::placeResidents(World& world, std::uint32_t seed) {
    const auto cells = candidates(world,6,seed^0x95ab137u);
    if (cells.size() < world.residents.size()) throw std::runtime_error("Insufficient settlement ground");
    for (std::size_t i = 0; i < world.residents.size(); ++i) {
        auto& resident = world.residents[i];
        resident.x = cells[i]%mapWidth+.5f;
        resident.y = cells[i]/mapWidth+.5f;
        resident.home = resident.goal = cells[i];
        resident.task = Task::Idle;
        resident.path.clear();
        resident.next = 0;
    }
}

void SettlementLayout::placeHomes(World& world, std::uint32_t seed) {
    const auto cells = candidates(world,16,seed^0xfa951237u);
    for (std::size_t i = 0; i < world.residents.size(); ++i) {
        bool placed = false;
        for (int cell : cells) {
            const int x = cell%mapWidth, y = cell/mapWidth;
            bool tooClose = false;
            for (const auto& prop : world.props)
                if (!prop.tree && std::hypot(prop.x-(x+1.0f),prop.y-(y+1.0f)) < 5) tooClose = true;
            if (tooClose || !world.walkable(x,y+2) || !world.placeable(cell)) continue;
            world.props.push_back({x+1.0f,y+1.0f,.9f,.9f,3,false,"Casa",world.nextProp++,static_cast<int>(i),100});
            world.residents[i].home = cell+2*mapWidth;
            world.rebuildNavigation();
            placed = true;
            break;
        }
        if (!placed) throw std::runtime_error("Cannot place an accessible home");
    }
}
}
