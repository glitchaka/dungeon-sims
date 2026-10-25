#pragma once
#include <string>
namespace isla {
class TerrainRenderer;
void loadTerrainMaterials(TerrainRenderer& renderer,const std::string& path);
void loadTerrainBlocks(TerrainRenderer& renderer,const std::string& directory);
}
