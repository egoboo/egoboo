#pragma once

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

#include "egolib/_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_wawalite_water_layer;
    typedef struct s_wawalite_water_layer wawalite_water_layer_t;

    struct s_wawalite_water;
    typedef struct s_wawalite_water wawalite_water_t;

    struct s_wawalite_physics;
    typedef struct s_wawalite_physics wawalite_physics_t;

    struct s_wawalite_animtile;
    typedef struct s_wawalite_animtile wawalite_animtile_t;

    struct s_wawalite_damagetile;
    typedef struct s_wawalite_damagetile wawalite_damagetile_t;

    struct s_wawalite_weather;
    typedef struct s_wawalite_weather wawalite_weather_t;

    struct s_wawalite_graphics;
    typedef struct s_wawalite_graphics wawalite_graphics_t;

    struct s_wawalite_camera;
    typedef struct s_wawalite_camera wawalite_camera_t;

    struct s_wawalite_fog;
    typedef struct s_wawalite_fog wawalite_fog_t;

    struct s_wawalite_data;
    typedef struct s_wawalite_data wawalite_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWATERLAYER 2                             ///< Maximum water layers

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the water layer data in "wawalite.txt"
    struct s_wawalite_water_layer
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
    struct s_wawalite_water
    {
        int                    layer_count;
        wawalite_water_layer_t layer[MAXWATERLAYER];

        float  surface_level;    ///< Surface level for water striders
        float  douse_level;         ///< Surface level for torches
        Uint8  spek_start;         ///< Specular begins at which light value
        Uint8  spek_level;         ///< General specular amount (0-255)
        C_BOOLEAN is_water;         ///< Is it water?  ( Or lava... )
        C_BOOLEAN overlay_req;
        C_BOOLEAN background_req;

        C_BOOLEAN light;            ///< Is it light ( default is alpha )

        float  foregroundrepeat;
        float  backgroundrepeat;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the physics data in "wawalite.txt"
    struct s_wawalite_physics
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
    struct s_wawalite_animtile
    {
        Uint32 update_and;
        Uint32 frame_and;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the damagetile data in "wawalite.txt"
    struct s_wawalite_damagetile
    {
        Uint32 amount;
        int    damagetype;

        int    part_gpip;
        Uint32 partand;
        int    sound_index;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the weather data in "wawalite.txt"
    struct s_wawalite_weather
    {
        C_BOOLEAN  over_water;
        int     timer_reset;
        int     part_gpip;           ///< Which particle to spawn?
        STRING  weather_name;
    };

//--------------------------------------------------------------------------------------------

    /// A wrapper for the graphics data in "wawalite.txt"
    struct s_wawalite_graphics
    {
        C_BOOLEAN exploremode;
        C_BOOLEAN usefaredge;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the camera data in "wawalite.txt"
    struct s_wawalite_camera
    {
        C_BOOLEAN swing;
        float  swing_rate;
        float  swing_amp;
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the fog data in "wawalite.txt"
    struct s_wawalite_fog
    {
        C_BOOLEAN found;
        float  top;
        float  bottom;
        float  red;
        float  grn;
        float  blu;
        C_BOOLEAN affects_water;
    };

//--------------------------------------------------------------------------------------------

/// An internal representation of the data in "wawalite.txt"
    struct s_wawalite_data
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

    C_BOOLEAN            write_wawalite_file_vfs( const wawalite_data_t * pdata );
    wawalite_data_t * read_wawalite_file_vfs( const char *filename, wawalite_data_t * pdata );

    wawalite_data_t * wawalite_limit( wawalite_data_t * pdata );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_formats_wawalite_h
