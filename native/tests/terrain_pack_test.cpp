#include "TerrainPack.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
 try {
  if(argc!=2)throw std::runtime_error("Pack path required");
  isla::TerrainPack pack;pack.load(argv[1]);
  isla::World world;world.generateIsland(1749);
  auto original=pack.render(world);
  if(original.size()!=static_cast<std::size_t>(isla::TerrainPack::width)*isla::TerrainPack::height*4)throw std::runtime_error("Unexpected render dimensions");
  for(std::size_t i=3;i<original.size();i+=4)if(original[i]!=255)throw std::runtime_error("Transparent ground pixel");
  world.elevation.fill(20);
  if(original!=pack.render(world))throw std::runtime_error("Height displaced the nadir projection");
  world.terrain.fill(isla::Terrain::Sea);
  if(original==pack.render(world))throw std::runtime_error("World edits did not change terrain render");
  bool rejected=false;try{isla::TerrainPack missing;missing.load(std::string(argv[1])+".does-not-exist");}catch(const std::exception&){rejected=true;}
  if(!rejected)throw std::runtime_error("Missing pack accepted");
  std::cout<<"Top-down pack checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
