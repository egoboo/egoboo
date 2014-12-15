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

/// @file egoboo_setup.c
/// @brief Functions for handling the setup.txt file
/// @details

#include "egolib/egoboo_setup.h"

#include "egolib/log.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/file_formats/configfile.h"
#include "egolib/extensions/ogl_texture.h"

#include "egolib/_math.inl"

// includes for egoboo constants
#include "game/camera.h"            // for CAM_TURN_*
#include "game/renderer_2d.h"       // for EGO_MESSAGE_MAX

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Macros for reading values from a ConfigFile
//  - Must have a valid ConfigFilePtr_t named _lpConfigSetup
//  - Must have a string named lCurSectionName to define the section
//  - Must have temporary variables defined of the correct type (lTempBool, lTempInt, and lTempStr)

#define GetKey_bool(LABEL, VAR, DEFAULT) \
    { \
        if ( 0 == ConfigFile_GetValue_Boolean( _lpConfigSetup, lCurSectionName, LABEL, &lTempBool ) ) \
        { \
            lTempBool = DEFAULT; \
        } \
        VAR = lTempBool; \
    }

#define GetKey_int(LABEL, VAR, DEFAULT) \
    { \
        if ( 0 == ConfigFile_GetValue_Int( _lpConfigSetup, lCurSectionName, LABEL, &lTempInt ) ) \
        { \
            lTempInt = DEFAULT; \
        } \
        VAR = lTempInt; \
    }

// Don't make LEN larger than 64
#define GetKey_string(LABEL, VAR, LEN, DEFAULT) \
    { \
        if ( 0 == ConfigFile_GetValue_String( _lpConfigSetup, lCurSectionName, LABEL, lTempStr, SDL_arraysize( lTempStr ) ) ) \
        { \
            strncpy( lTempStr, DEFAULT, SDL_arraysize( lTempStr ) ); \
        } \
        if ( lTempStr != VAR ) \
        { \
            strncpy( VAR, lTempStr, LEN ); \
        } \
        VAR[(LEN) - 1] = CSTR_END; \
    }

#define SetKey_bool(LABEL, VAR)     ConfigFile_SetValue_Boolean( _lpConfigSetup, lCurSectionName, LABEL, VAR )
#define SetKey_int(LABEL, VAR)      ConfigFile_SetValue_Int( _lpConfigSetup, lCurSectionName, LABEL, VAR )
#define SetKey_string( LABEL, VAR ) ConfigFile_SetValue_String( _lpConfigSetup, lCurSectionName, LABEL, VAR )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static C_BOOLEAN _setup_started = C_FALSE;
static STRING _config_filename = EMPTY_CSTR;
static ConfigFilePtr_t _lpConfigSetup = NULL;

static egoboo_config_t cfg_default;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

egoboo_config_t  cfg;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void egoboo_config__init( egoboo_config_t * pcfg );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C"
{
#endif
/// download the data from the egoboo_config_t data structure to program
/// @note this function must be implemented by the user
    extern C_BOOLEAN config_download( egoboo_config_t * pcfg, C_BOOLEAN synch_from_file );

/// convert program settings to an egoboo_config_t data structure
/// @note this function must be implemented by the user
    extern C_BOOLEAN config_upload( egoboo_config_t * pcfg );

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void egoboo_config__init( egoboo_config_t * pcfg )
{
    // {GRAPHIC}
    pcfg->fullscreen_req        = C_FALSE;        // Start in fullscreen?
    pcfg->scrd_req              = 24;                 // Screen bit depth
    pcfg->scrz_req              = 8;                // Screen z-buffer depth ( 8 unsupported )
    pcfg->scrx_req              = 640;               // Screen X size
    pcfg->scry_req              = 480;               // Screen Y size
    pcfg->use_perspective       = C_FALSE;      // Perspective correct textures?
    pcfg->use_dither            = C_FALSE;           // Dithering?
    pcfg->reflect_fade          = C_TRUE;            // 255 = Don't fade reflections
    pcfg->reflect_allowed       = C_FALSE;            // Reflections?
    pcfg->reflect_prt           = C_FALSE;         // Reflect particles?
    pcfg->shadow_allowed        = C_FALSE;            // Shadows?
    pcfg->shadow_sprite         = C_FALSE;        // Shadow sprites?
    pcfg->use_phong             = C_TRUE;              // Do phong overlay?
    pcfg->twolayerwater_allowed = C_TRUE;      // Two layer water?
    pcfg->overlay_allowed       = C_FALSE;               // Allow large overlay?
    pcfg->background_allowed    = C_FALSE;            // Allow large background?
    pcfg->fog_allowed           = C_TRUE;
    pcfg->gouraud_req           = C_TRUE;              // Gouraud shading?
    pcfg->multisamples          = 0;                  // Antialiasing?
    pcfg->texturefilter_req     = TX_UNFILTERED;      // Texture filtering?
    pcfg->dyna_count_req        = 12;                 // Max number of lights to draw
    pcfg->framelimit            = 30;
    pcfg->particle_count_req    = 512;                              // max number of particles

    // {SOUND}
    pcfg->sound_allowed         = C_FALSE;
    pcfg->music_allowed         = C_FALSE;
    pcfg->music_volume          = 50;               // The sound volume of music
    pcfg->sound_volume          = 75;               // Volume of sounds played
    pcfg->sound_channel_count   = 16;               // Max number of sounds playing at the same time
    pcfg->sound_buffer_size     = 2048;             // Buffer chunk size
    pcfg->sound_highquality     = C_FALSE;           // High quality sounds
    pcfg->sound_footfall        = C_TRUE;            // Play footstep sounds

    // {NETWORK}
    pcfg->network_allowed       = C_FALSE;            // Try to connect?
    pcfg->network_lag           = 2;                             // Lag tolerance
    strncpy( pcfg->network_hostname,    "no host",      SDL_arraysize( pcfg->network_hostname ) );                            // Name for hosting session
    strncpy( pcfg->network_messagename, "little Raoul", SDL_arraysize( pcfg->network_messagename ) );                      // Name for messages

    // {GAME}
    pcfg->message_count_req     = 6;
    pcfg->message_duration      = 50;               // Time to keep the message alive
    pcfg->show_stats            = C_TRUE;            // Draw the status bars?
    pcfg->autoturncamera        = CAM_TURN_GOOD;    // Type of camera control...
    pcfg->feedback              = FEEDBACK_TEXT;    // What feedback does the player want
    pcfg->difficulty            = GAME_NORMAL;      // What is the current game difficulty

    // {DEBUG}
    pcfg->fps_allowed       = C_TRUE;             // FPS displayed?
    pcfg->hide_mouse        = C_TRUE;
    pcfg->grab_mouse        = C_TRUE;
    pcfg->dev_mode          = C_FALSE;
    pcfg->sdl_image_allowed = C_TRUE;    // Allow advanced SDL_Image functions?

    // other values
    pcfg->messageon_req     = ( pcfg->message_count_req > 0 );  // make it consistent with the default
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_begin( void )
{
    if ( _setup_started ) return C_TRUE;

    // set the default Egoboo values
    egoboo_config__init( &cfg_default );

    // Read the local setup.txt
    if ( fs_ensureUserFile( "setup.txt", C_TRUE ) )
    {
        snprintf( _config_filename, SDL_arraysize( _config_filename ), "%s" SLASH_STR "setup.txt", fs_getUserDirectory() );

        // do NOT force the file to open in a read directory if it doesn't exist. this will cause a failure in
        // linux if the directory is read-only
        _lpConfigSetup = ConfigFile_Load( _config_filename, C_FALSE );
    }

    if ( NULL != _lpConfigSetup )
    {
        _setup_started = C_TRUE;
    }

    return _setup_started;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_end( void )
{
    return ConfigFile_succeed == ConfigFile_destroy( &_lpConfigSetup );
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_read_vfs( void )
{
    /// @author BB
    /// @details read the setup file

    C_BOOLEAN retval;

    if ( !setup_begin() ) return C_FALSE;

    //Did something go wrong?
    retval = ( NULL != _lpConfigSetup );

    if ( retval )
    {
        log_info( "Loaded setup file - \"%s\".\n", _config_filename );
    }
    else
    {
        log_error( "Could not load setup settings: \"%s\"\n", _config_filename );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_write_vfs( void )
{
    /// @author BB
    /// @details save the current setup file

    ConfigFile_retval retval  = ConfigFile_fail;
    C_BOOLEAN            success = C_FALSE;

    if ( !setup_begin() ) return C_FALSE;

    retval = ConfigFile_SaveAs( _lpConfigSetup, _config_filename );

    success = C_FALSE;
    if ( ConfigFile_succeed != retval )
    {
        success = C_FALSE;
        log_warning( "Failed to save setup.txt!\n" );
    }
    else
    {
        success = C_TRUE;
    }

    return success;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_download( egoboo_config_t * pcfg )
{
    /// @author BB
    /// @details download the ConfigFile_t keys into game variables
    ///     use default values to fill in any missing keys

    const char *lCurSectionName;
    config_bool_t lTempBool;
    Sint32 lTempInt;
    STRING lTempStr;

    if ( NULL == _lpConfigSetup || NULL == pcfg ) return C_FALSE;

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

    // Perspective correction
    GetKey_bool( "PERSPECTIVE_CORRECT", pcfg->use_perspective, cfg_default.use_perspective );

    // Enable dithering?
    GetKey_bool( "DITHERING", pcfg->use_dither, cfg_default.use_dither );

    // Reflection fadeout
    GetKey_bool( "FLOOR_REFLECTION_FADEOUT", lTempBool, 0 != cfg_default.reflect_fade );
    pcfg->reflect_fade = lTempBool ? 255 : 0;

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

    // Do Gouraud shading?
    GetKey_bool( "GOURAUD_SHADING", pcfg->gouraud_req, cfg_default.gouraud_req );

    // Enable antialiasing?
    GetKey_int( "ANTIALIASING", pcfg->multisamples, cfg_default.multisamples );

    // coerce a "valid" multisample value
    pcfg->multisamples = CLIP( pcfg->multisamples, 0, 4 );

    // Do we do texture filtering?
    GetKey_string( "TEXTURE_FILTERING", lTempStr, 24, "LINEAR" );
    pcfg->texturefilter_req =  cfg_default.texturefilter_req;
    if ( 'U' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_UNFILTERED;
    if ( 'L' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_LINEAR;
    if ( 'M' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_MIPMAP;
    if ( 'B' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_BILINEAR;
    if ( 'T' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_TRILINEAR_1;
    if ( '2' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_TRILINEAR_2;
    if ( 'A' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->texturefilter_req = TX_ANISOTROPIC;

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
    pcfg->sound_highquality_base = pcfg->sound_highquality;

    // Extra high sound quality?
    GetKey_bool( "ENABLE_FOOTSTEPS", pcfg->sound_footfall, cfg_default.sound_footfall );

    //*********************************************
    //* CONTROL Section
    //*********************************************

    lCurSectionName = "GAME";

    // Which difficulty mode do we use?
    GetKey_string( "DIFFICULTY_MODE", lTempStr, 24, "NORMAL" );
    pcfg->difficulty = cfg_default.difficulty;
    if ( 'E' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->difficulty = GAME_EASY;
    if ( 'N' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->difficulty = GAME_NORMAL;
    if ( 'H' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->difficulty = GAME_HARD;

    //Feedback
    GetKey_int( "FEEDBACK", lTempInt, cfg_default.feedback );
    pcfg->feedback = ( FEEDBACK_TYPE )lTempInt;

    // Camera control mode
    GetKey_string( "AUTOTURN_CAMERA", lTempStr, 24, "GOOD" );
    pcfg->autoturncamera = cfg_default.autoturncamera;
    if ( 'G' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->autoturncamera = CAM_TURN_GOOD;
    else if ( 'T' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->autoturncamera = CAM_TURN_AUTO;
    else if ( 'F' == char_toupper(( unsigned )lTempStr[0] ) )  pcfg->autoturncamera = CAM_TURN_NONE;

    // Max number of messages displayed
    GetKey_int( "MAX_TEXT_MESSAGE", pcfg->message_count_req, cfg_default.message_count_req );

    // Max number of messages displayed
    GetKey_int( "MESSAGE_DURATION", pcfg->message_duration, cfg_default.message_duration );

    //*********************************************
    //* NETWORK Section
    //*********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    GetKey_bool( "NETWORK_ON", pcfg->network_allowed, cfg_default.network_allowed );

    // Max lag
    GetKey_int( "LAG_TOLERANCE", pcfg->network_lag, cfg_default.network_lag );

    // Name or IP of the host or the target to join
    GetKey_string( "HOST_NAME", pcfg->network_hostname, SDL_arraysize( pcfg->network_hostname ), cfg_default.network_hostname );

    // Multiplayer name
    GetKey_string( "MULTIPLAYER_NAME", pcfg->network_messagename, SDL_arraysize( pcfg->network_messagename ), cfg_default.network_messagename );

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

    // Show status bars? (Life, mana, character icons, etc.)
    GetKey_bool( "STATUS_BAR", pcfg->show_stats, cfg_default.show_stats );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_upload( egoboo_config_t * pcfg )
{
    /// @author BB
    /// @details upload game variables into the ConfigFile_t keys

    const char  *lCurSectionName;
    if ( NULL == _lpConfigSetup || NULL == pcfg ) return C_FALSE;

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

    // Perspective correction
    SetKey_bool( "PERSPECTIVE_CORRECT", pcfg->use_perspective );

    // Enable dithering?
    SetKey_bool( "DITHERING", pcfg->use_dither );

    // Reflection fadeout
    SetKey_bool( "FLOOR_REFLECTION_FADEOUT", 0 != pcfg->reflect_fade );

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

    // Do Gouraud shading?
    SetKey_bool( "GOURAUD_SHADING", pcfg->gouraud_req );

    // Enable antialiasing?
    SetKey_int( "ANTIALIASING", pcfg->multisamples );

    // Do we do texture filtering?
    switch ( pcfg->texturefilter_req )
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

    // Draw phong mapping?
    SetKey_bool( "ENABLE_FOOTSTEPS", pcfg->sound_footfall );

    //*********************************************
    //* GAME Section
    //*********************************************

    lCurSectionName = "GAME";

    // Save diffculty mode
    switch ( pcfg->difficulty )
    {
        case GAME_EASY:         SetKey_string( "DIFFICULTY_MODE", "EASY" ); break;
        case GAME_HARD:         SetKey_string( "DIFFICULTY_MODE", "HARD" ); break;

        default:
        case GAME_NORMAL:       SetKey_string( "DIFFICULTY_MODE", "NORMAL" ); break;
    }

    // Feedback type
    SetKey_int( "FEEDBACK", pcfg->feedback );

    // Camera control mode
    switch ( pcfg->autoturncamera )
    {
        case CAM_TURN_NONE:  SetKey_bool( "AUTOTURN_CAMERA", C_FALSE ); break;
        case CAM_TURN_GOOD:  SetKey_string( "AUTOTURN_CAMERA", "GOOD" ); break;

        default:
        case CAM_TURN_AUTO : SetKey_bool( "AUTOTURN_CAMERA", C_TRUE );  break;
    }

    // Max number of messages displayed
    SetKey_int( "MAX_TEXT_MESSAGE", !pcfg->messageon_req ? 0 : CLIP( pcfg->message_count_req, EGO_MESSAGE_MIN, EGO_MESSAGE_MAX ) );

    // Max number of messages displayed
    SetKey_int( "MESSAGE_DURATION", pcfg->message_duration );

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

    // Show status bars? (Life, mana, character icons, etc.)
    SetKey_bool( "STATUS_BAR", pcfg->show_stats );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
C_BOOLEAN config_synch( egoboo_config_t * pcfg, C_BOOLEAN synch_from_file )
{
    if ( !config_download( pcfg, synch_from_file ) )
    {
        return C_FALSE;
    }

    if ( !config_upload( pcfg ) )
    {
        return C_FALSE;
    }

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void setup_init_base_vfs_paths( void )
{
    /// @author BB
    /// @details set the basic mount points used by the main program

    //---- tell the vfs to add the basic search paths
    vfs_set_base_search_paths();

    //---- mount all of the default global directories

    // mount the global basicdat directory t the beginning of the list
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat", "mp_data", 1 );

    // Create a mount point for the /user/modules directory
    vfs_add_mount_point( fs_getUserDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /data/modules directory
    vfs_add_mount_point( fs_getDataDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /user/players directory
    vfs_add_mount_point( fs_getUserDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /data/players directory
    //vfs_add_mount_point( fs_getDataDirectory(), "players", "mp_players", 1 );     //ZF> Let's remove the local players folder since it caused so many problems for people

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "remote", "mp_remote", 1 );
}

//--------------------------------------------------------------------------------------------
void setup_clear_base_vfs_paths( void )
{
    /// @author BB
    /// @details clear out the basic mount points

    vfs_remove_mount_point( "mp_data" );
    vfs_remove_mount_point( "mp_modules" );
    vfs_remove_mount_point( "mp_players" );
    vfs_remove_mount_point( "mp_remote" );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
C_BOOLEAN setup_init_module_vfs_paths( const char * mod_path )
{
    /// @author BB
    /// @details set up the virtual mount points for the module's data
    ///               and objects

    const char * path_seperator_1, * path_seperator_2;
    const char * mod_dir_ptr;
    STRING mod_dir_string;

    STRING tmpDir;

    if ( INVALID_CSTR( mod_path ) ) return C_FALSE;

    // revert to the program's basic mount points
    setup_clear_module_vfs_paths();

    path_seperator_1 = strrchr( mod_path, SLASH_CHR );
    path_seperator_2 = strrchr( mod_path, NET_SLASH_CHR );
    path_seperator_1 = MAX( path_seperator_1, path_seperator_2 );

    if ( NULL == path_seperator_1 )
    {
        mod_dir_ptr = mod_path;
    }
    else
    {
        mod_dir_ptr = path_seperator_1 + 1;
    }

    strncpy( mod_dir_string, mod_dir_ptr, SDL_arraysize( mod_dir_string ) );

    //==== set the module-dependent mount points

    //---- add the "/modules/*.mod/objects" directories to mp_objects
    snprintf( tmpDir, SDL_arraysize( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "objects", mod_dir_string );

    // mount the user's module objects directory at the beginning of the mount point list
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_objects", 1 );

    // mount the global module objects directory next in the mount point list
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_objects", 1 );

    //---- add the "/basicdat/globalobjects/*" directories to mp_objects
    //ZF> TODO: Maybe we should dynamically search for all folders in this directory and add them as valid mount points?
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "items",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "magic_item",       "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "misc",             "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "monsters",         "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "players",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "potions",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "unique",           "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "weapons",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "work_in_progress", "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "traps",            "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "pets",             "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "scrolls",          "mp_objects", 1 );
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalobjects" SLASH_STR "armor",            "mp_objects", 1 );

    //---- add the "/modules/*.mod/gamedat" directory to mp_data
    snprintf( tmpDir, SDL_arraysize( tmpDir ), "modules" SLASH_STR "%s" SLASH_STR "gamedat",  mod_dir_string );

    // mount the user's module gamedat directory at the beginning of the mount point list
    vfs_add_mount_point( fs_getUserDirectory(), tmpDir, "mp_data", 1 );

    // append the global module gamedat directory
    vfs_add_mount_point( fs_getDataDirectory(), tmpDir, "mp_data", 1 );

    // put the global globalparticles data after the module gamedat data
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat" SLASH_STR "globalparticles", "mp_data", 1 );

    return C_TRUE;
}

//--------------------------------------------------------------------------------------------
void setup_clear_module_vfs_paths( void )
{
    /// @author BB
    /// @details clear out the all mount points

    // clear out the basic mount points
    setup_clear_base_vfs_paths();

    // clear out the module's mount points
    vfs_remove_mount_point( "mp_objects" );

    // set up the basic mount points again
    setup_init_base_vfs_paths();
}
