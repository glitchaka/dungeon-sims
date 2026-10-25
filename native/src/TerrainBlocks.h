#pragma once
#include <array>
#include <string>
#include <vector>

namespace isla {
class World;
struct TerrainBlockImage {
    int width{},height{};
    std::vector<unsigned char> rgba;
};
struct TerrainBlock {
    std::string id,category;
    int sheet{},cornerMask{-1};
    std::array<int,4> source{};
    std::vector<std::array<int,2>> footprint;
};
struct TerrainBlockPlacement {const TerrainBlock* block{};int x{},y{},step{};};
// Composes the original directional art; never rotates or mirrors an atlas.
// Layout is derived from the current physical world, including terrain edits.
class TerrainBlocks {
public:
    void assign(std::vector<TerrainBlockImage> sheets,std::vector<TerrainBlock> blocks);
    bool empty() const{return blocks_.empty();}
    const std::vector<TerrainBlockImage>& images() const{return sheets_;}
    const std::vector<TerrainBlock>& definitions() const{return blocks_;}
    std::vector<TerrainBlockPlacement> layout(const World& world) const;
    static std::array<float,4> bounds(const World& world,const TerrainBlockPlacement& placement,float scale);
    void compose(const World& world,std::vector<unsigned char>& pixels,int scale) const;
private:
    std::vector<TerrainBlockImage> sheets_;
    std::vector<TerrainBlock> blocks_;
};
}
