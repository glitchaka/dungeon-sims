#pragma once
#include "World.h"
#include <string>
#include <vector>
namespace isla {
class TerrainPack {
public:
    void load(const std::string& path);std::vector<unsigned char> render(const World& world) const;
    static constexpr int scale=12,width=mapWidth*scale,height=mapHeight*scale;
private:int atlasWidth_{},atlasHeight_{};std::vector<unsigned char> pixels_;
};
}
