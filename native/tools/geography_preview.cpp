#include "TerrainRenderer.h"
#include "TerrainMaterials.h"
#include <filesystem>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc,char** argv) {
    try {
        if(argc<2) throw std::runtime_error("Usage: isla_geography_preview output.bmp [seed]");
        auto world=std::make_unique<isla::World>();
        world->generateIsland(argc>2?static_cast<std::uint32_t>(std::stoul(argv[2])):1749u);
        isla::TerrainRenderer renderer;
        isla::loadTerrainMaterials(renderer,(std::filesystem::absolute(argv[0]).parent_path()/"assets/terrain-materials-v5.png").string());
        auto pixels=renderer.render(*world);
        for(std::size_t i=0;i<pixels.size();i+=4) std::swap(pixels[i],pixels[i+2]);
        std::array<unsigned char,54> header{};
        const auto field=[&](int offset,std::uint32_t value) {
            for(int i=0;i<4;++i) header[offset+i]=static_cast<unsigned char>(value>>(i*8));
        };
        header[0]='B';header[1]='M';
        field(2,static_cast<std::uint32_t>(54+pixels.size()));
        field(10,54);field(14,40);
        field(18,isla::TerrainRenderer::width);
        field(22,static_cast<std::uint32_t>(-isla::TerrainRenderer::height));
        header[26]=1;header[28]=32;
        field(34,static_cast<std::uint32_t>(pixels.size()));
        std::ofstream output(argv[1],std::ios::binary);
        output.write(reinterpret_cast<const char*>(header.data()),header.size());
        output.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
        if(!output) throw std::runtime_error("Cannot write geography preview");
        std::cout<<"Rendered actual world height field to "<<argv[1]<<'\n';
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
