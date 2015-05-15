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

/// @file egolib/FileFormats/wawalite_file.c
/// @brief Functions to read and write the Egoboo's wawalite.txt file
/// @details

#include "egolib/FileFormats/wawalite_file.h"
#include "egolib/Profiles/_Include.hpp"
#include "egolib/log.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

wawalite_data_t wawalite_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static const int WAWALITE_FILE_VERSION = 2;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
wawalite_water_t *wawalite_water_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_water_t *profile)
{
    if (!profile)
    {
        throw std::invalid_argument("nullptr == profile");
    }

    // Reset to defaults.
    *profile = wawalite_water_t::getDefaults();

    // Read the water data.
    profile->layer_count = vfs_get_next_int(ctxt);
    profile->spek_start = vfs_get_next_int(ctxt);
    profile->spek_level = vfs_get_next_int(ctxt);
    profile->douse_level = vfs_get_next_float(ctxt);
    profile->surface_level = vfs_get_next_float(ctxt);
    profile->light = vfs_get_next_bool(ctxt);
    profile->is_water = vfs_get_next_bool(ctxt);
    profile->overlay_req = vfs_get_next_bool(ctxt);
    profile->background_req = vfs_get_next_bool(ctxt);

    // General data info
    profile->layer[0].dist[XX] = vfs_get_next_float(ctxt);
    profile->layer[0].dist[YY] = vfs_get_next_float(ctxt);
    profile->layer[1].dist[XX] = vfs_get_next_float(ctxt);
    profile->layer[1].dist[YY] = vfs_get_next_float(ctxt);
    profile->foregroundrepeat = vfs_get_next_float(ctxt);
    profile->backgroundrepeat = vfs_get_next_float(ctxt);

    // Read the first water layer.
    profile->layer[0].z = vfs_get_next_float(ctxt);
    profile->layer[0].alpha = vfs_get_next_int(ctxt);
    profile->layer[0].frame_add = vfs_get_next_int(ctxt);
    profile->layer[0].light_dir = vfs_get_next_int(ctxt);
    profile->layer[0].light_add = vfs_get_next_int(ctxt);
    profile->layer[0].amp = vfs_get_next_float(ctxt);
    profile->layer[0].tx_add[SS] = vfs_get_next_float(ctxt);
    profile->layer[0].tx_add[TT] = vfs_get_next_float(ctxt);

    // Read the second water layer.
    profile->layer[1].z = vfs_get_next_int(ctxt);
    profile->layer[1].alpha = vfs_get_next_int(ctxt);
    profile->layer[1].frame_add = vfs_get_next_int(ctxt);
    profile->layer[1].light_dir = vfs_get_next_int(ctxt);
    profile->layer[1].light_add = vfs_get_next_int(ctxt);
    profile->layer[1].amp = vfs_get_next_float(ctxt);
    profile->layer[1].tx_add[SS] = vfs_get_next_float(ctxt);
    profile->layer[1].tx_add[TT] = vfs_get_next_float(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_light_t *wawalite_light_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_light_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    *profile = wawalite_light_t::getDefaults();

    profile->light_x = 1.00f;
    profile->light_y = 1.00f;
    profile->light_z = 0.50f;
    profile->light_a = 0.20f;

    profile->light_x = vfs_get_next_float(ctxt);
    profile->light_y = vfs_get_next_float(ctxt);
    profile->light_z = vfs_get_next_float(ctxt);
    profile->light_a = vfs_get_next_float(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_physics_t *wawalite_physics_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_physics_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    // Reset to defaults.
    *profile = wawalite_physics_t::getDefaults();

    profile->hillslide = vfs_get_next_float(ctxt);
    profile->slippyfriction = vfs_get_next_float(ctxt);
    profile->airfriction = vfs_get_next_float(ctxt);
    profile->waterfriction = vfs_get_next_float(ctxt);
    profile->noslipfriction = vfs_get_next_float(ctxt);
    profile->gravity = vfs_get_next_float(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_animtile_t *wawalite_animtile_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_animtile_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    // Reset to defaults.
    *profile = wawalite_animtile_t::getDefaults();

    profile->update_and = vfs_get_next_int(ctxt);
    profile->frame_and = vfs_get_next_int(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_damagetile_t *wawalite_damagetile_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_damagetile_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    // Reset to defaults.
    *profile = wawalite_damagetile_t::getDefaults();

    // damage tile
    profile->amount = vfs_get_next_int(ctxt);
    /// @todo pass the load name
    profile->damagetype = vfs_get_next_damage_type(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_weather_t *wawalite_weather_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_weather_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    // Reset to defaults.
    *profile = wawalite_weather_t::getDefaults();

    // weather data
    profile->part_gpip = PIP_WEATHER;
    if (enclosing->version >= WAWALITE_FILE_VERSION)
    {
        STRING line;

        //Parse the weather type line
        vfs_get_next_string_lit(ctxt, line, SDL_arraysize(line));
        strncpy(profile->weather_name, strupr(line), SDL_arraysize(profile->weather_name));

        // convert the text in the calling function
        profile->part_gpip = -1;
    }

    profile->over_water = vfs_get_next_bool(ctxt);
    profile->timer_reset = vfs_get_next_int(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_graphics_t *wawalite_graphics_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_graphics_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }
    // Reset to defaults.
    *profile = wawalite_graphics_t::getDefaults();

    profile->exploremode = vfs_get_next_bool(ctxt);
    profile->usefaredge = vfs_get_next_bool(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_camera_t *wawalite_camera_t::read(ReadContext& ctxt, wawalite_data_t *enclosing, wawalite_camera_t *profile)
{
    if (!profile)
    {
        throw std::invalid_argument("nullptr == profile");
    }
    // Reset to defaults.
    *profile = wawalite_camera_t::getDefaults();

    profile->swing_rate = vfs_get_next_float(ctxt);
    profile->swing_amp = vfs_get_next_float(ctxt);

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_fog_t *wawalite_fog_t::read(ReadContext &ctxt, wawalite_data_t *enclosing, wawalite_fog_t *profile)
{
    if (!profile)
    {
        return nullptr;
    }

    // Read unnecessary data...  Only read if it exists...
    if (ctxt.skipToColon(true))
    {
        profile->found = true;
        profile->top = ctxt.readReal();
        profile->bottom = vfs_get_next_float(ctxt);
        profile->red = vfs_get_next_float(ctxt) * 255;
        profile->grn = vfs_get_next_float(ctxt) * 255;
        profile->blu = vfs_get_next_float(ctxt) * 255;
        profile->affects_water = vfs_get_next_bool(ctxt);
    }

    return profile;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t *wawalite_data_read(const char *filename, wawalite_data_t *profile)
{
    /**
     * @brief
     *  Read environmental data for the current module.
     */
    if (!profile)
    {
        throw std::invalid_argument("nullptr == profile");
    }
    // Reset to defaults.
    *profile = wawalite_data_t::getDefaults();

    ReadContext ctxt(filename);
    if (!ctxt.ensureOpen())
    {
        log_warning("unable to read water and weather file `%s`\n", filename);
        return nullptr;
    }

    //First figure out what version of wawalite this is, so that we know what data we
    //should expect to load
    profile->version = vfs_get_version(ctxt);

    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    profile->seed = vfs_get_next_bool(ctxt);

    wawalite_water_t::read(ctxt, profile, &(profile->water));
    wawalite_light_t::read(ctxt, profile, &(profile->light));
    wawalite_physics_t::read(ctxt, profile, &(profile->phys));
    wawalite_animtile_t::read(ctxt, profile, &(profile->animtile));
    wawalite_damagetile_t::read(ctxt, profile, &(profile->damagetile));
    wawalite_weather_t::read(ctxt, profile, &(profile->weather));
    wawalite_graphics_t::read(ctxt, profile, &(profile->graphics));
    wawalite_camera_t::read(ctxt, profile, &(profile->camera));
    wawalite_fog_t::read(ctxt, profile, &(profile->fog));
    if (profile->fog.found)
    {
        // Read extra stuff for damage tile particles...
        if (ctxt.skipToColon(true))
        {
            profile->damagetile.part_gpip = vfs_get_local_particle_profile_ref(ctxt);
            profile->damagetile.partand = vfs_get_next_int(ctxt);
            profile->damagetile.sound_index = vfs_get_next_int(ctxt);
        }
    }

    return profile;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool wawalite_water_t::write(vfs_FILE * filewrite, const wawalite_data_t *enclosing, const wawalite_water_t *profile)
{
    if (!filewrite || !profile)
    {
        return false;
    }

    // Read water data first
    vfs_put_int(filewrite,   "Number of Water Layers ( 1=Fast, 2=Good )           :", profile->layer_count);
    vfs_put_int(filewrite,   "Water specular start ( 0-255 )                      :", profile->spek_start);
    vfs_put_int(filewrite,   "Water specular level ( 0-255 )                      :", profile->spek_level);
    vfs_put_float(filewrite, "Water douse level ( For torches, 90 )               :", profile->douse_level);
    vfs_put_float(filewrite, "Water surface level ( For water striders, 55 )      :", profile->surface_level);
    vfs_put_bool(filewrite,  "Water light ( TRUE or FALSE )                       :", profile->light);
    vfs_put_bool(filewrite,  "Water is really water? ( TRUE or FALSE )            :", profile->is_water);
    vfs_put_bool(filewrite,  "Use watertop.bmp as an overlay? ( TRUE or FALSE )   :", profile->overlay_req);
    vfs_put_bool(filewrite,  "Use waterlow.bmp as a background? ( TRUE or FALSE ) :", profile->background_req);

    // General data info
    vfs_put_float(filewrite, "Foreground distance effect X ( 0.0 to 1.0 )    :", profile->layer[0].dist[XX]);
    vfs_put_float(filewrite, "Foreground distance effect Y ( 0.0 to 1.0 )    :", profile->layer[0].dist[YY]);
    vfs_put_float(filewrite, "Background distance effect X ( 0.0 to 1.0 )    :", profile->layer[1].dist[XX]);
    vfs_put_float(filewrite, "Background distance effect Y ( 0.0 to 1.0 )    :", profile->layer[1].dist[YY]);
    vfs_put_float(filewrite, "Number of foreground repeats ( 1 to 5 )        :", profile->foregroundrepeat);
    vfs_put_float(filewrite, "Number of background repeats ( 1 to 5 )        :", profile->backgroundrepeat);

    // Read data on first water layer
    vfs_put_float(filewrite, "Level 0... Base water level ( 85 )  :", profile->layer[0].z);
    vfs_put_int(filewrite,   "Level 0... Alpha level ( 100 )      :", profile->layer[0].alpha);
    vfs_put_int(filewrite,   "Level 0... Wave speed ( 3 )         :", profile->layer[0].frame_add);
    vfs_put_int(filewrite,   "Level 0... Brightness ( 15 )        :", profile->layer[0].light_dir);
    vfs_put_int(filewrite,   "Level 0... Ambient light ( 15 )     :", profile->layer[0].light_add);
    vfs_put_float(filewrite, "Level 0... Wave amplitude ( 7.0 )   :", profile->layer[0].amp);
    vfs_put_float(filewrite, "Level 0... U speed ( .0002 )        :", profile->layer[0].tx_add[SS]);
    vfs_put_float(filewrite, "Level 0... V speed ( .0002 )        :", profile->layer[0].tx_add[TT]);

    // Read data on second water layer
    vfs_put_int(filewrite,   "Level 1... Base water level ( 85 )  :", profile->layer[1].z);
    vfs_put_int(filewrite,   "Level 1... Alpha level ( 100 )      :", profile->layer[1].alpha);
    vfs_put_int(filewrite,   "Level 1... Wave speed ( 3 )         :", profile->layer[1].frame_add);
    vfs_put_int(filewrite,   "Level 1... Brightness ( 15 )        :", profile->layer[1].light_dir);
    vfs_put_int(filewrite,   "Level 1... Ambient light ( 15 )     :", profile->layer[1].light_add);
    vfs_put_float(filewrite, "Level 1... Wave amplitude ( 7.0 )   :", profile->layer[1].amp);
    vfs_put_float(filewrite, "Level 1... U speed ( .0002 )        :", profile->layer[1].tx_add[SS]);
    vfs_put_float(filewrite, "Level 1... V speed ( .0002 )        :", profile->layer[1].tx_add[TT]);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_light_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_light_t *profile)
{
    if (!filewrite || !profile) return false;

    // Read light data second
    vfs_put_float(filewrite, "Light X direction ( 1.00 )        :", profile->light_x);
    vfs_put_float(filewrite, "Light Y direction ( 1.00 )        :", profile->light_y);
    vfs_put_float(filewrite, "Light Z direction ( 0.50 )        :", profile->light_z);
    vfs_put_float(filewrite, "Ambient light ( 0.20 )            :", profile->light_a);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_physics_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_physics_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // Read tile data third
    vfs_put_float(filewrite, "Hillslide ( 1.00 )              :", profile->hillslide);
    vfs_put_float(filewrite, "Slippy friction ( 1.00 )        :", profile->slippyfriction);
    vfs_put_float(filewrite, "Air friction ( .95 )            :", profile->airfriction);
    vfs_put_float(filewrite, "Water friction ( .85 )          :", profile->waterfriction);
    vfs_put_float(filewrite, "Normal friction ( .95 )         :", profile->noslipfriction);
    vfs_put_float(filewrite, "Gravity ( -1.0 )                :", profile->gravity);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_animtile_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_animtile_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // animated tile
    vfs_put_int(filewrite, "Animated tile update AND ( 0, 1, 3, 7, 15, 31 )        :", profile->update_and);
    vfs_put_int(filewrite, "Animated tile frame AND ( 3 == 4 frame, 7 == 8 frame ) :", profile->frame_and);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_damagetile_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_damagetile_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // basic damage tile
    vfs_put_int(filewrite, "Damage tile damage ( 0 to 65535, 512 is 1 life block )  :", profile->amount);
    vfs_put_damage_type(filewrite, "Damage tile damage type ( SLASH, CRUSH, POKE, HOLY\n"
                                   "                          EVIL, FIRE, ICE, ZAP )  :", profile->damagetype);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_weather_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_weather_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // weather data
    vfs_printf(filewrite,   "Weather particle effect ( NONE, LAVA, RAIN or SNOW ): %s", profile->weather_name);
    vfs_put_bool(filewrite, "Weather particles only over water ( TRUE or FALSE ) :", profile->over_water);
    vfs_put_int(filewrite,  "Weather particle spawn rate ( 0 to 100, 0 is none ) :", profile->timer_reset);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_graphics_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_graphics_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // graphics options
    vfs_put_bool(filewrite, "Explore mode ( TRUE or FALSE )                         :", profile->exploremode);
    vfs_put_bool(filewrite, "Far Edge mode...  For outside levels ( TRUE or FALSE ) :", profile->usefaredge);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_camera_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_camera_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // camera data
    vfs_put_float(filewrite, "Camera swing rate ( 0 to 100 )                :", profile->swing_rate);
    vfs_put_float(filewrite, "Camera swing amplitude ( 0, or .002 to .100 ) :", profile->swing_amp);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_fog_t::write(vfs_FILE *filewrite, const wawalite_data_t *enclosing, const wawalite_fog_t *profile)
{
    if (NULL == filewrite || NULL == profile) return false;

    // write optional data...  Only read if it exists...
    if (!profile->found) return true;

    vfs_printf( filewrite, "\n\n// Fog Expansion...  Leave this out for no fog...\n" );
    vfs_put_float(filewrite, "Fog top z ( 0 to 100 )                            :", profile->top);
    vfs_put_float(filewrite, "Fog bottom z ( 0 )                                :", profile->bottom);
    vfs_put_float(filewrite, "Fog Red ( 0.0 to 1.0 )                            :", profile->red);
    vfs_put_float(filewrite, "Fog Green ( 0.0 to 1.0 )                          :", profile->grn);
    vfs_put_float(filewrite, "Fog Blue ( 0.0 to 1.0 )                           :", profile->blu);
    vfs_put_bool(filewrite, "Fog affects water ( TRUE or FALSE )               :", profile->affects_water);

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_data_write(const char *filename,const wawalite_data_t *profile)
{
    if (!profile)
    {
        throw std::invalid_argument("nullptr == profile");
    }
    vfs_FILE *filewrite = vfs_openWrite(filename);
    if ( NULL == filewrite )
    {
        log_warning("%s:%d: unable to write file `%s`\n", __FILE__, __LINE__, filename);
        return false;
    }

    // Add file verison number
    vfs_put_version( filewrite, WAWALITE_FILE_VERSION );

    // file header
    vfs_printf( filewrite, "// This file tells the game how to model lighting and water...\n" );
    vfs_printf( filewrite, "// Please fill in all of the data even if you only use one layer of\n" );
    vfs_printf( filewrite, "// water. 2 is the maximum number of layers.\n" );
    vfs_printf( filewrite, "// This file also gives information regarding damage tiles and\n" );
    vfs_printf( filewrite, "// friction for the module.\n" );
    vfs_printf( filewrite, "\n\n" );

    // random map
    vfs_put_int(filewrite, "Random map ( TRUE or FALSE ) ( doesn't work )           :", profile->seed);

    wawalite_water_t::write(filewrite, profile, &(profile->water));
    wawalite_light_t::write(filewrite, profile, &(profile->light));
    wawalite_physics_t::write(filewrite, profile, &(profile->phys));
    wawalite_animtile_t::write(filewrite, profile, &(profile->animtile));
    wawalite_damagetile_t::write(filewrite, profile, &(profile->damagetile));
    wawalite_weather_t::write(filewrite, profile, &(profile->weather));
    wawalite_graphics_t::write(filewrite, profile, &(profile->graphics));
    wawalite_camera_t::write(filewrite, profile, &(profile->camera));
    wawalite_fog_t::write(filewrite, profile, &(profile->fog));

    if (profile->fog.found)
    {
        vfs_printf(filewrite, "\n\n// Damage tile expansion...  Must have fog first...\n");
        vfs_put_int(filewrite, "Weather particle to spawn ( 4 or 5, 6 is splash )  :", profile->damagetile.part_gpip);
        vfs_put_int(filewrite, "Particle timing AND ( 1, 3, 7, 15, etc. )          :", profile->damagetile.partand);
        vfs_put_int(filewrite, "Damage sound ( 0 to 4 )                            :", profile->damagetile.sound_index);
    }

    vfs_close(filewrite);

    return true;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t *wawalite_limit(wawalite_data_t *profile)
{
    if (!profile)
    {
        throw std::invalid_argument("nullptr == profile");
    }

    // Limit the sound index.
    profile->damagetile.sound_index = CLIP(profile->damagetile.sound_index, -1, 30);

    for (size_t i = 0; i < MAXWATERLAYER; ++i)
    {
        profile->water.layer[i].light_dir = CLIP(profile->water.layer[i].light_dir, (Uint32)0, (Uint32)63);
        profile->water.layer[i].light_add = CLIP(profile->water.layer[i].light_add, (Uint32)0, (Uint32)63);
    }

    return profile;
}
