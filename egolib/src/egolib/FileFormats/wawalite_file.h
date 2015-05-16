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

/// @file egolib/FileFormats/wawalite_file.h
/// @details loading the environment definitions for a module

#pragma once

#include "egolib/_math.h"
#include "egolib/Logic/Damage.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Profiles/LocalParticleProfileRef.hpp"

struct ReadContext;
struct wawalite_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWATERLAYER 2                             ///< Maximum water layers

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the water layer data in "wawalite.txt"
    struct wawalite_water_layer_t
    {
        uint32_t frame_add;      ///< Speed

        float z;                 ///< Base height of water
        float amp;               ///< Amplitude of waves

        fvec2_t dist;            ///< For distant backgrounds
        uint32_t light_dir;      ///< direct  reflectivity 0 - 63
        uint32_t light_add;      ///< ambient reflectivity 0 - 63

        fvec2_t tx_add;          ///< Texture movement
        uint8_t alpha;           ///< Transparency

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
            // default ctor
        }

        wawalite_water_layer_t(const wawalite_water_layer_t& other) :
            frame_add(other.frame_add),
            z(other.z),
            amp(other.amp),
            dist(other.dist),
            light_dir(other.light_dir),
            light_add(other.light_add),
            tx_add(other.tx_add),
            alpha(other.alpha)
        {
            // copy ctor
        }

        wawalite_water_layer_t& operator=(const wawalite_water_layer_t& other)
        {
            frame_add = other.frame_add;
            z = other.z;
            amp = other.amp;
            dist = other.dist;
            light_dir = other.light_dir;
            light_add = other.light_add;
            tx_add = other.tx_add;
            alpha = other.alpha;
            return *this;
        }

    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the water data in "wawalite.txt"
    struct wawalite_water_t
    {
        int layer_count;
        wawalite_water_layer_t layer[MAXWATERLAYER];

        float surface_level;    ///< Surface level for water striders
        float douse_level;      ///< Surface level for torches
        uint8_t spek_start;     ///< Specular begins at which light value
        uint8_t spek_level;     ///< General specular amount (0-255)
        bool is_water;          ///< Is it water?  ( Or lava... )
        bool overlay_req;
        bool background_req;

        bool light;             ///< Is it light ( default is alpha )

        float foregroundrepeat;
        float backgroundrepeat;

        static const wawalite_water_t& getDefaults()
        {
            static const wawalite_water_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_water_t() :
            layer_count(0),
            layer(),
            surface_level(0.0f),
            douse_level(0.0f),
            spek_start(128),
            spek_level(128),
            is_water(true),
            overlay_req(false),
            background_req(false),
            light(false),
            foregroundrepeat(1.0f),
            backgroundrepeat(1.0f)
        {
            // default ctor            
        }

        wawalite_water_t(const wawalite_water_t& other) :
            layer_count(other.layer_count),
            /*layer(other.layer),*/ /* See below. */
            surface_level(other.surface_level),
            douse_level(other.douse_level),
            spek_start(other.spek_start),
            spek_level(other.spek_level),
            is_water(other.is_water),
            overlay_req(other.overlay_req),
            background_req(other.background_req),
            light(other.light),
            foregroundrepeat(other.foregroundrepeat),
            backgroundrepeat(other.backgroundrepeat)
        {
            for (int i = 0; i < layer_count; ++i)
            {
                layer[i] = other.layer[i];
            }        
        }

        wawalite_water_t& operator=(const wawalite_water_t& other)
        {
            layer_count = other.layer_count;
            /*layer(other.layer),*/ /* See below. */
            surface_level = other.surface_level;
            douse_level = other.douse_level;
            spek_start = other.spek_start;
            spek_level = other.spek_level;
            is_water = other.is_water;
            overlay_req = other.overlay_req;
            background_req = other.background_req;
            light = other.light;
            foregroundrepeat = other.foregroundrepeat;
            backgroundrepeat = other.backgroundrepeat;
            for (int i = 0; i < layer_count; ++i)
            {
                layer[i] = other.layer[i];
            }
            return *this;
        }

        static wawalite_water_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_water_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_water_t *profile);

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

        static const wawalite_physics_t& getDefaults()
        {
            static const wawalite_physics_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_physics_t() :
            hillslide(0.0f),
            slippyfriction(0.0f),
            airfriction(0.0f),
            waterfriction(0.0f),
            noslipfriction(0.0f),
            gravity(0.0f)
        {
            // default ctor
        }

        wawalite_physics_t(const wawalite_physics_t& other) :
            hillslide(other.hillslide),
            slippyfriction(other.slippyfriction),
            airfriction(other.airfriction),
            waterfriction(other.waterfriction),
            noslipfriction(other.noslipfriction),
            gravity(other.gravity)
        {
            // copy ctor
        }

        wawalite_physics_t& operator=(const wawalite_physics_t& other)
        {
            hillslide = other.hillslide;
            slippyfriction = other.slippyfriction;
            airfriction = other.airfriction;
            waterfriction = other.waterfriction;
            noslipfriction = other.noslipfriction;
            gravity = other.gravity;
            return *this;
        }

        static wawalite_physics_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_physics_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_physics_t *profile);

    };



//--------------------------------------------------------------------------------------------

/// A wrapper for the animated tile data in "wawalite.txt"
    struct wawalite_animtile_t
    {

        uint32_t update_and; ///< @default A new tile every 7 frames (i.e. 7).
        uint32_t frame_and;  ///< @default Only four frames (i.e. 3).

        /**
         * @brief
         *  Get animated tile data default values.
         * @return
         *  the animated tile data default values.
         */
        static const wawalite_animtile_t& getDefaults()
        {
            static const wawalite_animtile_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_animtile_t() :
            update_and(7),
            frame_and(3)
        {
            // default ctor
        }

        wawalite_animtile_t(const wawalite_animtile_t& other) :
            update_and(other.update_and),
            frame_and(other.frame_and)
        {
            // copy ctor
        }

        wawalite_animtile_t& operator=(const wawalite_animtile_t& other)
        {
            update_and = other.update_and;
            frame_and = other.frame_and;
            return *this;
        }

        static wawalite_animtile_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_animtile_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_animtile_t *profile);

    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the damagetile data in "wawalite.txt"
    struct wawalite_damagetile_t
    {
        uint32_t amount; ///< The amount of damage dealt.
        DamageType damagetype;  ///< The type of damage dealt.

        LocalParticleProfileRef part_gpip;
        uint32_t partand;
        int sound_index;

        /**
         * @brief
         *  Get damage tile data default values.
         * @return
         *  the damage tile data default values
         */
        static const wawalite_damagetile_t& getDefaults()
        {
            static const wawalite_damagetile_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_damagetile_t() :
            amount(256),
            damagetype(DAMAGE_FIRE),
            part_gpip(),
            partand(255),
            sound_index(-1)
        {
            // default ctor
        }

        wawalite_damagetile_t(const wawalite_damagetile_t& other) :
            amount(other.amount),
            damagetype(other.damagetype),
            part_gpip(other.part_gpip),
            partand(other.partand),
            sound_index(other.sound_index)
        {
            // default ctor
        }

        wawalite_damagetile_t& operator=(const wawalite_damagetile_t& other)
        {
            amount = other.amount;
            damagetype = other.damagetype;
            part_gpip = other.part_gpip;
            partand = other.partand;
            sound_index = other.sound_index;
            return *this;
        }

        static wawalite_damagetile_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_damagetile_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_damagetile_t *profile);

    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the weather data in "wawalite.txt"
    struct wawalite_weather_t
    {
        bool over_water;
        int timer_reset;
        LocalParticleProfileRef part_gpip; ///< Which particle to spawn?
        STRING weather_name;

        /**
         * @brief
         *  Get weather data default values.
         * @return
         *  the weather data default values.
         */
        static const wawalite_weather_t& getDefaults()
        {
            static const wawalite_weather_t DEFAULTS;
            return DEFAULTS;
        }


        wawalite_weather_t() :
            over_water(false),
            timer_reset(10),
            part_gpip(-1)
        {
			weather_name[0] = '\0';
        }

        wawalite_weather_t(const wawalite_weather_t& other) :
            over_water(other.over_water),
            timer_reset(other.timer_reset),
            part_gpip(other.part_gpip)
        {
            strcpy(weather_name, other.weather_name);
        }

        wawalite_weather_t& operator=(const wawalite_weather_t& other)
        {
            over_water = other.over_water;
            timer_reset = other.timer_reset;
            part_gpip = other.part_gpip;
            strcpy(weather_name, other.weather_name);
            return *this;
        }

        static wawalite_weather_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_weather_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_weather_t *profile);

    };

//--------------------------------------------------------------------------------------------

    /// A wrapper for the graphics data in "wawalite.txt"
    struct wawalite_graphics_t
    {

        bool exploremode;
        bool usefaredge;

        /**
         * @brief
         *  Get graphics data default values.
         * @return
         *  the graphics data default values.
         */
        static const wawalite_graphics_t& getDefaults()
        {
            static const wawalite_graphics_t DEFAULTS;
            return DEFAULTS;
        }


        wawalite_graphics_t() :
            exploremode(false),
            usefaredge(false)
        {
            // default ctor   
        }

        wawalite_graphics_t(const wawalite_graphics_t& other) :
            exploremode(other.exploremode),
            usefaredge(other.usefaredge)
        {
            // copy ctor
        }

        wawalite_graphics_t& operator=(const wawalite_graphics_t& other)
        {
            exploremode = other.exploremode;
            usefaredge = other.usefaredge;
            return *this;
        }

        static wawalite_graphics_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_graphics_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_graphics_t *profile);

    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the camera data in "wawalite.txt"
    struct wawalite_camera_t
    {
        bool swing;
        float swing_rate;
        float swing_amp;

        /**
         * @brief
         *  Get camera data default values.
         * @return
         *  the camera data default values.
         */
        static const wawalite_camera_t& getDefaults()
        {
            static const wawalite_camera_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_camera_t() :
            swing(false),
            swing_rate(0.0f),
            swing_amp(0.0f)
        {
            // default ctor
        }

        wawalite_camera_t(const wawalite_camera_t& other) :
            swing(other.swing),
            swing_rate(other.swing_rate),
            swing_amp(other.swing_amp)
        {
            // copy ctor
        }

        wawalite_camera_t& operator=(const wawalite_camera_t& other)
        {
            swing = other.swing;
            swing_rate = other.swing_rate;
            swing_amp = other.swing_amp;
            return *this;
        }

        static wawalite_camera_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_camera_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_camera_t *profile);

    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the fog data in "wawalite.txt"
    struct wawalite_fog_t
    {
        bool found;
        float top;
        float bottom;
        float red, grn, blu; ///< @todo Should be Ego::Math::Colour3f.
        bool affects_water;

        /**
         * @brief
         *  Get fog data default values.
         * @return
         *  the fog data default values.
         */
        static const wawalite_fog_t& getDefaults()
        {
            static const wawalite_fog_t DEFAULTS;
            return DEFAULTS;
        }

        wawalite_fog_t() :
            found(false),
            top(0),
            bottom(-100),
            red(255),
            grn(255),
            blu(255),
            affects_water(true)
        {
            // default ctor
        }

        wawalite_fog_t(const wawalite_fog_t& other) :
            found(other.found),
            top(other.top),
            bottom(other.bottom),
            red(other.red),
            grn(other.grn),
            blu(other.blu),
            affects_water(other.affects_water)
        {
            // copy ctor
        }

        wawalite_fog_t& operator=(const wawalite_fog_t& other)
        {
            found = other.found;
            top = other.top;
            bottom = other.bottom;
            red = other.red;
            grn = other.grn;
            blu = other.blu;
            affects_water = other.affects_water;
            return *this;
        }

        static wawalite_fog_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_fog_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_fog_t *profile);
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the lighting data in "wawalite.txt"
    struct wawalite_light_t
    {
        /**
         * @brief
         *  Directional light vector.
         * @todo
         *  Egoboo does not allow for specifying a directional light colour.
         * @todo
         *  Should be fvec3_t.
         * @todo
         *  A direction *must* be provided, it may not be @a 0.
         */
        float light_x, light_y, light_z; ///< @todo Should be fvec3_t.
        /**
         * @brief
         *  Ambient light brightness.
         * @todo
         *  Egoboo does not allow for specifying an ambient light colour,
         *  only its alpha value; the effective ambient light color is given by
         *  (1, 1, 1, light_a).
         */
        float light_a;

        /**
         * @brief
         *  Get light data default values.
         * @return
         *  the light data default values.
         */
        static const wawalite_light_t& getDefaults()
        {
            static const wawalite_light_t DEFAULTS;
            return DEFAULTS;
        }

        /** @todo The defaults are not reasonable as the directional light direction is (0,0,0). */
        wawalite_light_t() :
            light_x(0.0f),
            light_y(0.0f),
            light_z(0.0f),
            light_a(0.0f)
        {
            // default ctor
        }

        wawalite_light_t(const wawalite_light_t& other) :
            light_x(other.light_x),
            light_y(other.light_y),
            light_z(other.light_z),
            light_a(other.light_a)
        {
            // copy ctor
        }

        wawalite_light_t& operator=(const wawalite_light_t& other)
        {
            light_x = other.light_x;
            light_y = other.light_y;
            light_z = other.light_z;
            light_a = other.light_a;
            return *this;
        }

        static wawalite_light_t *read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_light_t *profile);
        static bool write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_light_t *profile);

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
        wawalite_light_t      light;

        /**
         * @brief
         *  Get data default values.
         * @return
         *  the data default values.
         */
        static const wawalite_data_t& getDefaults()
        {
            static const wawalite_data_t DEFAULTS;
            return DEFAULTS;
        }

        /// @todo Version default should be the current version.
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
            light()
        {
            // default ctor
        }


        wawalite_data_t(const wawalite_data_t& other) :
            seed(other.seed),
            version(other.version),
            water(other.water),
            phys(other.phys),
            animtile(other.animtile),
            damagetile(other.damagetile),
            weather(other.weather),
            graphics(other.graphics),
            camera(other.camera),
            fog(other.fog),
            light(other.light)
        {
            // copy ctor
        }

        wawalite_data_t& operator=(const wawalite_data_t& other)
        {
            seed = other.seed;
            version = other.version;
            water = other.water;
            phys = other.phys;
            animtile = other.animtile;
            damagetile = other.damagetile;
            weather = other.weather;
            graphics = other.graphics;
            camera = other.camera;
            fog = other.fog;
            light = other.light;
            return *this;
        }

    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern wawalite_data_t wawalite_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    /**
     * @brief
     *  Write environmental data to current module.
     * @param filename
     *  the filename
     * @param self
     *  the environmental data
     */
    bool wawalite_data_write(const char *filename,const wawalite_data_t *profile);
    /**
     * @brief
     *  Read environmental data from current module.
     * @param filename
     *  the filename
     * @param self
     *  the environmental data
     */
    wawalite_data_t *wawalite_data_read(const char *filename, wawalite_data_t *profile);
    wawalite_data_t *wawalite_limit(wawalite_data_t *self);
