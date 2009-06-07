#pragma once

#include "ogl_extensions.h"
#include "SDL_extensions.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int powerOfTwo( int input );
    SDL_bool SDL_GL_uploadSurface( SDL_Surface *surface, GLuint texture, GLfloat *texCoords );
    SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new);

    extern const Uint32 sdl_a_mask;
    extern const Uint32 sdl_b_mask;
    extern const Uint32 sdl_g_mask;
    extern const Uint32 sdl_r_mask;

    extern const Uint32 sdl_a_shift;
    extern const Uint32 sdl_b_shift;
    extern const Uint32 sdl_g_shift;
    extern const Uint32 sdl_r_shift;

#ifdef __cplusplus
};
#endif
