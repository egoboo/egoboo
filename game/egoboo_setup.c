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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
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

#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Macros for reading values from a ConfigFile
//   - Must have a valid ConfigFilePtr_t named lConfigSetup
//   - Must have a string named lCurSectionName to define the section
//   - Must have temporary variables defined of the correct type (lTempBool, lTempInt, and lTempStr)

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
        if ( ConfigFile_GetValue_String( lConfigSetup, lCurSectionName, (label), lTempStr, sizeof( lTempStr ) / sizeof( *lTempStr ) ) == 0 ) \
        { \
            strncpy( lTempStr, (default), sizeof( lTempStr ) / sizeof( *lTempStr ) ); \
        } \
        strncpy( (var), lTempStr, (len) ); \
        (var)[(len) - 1] = '\0'; \
    }

#define SetKey_bool(label, var)     ConfigFile_SetValue_Boolean( lConfigSetup, lCurSectionName, (label), (var) )
#define SetKey_int(label, var)      ConfigFile_SetValue_Int( lConfigSetup, lCurSectionName, (label), (var) )
#define SetKey_string( label, var ) ConfigFile_SetValue_String( lConfigSetup, lCurSectionName, (label), (var) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ConfigFilePtr_t lConfigSetup = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t setup_quit()
{
    return ConfigFile_succeed == ConfigFile_destroy( &lConfigSetup );
}

//--------------------------------------------------------------------------------------------
bool_t setup_read( const char* filename )
{
    // BB> read the setup file

    lConfigSetup = LoadConfigFile( filename );

    return NULL != lConfigSetup;
}

//--------------------------------------------------------------------------------------------
bool_t setup_write()
{
    // BB> save the current setup file

    return ConfigFile_succeed == SaveConfigFile( lConfigSetup );
}

//--------------------------------------------------------------------------------------------
bool_t setup_download()
{
    // BB > download the ConfigFile_t keys into game variables
    //      use default values to fill in any missing keys

    char  *lCurSectionName;
    bool_t lTempBool;
    Sint32 lTempInt;
    char   lTempStr[256];
    if (NULL == lConfigSetup) return bfalse;

    // *********************************************
    // * GRAPHIC Section
    // *********************************************

    lCurSectionName = "GRAPHIC";

    // Draw z reflection?
    GetKey_bool( "Z_REFLECTION", zreflect, bfalse );

    // Max number of vertrices (Should always be 100!)
    GetKey_int( "MAX_NUMBER_VERTICES", mesh_maxtotalvertices, 100 );
    mesh_maxtotalvertices *= 1024;

    // Do fullscreen?
    GetKey_bool( "FULLSCREEN", fullscreen, bfalse );

    // Screen Size
    GetKey_int( "SCREENSIZE_X", scrx, 640 );
    GetKey_int( "SCREENSIZE_Y", scry, 480 );

    // Color depth
    GetKey_int( "COLOR_DEPTH", scrd, 32 );
	scrd = 32;	//Force 32 bit

    // The z depth
    GetKey_int( "Z_DEPTH", scrz, 8 );

    // Max number of messages displayed
    GetKey_int( "MAX_TEXT_MESSAGE", maxmessage, 6 );
    messageon = btrue;
    if ( maxmessage < 1 )  { maxmessage = 1;  messageon = bfalse; }
    if ( maxmessage > MAXMESSAGE )  { maxmessage = MAXMESSAGE; }

    // Max number of messages displayed
    GetKey_int( "MESSAGE_DURATION", messagetime, 50 );

    // Show status bars? (Life, mana, character icons, etc.)
    GetKey_bool( "STATUS_BAR", staton, btrue );
    wraptolerance = 32;
    if ( staton )
    {
        wraptolerance = 90;
    }

    // Perspective correction
    GetKey_bool( "PERSPECTIVE_CORRECT", perspective, bfalse );

    // Enable dithering? (Reduces quality but increases preformance)
    GetKey_bool( "DITHERING", dither, bfalse );

    // Reflection fadeout
    GetKey_bool( "FLOOR_REFLECTION_FADEOUT", lTempBool, bfalse );
    if ( lTempBool )
    {
        reffadeor = 0;
    }
    else
    {
        reffadeor = 255;
    }

    // Draw Reflection?
    GetKey_bool( "REFLECTION", refon, bfalse );

    // Draw shadows?
    GetKey_bool( "SHADOWS", shaon, bfalse );

    // Draw good shadows?
    GetKey_bool( "SHADOW_AS_SPRITE", shasprite, btrue );

    // Draw phong mapping?
    GetKey_bool( "PHONG", phongon, btrue );

    // Draw water with more layers?
    GetKey_bool( "MULTI_LAYER_WATER", twolayerwateron, bfalse );

    // Allow overlay effects?
    GetKey_bool( "OVERLAY", overlayvalid, bfalse );

    // Allow backgrounds?
    GetKey_bool( "BACKGROUND", backgroundvalid, bfalse );

    // Enable fog?
    GetKey_bool( "FOG", fogallowed, bfalse );

    // Do gourad shading?
    GetKey_bool( "GOURAUD_SHADING", lTempBool, btrue );
    shading = lTempBool ? GL_SMOOTH : GL_FLAT;

    // Enable antialiasing?
    GetKey_int( "ANTIALIASING", antialiasing, bfalse );

    // Do we do texture filtering?
    GetKey_string( "TEXTURE_FILTERING", lTempStr, 24, "LINEAR" );
    if ( lTempStr[0] == 'U' || lTempStr[0] == 'u' )  texturefilter = TX_UNFILTERED;
    if ( lTempStr[0] == 'L' || lTempStr[0] == 'l' )  texturefilter = TX_LINEAR;
    if ( lTempStr[0] == 'M' || lTempStr[0] == 'm' )  texturefilter = TX_MIPMAP;
    if ( lTempStr[0] == 'B' || lTempStr[0] == 'b' )  texturefilter = TX_BILINEAR;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  texturefilter = TX_TRILINEAR_1;
    if ( lTempStr[0] == '2'                       )  texturefilter = TX_TRILINEAR_2;
    if ( lTempStr[0] == 'A' || lTempStr[0] == 'a' )  texturefilter = TX_ANISOTROPIC;

    // Max number of lights
    GetKey_int( "MAX_DYNAMIC_LIGHTS", dyna_list_max, 12 );
    if ( dyna_list_max > TOTALMAXDYNA ) dyna_list_max = TOTALMAXDYNA;

    // Get the FPS limit
    GetKey_int( "MAX_FPS_LIMIT", framelimit, 30 );

    // Get the particle limit
    GetKey_int( "MAX_PARTICLES", maxparticles, 512 );
    if (maxparticles > TOTALMAXPRT) maxparticles = TOTALMAXPRT;

    // *********************************************
    // * SOUND Section
    // *********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    GetKey_bool( "SOUND", soundvalid, bfalse );

    // Enable music
    GetKey_bool( "MUSIC", musicvalid, bfalse );

    // Music volume
    GetKey_int( "MUSIC_VOLUME", musicvolume, 50 );

    // Sound volume
    GetKey_int( "SOUND_VOLUME", soundvolume, 75 );

    // Max number of sound channels playing at the same time
    GetKey_int( "MAX_SOUND_CHANNEL", maxsoundchannel, 16 );
    if ( maxsoundchannel < 8 ) maxsoundchannel = 8;
    if ( maxsoundchannel > 128 ) maxsoundchannel = 128;

    // The output buffer size
    GetKey_int( "OUTPUT_BUFFER_SIZE", buffersize, 2048 );
    if ( buffersize < 512 ) buffersize = 512;
    if ( buffersize > 8196 ) buffersize = 8196;

    // *********************************************
    // * CONTROL Section
    // *********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    GetKey_string( "AUTOTURN_CAMERA", lTempStr, 24, "GOOD" );
    if ( lTempStr[0] == 'G' || lTempStr[0] == 'g' )  autoturncamera = 255;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  autoturncamera = btrue;
    if ( lTempStr[0] == 'F' || lTempStr[0] == 'f' )  autoturncamera = bfalse;

    // *********************************************
    // * NETWORK Section
    // *********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    GetKey_bool( "NETWORK_ON", networkon, bfalse );

    // Max lag
    GetKey_int( "LAG_TOLERANCE", lag, 2 );

    // Name or IP of the host or the target to join
    GetKey_string( "HOST_NAME", nethostname, 64, "no host" );

    // Multiplayer name
    GetKey_string( "MULTIPLAYER_NAME", netmessagename, 64, "little Raoul" );

    // *********************************************
    // * DEBUG Section
    // *********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    GetKey_bool( "DISPLAY_FPS", fpson, btrue );
    GetKey_bool( "HIDE_MOUSE", gHideMouse, btrue );
    GetKey_bool( "GRAB_MOUSE", gGrabMouse, btrue );
    GetKey_bool( "DEV_MODE", gDevMode, btrue );
    GetKey_bool( "SDL_IMAGE", use_sdl_image, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_upload()
{
    // BB > upload game variables into the ConfigFile_t keys

    char  *lCurSectionName;
    if (NULL == lConfigSetup) return bfalse;

    // *********************************************
    // * GRAPHIC Section
    // *********************************************

    lCurSectionName = "GRAPHIC";

    // Draw z reflection?
    SetKey_bool( "Z_REFLECTION", zreflect );

    // Max number of vertrices (Should always be 100!)
    SetKey_int( "MAX_NUMBER_VERTICES", mesh_maxtotalvertices / 1024 );

    // Do fullscreen?
    SetKey_bool( "FULLSCREEN", fullscreen );

    // Screen Size
    SetKey_int( "SCREENSIZE_X", scrx );
    SetKey_int( "SCREENSIZE_Y", scry );

    // Color depth
    SetKey_int( "COLOR_DEPTH", scrd );

    // The z depth
    SetKey_int( "Z_DEPTH", scrz );

    // Max number of messages displayed
    SetKey_int( "MAX_TEXT_MESSAGE", messageon ? maxmessage : 0 );

    // Max number of messages displayed
    SetKey_int( "MESSAGE_DURATION", messagetime );

    // Show status bars? (Life, mana, character icons, etc.)
    SetKey_bool( "STATUS_BAR", staton );

    // Perspective correction
    SetKey_bool( "PERSPECTIVE_CORRECT", perspective );

    // Enable dithering? (Reduces quality but increases preformance)
    SetKey_bool( "DITHERING", dither );

    // Reflection fadeout
    SetKey_bool( "FLOOR_REFLECTION_FADEOUT", reffadeor != 0 );

    // Draw Reflection?
    SetKey_bool( "REFLECTION", refon );

    // Draw shadows?
    SetKey_bool( "SHADOWS", shaon );

    // Draw good shadows?
    SetKey_bool( "SHADOW_AS_SPRITE", shasprite );

    // Draw phong mapping?
    SetKey_bool( "PHONG", phongon );

    // Draw water with more layers?
    SetKey_bool( "MULTI_LAYER_WATER", twolayerwateron );

    // Allow overlay effects?
    SetKey_bool( "OVERLAY", overlayvalid );

    // Allow backgrounds?
    SetKey_bool( "BACKGROUND", backgroundvalid );

    // Enable fog?
    SetKey_bool( "FOG", fogallowed );

    // Do gourad shading?
    SetKey_bool( "GOURAUD_SHADING", shading == GL_SMOOTH );

    // Enable antialiasing?
    SetKey_int( "ANTIALIASING", antialiasing );

    // Do we do texture filtering?
    switch (texturefilter)
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
    SetKey_int( "MAX_DYNAMIC_LIGHTS", dyna_list_max );

    // Get the FPS limit
    SetKey_int( "MAX_FPS_LIMIT", framelimit );

    // Get the particle limit
    SetKey_int( "MAX_PARTICLES", maxparticles );

    // *********************************************
    // * SOUND Section
    // *********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    SetKey_bool( "SOUND", soundvalid );

    // Enable music
    SetKey_bool( "MUSIC", musicvalid );

    // Music volume
    SetKey_int( "MUSIC_VOLUME", musicvolume );

    // Sound volume
    SetKey_int( "SOUND_VOLUME", soundvolume );

    // Max number of sound channels playing at the same time
    SetKey_int( "MAX_SOUND_CHANNEL", maxsoundchannel );

    // The output buffer size
    SetKey_int( "OUTPUT_BUFFER_SIZE", buffersize );

    // *********************************************
    // * CONTROL Section
    // *********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    switch ( autoturncamera )
    {
        case bfalse: SetKey_bool( "AUTOTURN_CAMERA", bfalse ); break;
        case 255:    SetKey_string( "AUTOTURN_CAMERA", "GOOD" ); break;

        default:
        case btrue : SetKey_bool( "AUTOTURN_CAMERA", btrue );  break;
    }

    // *********************************************
    // * NETWORK Section
    // *********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    SetKey_bool( "NETWORK_ON", networkon );

    // Name or IP of the host or the target to join
    SetKey_string( "HOST_NAME", nethostname );

    // Multiplayer name
    SetKey_string( "MULTIPLAYER_NAME", netmessagename );

    // Max lag
    SetKey_int( "LAG_TOLERANCE", lag );

    // *********************************************
    // * DEBUG Section
    // *********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    SetKey_bool( "DISPLAY_FPS", fpson );
    SetKey_bool( "HIDE_MOUSE", gHideMouse );
    SetKey_bool( "GRAB_MOUSE", gGrabMouse );
    SetKey_bool( "DEV_MODE", gDevMode );
    SetKey_bool( "SDL_IMAGE", use_sdl_image );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void export_control( FILE * filewrite, const char * text, Sint32 device, control_t * pcontrol )
{
    STRING write;

    snprintf( write, sizeof(write), "%s : %s\n", text, scantag_get_string(device, pcontrol->tag, pcontrol->is_key) );
    fputs( write, filewrite );
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_load( const char *szFilename )
{
    // ZZ> This function reads the controls.txt file
    FILE* fileread;
    char currenttag[TAGSIZE];
    int i, cnt;

    parse_filename = "";

    fileread = fopen( szFilename, "r" );
    if (NULL == fileread)
    {
        log_warning("Could not load input settings (%s)!\n", szFilename);
        return bfalse;
    }

    parse_filename = szFilename;

    // set the number of valid controls to be 0
    input_device_count = 0;

    // read the keyboard controls
    for (i = KEY_CONTROL_BEGIN; i <= KEY_CONTROL_END; i++)
    {
        goto_colon( fileread ); fscanf( fileread, "%s", currenttag );
        controls[INPUT_DEVICE_KEYBOARD].control[i].tag    = scantag_get_value( currenttag );
        controls[INPUT_DEVICE_KEYBOARD].control[i].is_key = ( currenttag[0] == 'K' );
    };
    controls[INPUT_DEVICE_KEYBOARD].device = INPUT_DEVICE_KEYBOARD;
    controls[INPUT_DEVICE_KEYBOARD].count = i;
    input_device_count++;

    // read the mouse controls
    for (i = MOS_CONTROL_BEGIN; i <= MOS_CONTROL_END; i++)
    {
        goto_colon( fileread ); fscanf( fileread, "%s", currenttag );
        controls[INPUT_DEVICE_MOUSE].control[i].tag    = scantag_get_value( currenttag );
        controls[INPUT_DEVICE_MOUSE].control[i].is_key = ( currenttag[0] == 'K' );
    };
    controls[INPUT_DEVICE_MOUSE].device = INPUT_DEVICE_MOUSE;
    controls[INPUT_DEVICE_MOUSE].count = i;
    input_device_count++;

    // read in however many joysticks there are...
    for ( cnt = 0; !feof(fileread) && cnt < MAXJOYSTICK; cnt++ )
    {
        for (i = JOY_CONTROL_BEGIN; i <= JOY_CONTROL_END; i++)
        {
            goto_colon( fileread ); fscanf( fileread, "%s", currenttag );
            controls[INPUT_DEVICE_JOY + cnt].control[i].tag    = scantag_get_value( currenttag );
            controls[INPUT_DEVICE_JOY + cnt].control[i].is_key = ( currenttag[0] == 'K' );
        };
        controls[INPUT_DEVICE_JOY + cnt].device = INPUT_DEVICE_JOY + cnt;
        controls[INPUT_DEVICE_JOY + cnt].count = i;
        input_device_count++;
    }

    fclose( fileread );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_save( const char* szFilename)
{
    //ZF> This function saves all current game settings to "controls.txt"
    device_controls_t * pdevice;
    FILE* filewrite;
    STRING write;
    Uint32 i;

    filewrite = fopen( szFilename, "w" );
    if ( NULL == filewrite )
    {
        log_warning("Could not save input settings (%s)!\n", szFilename);
        return bfalse;
    }

    //Just some information
    fputs( "Controls\n", filewrite );
    fputs( "========\n", filewrite );
    fputs( "This file lets users modify the handling of input devices.\n", filewrite );
    fputs( "See the game manual for a list of settings and more info.\n", filewrite );
    fputs( "Note that you can mix KEY_ type settings with other \n", filewrite );
    fputs( "devices... Write the input after the colons!\n\n", filewrite );

    fputs( "General Controls\n", filewrite );
    fputs( "========\n", filewrite );
    fputs( "These are general controls and cannot be changed\n", filewrite );
    fputs( "ESC                   - End module\n", filewrite );
    fputs( "SPACE                 - Respawn character (if dead and possible)\n", filewrite );
    fputs( "1 to 7                - Show character detailed stats\n", filewrite );
    fputs( "LEFT SHIFT   + 1 to 7 - Show selected character armor without magic enchants\n", filewrite );
    fputs( "LEFT CONTROL + 1 to 7 - Show armor stats with magic enchants included\n", filewrite );
    fputs( "LEFT ALT     + 1 to 7 - Show character magic enchants\n", filewrite );
    fputs( "F11                   - Take screenshot\n", filewrite );
    fputs( "\n", filewrite );

    //The actual settings
    pdevice = controls + INPUT_DEVICE_KEYBOARD;
    fputs( "Keyboard\n", filewrite );
    fputs( "========\n", filewrite );
    export_control( filewrite, "Jump\t\t\t\t", pdevice->device, pdevice->control + CONTROL_JUMP );
    export_control( filewrite, "Left Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
    export_control( filewrite, "Left Hand Get/Drop\t", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
    export_control( filewrite, "Left Hand Inventory ", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
    export_control( filewrite, "Right Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
    export_control( filewrite, "Right Hand Get/Drop ", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
    export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
    export_control( filewrite, "Send Message\t\t", pdevice->device, pdevice->control + CONTROL_MESSAGE );
    export_control( filewrite, "Camera Rotate Left\t", pdevice->device, pdevice->control + CONTROL_CAMERA_LEFT );
    export_control( filewrite, "Camera Rotate Right ", pdevice->device, pdevice->control + CONTROL_CAMERA_RIGHT );
    export_control( filewrite, "Camera Zoom In\t\t", pdevice->device, pdevice->control + CONTROL_CAMERA_IN );
    export_control( filewrite, "Camera Zoom Out\t", pdevice->device, pdevice->control + CONTROL_CAMERA_OUT );
    export_control( filewrite, "Up\t\t\t\t\t", pdevice->device, pdevice->control + CONTROL_UP );
    export_control( filewrite, "Down\t\t\t\t", pdevice->device, pdevice->control + CONTROL_DOWN );
    export_control( filewrite, "Left\t\t\t\t", pdevice->device, pdevice->control + CONTROL_LEFT );
    export_control( filewrite, "Right\t\t\t\t", pdevice->device, pdevice->control + CONTROL_RIGHT );

    pdevice = controls + INPUT_DEVICE_MOUSE;
    fputs( "\n\nMouse\n", filewrite );
    fputs( "========\n", filewrite );
    export_control( filewrite, "Jump\t\t\t\t", pdevice->device, pdevice->control + CONTROL_JUMP );
    export_control( filewrite, "Left Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
    export_control( filewrite, "Left Hand Get/Drop\t", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
    export_control( filewrite, "Left Hand Inventory\t", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
    export_control( filewrite, "Right Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
    export_control( filewrite, "Right Hand Get/Drop\t", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
    export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
    export_control( filewrite, "Camera Control Mode\t", pdevice->device, pdevice->control + CONTROL_CAMERA );

    // export all known joysticks
    for ( i = INPUT_DEVICE_JOY; i < input_device_count; i++)
    {
        pdevice = controls + i;

        snprintf( write, sizeof(write), "\n\nJoystick %d\n", i - INPUT_DEVICE_JOY );
        fputs( write, filewrite );
        fputs( "========\n", filewrite );
        export_control( filewrite, "Jump\t\t\t\t", pdevice->device, pdevice->control + CONTROL_JUMP );
        export_control( filewrite, "Left Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
        export_control( filewrite, "Left Hand Get/Drop\t", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
        export_control( filewrite, "Left Hand Inventory\t", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
        export_control( filewrite, "Right Hand Use\t\t", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
        export_control( filewrite, "Right Hand Get/Drop\t", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
        export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
        export_control( filewrite, "Camera Control Mode\t", pdevice->device, pdevice->control + CONTROL_CAMERA );
    }

    //All done
    fclose(filewrite);

    return btrue;
}

