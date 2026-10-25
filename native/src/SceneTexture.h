#pragma once
#include <ra/Engine.h>
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include <vector>

namespace isla {
// Local rendering adapter: the framework's pixel-art textures remain unchanged.
// Detailed scenery uses premultiplied alpha and a mip chain to avoid shimmer.
class SceneTexture {
public:
    ~SceneTexture(){reset();}
    SceneTexture()=default;
    SceneTexture(const SceneTexture&)=delete;
    SceneTexture& operator=(const SceneTexture&)=delete;
    void load(SDL_Renderer* renderer,const std::string& path,int spriteCount=0);
    void loadGrid(SDL_Renderer* renderer,const std::string& path,int columns,int rows);
    ra::Rect gridFrame(int frame) const;
    void upload(SDL_Renderer* renderer,const unsigned char* rgba,int imageWidth,int imageHeight);
    void draw(ra::Rect destination) const;
    void draw(ra::Rect source,ra::Rect destination) const;
    void drawGeometry(const std::vector<SDL_Vertex>& vertices,const std::vector<int>& indices,float minification=1) const;
    void reset();
    explicit operator bool() const{return !levels_.empty();}
    int width{},height{};
    std::array<ra::Rect,8> sprites{};
private:
    SDL_Renderer* renderer_{};
    int columns_{},rows_{};
    std::vector<SDL_Texture*> levels_;
    void build(SDL_Renderer* renderer,SDL_Surface* source);
    void extract(SDL_Surface* source,int spriteCount);
};
}
