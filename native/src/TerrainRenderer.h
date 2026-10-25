#pragma once
#include "World.h"
#include "TerrainBlocks.h"
#include <vector>
#include <utility>

namespace isla {
// Physical heightfield with a shared projection for drawing and picking.
// Runtime terrain art is composed from the original modular block catalog.
class TerrainRenderer {
public:
    void setMaterials(std::vector<unsigned char> pixels,int width,int height);
    void setBlocks(TerrainBlocks blocks){blocks_=std::move(blocks);}
    const TerrainBlocks& blocks() const{return blocks_;}
    std::vector<unsigned char> render(const World& world,bool composeBlocks=true) const;
    static constexpr int scale=12,width=mapWidth*scale,height=mapHeight*scale;
private:
    std::vector<unsigned char> materials_;
    int materialWidth_{},materialHeight_{};
    TerrainBlocks blocks_;
};
}
