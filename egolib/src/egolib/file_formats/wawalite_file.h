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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct wawalite_water_layer_t;
    struct wawalite_water_t;
    struct wawalite_physics_t;
    struct wawalite_animtile_t;
    struct wawalite_damagetile_t;
    struct wawalite_weather_t;
    struct wawalite_graphics_t;
    struct wawalite_camera_t;
    struct wawalite_fog_t;
    struct wawalite_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWATERLAYER 2                             ///< Maximum water layers

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the water layer data in "wawalite.txt"
    struct wawalite_water_layer_t
    {
        uint32_t  frame_add;      ///< Speed

        float   z;                ///< Base height of water
        float   amp;            ///< Amplitude of waves

        fvec2_t dist;            ///< For distant backgrounds
        uint32_t  light_dir;        ///< direct  reflectivity 0 - 63
        uint32_t  light_add;        ///< ambient reflectivity 0 - 63

        fvec2_t tx_add;            ///< Texture movement
        uint8_t   alpha;            ///< Transparency

        wawalite_water_layer_t() :
            frame_add(0),
            z(0.0f),
            amp(0.0f),
            dist(0, 0),
            light_dir(0),
            light_add(0),
            tx_add(0, 0),
            alpha(0)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the water data in "wawalite.txt"
    struct wawalite_water_t
    {
        int                    layer_count;
        wawalite_water_layer_t layer[MAXWATERLAYER];

        float  surface_level;    ///< Surface level for water striders
        float  douse_level;         ///< Surface level for torches
        uint8_t  spek_start;         ///< Specular begins at which light value
        uint8_t  spek_level;         ///< General specular amount (0-255)
        bool is_water;         ///< Is it water?  ( Or lava... )
        bool overlay_req;
        bool background_req;

        bool light;            ///< Is it light ( default is alpha )

        float  foregroundrepeat;
        float  backgroundrepeat;

        wawalite_water_t() :
            layer_count(0),
            layer(),
            surface_level(0.0f),
            douse_level(0.0f),
            spek_start(0),
            spek_level(0),
            is_water(true),
            overlay_req(false),
            background_req(false),
            light(false),
            foregroundrepeat(0.0f),
            backgroundrepeat(0.0f)
        {
            //default ctor            
        }
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

        wawalite_physics_t() :
            hillslide(0.0f),
            slippyfriction(0.0f),
            airfriction(0.0f),
            waterfriction(0.0f),
            noslipfriction(0.0f),
            gravity(0.0f)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the animated tile data in "wawalite.txt"
    struct wawalite_animtile_t
    {
        uint32_t update_and;
        uint32_t frame_and;

        wawalite_animtile_t() :
            update_and(0),
            frame_and(0)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the damagetile data in "wawalite.txt"
    struct wawalite_damagetile_t
    {
        uint32_t amount;
        int    damagetype;

        int    part_gpip;
        uint32_t partand;
        int    sound_index;

        wawalite_damagetile_t() :
            amount(0),
            damagetype(0),
            part_gpip(-1),
            partand(0),
            sound_index(-1)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the weather data in "wawalite.txt"
    struct wawalite_weather_t
    {
        bool  over_water;
        int     timer_reset;
        int     part_gpip;           ///< Which particle to spawn?
        STRING  weather_name;

        wawalite_weather_t() :
            over_water(false),
            timer_reset(0),
            part_gpip(-1)
        {
			weather_name[0] = '\0';
        }
    };

//--------------------------------------------------------------------------------------------

    /// A wrapper for the graphics data in "wawalite.txt"
    struct wawalite_graphics_t
    {
        bool exploremode;
        bool usefaredge;

        wawalite_graphics_t() :
            exploremode(false),
            usefaredge(false)
        {
            //default ctor   
        }
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the camera data in "wawalite.txt"
    struct wawalite_camera_t
    {
        bool swing;
        float  swing_rate;
        float  swing_amp;

        wawalite_camera_t() :
            swing(false),
            swing_rate(0.0f),
            swing_amp(0.0f)
        {
            //default ctor
        }
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

        wawalite_fog_t() :
            found(false),
            top(0),
            bottom(0),
            red(0),
            grn(0),
            blu(0),
            affects_water(false)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// An internal representation of the data in "wawalite.txt"
    struct wawalite_data_t
    {
        uint32_t seed;
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

        wawalite_data_t() :
            seed(0),
            version(0),
            water(),
            phys(),
            animtile(),
            damagetile(),
            weather(),
            graphics(),
            camera(),
            fog(),
            light_x(0.0f),
            light_y(0.0f),
            light_z(0.0f),
            light_a(0.0f)
        {
            //default ctor
        }
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern wawalite_data_t wawalite_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    bool            write_wawalite_file_vfs( const wawalite_data_t * pdata );
    wawalite_data_t * read_wawalite_file_vfs( const char *filename, wawalite_data_t * pdata );

    wawalite_data_t * wawalite_limit( wawalite_data_t * pdata );
