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

#include "egoboo_setup.h"

#include "log.h"
#include "configfile.h"
#include "graphic.h"
#include "input.h"
#include "particle.h"
#include "sound.h"
#include "network.h"
#include "camera.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static STRING _config_filename = EMPTY_CSTR;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Macros for reading values from a ConfigFile
//  - Must have a valid ConfigFilePtr_t named lConfigSetup
//  - Must have a string named lCurSectionName to define the section
//  - Must have temporary variables defined of the correct type (lTempBool, lTempInt, and lTempStr)

#define GetKey_bool(label, var, default) \
    { \
        if ( ConfigFile_GetValue_Boolean( lConfigSetup, lCurSectionName, (label), &lTempBool ) == 0 ) \
        { \
            lTempBool = (default); \
        } \
        (var) = lTempBool; \
    }

#define GetKey_int(label, var, default) \
    { \
        if ( ConfigFile_GetValue_Int( lConfigSetup, lCurSectionName, (label), &lTempInt ) == 0 ) \
        { \
            lTempInt = (default); \
        } \
        (var) = lTempInt; \
    }

// Don't make len larger than 64
#define GetKey_string(label, var, len, default) \
    { \
        if ( ConfigFile_GetValue_String( lConfigSetup, lCurSectionName, (label), lTempStr, SDL_arraysize( lTempStr ) ) == 0 ) \
        { \
            strncpy( lTempStr, (default), SDL_arraysize( lTempStr ) ); \
        } \
        strncpy( (var), lTempStr, (len) ); \
        (var)[(len) - 1] = CSTR_END; \
    }

#define SetKey_bool(label, var)     ConfigFile_SetValue_Boolean( lConfigSetup, lCurSectionName, (label), (var) )
#define SetKey_int(label, var)      ConfigFile_SetValue_Int( lConfigSetup, lCurSectionName, (label), (var) )
#define SetKey_string( label, var ) ConfigFile_SetValue_String( lConfigSetup, lCurSectionName, (label), (var) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ConfigFilePtr_t lConfigSetup = NULL;
static egoboo_config_t cfg_default;

egoboo_config_t cfg;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void egoboo_config_init()
{
    memset( &cfg_default, 0, sizeof(egoboo_config_t) );

    // {GRAPHIC}
    cfg_default.fullscreen_req        = bfalse;        // Start in fullscreen?
    cfg_default.scrd_req              = 32;                 // Screen bit depth
    cfg_default.scrz_req              = 8;                // Screen z-buffer depth ( 8 unsupported )
    cfg_default.scrx_req              = 640;               // Screen X size
    cfg_default.scry_req              = 480;               // Screen Y size
    cfg_default.message_count_req     = 6;
    cfg_default.message_duration      = 50;                     // Time to keep the message alive
    cfg_default.StatusList_on                = btrue;               // Draw the status bars?
    cfg_default.use_perspective       = bfalse;      // Perspective correct textures?
    cfg_default.use_dither            = bfalse;           // Dithering?
    cfg_default.reflect_fade          = btrue;            // 255 = Don't fade reflections
    cfg_default.reflect_allowed       = bfalse;            // Reflections?
    cfg_default.reflect_prt           = bfalse;         // Reflect particles?
    cfg_default.shadow_allowed        = bfalse;            // Shadows?
    cfg_default.shadow_sprite         = bfalse;        // Shadow sprites?
    cfg_default.use_phong             = btrue;              // Do phong overlay?
    cfg_default.twolayerwater_allowed = btrue;      // Two layer water?
    cfg_default.overlay_allowed       = bfalse;               // Allow large overlay?
    cfg_default.background_allowed    = bfalse;            // Allow large background?
    cfg_default.fog_allowed           = btrue;
    cfg_default.gourard_req           = btrue;              // Gourad shading?
    cfg_default.multisamples          = 0;                  // Antialiasing?
    cfg_default.texturefilter_req     = TX_UNFILTERED;      // Texture filtering?
    cfg_default.dyna_count_req        = 12;                 // Max number of lights to draw
    cfg_default.framelimit            = 30;
    cfg_default.particle_count_req    = 512;                              // max number of particles

    // {SOUND}
    cfg_default.sound_allowed         = bfalse;
    cfg_default.music_allowed         = bfalse;
    cfg_default.music_volume          = 50;                            // The sound volume of music
    cfg_default.sound_volume          = 75;          // Volume of sounds played
    cfg_default.sound_channel_count   = 16;      // Max number of sounds playing at the same time
    cfg_default.sound_buffer_size     = 2048;

    // {CONTROL}
    cfg_default.autoturncamera        = 255;             // Type of camera control...

    // {NETWORK}
    cfg_default.network_allowed       = bfalse;            // Try to connect?
    cfg_default.network_lag           = 2;                             // Lag tolerance
    strncpy( cfg_default.network_hostname,    "no host",      SDL_arraysize(cfg_default.network_hostname)   );                            // Name for hosting session
    strncpy( cfg_default.network_messagename, "little Raoul", SDL_arraysize(cfg_default.network_messagename));                         // Name for messages

    // {DEBUG}
    cfg_default.fps_allowed       = btrue;             // FPS displayed?
    cfg_default.grab_mouse        = btrue;
    cfg_default.hide_mouse        = btrue;
    cfg_default.dev_mode          = bfalse;
    cfg_default.sdl_image_allowed = btrue;    // Allow advanced SDL_Image functions?
    cfg_default.difficulty        = GAME_NORMAL;    // What is the current game difficulty
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t setup_quit()
{
    return ConfigFile_succeed == ConfigFile_destroy( &lConfigSetup );
}

//--------------------------------------------------------------------------------------------
bool_t setup_read( const char* filename )
{
    /// @details BB@> read the setup file

    if( INVALID_CSTR(filename) ) return bfalse;

    strncpy( _config_filename, filename, SDL_arraysize(_config_filename) );

    // do NOT force the file to open in a read directory if it doesn't exist. this will cause a failure in
    // linux if the directory is read-only
    lConfigSetup = LoadConfigFile( vfs_resolveReadFilename( _config_filename ), bfalse );

    // if the file can't be opened, try to create something
    if( NULL == lConfigSetup )
    {
        // this will actually create the file within the write directory
        // more of a hack than an ideal solution
        vfs_FILE * ftmp = vfs_openAppend( _config_filename );
        vfs_close( ftmp );

        // now try to read it from the write directory (which should be ahead of all the
        // read directories on the search path
        lConfigSetup = LoadConfigFile( vfs_resolveReadFilename( _config_filename ), bfalse );

    }

    return NULL != lConfigSetup;
}

//--------------------------------------------------------------------------------------------
bool_t setup_write()
{
    /// @details BB@> save the current setup file

    if( INVALID_CSTR(_config_filename) ) return bfalse;

    return ConfigFile_succeed == SaveConfigFileAs( lConfigSetup, vfs_resolveWriteFilename( _config_filename ) );
}

//--------------------------------------------------------------------------------------------
bool_t setup_download(egoboo_config_t * pcfg)
{
    /// @details BB@> download the ConfigFile_t keys into game variables
    ///     use default values to fill in any missing keys

    char  *lCurSectionName;
    bool_t lTempBool;
    Sint32 lTempInt;
    STRING lTempStr;

    if (NULL == lConfigSetup || NULL == pcfg) return bfalse;

    // set the default egoboo values
    egoboo_config_init();

    memcpy( pcfg, &cfg_default, sizeof(egoboo_config_t) );

    //*********************************************
    //* GRAPHIC Section
    //*********************************************

    lCurSectionName = "GRAPHIC";

    // Do fullscreen?
    GetKey_bool( "FULLSCREEN", pcfg->fullscreen_req, cfg_default.fullscreen_req );

    // Screen Size
    GetKey_int( "SCREENSIZE_X", pcfg->scrx_req, cfg_default.scrx_req );
    GetKey_int( "SCREENSIZE_Y", pcfg->scry_req, cfg_default.scry_req );

    // Color depth
    GetKey_int( "COLOR_DEPTH", pcfg->scrd_req, cfg_default.scrd_req );

    // The z depth
    GetKey_int( "Z_DEPTH", pcfg->scrz_req, cfg_default.scrz_req );

    // Max number of messages displayed
    GetKey_int( "MAX_TEXT_MESSAGE", pcfg->message_count_req, cfg_default.message_count_req );

    // Max number of messages displayed
    GetKey_int( "MESSAGE_DURATION", pcfg->message_duration, cfg_default.message_duration );

    // Show status bars? (Life, mana, character icons, etc.)
    GetKey_bool( "STATUS_BAR", pcfg->StatusList_on, cfg_default.StatusList_on );

    // Perspective correction
    GetKey_bool( "PERSPECTIVE_CORRECT", pcfg->use_perspective, cfg_default.use_perspective );

    // Enable dithering?
    GetKey_bool( "DITHERING", pcfg->use_dither, cfg_default.use_dither );

    // Reflection fadeout
    GetKey_bool( "FLOOR_REFLECTION_FADEOUT", pcfg->reflect_fade, cfg_default.reflect_fade );

    // Draw Reflection?
    GetKey_bool( "REFLECTION", pcfg->reflect_allowed, cfg_default.reflect_allowed );

    // Draw particles in reflection?
    GetKey_bool( "PARTICLE_REFLECTION", pcfg->reflect_prt, cfg_default.reflect_prt );

    // Draw shadows?
    GetKey_bool( "SHADOWS", pcfg->shadow_allowed, cfg_default.shadow_allowed );

    // Draw good shadows?
    GetKey_bool( "SHADOW_AS_SPRITE", pcfg->shadow_sprite, cfg_default.shadow_sprite );

    // Draw phong mapping?
    GetKey_bool( "PHONG", pcfg->use_phong, cfg_default.use_phong );

    // Draw water with more layers?
    GetKey_bool( "MULTI_LAYER_WATER", pcfg->twolayerwater_allowed, cfg_default.twolayerwater_allowed );

    // Allow overlay effects?
    GetKey_bool( "OVERLAY", pcfg->overlay_allowed, cfg_default.overlay_allowed );

    // Allow backgrounds?
    GetKey_bool( "BACKGROUND", pcfg->background_allowed, cfg_default.background_allowed );

    // Enable fog?
    GetKey_bool( "FOG", pcfg->fog_allowed, cfg_default.fog_allowed );

    // Do gourad shading?
    GetKey_bool( "GOURAUD_SHADING", pcfg->gourard_req, cfg_default.gourard_req );

    // Enable antialiasing?
    GetKey_int( "ANTIALIASING", pcfg->multisamples, cfg_default.multisamples );

    // coerce a "valid" multisample value
    pcfg->multisamples = CLIP(pcfg->multisamples, 0, 4);

    // Do we do texture filtering?
    GetKey_string( "TEXTURE_FILTERING", lTempStr, 24, "LINEAR" );
    pcfg->texturefilter_req =  cfg_default.texturefilter_req;
    if ( 'U' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_UNFILTERED;
    if ( 'L' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_LINEAR;
    if ( 'M' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_MIPMAP;
    if ( 'B' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_BILINEAR;
    if ( 'T' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_TRILINEAR_1;
    if ( '2' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_TRILINEAR_2;
    if ( 'A' == toupper(lTempStr[0]) )  pcfg->texturefilter_req = TX_ANISOTROPIC;

    // Max number of lights
    GetKey_int( "MAX_DYNAMIC_LIGHTS", pcfg->dyna_count_req, cfg_default.dyna_count_req );

    // Get the FPS limit
    GetKey_int( "MAX_FPS_LIMIT", pcfg->framelimit, 30 );

    // Get the particle limit
    GetKey_int( "MAX_PARTICLES", pcfg->particle_count_req, cfg_default.particle_count_req );

    //*********************************************
    //* SOUND Section
    //*********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    GetKey_bool( "SOUND", pcfg->sound_allowed, cfg_default.sound_allowed );

    // Enable music
    GetKey_bool( "MUSIC", pcfg->music_allowed, cfg_default.music_allowed );

    // Music volume
    GetKey_int( "MUSIC_VOLUME", pcfg->music_volume, cfg_default.music_volume );

    // Sound volume
    GetKey_int( "SOUND_VOLUME", pcfg->sound_volume, cfg_default.sound_volume );

    // Max number of sound channels playing at the same time
    GetKey_int( "MAX_SOUND_CHANNEL", pcfg->sound_channel_count, cfg_default.sound_channel_count );

    // The output buffer size
    GetKey_int( "OUTPUT_BUFFER_SIZE", pcfg->sound_buffer_size, cfg_default.sound_buffer_size );

    // Extra high sound quality?
    GetKey_bool( "HIGH_SOUND_QUALITY", pcfg->sound_highquality, cfg_default.sound_highquality );

    //*********************************************
    //* CONTROL Section
    //*********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    GetKey_string( "AUTOTURN_CAMERA", lTempStr, 24, "GOOD" );
    pcfg->autoturncamera = cfg_default.autoturncamera;
    if ( 'G' == toupper(lTempStr[0]) )  pcfg->autoturncamera = CAMTURN_GOOD;
    else if ( 'T' == toupper(lTempStr[0]) )  pcfg->autoturncamera = CAMTURN_AUTO;
    else if ( 'F' == toupper(lTempStr[0]) )  pcfg->autoturncamera = CAMTURN_NONE;

    //*********************************************
    //* NETWORK Section
    //*********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    GetKey_bool( "NETWORK_ON", pcfg->network_allowed, cfg_default.network_allowed );

    // Max lag
    GetKey_int( "LAG_TOLERANCE", pcfg->network_lag, cfg_default.network_lag );

    // Name or IP of the host or the target to join
    GetKey_string( "HOST_NAME", pcfg->network_hostname, SDL_arraysize(pcfg->network_hostname), cfg_default.network_hostname );

    // Multiplayer name
    GetKey_string( "MULTIPLAYER_NAME", pcfg->network_messagename, SDL_arraysize(pcfg->network_messagename), cfg_default.network_messagename );

    //*********************************************
    //* DEBUG Section
    //*********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    GetKey_bool( "DISPLAY_FPS", pcfg->fps_allowed,       cfg_default.fps_allowed );
    GetKey_bool( "HIDE_MOUSE",  pcfg->hide_mouse,        cfg_default.hide_mouse );
    GetKey_bool( "GRAB_MOUSE",  pcfg->grab_mouse,        cfg_default.grab_mouse );
    GetKey_bool( "DEV_MODE",    pcfg->dev_mode,          cfg_default.dev_mode );
    GetKey_bool( "SDL_IMAGE",   pcfg->sdl_image_allowed, cfg_default.sdl_image_allowed );

    // Which difficulty mode do we use?
    GetKey_string( "DIFFICULTY_MODE", lTempStr, 24, "NORMAL" );
    pcfg->difficulty = cfg_default.difficulty;
    if ( 'E' == toupper(lTempStr[0]) )  pcfg->difficulty = GAME_EASY;
    if ( 'N' == toupper(lTempStr[0]) )  pcfg->difficulty = GAME_NORMAL;
    if ( 'H' == toupper(lTempStr[0]) )  pcfg->difficulty = GAME_HARD;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_synch( egoboo_config_t * pcfg )
{
    if ( NULL == pcfg ) return bfalse;

    // FPS display
    fpson = pcfg->fps_allowed;

    // message display
    messageon  = (pcfg->message_count_req > 0);
    maxmessage = CLIP(pcfg->message_count_req, 1, MAX_MESSAGE);

    wraptolerance = pcfg->StatusList_on ? 90 : 32;

    // Get the particle limit
    maxparticles = CLIP(pcfg->particle_count_req, 0, TOTAL_MAX_PRT);

    // sound options
    snd_config_synch( &snd, pcfg );

    // renderer options
    gfx_config_synch( &gfx, pcfg );

    // texture options
    oglx_texture_parameters_synch( &tex_params, pcfg );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_upload( egoboo_config_t * pcfg )
{
    /// @details BB@> upload game variables into the ConfigFile_t keys

    char  *lCurSectionName;
    if (NULL == lConfigSetup || NULL == pcfg) return bfalse;

    //*********************************************
    //* GRAPHIC Section
    //*********************************************

    lCurSectionName = "GRAPHIC";

    // Do fullscreen?
    SetKey_bool( "FULLSCREEN", pcfg->fullscreen_req );

    // Screen Size
    SetKey_int( "SCREENSIZE_X", pcfg->scrx_req );
    SetKey_int( "SCREENSIZE_Y", pcfg->scry_req );

    // Color depth
    SetKey_int( "COLOR_DEPTH", pcfg->scrd_req );

    // The z depth
    SetKey_int( "Z_DEPTH", pcfg->scrz_req );

    // Max number of messages displayed
    SetKey_int( "MAX_TEXT_MESSAGE", messageon ? pcfg->message_count_req : 0 );

    // Max number of messages displayed
    SetKey_int( "MESSAGE_DURATION", pcfg->message_duration );

    // Show status bars? (Life, mana, character icons, etc.)
    SetKey_bool( "STATUS_BAR", pcfg->StatusList_on );

    // Perspective correction
    SetKey_bool( "PERSPECTIVE_CORRECT", pcfg->use_perspective );

    // Enable dithering?
    SetKey_bool( "DITHERING", pcfg->use_dither );

    // Reflection fadeout
    SetKey_bool( "FLOOR_REFLECTION_FADEOUT", pcfg->reflect_fade );

    // Draw Reflection?
    SetKey_bool( "REFLECTION", pcfg->reflect_allowed );

    // Draw particles in reflection?
    SetKey_bool( "PARTICLE_REFLECTION", pcfg->reflect_prt );

    // Draw shadows?
    SetKey_bool( "SHADOWS", pcfg->shadow_allowed );

    // Draw good shadows?
    SetKey_bool( "SHADOW_AS_SPRITE", pcfg->shadow_sprite );

    // Draw phong mapping?
    SetKey_bool( "PHONG", pcfg->use_phong );

    // Draw water with more layers?
    SetKey_bool( "MULTI_LAYER_WATER", pcfg->twolayerwater_allowed );

    // Allow overlay effects?
    SetKey_bool( "OVERLAY", pcfg->overlay_allowed );

    // Allow backgrounds?
    SetKey_bool( "BACKGROUND", pcfg->background_allowed );

    // Enable fog?
    SetKey_bool( "FOG", pcfg->fog_allowed );

    // Do gourad shading?
    SetKey_bool( "GOURAUD_SHADING", pcfg->gourard_req );

    // Enable antialiasing?
    SetKey_int( "ANTIALIASING", pcfg->multisamples );

    // Do we do texture filtering?
    switch (pcfg->texturefilter_req)
    {
        case TX_UNFILTERED:  SetKey_string( "TEXTURE_FILTERING", "UNFILTERED" ); break;
        case TX_MIPMAP:      SetKey_string( "TEXTURE_FILTERING", "MIPMAP" ); break;
        case TX_BILINEAR:    SetKey_string( "TEXTURE_FILTERING", "BILINEAR" ); break;
        case TX_TRILINEAR_1: SetKey_string( "TEXTURE_FILTERING", "TRILINEAR" ); break;
        case TX_TRILINEAR_2: SetKey_string( "TEXTURE_FILTERING", "2_TRILINEAR" ); break;
        case TX_ANISOTROPIC: SetKey_string( "TEXTURE_FILTERING", "ANISOTROPIC" ); break;

        default:
        case TX_LINEAR:      SetKey_string( "TEXTURE_FILTERING", "LINEAR" ); break;
    }

    // Max number of lights
    SetKey_int( "MAX_DYNAMIC_LIGHTS", pcfg->dyna_count_req );

    // Get the FPS limit
    SetKey_int( "MAX_FPS_LIMIT", pcfg->framelimit );

    // Get the particle limit
    SetKey_int( "MAX_PARTICLES", pcfg->particle_count_req );

    //*********************************************
    //* SOUND Section
    //*********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    SetKey_bool( "SOUND", pcfg->sound_allowed );

    // Enable music
    SetKey_bool( "MUSIC", pcfg->music_allowed );

    // Music volume
    SetKey_int( "MUSIC_VOLUME", pcfg->music_volume );

    // Sound volume
    SetKey_int( "SOUND_VOLUME", pcfg->sound_volume );

    // Max number of sound channels playing at the same time
    SetKey_int( "MAX_SOUND_CHANNEL", pcfg->sound_channel_count );

    // The output buffer size
    SetKey_int( "OUTPUT_BUFFER_SIZE", pcfg->sound_buffer_size );

    // Extra high sound quality
    SetKey_bool( "HIGH_SOUND_QUALITY", pcfg->sound_highquality );

    //*********************************************
    //* CONTROL Section
    //*********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    switch ( pcfg->autoturncamera )
    {
        case CAMTURN_NONE: SetKey_bool( "AUTOTURN_CAMERA", bfalse ); break;
        case CAMTURN_GOOD:    SetKey_string( "AUTOTURN_CAMERA", "GOOD" ); break;

        default:
        case CAMTURN_AUTO : SetKey_bool( "AUTOTURN_CAMERA", btrue );  break;
    }

    //*********************************************
    //* NETWORK Section
    //*********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    SetKey_bool( "NETWORK_ON", pcfg->network_allowed );

    // Name or IP of the host or the target to join
    SetKey_string( "HOST_NAME", pcfg->network_hostname );

    // Multiplayer name
    SetKey_string( "MULTIPLAYER_NAME", pcfg->network_messagename );

    // Max lag
    SetKey_int( "LAG_TOLERANCE", pcfg->network_lag );

    //*********************************************
    //* DEBUG Section
    //*********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    SetKey_bool( "DISPLAY_FPS", pcfg->fps_allowed );
    SetKey_bool( "HIDE_MOUSE",  pcfg->hide_mouse );
    SetKey_bool( "GRAB_MOUSE",  pcfg->grab_mouse );
    SetKey_bool( "DEV_MODE",    pcfg->dev_mode );
    SetKey_bool( "SDL_IMAGE",   pcfg->sdl_image_allowed );

    // Save diffculty mode
    switch (pcfg->difficulty)
    {
        case GAME_EASY:         SetKey_string( "DIFFICULTY_MODE", "EASY" ); break;
        case GAME_HARD:         SetKey_string( "DIFFICULTY_MODE", "HARD" ); break;

        default:
        case GAME_NORMAL:       SetKey_string( "DIFFICULTY_MODE", "NORMAL" ); break;
    }

    return btrue;
}

