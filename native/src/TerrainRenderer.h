#pragma once
#include "World.h"
#include <vector>

namespace isla {
// Heightfield relief with a shared projection for drawing and picking.
// Material swatches contain no painted geography or baked world objects.
class TerrainRenderer {
public:
    void setMaterials(std::vector<unsigned char> pixels,int width,int height);
    std::vector<unsigned char> render(const World& world) const;
    static constexpr int scale=12,width=mapWidth*scale,height=mapHeight*scale;
private:
    std::vector<unsigned char> materials_;
    int materialWidth_{},materialHeight_{};
};
}
