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

/// @file egolib/file_formats/wawalite_file.c
/// @brief Functions to read and write the Egoboo's wawalite.txt file
/// @details

#include "egolib/file_formats/wawalite_file.h"
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

static wawalite_data_t _wawalite_file;

static bool wawalite_water_init(wawalite_water_t * pdata);
static bool wawalite_weather_init(wawalite_weather_t * pdata);
static bool wawalite_fog_init(wawalite_fog_t * pdata);
static bool wawalite_animtile_init(wawalite_animtile_t * pdata);
static bool wawalite_damagetile_init(wawalite_damagetile_t * pdata);

static wawalite_water_t *      read_wawalite_water(ReadContext& ctxt, wawalite_water_t * pwater );
static wawalite_data_t *       read_wawalite_light(ReadContext& ctxt, wawalite_data_t * pdata );
static wawalite_physics_t *    read_wawalite_physics(ReadContext& ctxt, wawalite_physics_t * pphys);
static wawalite_animtile_t *   read_wawalite_animtile(ReadContext& ctxt, wawalite_animtile_t * panimtile);
static wawalite_damagetile_t * read_wawalite_damagetile(ReadContext& ctxt, wawalite_damagetile_t * pdamagetile);
static wawalite_weather_t *    read_wawalite_weather(ReadContext& ctxt, wawalite_data_t * pdata);
static wawalite_graphics_t *   read_wawalite_graphics(ReadContext& ctxt, wawalite_graphics_t * pgraphics);
static wawalite_camera_t *     read_wawalite_camera(ReadContext& ctxt, wawalite_camera_t * pcamera);
static wawalite_data_t *       read_wawalite_fog(ReadContext& ctxt, wawalite_data_t * pdata);

static bool write_wawalite_water( vfs_FILE * filewrite, const wawalite_water_t * pwater );
static bool write_wawalite_light( vfs_FILE * filewrite, const wawalite_data_t * pdata );
static bool write_wawalite_physics( vfs_FILE * filewrite, const wawalite_physics_t * pphys );
static bool write_wawalite_animtile( vfs_FILE * filewrite, const wawalite_animtile_t * panimtile );
static bool write_wawalite_damagetile( vfs_FILE * filewrite, const wawalite_damagetile_t * pdamagetile );
static bool write_wawalite_weather( vfs_FILE * filewrite, const wawalite_weather_t * pweather );
static bool write_wawalite_graphics( vfs_FILE * filewrite, const wawalite_graphics_t * pgraphics );
static bool write_wawalite_camera( vfs_FILE * filewrite, const wawalite_camera_t * pcamera );
static bool write_wawalite_fog( vfs_FILE * filewrite, const wawalite_data_t * pdata );

static const int WAWALITE_FILE_VERSION = 2;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
wawalite_data_t * wawalite_data_init( wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return NULL;

    SDL_memset( pdata, 0, sizeof( wawalite_data_t ) );

    wawalite_water_init( &( pdata->water ) );
    wawalite_weather_init( &( pdata->weather ) );
    wawalite_fog_init( &( pdata->fog ) );
    wawalite_damagetile_init( &( pdata->damagetile ) );
    wawalite_animtile_init( &( pdata->animtile ) );

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
wawalite_water_t * read_wawalite_water(ReadContext& ctxt, wawalite_water_t * pwater )
{
    if ( NULL == pwater ) return pwater;

    BLANK_STRUCT_PTR( pwater )

    // Read water data first
    pwater->layer_count = vfs_get_next_int(ctxt);
    pwater->spek_start = vfs_get_next_int(ctxt);
    pwater->spek_level = vfs_get_next_int(ctxt);
    pwater->douse_level = vfs_get_next_float(ctxt);
    pwater->surface_level = vfs_get_next_float(ctxt);
    pwater->light = vfs_get_next_bool(ctxt);
    pwater->is_water = vfs_get_next_bool(ctxt);
    pwater->overlay_req = vfs_get_next_bool(ctxt);
    pwater->background_req = vfs_get_next_bool(ctxt);

    // General data info
    pwater->layer[0].dist.x = vfs_get_next_float(ctxt);
    pwater->layer[0].dist.y = vfs_get_next_float(ctxt);
    pwater->layer[1].dist.x = vfs_get_next_float(ctxt);
    pwater->layer[1].dist.y = vfs_get_next_float(ctxt);
    pwater->foregroundrepeat = vfs_get_next_float(ctxt);
    pwater->backgroundrepeat = vfs_get_next_float(ctxt);

    // Read data on first water layer
    pwater->layer[0].z = vfs_get_next_float(ctxt);
    pwater->layer[0].alpha = vfs_get_next_int(ctxt);
    pwater->layer[0].frame_add = vfs_get_next_int(ctxt);
    pwater->layer[0].light_dir = vfs_get_next_int(ctxt);
    pwater->layer[0].light_add = vfs_get_next_int(ctxt);
    pwater->layer[0].amp = vfs_get_next_float(ctxt);
    pwater->layer[0].tx_add.s = vfs_get_next_float(ctxt);
    pwater->layer[0].tx_add.t = vfs_get_next_float(ctxt);

    // Read data on second water layer
    pwater->layer[1].z = vfs_get_next_int(ctxt);
    pwater->layer[1].alpha = vfs_get_next_int(ctxt);
    pwater->layer[1].frame_add = vfs_get_next_int(ctxt);
    pwater->layer[1].light_dir = vfs_get_next_int(ctxt);
    pwater->layer[1].light_add = vfs_get_next_int(ctxt);
    pwater->layer[1].amp = vfs_get_next_float(ctxt);
    pwater->layer[1].tx_add.s = vfs_get_next_float(ctxt);
    pwater->layer[1].tx_add.t  = vfs_get_next_float(ctxt);

    return pwater;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_light(ReadContext& ctxt, wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    pdata->light_x = 1.00f;
    pdata->light_y = 1.00f;
    pdata->light_z = 0.50f;
    pdata->light_a = 0.20f;

    // Read light data second
    pdata->light_x = vfs_get_next_float(ctxt);
    pdata->light_y = vfs_get_next_float(ctxt);
    pdata->light_z = vfs_get_next_float(ctxt);
    pdata->light_a = vfs_get_next_float(ctxt);

    return pdata;
}

//--------------------------------------------------------------------------------------------
wawalite_physics_t * read_wawalite_physics(ReadContext& ctxt, wawalite_physics_t * pphys )
{
    if ( NULL == pphys ) return pphys;

    BLANK_STRUCT_PTR( pphys )

    // Read tile data third
    pphys->hillslide = vfs_get_next_float(ctxt);
    pphys->slippyfriction = vfs_get_next_float(ctxt);
    pphys->airfriction = vfs_get_next_float(ctxt);
    pphys->waterfriction = vfs_get_next_float(ctxt);
    pphys->noslipfriction = vfs_get_next_float(ctxt);
    pphys->gravity = vfs_get_next_float(ctxt);

    return pphys;
}

//--------------------------------------------------------------------------------------------
wawalite_animtile_t * read_wawalite_animtile(ReadContext& ctxt, wawalite_animtile_t * panimtile )
{
    if ( NULL == panimtile ) return panimtile;

    BLANK_STRUCT_PTR( panimtile )

    // animated tile
    panimtile->update_and = vfs_get_next_int(ctxt);
    panimtile->frame_and = vfs_get_next_int(ctxt);

    return panimtile;
}

//--------------------------------------------------------------------------------------------
wawalite_damagetile_t *read_wawalite_damagetile(ReadContext& ctxt, wawalite_damagetile_t * pdamagetile )
{
    if ( NULL == pdamagetile ) return pdamagetile;
    wawalite_damagetile_init( pdamagetile );            //Reset

    // damage tile
    pdamagetile->amount = vfs_get_next_int(ctxt);
    /// @todo pass the load name
    pdamagetile->damagetype = vfs_get_next_damage_type(ctxt);

    return pdamagetile;
}

//--------------------------------------------------------------------------------------------
wawalite_weather_t * read_wawalite_weather(ReadContext& ctxt, wawalite_data_t * pdata )
{
    wawalite_weather_t * pweather = &( pdata->weather );
    if ( NULL == pweather ) return pweather;

    BLANK_STRUCT_PTR( pweather )

    // weather data
    pweather->part_gpip = PIP_WEATHER;
    if ( pdata->version >= WAWALITE_FILE_VERSION )
    {
        STRING line;

        //Parse the weather type line
        vfs_get_next_string(ctxt, line, SDL_arraysize(line));
        strncpy( pweather->weather_name, strupr( line ), SDL_arraysize( pweather->weather_name ) );

        // convert the text in the calling function
        pweather->part_gpip = -1;
    }

    pweather->over_water = vfs_get_next_bool(ctxt);
    pweather->timer_reset = vfs_get_next_int(ctxt);

    return pweather;
}

//--------------------------------------------------------------------------------------------
wawalite_graphics_t * read_wawalite_graphics(ReadContext& ctxt, wawalite_graphics_t * pgraphics )
{
    if ( NULL == pgraphics ) return pgraphics;

    BLANK_STRUCT_PTR( pgraphics )

    // graphics options
    pgraphics->exploremode = vfs_get_next_bool(ctxt);
    pgraphics->usefaredge = vfs_get_next_bool(ctxt);

    return pgraphics;
}

//--------------------------------------------------------------------------------------------
wawalite_camera_t * read_wawalite_camera(ReadContext& ctxt, wawalite_camera_t * pcamera )
{
    if ( NULL == pcamera ) return pcamera;

    BLANK_STRUCT_PTR( pcamera )

    // camera data
    pcamera->swing_rate = vfs_get_next_float(ctxt);
    pcamera->swing_amp = vfs_get_next_float(ctxt);

    return pcamera;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_fog(ReadContext &ctxt, wawalite_data_t *data)
{
    if (!data) return data;

    // Read unnecessary data...  Only read if it exists...
    if (ctxt.skipToColon(true))
    {
        data->fog.found = true;
        data->fog.top = vfs_get_float(ctxt);
        data->fog.bottom = vfs_get_next_float(ctxt);
        data->fog.red = vfs_get_next_float(ctxt) * 255;
        data->fog.grn = vfs_get_next_float(ctxt) * 255;
        data->fog.blu = vfs_get_next_float(ctxt) * 255;
        data->fog.affects_water = vfs_get_next_bool(ctxt);

        // Read extra stuff for damage tile particles...
        if (ctxt.skipToColon(true))
        {
            data->damagetile.part_gpip = ctxt.readInt();
            data->damagetile.partand = vfs_get_next_int(ctxt);
            data->damagetile.sound_index = vfs_get_next_int(ctxt);
        }
    }

    return data;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_file_vfs( const char *filename, wawalite_data_t * pdata )
{
    /// @author ZZ
    /// @details This function sets up water and lighting for the module

    if ( NULL == pdata ) pdata = &_wawalite_file;

    if (!wawalite_data_init(pdata)) return pdata;

    ReadContext ctxt(filename);
    if (!ctxt.ensureOpen()) {
        log_warning("unable to read water and weather file `%s`\n", filename);
        return nullptr;
    }

    //First figure out what version of wawalite this is, so that we know what data we
    //should expect to load
    pdata->version = vfs_get_version(ctxt);

    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    pdata->seed = vfs_get_next_bool(ctxt);

    read_wawalite_water(ctxt, &(pdata->water));
    read_wawalite_light(ctxt, pdata);
    read_wawalite_physics(ctxt, &(pdata->phys));
    read_wawalite_animtile(ctxt, &(pdata->animtile));
    read_wawalite_damagetile(ctxt, &(pdata->damagetile));
    read_wawalite_weather(ctxt, pdata);
    read_wawalite_graphics(ctxt, &(pdata->graphics));
    read_wawalite_camera(ctxt, &(pdata->camera));
    read_wawalite_fog(ctxt, pdata);

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool write_wawalite_water( vfs_FILE * filewrite, const wawalite_water_t * pwater )
{
    if ( NULL == filewrite || NULL == pwater ) return false;

    // Read water data first
    vfs_put_int( filewrite, "Number of Water Layers ( 1=Fast, 2=Good )           :", pwater->layer_count );
    vfs_put_int( filewrite, "Water specular start ( 0-255 )                      :", pwater->spek_start );
    vfs_put_int( filewrite, "Water specular level ( 0-255 )                      :", pwater->spek_level );
    vfs_put_float( filewrite, "Water douse level ( For torches, 90 )               :", pwater->douse_level );
    vfs_put_float( filewrite, "Water surface level ( For water striders, 55 )      :", pwater->surface_level );
    vfs_put_bool( filewrite, "Water light ( TRUE or FALSE )                       :", pwater->light );
    vfs_put_bool( filewrite, "Water is really water? ( TRUE or FALSE )            :", pwater->is_water );
    vfs_put_bool( filewrite, "Use watertop.bmp as an overlay? ( TRUE or FALSE )   :", pwater->overlay_req );
    vfs_put_bool( filewrite, "Use waterlow.bmp as a background? ( TRUE or FALSE ) :", pwater->background_req );

    // General data info
    vfs_put_float( filewrite, "Foreground distance effect X ( 0.0 to 1.0 )    :", pwater->layer[0].dist.x );
    vfs_put_float( filewrite, "Foreground distance effect Y ( 0.0 to 1.0 )    :", pwater->layer[0].dist.y );
    vfs_put_float( filewrite, "Background distance effect X ( 0.0 to 1.0 )    :", pwater->layer[1].dist.x );
    vfs_put_float( filewrite, "Background distance effect Y ( 0.0 to 1.0 )    :", pwater->layer[1].dist.y );
    vfs_put_float( filewrite, "Number of foreground repeats ( 1 to 5 )        :", pwater->foregroundrepeat );
    vfs_put_float( filewrite, "Number of background repeats ( 1 to 5 )        :", pwater->backgroundrepeat );

    // Read data on first water layer
    vfs_put_float( filewrite, "Level 0... Base water level ( 85 )  :", pwater->layer[0].z );
    vfs_put_int( filewrite, "Level 0... Alpha level ( 100 )      :", pwater->layer[0].alpha );
    vfs_put_int( filewrite, "Level 0... Wave speed ( 3 )         :", pwater->layer[0].frame_add );
    vfs_put_int( filewrite, "Level 0... Brightness ( 15 )        :", pwater->layer[0].light_dir );
    vfs_put_int( filewrite, "Level 0... Ambient light ( 15 )     :", pwater->layer[0].light_add );
    vfs_put_float( filewrite, "Level 0... Wave amplitude ( 7.0 )   :", pwater->layer[0].amp );
    vfs_put_float( filewrite, "Level 0... U speed ( .0002 )        :", pwater->layer[0].tx_add.s );
    vfs_put_float( filewrite, "Level 0... V speed ( .0002 )        :", pwater->layer[0].tx_add.t );

    // Read data on second water layer
    vfs_put_int( filewrite, "Level 1... Base water level ( 85 )  :",  pwater->layer[1].z );
    vfs_put_int( filewrite, "Level 1... Alpha level ( 100 )      :",  pwater->layer[1].alpha );
    vfs_put_int( filewrite, "Level 1... Wave speed ( 3 )         :",  pwater->layer[1].frame_add );
    vfs_put_int( filewrite, "Level 1... Brightness ( 15 )        :",  pwater->layer[1].light_dir );
    vfs_put_int( filewrite, "Level 1... Ambient light ( 15 )     :",  pwater->layer[1].light_add );
    vfs_put_float( filewrite, "Level 1... Wave amplitude ( 7.0 )   :",  pwater->layer[1].amp );
    vfs_put_float( filewrite, "Level 1... U speed ( .0002 )        :",  pwater->layer[1].tx_add.s );
    vfs_put_float( filewrite, "Level 1... V speed ( .0002 )        :",  pwater->layer[1].tx_add.t );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_light(vfs_FILE * filewrite, const wawalite_data_t * pdata)
{
    if ( NULL == filewrite || NULL == pdata ) return false;

    // Read light data second
    vfs_put_float( filewrite, "Light X direction ( 1.00 )        :", pdata->light_x );
    vfs_put_float( filewrite, "Light Y direction ( 1.00 )        :", pdata->light_y );
    vfs_put_float( filewrite, "Light Z direction ( 0.50 )        :", pdata->light_z );
    vfs_put_float( filewrite, "Ambient light ( 0.20 )            :", pdata->light_a );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_physics( vfs_FILE * filewrite, const wawalite_physics_t * pphys )
{
    if ( NULL == filewrite || NULL == pphys ) return false;

    // Read tile data third
    vfs_put_float( filewrite, "Hillslide ( 1.00 )              :", pphys->hillslide );
    vfs_put_float( filewrite, "Slippy friction ( 1.00 )        :", pphys->slippyfriction );
    vfs_put_float( filewrite, "Air friction ( .95 )            :", pphys->airfriction );
    vfs_put_float( filewrite, "Water friction ( .85 )          :", pphys->waterfriction );
    vfs_put_float( filewrite, "Normal friction ( .95 )         :", pphys->noslipfriction );
    vfs_put_float( filewrite, "Gravity ( -1.0 )                :", pphys->gravity );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_animtile( vfs_FILE * filewrite, const wawalite_animtile_t * panimtile )
{
    if ( NULL == filewrite || NULL == panimtile ) return false;

    // animated tile
    vfs_put_int( filewrite, "Animated tile update AND ( 0, 1, 3, 7, 15, 31 )        :", panimtile->update_and );
    vfs_put_int( filewrite, "Animated tile frame AND ( 3 == 4 frame, 7 == 8 frame ) :", panimtile->frame_and );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_damagetile( vfs_FILE * filewrite, const wawalite_damagetile_t * pdamagetile )
{
    if ( NULL == filewrite || NULL == pdamagetile ) return false;

    // basic damage tile
    vfs_put_int( filewrite, "Damage tile damage ( 0 to 65535, 512 is 1 life block )  :", pdamagetile->amount );
    vfs_put_damage_type( filewrite, "Damage tile damage type ( SLASH, CRUSH, POKE, HOLY\n                          EVIL, FIRE, ICE, ZAP )  :", pdamagetile->damagetype );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_weather( vfs_FILE * filewrite, const wawalite_weather_t * pweather )
{
    if ( NULL == filewrite || NULL == pweather ) return false;

    // weather data
    vfs_printf( filewrite, "Weather particle effect ( NONE, LAVA, RAIN or SNOW ): %s", pweather->weather_name );
    vfs_put_bool( filewrite, "Weather particles only over water ( TRUE or FALSE )  :", pweather->over_water );
    vfs_put_int( filewrite,  "Weather particle spawn rate ( 0 to 100, 0 is none )  :", pweather->timer_reset );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_graphics( vfs_FILE * filewrite, const wawalite_graphics_t * pgraphics )
{
    if ( NULL == filewrite || NULL == pgraphics ) return false;

    // graphics options
    vfs_put_bool( filewrite, "Explore mode ( TRUE or FALSE )                         :", pgraphics->exploremode );
    vfs_put_bool( filewrite, "Far Edge mode...  For outside levels ( TRUE or FALSE ) :", pgraphics->usefaredge );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_camera( vfs_FILE * filewrite, const wawalite_camera_t * pcamera )
{
    if ( NULL == filewrite || NULL == pcamera ) return false;

    // camera data
    vfs_put_float( filewrite, "Camera swing rate ( 0 to 100 )                :", pcamera->swing_rate );
    vfs_put_float( filewrite, "Camera swing amplitude ( 0, or .002 to .100 ) :", pcamera->swing_amp );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_fog( vfs_FILE * filewrite, const wawalite_data_t * pdata )
{
    if ( NULL == filewrite || NULL == pdata ) return false;

    // write optional data...  Only read if it exists...
    if ( !pdata->fog.found ) return true;

    vfs_printf( filewrite, "\n\n// Fog Expansion...  Leave this out for no fog...\n" );
    vfs_put_float( filewrite, "Fog top z ( 0 to 100 )                            :", pdata->fog.top );
    vfs_put_float( filewrite, "Fog bottom z ( 0 )                                :", pdata->fog.bottom );
    vfs_put_float( filewrite, "Fog Red ( 0.0 to 1.0 )                            :", pdata->fog.red );
    vfs_put_float( filewrite, "Fog Green ( 0.0 to 1.0 )                          :", pdata->fog.grn );
    vfs_put_float( filewrite, "Fog Blue ( 0.0 to 1.0 )                           :", pdata->fog.blu );
    vfs_put_bool( filewrite, "Fog affects water ( TRUE or FALSE )               :", pdata->fog.affects_water );

    vfs_printf( filewrite, "\n\n// Damage tile expansion...  Must have fog first...\n" );
    vfs_put_int( filewrite, "Weather particle to spawn ( 4 or 5, 6 is splash )  :", pdata->damagetile.part_gpip );
    vfs_put_int( filewrite, "Particle timing AND ( 1, 3, 7, 15, etc. )          :", pdata->damagetile.partand );
    vfs_put_int( filewrite, "Damage sound ( 0 to 4 )                            :", pdata->damagetile.sound_index );

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_file_vfs( const wawalite_data_t * pdata )
{
    /// @author ZZ
    /// @details This function sets up water and lighting for the module

    vfs_FILE*  filewrite;
    STRING newloadname;

    if ( NULL == pdata ) return false;

    filewrite = vfs_openWrite( "mp_data/wawalite.txt" );
    if ( NULL == filewrite )
    {
        log_warning( "Could not write file! (\"%s\")\n", newloadname );
        return false;
    }

    // Add file verison number
    vfs_put_version( filewrite, WAWALITE_FILE_VERSION );

    // file header
    vfs_printf( filewrite, "//   This file tells the game how to model lighting and water...\n" );
    vfs_printf( filewrite, "// Please fill in all of the data even if you only use one layer of\n" );
    vfs_printf( filewrite, "// water.  2 is the maximum number of layers.\n" );
    vfs_printf( filewrite, "//   This file also gives information regarding damage tiles and\n" );
    vfs_printf( filewrite, "// friction for the module.\n" );
    vfs_printf( filewrite, "\n\n" );

    // random map
    vfs_put_int( filewrite, "Random map ( TRUE or FALSE ) ( doesn't work )           :", pdata->seed );

    write_wawalite_water( filewrite, &( pdata->water ) );
    write_wawalite_light( filewrite, pdata );
    write_wawalite_physics( filewrite, &( pdata->phys ) );
    write_wawalite_animtile( filewrite, &( pdata->animtile ) );
    write_wawalite_damagetile( filewrite, &( pdata->damagetile ) );
    write_wawalite_weather( filewrite, &( pdata->weather ) );
    write_wawalite_graphics( filewrite, &( pdata->graphics ) );
    write_wawalite_camera( filewrite, &( pdata->camera ) );
    write_wawalite_fog( filewrite, pdata );

    vfs_close( filewrite );

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_water_init( wawalite_water_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR( pdata )

    pdata->spek_start =   128;
    pdata->spek_level =   128;
    pdata->is_water   = true;

    pdata->foregroundrepeat = 1;
    pdata->backgroundrepeat = 1;

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_weather_init( wawalite_weather_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR( pdata )

    pdata->timer_reset = 10;

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_fog_init( wawalite_fog_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR( pdata );

    pdata->bottom        = -100;
    pdata->red           = 255;
    pdata->grn           = 255;
    pdata->blu           = 255;
    pdata->affects_water = true;

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_animtile_init( wawalite_animtile_t * pdata )
{
    if ( NULL == pdata ) return false;

    BLANK_STRUCT_PTR( pdata )

    pdata->update_and    = 7;                        // New tile every 7 frames
    pdata->frame_and     = 3;              // Only 4 frames

    return true;
}

//--------------------------------------------------------------------------------------------
bool wawalite_damagetile_init( wawalite_damagetile_t * pdata )
{
    if ( NULL == pdata ) return false;

    pdata->part_gpip    = -1;
    pdata->partand     = 255;
    pdata->sound_index = -1;
    pdata->damagetype  = DAMAGE_FIRE;
    pdata->amount      = 256;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
wawalite_data_t *  wawalite_limit( wawalite_data_t * pdata )
{
    int cnt;

    if ( NULL == pdata ) pdata = &_wawalite_file;

    // limit some values
    pdata->damagetile.sound_index = CLIP( pdata->damagetile.sound_index, -1, 30 );

    for ( cnt = 0; cnt < MAXWATERLAYER; cnt++ )
    {
        pdata->water.layer[cnt].light_dir = CLIP( pdata->water.layer[cnt].light_dir, (Uint32)0, (Uint32)63 );
        pdata->water.layer[cnt].light_add = CLIP( pdata->water.layer[cnt].light_add, (Uint32)0, (Uint32)63 );
    }

    return pdata;
}
