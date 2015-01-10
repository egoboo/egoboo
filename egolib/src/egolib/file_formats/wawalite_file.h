//********************************************************************************************
//*
//*    This file is part of Egoboo.
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

/// @file egolib/file_formats/wawalite_file.h
/// @details loading the environment definitions for a module

#pragma once

#include "egolib/_math.h"
#include "egolib/vec.h"

#if 0
#if defined(__cplusplus)
extern "C"
{
#endif
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct wawalite_water_layer_t;
#if 0
    typedef struct s_wawalite_water_layer wawalite_water_layer_t;
#endif
    struct wawalite_water_t;
#if 0
    typedef struct s_wawalite_water wawalite_water_t;
#endif
    struct wawalite_physics_t;
#if 0
    typedef struct s_wawalite_physics wawalite_physics_t;
#endif
    struct wawalite_animtile_t;
#if 0
    typedef struct s_wawalite_animtile wawalite_animtile_t;
#endif
    struct wawalite_damagetile_t;
#if 0
    typedef struct s_wawalite_damagetile wawalite_damagetile_t;
#endif
    struct wawalite_weather_t;
#if 0
    typedef struct s_wawalite_weather wawalite_weather_t;
#endif
    struct wawalite_graphics_t;
#if 0
    typedef struct s_wawalite_graphics wawalite_graphics_t;
#endif
    struct wawalite_camera_t;
#if 0
    typedef struct s_wawalite_camera wawalite_camera_t;
#endif
    struct wawalite_fog_t;
#if 0
    typedef struct s_wawalite_fog wawalite_fog_t;
#endif
    struct wawalite_data_t;
#if 0
    typedef struct s_wawalite_data wawalite_data_t;
#endif
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWATERLAYER 2                             ///< Maximum water layers

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the water layer data in "wawalite.txt"
    struct wawalite_water_layer_t
    {
        Uint32  frame_add;      ///< Speed

        float   z;                ///< Base height of water
        float   amp;            ///< Amplitude of waves

        fvec2_t dist;            ///< For distant backgrounds
        Uint32  light_dir;        ///< direct  reflectivity 0 - 63
        Uint32  light_add;        ///< ambient reflectivity 0 - 63

        fvec2_t tx_add;            ///< Texture movement
        Uint8   alpha;            ///< Transparency
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the water data in "wawalite.txt"
    struct wawalite_water_t
    {
        int                    layer_count;
        wawalite_water_layer_t layer[MAXWATERLAYER];

        float  surface_level;    ///< Surface level for water striders
        float  douse_level;         ///< Surface level for torches
        Uint8  spek_start;         ///< Specular begins at which light value
        Uint8  spek_level;         ///< General specular amount (0-255)
        bool is_water;         ///< Is it water?  ( Or lava... )
        bool overlay_req;
        bool background_req;

        bool light;            ///< Is it light ( default is alpha )

        float  foregroundrepeat;
        float  backgroundrepeat;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the physics data in "wawalite.txt"
    struct wawalite_physics_t
    {
        float hillslide;
        float slippyfriction;
        float airfriction;
        float waterfriction;
        float noslipfriction;
        float gravity;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the animated tile data in "wawalite.txt"
    struct wawalite_animtile_t
    {
        Uint32 update_and;
        Uint32 frame_and;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the damagetile data in "wawalite.txt"
    struct wawalite_damagetile_t
    {
        Uint32 amount;
        int    damagetype;

        int    part_gpip;
        Uint32 partand;
        int    sound_index;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the weather data in "wawalite.txt"
    struct wawalite_weather_t
    {
        bool  over_water;
        int     timer_reset;
        int     part_gpip;           ///< Which particle to spawn?
        STRING  weather_name;
    };

//--------------------------------------------------------------------------------------------

    /// A wrapper for the graphics data in "wawalite.txt"
    struct wawalite_graphics_t
    {
        bool exploremode;
        bool usefaredge;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the camera data in "wawalite.txt"
    struct wawalite_camera_t
    {
        bool swing;
        float  swing_rate;
        float  swing_amp;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the fog data in "wawalite.txt"
    struct wawalite_fog_t
    {
        bool found;
        float  top;
        float  bottom;
        float  red;
        float  grn;
        float  blu;
        bool affects_water;
    };

//--------------------------------------------------------------------------------------------

/// An internal representation of the data in "wawalite.txt"
    struct wawalite_data_t
    {
        Uint32 seed;
        Sint8 version;

        wawalite_water_t      water;
        wawalite_physics_t    phys;
        wawalite_animtile_t   animtile;
        wawalite_damagetile_t damagetile;

        wawalite_weather_t    weather;
        wawalite_graphics_t   graphics;
        wawalite_camera_t     camera;
        wawalite_fog_t        fog;

        float light_x;
        float light_y;
        float light_z;
        float light_a;
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern wawalite_data_t wawalite_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    bool            write_wawalite_file_vfs( const wawalite_data_t * pdata );
    wawalite_data_t * read_wawalite_file_vfs( const char *filename, wawalite_data_t * pdata );

    wawalite_data_t * wawalite_limit( wawalite_data_t * pdata );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
#if defined(__cplusplus)
}
#endif
#endif