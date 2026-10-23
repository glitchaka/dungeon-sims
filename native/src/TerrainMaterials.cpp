#include "TerrainMaterials.h"
#include "TerrainRenderer.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstring>
#include <stdexcept>

namespace isla {
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
