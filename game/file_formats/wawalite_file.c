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

/// @file file_formats/wawalite_file.c
/// @brief Functions to read and write the Egoboo's wawalite.txt file
/// @details

#include "wawalite_file.h"
#include "char.inl"

#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_math.inl"
#include "pip_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
wawalite_data_t wawalite_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static wawalite_data_t _wawalite_file;

static bool_t wawalite_water_init( wawalite_water_t * pdata );
static bool_t wawalite_weather_init( wawalite_weather_t * pdata );
static bool_t wawalite_fog_init( wawalite_fog_t * pdata );
static bool_t wawalite_animtile_init( wawalite_animtile_t * pdata );
static bool_t wawalite_damagetile_init( wawalite_damagetile_t * pdata );

static wawalite_water_t *      read_wawalite_water( vfs_FILE * fileread, wawalite_water_t * pwater );
static wawalite_data_t *       read_wawalite_light( vfs_FILE * fileread, wawalite_data_t * pdata );
static wawalite_physics_t *    read_wawalite_physics( vfs_FILE * fileread, wawalite_physics_t * pphys );
static wawalite_animtile_t *   read_wawalite_animtile( vfs_FILE * fileread, wawalite_animtile_t * panimtile );
static wawalite_damagetile_t * read_wawalite_damagetile( vfs_FILE * fileread, wawalite_damagetile_t * pdamagetile );
static wawalite_weather_t *    read_wawalite_weather( vfs_FILE * fileread, wawalite_data_t * pdata );
static wawalite_graphics_t *   read_wawalite_graphics( vfs_FILE * fileread, wawalite_graphics_t * pgraphics );
static wawalite_camera_t *     read_wawalite_camera( vfs_FILE * fileread, wawalite_camera_t * pcamera );
static wawalite_data_t *       read_wawalite_fog( vfs_FILE * fileread, wawalite_data_t * pdata );

static bool_t write_wawalite_water( vfs_FILE * filewrite, wawalite_water_t * pwater );
static bool_t write_wawalite_light( vfs_FILE * filewrite, wawalite_data_t * pdata );
static bool_t write_wawalite_physics( vfs_FILE * filewrite, wawalite_physics_t * pphys );
static bool_t write_wawalite_animtile( vfs_FILE * filewrite, wawalite_animtile_t * panimtile );
static bool_t write_wawalite_damagetile( vfs_FILE * filewrite, wawalite_damagetile_t * pdamagetile );
static bool_t write_wawalite_weather( vfs_FILE * filewrite, wawalite_weather_t * pweather );
static bool_t write_wawalite_graphics( vfs_FILE * filewrite, wawalite_graphics_t * pgraphics );
static bool_t write_wawalite_camera( vfs_FILE * filewrite, wawalite_camera_t * pcamera );
static bool_t write_wawalite_fog( vfs_FILE * filewrite, wawalite_data_t * pdata );

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
wawalite_water_t * read_wawalite_water( vfs_FILE * fileread, wawalite_water_t * pwater )
{
    if ( NULL == pwater ) return pwater;

    memset( pwater, 0, sizeof( *pwater ) );

    if ( NULL == fileread ) return pwater;

    // Read water data first
    pwater->layer_count    = fget_next_int( fileread );
    pwater->spek_start     = fget_next_int( fileread );
    pwater->spek_level     = fget_next_int( fileread );
    pwater->douse_level    = fget_next_float( fileread );
    pwater->surface_level  = fget_next_float( fileread );
    pwater->light          = fget_next_bool( fileread );
    pwater->is_water       = fget_next_bool( fileread );
    pwater->overlay_req    = fget_next_bool( fileread );
    pwater->background_req = fget_next_bool( fileread );

    // General data info
    pwater->layer[0].dist.x  = fget_next_float( fileread );
    pwater->layer[0].dist.y  = fget_next_float( fileread );
    pwater->layer[1].dist.x  = fget_next_float( fileread );
    pwater->layer[1].dist.y  = fget_next_float( fileread );
    pwater->foregroundrepeat = fget_next_float( fileread );
    pwater->backgroundrepeat = fget_next_float( fileread );

    // Read data on first water layer
    pwater->layer[0].z          = fget_next_float( fileread );
    pwater->layer[0].alpha      = fget_next_int( fileread );
    pwater->layer[0].frame_add  = fget_next_int( fileread );
    pwater->layer[0].light_dir  = fget_next_int( fileread );
    pwater->layer[0].light_add  = fget_next_int( fileread );
    pwater->layer[0].amp        = fget_next_float( fileread );
    pwater->layer[0].tx_add.s   = fget_next_float( fileread );
    pwater->layer[0].tx_add.t   = fget_next_float( fileread );

    // Read data on second water layer
    pwater->layer[1].z          = fget_next_int( fileread );
    pwater->layer[1].alpha      = fget_next_int( fileread );
    pwater->layer[1].frame_add  = fget_next_int( fileread );
    pwater->layer[1].light_dir  = fget_next_int( fileread );
    pwater->layer[1].light_add  = fget_next_int( fileread );
    pwater->layer[1].amp        = fget_next_float( fileread );
    pwater->layer[1].tx_add.s  = fget_next_float( fileread );
    pwater->layer[1].tx_add.t  = fget_next_float( fileread );

    return pwater;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_light( vfs_FILE * fileread, wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    pdata->light_x = 1.00f;
    pdata->light_y = 1.00f;
    pdata->light_z = 0.50f;
    pdata->light_a = 0.20f;

    if ( NULL == fileread ) return pdata;

    // Read light data second
    pdata->light_x = fget_next_float( fileread );
    pdata->light_y = fget_next_float( fileread );
    pdata->light_z = fget_next_float( fileread );
    pdata->light_a = fget_next_float( fileread );

    return pdata;
}

//--------------------------------------------------------------------------------------------
wawalite_physics_t * read_wawalite_physics( vfs_FILE * fileread, wawalite_physics_t * pphys )
{
    if ( NULL == pphys ) return pphys;

    memset( pphys, 0, sizeof( *pphys ) );

    if ( NULL == fileread ) return pphys;

    // Read tile data third
    pphys->hillslide      = fget_next_float( fileread );
    pphys->slippyfriction = fget_next_float( fileread );
    pphys->airfriction    = fget_next_float( fileread );
    pphys->waterfriction  = fget_next_float( fileread );
    pphys->noslipfriction = fget_next_float( fileread );
    pphys->gravity        = fget_next_float( fileread );

    return pphys;
}

//--------------------------------------------------------------------------------------------
wawalite_animtile_t * read_wawalite_animtile( vfs_FILE * fileread, wawalite_animtile_t * panimtile )
{
    if ( NULL == panimtile ) return panimtile;

    memset( panimtile, 0, sizeof( *panimtile ) );

    if ( NULL == fileread ) return panimtile;

    // animated tile
    panimtile->update_and = fget_next_int( fileread );
    panimtile->frame_and  = fget_next_int( fileread );

    return panimtile;
}

//--------------------------------------------------------------------------------------------
wawalite_damagetile_t * read_wawalite_damagetile( vfs_FILE * fileread, wawalite_damagetile_t * pdamagetile )
{
    if ( NULL == pdamagetile ) return pdamagetile;
    if ( NULL == fileread ) return pdamagetile;
    wawalite_damagetile_init( pdamagetile );            //Reset

    // damage tile
    pdamagetile->amount     = fget_next_int( fileread );
    pdamagetile->damagetype = fget_next_damage_type( fileread );

    return pdamagetile;
}

//--------------------------------------------------------------------------------------------
wawalite_weather_t * read_wawalite_weather( vfs_FILE * fileread, wawalite_data_t * pdata )
{
    wawalite_weather_t * pweather = &( pdata->weather );
    if ( NULL == pweather ) return pweather;

    memset( pweather, 0, sizeof( *pweather ) );

    if ( NULL == fileread ) return pweather;

    // weather data
    if ( pdata->version >= 2 )
    {
        pweather->part_gpip = fget_next_int( fileread );          //@todo: allow text to be read here
    }
    else
    {
        pweather->part_gpip = PIP_WEATHER4;           //Default if we use a older versioned wawalite.txt
    }
    pweather->over_water  = fget_next_bool( fileread );
    pweather->timer_reset = fget_next_int( fileread );

    return pweather;
}

//--------------------------------------------------------------------------------------------
wawalite_graphics_t * read_wawalite_graphics( vfs_FILE * fileread, wawalite_graphics_t * pgraphics )
{
    if ( NULL == pgraphics ) return pgraphics;

    memset( pgraphics, 0, sizeof( *pgraphics ) );

    if ( NULL == fileread ) return pgraphics;

    // graphics options
    pgraphics->exploremode = fget_next_bool( fileread );
    pgraphics->usefaredge  = fget_next_bool( fileread );

    return pgraphics;
}

//--------------------------------------------------------------------------------------------
wawalite_camera_t * read_wawalite_camera( vfs_FILE * fileread, wawalite_camera_t * pcamera )
{
    if ( NULL == pcamera ) return pcamera;

    memset( pcamera, 0, sizeof( *pcamera ) );

    if ( NULL == fileread ) return pcamera;

    // camera data
    pcamera->swingrate = fget_next_float( fileread );
    pcamera->swingamp  = fget_next_float( fileread );

    return pcamera;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_fog( vfs_FILE * fileread, wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    if ( NULL == fileread ) return pdata;

    // Read unnecessary data...  Only read if it exists...
    if ( goto_colon( NULL, fileread, btrue ) )
    {
        pdata->fog.found         = btrue;
        pdata->fog.top           = fget_float( fileread );
        pdata->fog.bottom        = fget_next_float( fileread );
        pdata->fog.red           = fget_next_float( fileread ) * 255;
        pdata->fog.grn           = fget_next_float( fileread ) * 255;
        pdata->fog.blu           = fget_next_float( fileread ) * 255;
        pdata->fog.affects_water = fget_next_bool( fileread );

        // Read extra stuff for damage tile particles...
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            pdata->damagetile.part_gpip    = fget_int( fileread );
            pdata->damagetile.partand     = fget_next_int( fileread );
            pdata->damagetile.sound_index = fget_next_int( fileread );
        }
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_file_vfs( const char *filename, wawalite_data_t * pdata )
{
    /// @details ZZ@> This function sets up water and lighting for the module

    vfs_FILE*  fileread;
    STRING newloadname;

    if ( NULL == pdata ) pdata = &_wawalite_file;

    if ( NULL == wawalite_data_init( pdata ) ) return pdata;

    fileread = vfs_openRead( filename );
    if ( NULL == fileread )
    {
        log_warning( "Could not read file! (\"%s\")\n", newloadname );
        return pdata;
    }

    //First figure out what version of wawalite this is, so that we know what data we
    //should expect to load
    pdata->version = fget_version( fileread );

    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    pdata->seed = fget_next_bool( fileread );

    read_wawalite_water( fileread, &( pdata->water ) );
    read_wawalite_light( fileread, pdata );
    read_wawalite_physics( fileread, &( pdata->phys ) );
    read_wawalite_animtile( fileread, &( pdata->animtile ) );
    read_wawalite_damagetile( fileread, &( pdata->damagetile ) );
    read_wawalite_weather( fileread, pdata );
    read_wawalite_graphics( fileread, &( pdata->graphics ) );
    read_wawalite_camera( fileread, &( pdata->camera ) );
    read_wawalite_fog( fileread, pdata );

    vfs_close( fileread );

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t write_wawalite_water( vfs_FILE * filewrite, wawalite_water_t * pwater )
{
    if ( NULL == filewrite || NULL == pwater ) return bfalse;

    // Read water data first
    fput_int( filewrite, "Number of Water Layers ( 1=Fast, 2=Good )           :", pwater->layer_count );
    fput_int( filewrite, "Water specular start ( 0-255 )                      :", pwater->spek_start );
    fput_int( filewrite, "Water specular level ( 0-255 )                      :", pwater->spek_level );
    fput_float( filewrite, "Water douse level ( For torches, 90 )               :", pwater->douse_level );
    fput_float( filewrite, "Water surface level ( For water striders, 55 )      :", pwater->surface_level );
    fput_bool( filewrite, "Water light ( TRUE or FALSE )                       :", pwater->light );
    fput_bool( filewrite, "Water is really water? ( TRUE or FALSE )            :", pwater->is_water );
    fput_bool( filewrite, "Use watertop.bmp as an overlay? ( TRUE or FALSE )   :", pwater->overlay_req );
    fput_bool( filewrite, "Use waterlow.bmp as a background? ( TRUE or FALSE ) :", pwater->background_req );

    // General data info
    fput_float( filewrite, "Foreground distance effect X ( 0.0 to 1.0 )    :", pwater->layer[0].dist.x );
    fput_float( filewrite, "Foreground distance effect Y ( 0.0 to 1.0 )    :", pwater->layer[0].dist.y );
    fput_float( filewrite, "Background distance effect X ( 0.0 to 1.0 )    :", pwater->layer[1].dist.x );
    fput_float( filewrite, "Background distance effect Y ( 0.0 to 1.0 )    :", pwater->layer[1].dist.y );
    fput_float( filewrite, "Number of foreground repeats ( 1 to 5 )        :", pwater->foregroundrepeat );
    fput_float( filewrite, "Number of background repeats ( 1 to 5 )        :", pwater->backgroundrepeat );

    // Read data on first water layer
    fput_float( filewrite, "Level 0... Base water level ( 85 )  :", pwater->layer[0].z );
    fput_int( filewrite, "Level 0... Alpha level ( 100 )      :", pwater->layer[0].alpha );
    fput_int( filewrite, "Level 0... Wave speed ( 3 )         :", pwater->layer[0].frame_add );
    fput_int( filewrite, "Level 0... Brightness ( 15 )        :", pwater->layer[0].light_dir );
    fput_int( filewrite, "Level 0... Ambient light ( 15 )     :", pwater->layer[0].light_add );
    fput_float( filewrite, "Level 0... Wave amplitude ( 7.0 )   :", pwater->layer[0].amp );
    fput_float( filewrite, "Level 0... U speed ( .0002 )        :", pwater->layer[0].tx_add.s );
    fput_float( filewrite, "Level 0... V speed ( .0002 )        :", pwater->layer[0].tx_add.t );

    // Read data on second water layer
    fput_int( filewrite, "Level 1... Base water level ( 85 )  :",  pwater->layer[1].z );
    fput_int( filewrite, "Level 1... Alpha level ( 100 )      :",  pwater->layer[1].alpha );
    fput_int( filewrite, "Level 1... Wave speed ( 3 )         :",  pwater->layer[1].frame_add );
    fput_int( filewrite, "Level 1... Brightness ( 15 )        :",  pwater->layer[1].light_dir );
    fput_int( filewrite, "Level 1... Ambient light ( 15 )     :",  pwater->layer[1].light_add );
    fput_float( filewrite, "Level 1... Wave amplitude ( 7.0 )   :",  pwater->layer[1].amp );
    fput_float( filewrite, "Level 1... U speed ( .0002 )        :",  pwater->layer[1].tx_add.s );
    fput_float( filewrite, "Level 1... V speed ( .0002 )        :",  pwater->layer[1].tx_add.t );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_light( vfs_FILE * filewrite, wawalite_data_t * pdata )
{
    if ( NULL == filewrite || NULL == pdata ) return bfalse;

    // Read light data second
    fput_float( filewrite, "Light X direction ( 1.00 )        :", pdata->light_x );
    fput_float( filewrite, "Light Y direction ( 1.00 )        :", pdata->light_y );
    fput_float( filewrite, "Light Z direction ( 0.50 )        :", pdata->light_z );
    fput_float( filewrite, "Ambient light ( 0.20 )            :", pdata->light_a );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_physics( vfs_FILE * filewrite, wawalite_physics_t * pphys )
{
    if ( NULL == filewrite || NULL == pphys ) return bfalse;

    // Read tile data third
    fput_float( filewrite, "Hillslide ( 1.00 )              :", pphys->hillslide );
    fput_float( filewrite, "Slippy friction ( 1.00 )        :", pphys->slippyfriction );
    fput_float( filewrite, "Air friction ( .95 )            :", pphys->airfriction );
    fput_float( filewrite, "Water friction ( .85 )          :", pphys->waterfriction );
    fput_float( filewrite, "Normal friction ( .95 )         :", pphys->noslipfriction );
    fput_float( filewrite, "Gravity ( -1.0 )                :", pphys->gravity );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_animtile( vfs_FILE * filewrite, wawalite_animtile_t * panimtile )
{
    if ( NULL == filewrite || NULL == panimtile ) return bfalse;

    // animated tile
    fput_int( filewrite, "Animated tile update AND ( 0, 1, 3, 7, 15, 31 )        :", panimtile->update_and );
    fput_int( filewrite, "Animated tile frame AND ( 3 == 4 frame, 7 == 8 frame ) :", panimtile->frame_and );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_damagetile( vfs_FILE * filewrite, wawalite_damagetile_t * pdamagetile )
{
    if ( NULL == filewrite || NULL == pdamagetile ) return bfalse;

    // basic damage tile
    fput_int( filewrite, "Damage tile damage ( 0 to 65535, 512 is 1 life block )  :", pdamagetile->amount );
    fput_damage_type( filewrite, "Damage tile damage type ( SLASH, CRUSH, POKE, HOLY\n                          EVIL, FIRE, ICE, ZAP )  :", pdamagetile->damagetype );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_weather( vfs_FILE * filewrite, wawalite_weather_t * pweather )
{
    if ( NULL == filewrite || NULL == pweather ) return bfalse;

    // weather data
    fput_int( filewrite, "Weather particle effect ( 0 to 10, RAIN or SNOW )  :", pweather->part_gpip );
    fput_bool( filewrite, "Weather particles only over water ( TRUE or FALSE )  :", pweather->over_water );
    fput_int( filewrite, "Weather particle spawn rate ( 0 to 100, 0 is none )  :", pweather->timer_reset );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_graphics( vfs_FILE * filewrite, wawalite_graphics_t * pgraphics )
{
    if ( NULL == filewrite || NULL == pgraphics ) return bfalse;

    // graphics options
    fput_bool( filewrite, "Explore mode ( TRUE or FALSE )                         :", pgraphics->exploremode );
    fput_bool( filewrite, "Far Edge mode...  For outside levels ( TRUE or FALSE ) :", pgraphics->usefaredge );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_camera( vfs_FILE * filewrite, wawalite_camera_t * pcamera )
{
    if ( NULL == filewrite || NULL == pcamera ) return bfalse;

    // camera data
    fput_float( filewrite, "Camera swing rate ( 0 to 100 )                :", pcamera->swingrate );
    fput_float( filewrite, "Camera swing amplitude ( 0, or .002 to .100 ) :", pcamera->swingamp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_fog( vfs_FILE * filewrite, wawalite_data_t * pdata )
{
    if ( NULL == filewrite || NULL == pdata ) return bfalse;

    // write optional data...  Only read if it exists...
    if ( !pdata->fog.found ) return btrue;

    vfs_printf( filewrite, "\n\n// Fog Expansion...  Leave this out for no fog...\n" );
    fput_float( filewrite, "Fog top z ( 0 to 100 )                            :", pdata->fog.top );
    fput_float( filewrite, "Fog bottom z ( 0 )                                :", pdata->fog.bottom );
    fput_float( filewrite, "Fog Red ( 0.0 to 1.0 )                            :", pdata->fog.red );
    fput_float( filewrite, "Fog Green ( 0.0 to 1.0 )                          :", pdata->fog.grn );
    fput_float( filewrite, "Fog Blue ( 0.0 to 1.0 )                           :", pdata->fog.blu );
    fput_bool( filewrite, "Fog affects water ( TRUE or FALSE )               :", pdata->fog.affects_water );

    vfs_printf( filewrite, "\n\n// Damage tile expansion...  Must have fog first...\n" );
    fput_int( filewrite, "Weather particle to spawn ( 4 or 5, 6 is splash )  :", pdata->damagetile.part_gpip );
    fput_int( filewrite, "Particle timing AND ( 1, 3, 7, 15, etc. )          :", pdata->damagetile.partand );
    fput_int( filewrite, "Damage sound ( 0 to 4 )                            :", pdata->damagetile.sound_index );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite_file_vfs( wawalite_data_t * pdata )
{
    /// @details ZZ@> This function sets up water and lighting for the module

    vfs_FILE*  filewrite;
    STRING newloadname;

    if ( NULL == pdata ) return bfalse;

    filewrite = vfs_openWrite( "mp_data/wawalite.txt" );
    if ( NULL == filewrite )
    {
        log_warning( "Could not write file! (\"%s\")\n", newloadname );
        return bfalse;
    }

    // file header
    vfs_printf( filewrite, "//   This file tells the game how to model lighting and water...\n" );
    vfs_printf( filewrite, "// Please fill in all of the data even if you only use one layer of\n" );
    vfs_printf( filewrite, "// water.  2 is the maximum number of layers.\n" );
    vfs_printf( filewrite, "//   This file also gives information regarding damage tiles and\n" );
    vfs_printf( filewrite, "// friction for the module.\n" );
    vfs_printf( filewrite, "\n\n" );

    // random map
    fput_int( filewrite, "Random map ( TRUE or FALSE ) ( doesn't work )           :", pdata->seed );

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

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t wawalite_water_init( wawalite_water_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    pdata->spek_start =   128;
    pdata->spek_level =   128;
    pdata->is_water   = btrue;

    pdata->foregroundrepeat = 1;
    pdata->backgroundrepeat = 1;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t wawalite_weather_init( wawalite_weather_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    pdata->timer_reset = 10;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t wawalite_fog_init( wawalite_fog_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    pdata->top           = 100;
    pdata->bottom        = 0.0f;
    pdata->red           = 255;
    pdata->grn           = 255;
    pdata->blu           = 255;
    pdata->affects_water = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t wawalite_animtile_init( wawalite_animtile_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    pdata->update_and    = 7;                        // New tile every 7 frames
    pdata->frame_and     = 3;              // Only 4 frames

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t wawalite_damagetile_init( wawalite_damagetile_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    pdata->part_gpip    = -1;
    pdata->partand     = 255;
    pdata->sound_index = INVALID_SOUND;
    pdata->damagetype  = DAMAGE_FIRE;
    pdata->amount      = 256;

    return btrue;
}

