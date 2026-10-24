#include "TerrainRenderer.h"
#include "TerrainProjection.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){
 try {

  isla::TerrainRenderer pack;
  isla::World world;world.generateIsland(1749);
  const float x=world.market%isla::mapWidth+.5f,y=world.market/isla::mapWidth+.5f;
  const float projected=y-isla::TerrainProjection::elevation(world,x,y);
  const auto picked=isla::TerrainProjection::groundPoint(world,x,projected);
  if(std::abs(picked.first-x)>.001f||std::abs(picked.second-y)>.001f)
      throw std::runtime_error("Mouse picking does not use the rendered height projection");
  auto original=pack.render(world);
  if(original.size()!=static_cast<std::size_t>(isla::TerrainRenderer::width)*isla::TerrainRenderer::height*4)throw std::runtime_error("Unexpected render dimensions");
  for(std::size_t i=3;i<original.size();i+=4)if(original[i]!=255)throw std::runtime_error("Transparent ground pixel");
  world.elevation.fill(20);
  const auto elevatedPick=isla::TerrainProjection::groundPoint(world,100,80-20*isla::TerrainProjection::heightScale);
  if(std::abs(elevatedPick.second-80)>.001f)throw std::runtime_error("Elevated surface picking is inconsistent");
  if(original==pack.render(world))throw std::runtime_error("Physical height does not affect visible relief");
  world.terrain.fill(isla::Terrain::Sea);
  if(original==pack.render(world))throw std::runtime_error("World edits did not change terrain render");
  std::cout<<"Procedural relief responds to physical elevation without a terrain atlas\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

