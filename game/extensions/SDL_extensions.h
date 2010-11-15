#pragma once
//********************************************************************************************
//*
//*    This file is part of the SDL extensions library.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @defgroup _sdl_extensions_ Extensions to SDL

/// @file extensions/SDL_extensions.h
/// @ingroup _sdl_extensions_
/// @brief Definitions for generic extensions to SDL
/// @details

#include <SDL.h>

#include <assert.h>
#include "file_common.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// bitwise operations
/*#if !defined(BOOL_TO_BIT)
#    define BOOL_TO_BIT(X)       ((X) ? 1 : 0 )
#endif

#if !defined(BIT_TO_BOOL)
#    define BIT_TO_BOOL(X)       ((1 == X) ? SDL_TRUE : SDL_FALSE )
#endif

#if !defined(HAS_SOME_BITS)
#    define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#endif

#if !defined(HAS_ALL_BITS)
#    define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#endif

#if !defined(HAS_NO_BITS)
#    define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#endif

#if !defined(MISSING_BITS)
#    define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))
#endif*/

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_oglx_video_parameters;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A structure holding some of SDL's video data
struct s_SDLX_sdl_video_flags
{
    unsigned hw_surface: 1;       ///< SDL_HWSURFACE   - Surface is in video memory
    unsigned async_blit: 1;       ///< SDL_ASYNCBLIT   - Use asynchronous blits if possible
    unsigned any_format: 1;       ///< SDL_ANYFORMAT   - Allow any video depth/pixel-format
    unsigned hw_palette: 1;       ///< SDL_HWPALETTE   - Surface has exclusive palette
    unsigned double_buf: 1;       ///< SDL_DOUBLEBUF   - Set up double-buffered video mode
    unsigned full_screen: 1;      ///< SDL_FULLSCREEN  - Surface is a full screen display
    unsigned opengl: 1;           ///< SDL_OPENGL      - Create an OpenGL rendering context
    unsigned opengl_blit: 1;      ///< SDL_OPENGLBLIT  - Create an OpenGL rendering context and use it for blitting
    unsigned resizable: 1;        ///< SDL_RESIZABLE   - This video mode may be resized
    unsigned no_frame: 1;         ///< SDL_NOFRAME     - No window caption or edge frame

    // read-only data
    unsigned use_hwaccel: 1;      ///< SDL_HWACCEL     - Surface blit uses hardware acceleration
    unsigned has_srccolorkey: 1;  ///< SDL_SRCCOLORKEY - Surface use colorkey blitting
    unsigned use_rleaccelok: 1;   ///< SDL_RLEACCELOK  - Private flag
    unsigned use_rleaccel: 1;     ///< SDL_RLEACCEL    - Colorkey blitting is accelerated with RLE
    unsigned use_srcalpha: 1;     ///< SDL_SRCALPHA    - Surface blit uses alpha blending
    unsigned is_prealloc: 1;      ///< SDL_PREALLOC    - Surface uses preallocated memory

};
typedef struct s_SDLX_sdl_video_flags SDLX_sdl_video_flags_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A structure holding all of the OpenGL data that can be queried through SDL
struct s_SDLX_sdl_gl_attrib
{
    // SDL_GL_* attribute data
    int color[4];           ///< SDL_GL_RED_SIZE, SDL_GL_GREEN_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_ALPHA_SIZE
    int buffer_size;        ///< SDL_GL_BUFFER_SIZE
    int doublebuffer;       ///< SDL_GL_DOUBLEBUFFER
    int depth_size;         ///< SDL_GL_DEPTH_SIZE
    int stencil_size;       ///< SDL_GL_STENCIL_SIZE
    int accum[4];           ///< SDL_GL_ACCUM_RED_SIZE, SDL_GL_ACCUM_GREEN_SIZE, SDL_GL_ACCUM_BLUE_SIZE, SDL_GL_ACCUM_ALPHA_SIZE
    int stereo;             ///< SDL_GL_STEREO
    int multi_buffers;      ///< SDL_GL_MULTISAMPLEBUFFERS
    int multi_samples;      ///< SDL_GL_MULTISAMPLESAMPLES
    int accelerated_visual; ///< SDL_GL_ACCELERATED_VISUAL
    int swap_control;       ///< SDL_GL_SWAP_CONTROL
};
typedef struct s_SDLX_sdl_gl_attrib SDLX_sdl_gl_attrib_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A representation of a SDL Screen state
struct s_SDLX_screen_info
{
    // JF - Added so that the video mode might be determined outside of the graphics code
    SDL_Surface * pscreen;

    SDL_Rect **   video_mode_list;

    char          szDriver[256];    ///< graphics driver name;

    int           d;                ///< Screen bit depth
    int           x;                ///< Screen X size
    int           y;                ///< Screen Y size

    // SDL OpenGL attributes
    SDLX_sdl_gl_attrib_t gl_att;

    // bitfield for SDL video flags
    SDLX_sdl_video_flags_t flags;

    // selected SDL bitfields
    unsigned hw_available: 1;
    unsigned wm_available: 1;
    unsigned blit_hw: 1;
    unsigned blit_hw_CC: 1;
    unsigned blit_hw_A: 1;
    unsigned blit_sw: 1;
    unsigned blit_sw_CC: 1;
    unsigned blit_sw_A: 1;
};
typedef struct s_SDLX_screen_info SDLX_screen_info_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Parameters for setting an SDL video state
struct s_SDLX_video_parameters
{
    int    width;
    int    height;
    int    depth;

    SDLX_sdl_video_flags_t flags;
    SDLX_sdl_gl_attrib_t   gl_att;

    SDL_Surface * surface;
};

typedef struct s_SDLX_video_parameters SDLX_video_parameters_t;

SDL_bool SDLX_video_parameters_default( SDLX_video_parameters_t * v );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern SDLX_screen_info_t sdl_scr;

extern const Uint32 rmask;
extern const Uint32 gmask;
extern const Uint32 bmask;
extern const Uint32 amask;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Grab the current SDL screen information
SDL_bool      SDLX_Get_Screen_Info( SDLX_screen_info_t * psi, SDL_bool display );

/// Use a SDLX_video_parameters_t structure to request a SDL video mode
SDL_Surface * SDLX_RequestVideoMode( SDLX_video_parameters_t * v, SDL_bool make_report );

/// Use a SDLX_video_parameters_t structure to try to set a SDL video mode directly
/// on success, it returns a pointer to the actual data used to set the mode. On failure,
/// it resets the mode to v_old (if possible), and returns a pointer to the restored parameters
SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, SDL_bool has_valid_mode, SDL_bool make_report );

/// Determine the minimum changes to the current pixel format to accomodate the requested format
// Mostly used to add an alpha channel to a SDL_Surface
SDL_bool SDLX_ExpandFormat( SDL_PixelFormat * pformat );

/// Set the FILE that SDL_extensions will use to dump debugging information.
/// If not set, it will default to stdout.
FILE * SDLX_set_stdout( FILE * pfile );

/// Dump the info on the given surface to whatever FILE SDL_extensions is using for stdout
void   SDLX_report_mode( SDL_Surface * surface, SDLX_video_parameters_t * v );

