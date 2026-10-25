#include "TerrainMaterials.h"
#include "TerrainRenderer.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace isla {
void loadTerrainBlocks(TerrainRenderer& renderer,const std::string& directory) {
    const std::filesystem::path root(directory);
    std::ifstream input(root/"manifest.json");
    if(!input)throw std::runtime_error("Missing modular terrain manifest");
    const auto catalog=nlohmann::json::parse(input);
    if(catalog.at("version")!=3||catalog.at("camera").at("rotate_pixels")!=false)throw std::runtime_error("Unsupported modular terrain catalog");
    std::vector<std::string> names;std::vector<TerrainBlockImage> sheets;
    for(const auto& item:catalog.at("sheets")) {
        const auto name=item.get<std::string>();
        if(std::filesystem::path(name).filename()!=std::filesystem::path(name))throw std::runtime_error("Invalid terrain atlas filename");
        SDL_Surface* original=IMG_Load((root/name).string().c_str());
        if(!original)throw std::runtime_error("Cannot load modular terrain: "+name);
        SDL_Surface* rgba=SDL_ConvertSurface(original,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(original);
        if(!rgba)throw std::runtime_error(SDL_GetError());
        TerrainBlockImage image;image.width=rgba->w;image.height=rgba->h;
        if(image.width<1||image.height<1||image.width>8192||image.height>8192){SDL_DestroySurface(rgba);throw std::runtime_error("Invalid terrain atlas dimensions");}
        try {
            image.rgba.resize(static_cast<std::size_t>(image.width)*image.height*4);
            for(int y=0;y<image.height;++y)std::memcpy(image.rgba.data()+static_cast<std::size_t>(y)*image.width*4,static_cast<unsigned char*>(rgba->pixels)+y*rgba->pitch,image.width*4);
        } catch(...){SDL_DestroySurface(rgba);throw;}
        SDL_DestroySurface(rgba);names.push_back(name);sheets.push_back(std::move(image));
    }
    std::vector<TerrainBlock> blocks;
    for(const auto& definition:catalog.at("blocks")) {
        TerrainBlock block;block.id=definition.at("id");block.category=definition.at("category");block.source=definition.at("sourceRect").get<std::array<int,4>>();
        block.footprint=definition.at("footprint").get<std::vector<std::array<int,2>>>();block.cornerMask=definition.value("cornerMask",-1);
        const auto sheet=definition.at("sheet").get<std::string>();const auto found=std::find(names.begin(),names.end(),sheet);
        if(found==names.end())throw std::runtime_error("Terrain block references an unknown atlas");
        block.sheet=static_cast<int>(found-names.begin());blocks.push_back(std::move(block));
    }
    TerrainBlocks terrain;terrain.assign(std::move(sheets),std::move(blocks));renderer.setBlocks(std::move(terrain));
}

void loadTerrainMaterials(TerrainRenderer& renderer,const std::string& path) {
    SDL_Surface* source=IMG_Load(path.c_str());
    if(!source) throw std::runtime_error("Cannot load terrain-materials-v5.png");
    SDL_Surface* surface=SDL_ConvertSurface(source,SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(source);
    if(!surface) throw std::runtime_error("Cannot convert terrain materials");
    const int width=surface->w,height=surface->h;
    if(width<96 || height<64 || width>8192 || height>8192 || width%3 || height%2) {
        SDL_DestroySurface(surface);
        throw std::runtime_error("Invalid 3x2 material atlas");
    }
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width)*height*4);
    for(int row=0;row<height;++row)
        std::memcpy(pixels.data()+static_cast<std::size_t>(row)*width*4,
            static_cast<unsigned char*>(surface->pixels)+row*surface->pitch,width*4);
    SDL_DestroySurface(surface);
    renderer.setMaterials(std::move(pixels),width,height);
}
}
