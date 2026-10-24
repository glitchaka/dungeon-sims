#pragma once
#include <SDL3/SDL.h>

namespace isla {
// Small procedural particles and emotes; never alters simulation state.
class ResidentEffects {
public:
    static void heart(SDL_Renderer*,float x,float y,float size);
    static void surprise(SDL_Renderer*,float x,float y,float size);

    static void impact(SDL_Renderer*,float x,float y,float size,double phase,bool wood);
};
}

