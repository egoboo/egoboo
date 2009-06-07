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

/* Egoboo - graphic.c
 * All sorts of stuff related to drawing the game, and all sorts of other stuff
 * (such as data loading) that really should not be in here.
 */

#include "graphic.h"

#include "log.h"
#include "script.h"
#include "camera.h"
#include "id_md2.h"
#include "input.h"
#include "char.h"
#include "particle.h"
#include "file_common.h"
#include "network.h"
#include "passage.h"
#include "menu.h"
#include "enchant.h"
#include "mad.h"
#include "script_compile.h"

#include "SDL_extensions.h"
#include "SDL_GL_extensions.h"

#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#if defined(USE_LUA_CONSOLE)
#    include "lua_console.h"
#else
#    include "egoboo_console.h"
#endif

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include <SDL_image.h>

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define BLIPSIZE 6

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

gfx_config_t     gfx;

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

Uint8            maxformattypes = 0;
STRING           TxFormatSupported[20];      // List of texture formats that we search for

GLXtexture       TxIcon[MAX_ICON];           // OpenGL icon surfaces
GLXtexture       TxFont;                     // OpenGL font surface
GLXtexture       TxBars;                     // OpenGL status bar surface
GLXtexture       TxBlip;                     // OpenGL you are here surface
GLXtexture       TxMap;                      // OpenGL map surface
GLXtexture       TxTexture[MAX_TEXTURE];     // All textures

Uint32          TxTitleImage_count = 0;
GLXtexture       TxTitleImage[MAXMODULE];    // OpenGL title image surfaces


Uint16                dolist_count = 0;
obj_registry_entity_t dolist[DOLIST_SIZE];

bool_t           meshnotexture = bfalse;
Uint16           meshlasttexture = (Uint16)~0;

renderlist_t     renderlist = {0, 0, 0, 0, 0};

float            light_a = 0.0f, light_x = 0.0f, light_y = 0.0f, light_z = 0.0f;
Uint8            lightdirectionlookup[65536];
float            lighttable_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
float            lighttable_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
float            indextoenvirox[MD2LIGHTINDICES];
float            lighttoenviroy[256];
Uint32           lighttospek[MAXSPEKLEVEL][256];

static bool_t _sdl_atexit_registered    = bfalse;
static bool_t _sdl_initialized_graphics = bfalse;
static bool_t _sdl_initialized_base     = bfalse;
static bool_t _ogl_initialized          = bfalse;

static void sdlinit_base();
static void sdlinit_graphics();

static float sinlut[MAXLIGHTROTATION];
static float coslut[MAXLIGHTROTATION];

// Camera optimization stuff
static float                   cornerx[4];
static float                   cornery[4];
static int                     cornerlistlowtohighy[4];
static float                   cornerlowx;
static float                   cornerhighx;
static float                   cornerlowy;
static float                   cornerhighy;

int rotmeshtopside;
int rotmeshbottomside;
int rotmeshup;
int rotmeshdown;

float       dyna_distancetobeat;           // The number to beat
int         dyna_list_max   = 8;           // Max number of lights to draw
int         dyna_list_count = 0;           // Number of dynamic lights
dynalight_t dyna_list[TOTAL_MAX_DYNA];

// Interface stuff
static rect_t             iconrect;                   // The 32x32 icon rectangle

static int                fontoffset;                 // Line up fonts from top of screen

static SDL_Rect           fontrect[NUMFONT];          // The font rectangles
static Uint8              fontxspacing[NUMFONT];      // The spacing stuff
static Uint8              fontyspacing; 
static rect_t             tabrect[NUMBAR];            // The tab rectangles
static rect_t             barrect[NUMBAR];            // The bar rectangles
static rect_t             bliprect[NUMBAR];           // The blip rectangles
static rect_t             maprect;                    // The map rectangle

static bool_t             gfx_page_flip_requested = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void flip_pages();

static void font_release();

static void project_view( camera_t * pcam );
static void make_dynalist( camera_t * pcam );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EnableTexturing()
{
    if ( !glIsEnabled( GL_TEXTURE_2D ) )
    {
        glEnable( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void DisableTexturing()
{
    if ( glIsEnabled( GL_TEXTURE_2D ) )
    {
        glDisable( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void move_water( void )
{
    // ZZ> This function animates the water overlays
    int layer;

    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
    {
        waterlayeru[layer] += waterlayeruadd[layer];
        waterlayerv[layer] += waterlayervadd[layer];
        if ( waterlayeru[layer] > 1.0f )  waterlayeru[layer] -= 1.0f;
        if ( waterlayerv[layer] > 1.0f )  waterlayerv[layer] -= 1.0f;
        if ( waterlayeru[layer] < -1.0f )  waterlayeru[layer] += 1.0f;
        if ( waterlayerv[layer] < -1.0f )  waterlayerv[layer] += 1.0f;

        waterlayerframe[layer] = ( waterlayerframe[layer] + waterlayerframeadd[layer] ) & WATERFRAMEAND;
    }
}

//--------------------------------------------------------------------------------------------
void tile_dictionary_load(tile_definition_t dict[], size_t dict_size)
{
    // ZZ> This function loads fan types for the terrain
    Uint32 cnt, entry, vertices, commandsize;
    int numfantype, fantype, bigfantype;
    int numcommand, command;
    int itmp;
    float ftmp;
    FILE* fileread;

    if ( NULL == dict || dict_size < 2 ) return;

    // Initialize all mesh types to 0
    for ( entry = 0; entry < dict_size; entry++ )
    {
        dict[entry].numvertices = 0;
        dict[entry].command_count = 0;
    }

    // Open the file and go to it
    fileread = fopen( "basicdat" SLASH_STR "fans.txt", "r" );
    if ( NULL == fileread )
    {
        log_error( "Cannot load the tile definitions \"basicdat" SLASH_STR "fans.txt\" \n" );
        return;
    }

    goto_colon( NULL, fileread, bfalse );
    fscanf( fileread, "%d", &numfantype );

    for ( fantype = 0; fantype < numfantype; fantype++ )
    {
        bigfantype = fantype + dict_size / 2;  // Duplicate for 64x64 tiles

        goto_colon( NULL, fileread, bfalse );
        fscanf( fileread, "%d", &vertices );
        dict[fantype].numvertices = vertices;
        dict[bigfantype].numvertices = vertices;  // Dupe

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%d", &itmp );

            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%f", &ftmp );
            dict[fantype].u[cnt] = ftmp;
            dict[bigfantype].u[cnt] = ftmp;  // Dupe

            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%f", &ftmp );
            dict[fantype].v[cnt] = ftmp;
            dict[bigfantype].v[cnt] = ftmp;  // Dupe
        }

        goto_colon( NULL, fileread, bfalse );
        fscanf( fileread, "%d", &numcommand );
        dict[fantype].command_count = numcommand;
        dict[bigfantype].command_count = numcommand;  // Dupe

        for ( entry = 0, command = 0; command < numcommand; command++ )
        {
            goto_colon( NULL, fileread, bfalse );
            fscanf( fileread, "%d", &commandsize );
            dict[fantype].command_entries[command] = commandsize;
            dict[bigfantype].command_entries[command] = commandsize;  // Dupe

            for ( cnt = 0; cnt < commandsize; cnt++ )
            {
                goto_colon( NULL, fileread, bfalse );
                fscanf( fileread, "%d", &itmp );
                dict[fantype].command_verts[entry] = itmp;
                dict[bigfantype].command_verts[entry] = itmp;  // Dupe

                entry++;
            }
        }
    }

    fclose( fileread );

    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < dict_size / 2; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = ( ( 0.6f / 32 ) + ( dict[entry].u[cnt] * 30.8f / 32 ) ) / 8;
            dict[entry].v[cnt] = ( ( 0.6f / 32 ) + ( dict[entry].v[cnt] * 30.8f / 32 ) ) / 8;
        }
    }

    // Do for big tiles too
    for ( /* nothing */; entry < dict_size; entry++ )
    {
        for ( cnt = 0; cnt < dict[entry].numvertices; cnt++ )
        {
            dict[entry].u[cnt] = ( ( 0.6f / 64 ) + ( dict[entry].u[cnt] * 62.8f / 64 ) ) / 4;
            dict[entry].v[cnt] = ( ( 0.6f / 64 ) + ( dict[entry].v[cnt] * 62.8f / 64 ) ) / 4;
        }
    }

}

//---------------------------------------------------------------------------------------------
void make_lighttospek( void )
{
    // ZZ> This function makes a light table to fake directional lighting
    int cnt, tnc;
    Uint8 spek;
    float fTmp, fPow;

    // New routine
    for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
    {
        for ( tnc = 0; tnc < 256; tnc++ )
        {
            fTmp = tnc / 256.0f;
            fPow = ( fTmp * 4.0f ) + 1;
            fTmp = POW( fTmp, fPow );
            fTmp = fTmp * cnt * 255.0f / MAXSPEKLEVEL;
            if ( fTmp < 0 ) fTmp = 0;
            if ( fTmp > 255 ) fTmp = 255;

            spek = fTmp;
            spek = spek >> 1;
            lighttospek[cnt][tnc] = ( 0xff000000 ) | ( spek << 16 ) | ( spek << 8 ) | ( spek );
        }
    }
}

//---------------------------------------------------------------------------------------------
void make_lightdirectionlookup()
{
    // ZZ> This function builds the lighting direction table
    //     The table is used to find which direction the light is coming
    //     from, based on the four corner vertices of a mesh tile.
    Uint32 cnt;
    Uint16 tl, tr, br, bl;
    int x, y;

    for ( cnt = 0; cnt < 65536; cnt++ )
    {
        tl = ( cnt & 0xf000 ) >> 12;
        tr = ( cnt & 0x0f00 ) >> 8;
        br = ( cnt & 0x00f0 ) >> 4;
        bl = ( cnt & 0x000f );
        x = br + tr - bl - tl;
        y = br + bl - tl - tr;
        lightdirectionlookup[cnt] = ( ATAN2( -y, x ) + PI ) * 256 / ( TWO_PI );
    }
}

//--------------------------------------------------------------------------------------------
int get_free_message()
{
    // This function finds the best message to use
    // Pick the first one
    int tnc = msgstart;
    msgstart++;
    msgstart = msgstart % maxmessage;
    return tnc;
}

//--------------------------------------------------------------------------------------------
void display_message( script_state_t * pstate, int message, Uint16 character )
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot, read, write, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;

    Uint16 target = ChrList[character].ai.target;
    Uint16 owner  = ChrList[character].ai.owner;
    if ( message < msgtotal )
    {
        slot = get_free_message();
        msgtime[slot] = cfg.message_duration;
        // Copy the message
        read = msgindex[message];
        cnt = 0;
        write = 0;
        cTmp = msgtext[read];  read++;

        while ( cTmp != 0 )
        {
            if ( cTmp == '%' )
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;
                if ( cTmp == 'n' )  // Name
                {
                    if ( ChrList[character].nameknown )
                        sprintf( szTmp, "%s", ChrList[character].name );
                    else
                    {
                        lTmp = CapList[ChrList[character].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[character].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[character].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'c' )  // Class name
                {
                    eread = CapList[ChrList[character].model].classname;
                }
                if ( cTmp == 't' )  // Target name
                {
                    if ( ChrList[target].nameknown )
                        sprintf( szTmp, "%s", ChrList[target].name );
                    else
                    {
                        lTmp = CapList[ChrList[target].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[target].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[target].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'o' )  // Owner name
                {
                    if ( ChrList[owner].nameknown )
                        sprintf( szTmp, "%s", ChrList[owner].name );
                    else
                    {
                        lTmp = CapList[ChrList[owner].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[owner].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[owner].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 's' )  // Target class name
                {
                    eread = CapList[ChrList[target].model].classname;
                }
                if ( cTmp >= '0' && cTmp <= '3' )  // Target's skin name
                {
                    eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
                }
                if ( NULL == pstate )
                {
                    sprintf( szTmp, "%%%c???", cTmp );
                }
                else
                {
                    if ( cTmp == 'd' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%d", pstate->distance );
                    }
                    if ( cTmp == 'x' )  // tmpx value
                    {
                        sprintf( szTmp, "%d", pstate->x );
                    }
                    if ( cTmp == 'y' )  // tmpy value
                    {
                        sprintf( szTmp, "%d", pstate->y );
                    }
                    if ( cTmp == 'D' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%2d", pstate->distance );
                    }
                    if ( cTmp == 'X' )  // tmpx value
                    {
                        sprintf( szTmp, "%2d", pstate->x );
                    }
                    if ( cTmp == 'Y' )  // tmpy value
                    {
                        sprintf( szTmp, "%2d", pstate->y );
                    }
                }
                if ( cTmp == 'a' )  // Character's ammo
                {
                    if ( ChrList[character].ammoknown )
                        sprintf( szTmp, "%d", ChrList[character].ammo );
                    else
                        sprintf( szTmp, "?" );
                }
                if ( cTmp == 'k' )  // Kurse state
                {
                    if ( ChrList[character].iskursed )
                        sprintf( szTmp, "kursed" );
                    else
                        sprintf( szTmp, "unkursed" );
                }
                if ( cTmp == 'p' )  // Character's possessive
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }
                if ( cTmp == 'm' )  // Character's gender
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "female " );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "male " );
                        }
                        else
                        {
                            sprintf( szTmp, " " );
                        }
                    }
                }
                if ( cTmp == 'g' )  // Target's possessive
                {
                    if ( ChrList[target].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[target].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }

                cTmp = *eread;  eread++;

                while ( cTmp != 0 && write < MESSAGESIZE - 1 )
                {
                    msgtextdisplay[slot][write] = cTmp;
                    cTmp = *eread;  eread++;
                    write++;
                }
            }
            else
            {
                // Copy the letter
                if ( write < MESSAGESIZE - 1 )
                {
                    msgtextdisplay[slot][write] = cTmp;
                    write++;
                }
            }

            cTmp = msgtext[read];  read++;
            cnt++;
        }

        msgtextdisplay[slot][write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
// This needs work
void Begin3DMode( camera_t * pcam )
{
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadMatrixf( pcam->mProjection.v );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadMatrixf( pcam->mView.v );
}

void End3DMode()
{
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
}

/********************> Begin2DMode() <*****/
void Begin2DMode( void )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();                  // Reset The Projection Matrix
    glOrtho( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );        // Set up an orthogonal projection

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
}

/********************> End2DMode() <*****/
void End2DMode( void )
{
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );
}

//--------------------------------------------------------------------------------------------
// ZF> Load all the global icons used in all modules
bool_t load_all_global_icons()
{
    //Setup
    bool_t result = bfalse;
    globalicon_count = 0;

    //Now load every icon
    nullicon = globalicon_count;
    result = load_one_icon( "basicdat" SLASH_STR "nullicon" );
    keybicon = globalicon_count;
    result = load_one_icon( "basicdat" SLASH_STR "keybicon" );
    mousicon = globalicon_count;
    result = load_one_icon( "basicdat" SLASH_STR "mousicon" );
    joyaicon = globalicon_count;
    result = load_one_icon( "basicdat" SLASH_STR "joyaicon" );
    joybicon = globalicon_count;
    result = load_one_icon( "basicdat" SLASH_STR "joybicon" );

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t load_one_icon( const char *szLoadName )
{
    // ZZ> This function is used to load an icon.  Most icons are loaded
    //     without this function though...
    if ( INVALID_TX_ID == ego_texture_load( TxIcon + globalicon_count,  szLoadName, INVALID_KEY ) )
    {
        return bfalse;
    }

    globalicon_count++;
    return btrue;
}

//---------------------------------------------------------------------------------------------
void init_all_icons()
{
    // ZZ> This function sets the icon pointers to NULL
    int cnt;

    for ( cnt = 0; cnt < MAX_ICON; cnt++ )
    {
        GLXtexture_new( TxIcon + cnt );
    }

    iconrect.left = 0;
    iconrect.right = 32;
    iconrect.top = 0;
    iconrect.bottom = 32;

    release_all_icons();
}

//---------------------------------------------------------------------------------------------
void init_all_titleimages()
{
    // ZZ> This function clears out all of the title images
    int cnt;

    for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    {
        GLXtexture_new( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void init_bars()
{
    int cnt;

    GLXtexture_new( &TxBars );

    // Make the blit rectangles
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        tabrect[cnt].left = 0;
        tabrect[cnt].right = TABX;
        tabrect[cnt].top = cnt * BARY;
        tabrect[cnt].bottom = ( cnt + 1 ) * BARY;

        barrect[cnt].left = TABX;
        barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt].top = tabrect[cnt].top;
        barrect[cnt].bottom = tabrect[cnt].bottom;
    }
}

//---------------------------------------------------------------------------------------------
void init_blip()
{
    int cnt;

    GLXtexture_new( &TxBlip );

    // Set up the rectangles
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        bliprect[cnt].left   = cnt * BLIPSIZE;
        bliprect[cnt].right  = ( cnt * BLIPSIZE ) + BLIPSIZE;
        bliprect[cnt].top    = 0;
        bliprect[cnt].bottom = BLIPSIZE;
    }

}

//---------------------------------------------------------------------------------------------
void init_map()
{
    // ZZ> This function releases all the map images
    GLXtexture_new( &TxMap );

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;
}

//---------------------------------------------------------------------------------------------
void init_all_textures()
{
    // ZZ> This function clears out all of the textures
    int cnt;

    for ( cnt = 0; cnt < MAX_TEXTURE; cnt++ )
    {
        GLXtexture_new( TxTexture + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_models()
{
    // ZZ> This function initializes all of the model profiles

    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipList + cnt, 0, sizeof(pip_t) );
    }

    for ( cnt = 0; cnt < MAXEVE; cnt++ )
    {
        memset( EveList + cnt, 0, sizeof(pip_t) );
    }

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( CapList + cnt, 0, sizeof(cap_t) );
    };

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( MadList + cnt, 0, sizeof(mad_t) );

        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
        MadList[cnt].ai = 0;
    }

    reset_all_ai_scripts();

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
    // ZZ> This function clears out all of the icons
    int cnt;

    // release all icon textures
    for ( cnt = 0; cnt < MAX_ICON; cnt++ )
    {
        GLXtexture_Release( TxIcon + cnt );
    }
    globalicon_count = 0;

    // remove the texture references
    for ( cnt = 0; cnt < MAX_TEXTURE; cnt++ )
    {
        skintoicon[cnt] = 0;
    }
        
    bookicon_count = 0;
    for ( cnt = 0; cnt < MAXSKIN; cnt++ )
    {
        bookicon[cnt] = 0;
    }
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
    // ZZ> This function clears out all of the title images
    int cnt;

    for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    {
        GLXtexture_Release( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_bars()
{
    GLXtexture_Release( &TxBars );
}

//---------------------------------------------------------------------------------------------
void release_blip()
{
    GLXtexture_Release( &TxBlip );

    youarehereon = bfalse;
    numblip      = 0;
}

//---------------------------------------------------------------------------------------------
void release_map()
{
    GLXtexture_Release( &TxMap );

    mapvalid = bfalse;
    mapon    = bfalse;
}

//---------------------------------------------------------------------------------------------
void release_all_textures()
{
    // ZZ> This function releases all of the textures
    int cnt;

    for ( cnt = 0; cnt < MAX_TEXTURE; cnt++ )
    {
        GLXtexture_Release( TxTexture + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_models()
{
    // ZZ> This function clears out all of the models
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipList + cnt, 0, sizeof(pip_t) );
    }

    for ( cnt = 0; cnt < MAXEVE; cnt++ )
    {
        memset( EveList + cnt, 0, sizeof(pip_t) );
    }

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( CapList + cnt, 0, sizeof(cap_t) );
    };

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( MadList + cnt, 0, sizeof(mad_t) );
        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
        MadList[cnt].ai = 0;
    }

    md2_loadframe = 0;

    reset_all_ai_scripts();
}

//--------------------------------------------------------------------------------------------
void debug_message( const char *text )
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot = get_free_message();
    // Copy the message
    int write = 0;
    int read = 0;
    char cTmp = text[read];  read++;
    msgtime[slot] = cfg.message_duration;

    while ( cTmp != 0 )
    {
        msgtextdisplay[slot][write] = cTmp;
        write++;
        cTmp = text[read];  read++;
    }

    msgtextdisplay[slot][write] = 0;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    // ZZ> This function resets the end-module text
	endtextwrite = sprintf( endtext, "The game has ended..." );

	/*
    if ( numpla > 1 )
    {
        endtextwrite = sprintf( endtext, "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( numpla == 0 )
        {
            // No players???
            endtextwrite = sprintf( endtext, "The game has ended..." );
        }
        else
        {
            // One player
            endtextwrite = sprintf( endtext, "Sadly, no trace was ever found..." );
        }
    }
	*/

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
void append_end_text( script_state_t * pstate, int message, Uint16 character )
{
    // ZZ> This function appends a message to the end-module text
    int read, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;
    Uint16 target, owner;

    target = ChrList[character].ai.target;
    owner = ChrList[character].ai.owner;
    if ( message < msgtotal )
    {
        // Copy the message
        read = msgindex[message];
        cnt = 0;
        cTmp = msgtext[read];  read++;

        while ( cTmp != 0 )
        {
            if ( cTmp == '%' )
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;
                if ( cTmp == 'n' )  // Name
                {
                    if ( ChrList[character].nameknown )
                        sprintf( szTmp, "%s", ChrList[character].name );
                    else
                    {
                        lTmp = CapList[ChrList[character].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[character].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[character].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'c' )  // Class name
                {
                    eread = CapList[ChrList[character].model].classname;
                }
                if ( cTmp == 't' )  // Target name
                {
                    if ( ChrList[target].nameknown )
                        sprintf( szTmp, "%s", ChrList[target].name );
                    else
                    {
                        lTmp = CapList[ChrList[target].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[target].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[target].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'o' )  // Owner name
                {
                    if ( ChrList[owner].nameknown )
                        sprintf( szTmp, "%s", ChrList[owner].name );
                    else
                    {
                        lTmp = CapList[ChrList[owner].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[owner].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[owner].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 's' )  // Target class name
                {
                    eread = CapList[ChrList[target].model].classname;
                }
                if ( cTmp >= '0' && cTmp <= '3' )  // Target's skin name
                {
                    eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
                }
                if ( NULL == pstate )
                {
                    sprintf( szTmp, "%%%c???", cTmp );
                }
                else
                {
                    if ( cTmp == 'd' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%d", pstate->distance );
                    }
                    if ( cTmp == 'x' )  // tmpx value
                    {
                        sprintf( szTmp, "%d", pstate->x );
                    }
                    if ( cTmp == 'y' )  // tmpy value
                    {
                        sprintf( szTmp, "%d", pstate->y );
                    }
                    if ( cTmp == 'D' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%2d", pstate->distance );
                    }
                    if ( cTmp == 'X' )  // tmpx value
                    {
                        sprintf( szTmp, "%2d", pstate->x );
                    }
                    if ( cTmp == 'Y' )  // tmpy value
                    {
                        sprintf( szTmp, "%2d", pstate->y );
                    }

                }
                if ( cTmp == 'a' )  // Character's ammo
                {
                    if ( ChrList[character].ammoknown )
                        sprintf( szTmp, "%d", ChrList[character].ammo );
                    else
                        sprintf( szTmp, "?" );
                }
                if ( cTmp == 'k' )  // Kurse state
                {
                    if ( ChrList[character].iskursed )
                        sprintf( szTmp, "kursed" );
                    else
                        sprintf( szTmp, "unkursed" );
                }
                if ( cTmp == 'p' )  // Character's possessive
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }
                if ( cTmp == 'm' )  // Character's gender
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "female " );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "male " );
                        }
                        else
                        {
                            sprintf( szTmp, " " );
                        }
                    }
                }
                if ( cTmp == 'g' )  // Target's possessive
                {
                    if ( ChrList[target].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[target].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }
				if ( cTmp == '#' )  // New line (enter)
                {
					sprintf( szTmp, "\n" );
                }

                // Copy the generated text
                cTmp = *eread;  eread++;

                while ( cTmp != 0 && endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    cTmp = *eread;  eread++;
                    endtextwrite++;
                }
            }
            else
            {
                // Copy the letter
                if ( endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    endtextwrite++;
                }
            }

            cTmp = msgtext[read];  read++;
            cnt++;
        }
    }

    endtext[endtextwrite] = 0;

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
void create_szfpstext( int frames )
{
    // ZZ> This function fills in the number of frames in "000 Frames per Second"
    frames = frames & 511;
    szfpstext[0] = '0' + ( frames / 100 );
    szfpstext[1] = '0' + ( ( frames / 10 ) % 10 );
    szfpstext[2] = '0' + ( frames % 10 );
}

//--------------------------------------------------------------------------------------------
void make_renderlist( camera_t * pcam )
{
    // ZZ> This function figures out which mesh fans to draw
    int cnt, fanx, fany;
    Uint32 fan;
    int row, run, numrow;
    int xlist[4], ylist[4];
    int leftnum, leftlist[4];
    int rightnum, rightlist[4];
    int fanrowstart[128], fanrowrun[128];
    int x, stepx, divx, basex;
    int from, to;

    tile_info_t * tlist = PMesh->mem.tile_list;

    // Find the render area corners
    project_view( pcam );

    // Clear old render lists
    for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
    {
        fan = renderlist.all[cnt];
        if( fan < PMesh->info.tiles_count )
        {
            tlist[fan].inrenderlist = bfalse;
        }
    }

    renderlist.all_count = 0;
    renderlist.ref_count = 0;
    renderlist.sha_count = 0;
    renderlist.drf_count = 0;
    renderlist.ndr_count = 0;

    // Make sure it doesn't die ugly !!!BAD!!!

    // It works better this way...
    cornery[cornerlistlowtohighy[3]] += 256;

    // Make life simpler
    xlist[0] = cornerx[cornerlistlowtohighy[0]];
    xlist[1] = cornerx[cornerlistlowtohighy[1]];
    xlist[2] = cornerx[cornerlistlowtohighy[2]];
    xlist[3] = cornerx[cornerlistlowtohighy[3]];
    ylist[0] = cornery[cornerlistlowtohighy[0]];
    ylist[1] = cornery[cornerlistlowtohighy[1]];
    ylist[2] = cornery[cornerlistlowtohighy[2]];
    ylist[3] = cornery[cornerlistlowtohighy[3]];

    // Find the center line

    divx = ylist[3] - ylist[0]; if ( divx < 1 ) return;
    stepx = xlist[3] - xlist[0];
    basex = xlist[0];

    // Find the points in each edge
    leftlist[0] = 0;  leftnum = 1;
    rightlist[0] = 0;  rightnum = 1;
    if ( xlist[1] < ( stepx*( ylist[1] - ylist[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 1;  leftnum++;
        cornerx[1] -= 512;
    }
    else
    {
        rightlist[rightnum] = 1;  rightnum++;
        cornerx[1] += 512;
    }
    if ( xlist[2] < ( stepx*( ylist[2] - ylist[0] ) / divx ) + basex )
    {
        leftlist[leftnum] = 2;  leftnum++;
        cornerx[2] -= 512;
    }
    else
    {
        rightlist[rightnum] = 2;  rightnum++;
        cornerx[2] += 512;
    }

    leftlist[leftnum] = 3;  leftnum++;
    rightlist[rightnum] = 3;  rightnum++;

    // Make the left edge ( rowstart )
    fany = ylist[0] >> TILE_BITS;
    row = 0;
    cnt = 1;
    while ( cnt < leftnum )
    {
        from = leftlist[cnt-1];  to = leftlist[cnt];
        x = xlist[from];
        divx = ylist[to] - ylist[from];
        stepx = 0;
        if ( divx > 0 )
        {
            stepx = ( ( xlist[to] - xlist[from] ) << TILE_BITS ) / divx;
        }

        x -= 256;
        run = ylist[to] >> TILE_BITS;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < PMesh->info.tiles_y )
            {
                fanx = x >> TILE_BITS;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= PMesh->info.tiles_x )  fanx = PMesh->info.tiles_x - 1;

                fanrowstart[row] = fanx;
                row++;
            }

            x += stepx;
            fany++;
        }

        cnt++;
    }
    numrow = row;

    // Make the right edge ( rowrun )
    fany = ylist[0] >> TILE_BITS;
    row = 0;
    cnt = 1;
    while ( cnt < rightnum )
    {
        from = rightlist[cnt-1];  to = rightlist[cnt];
        x = xlist[from];
        // x+=128;
        divx = ylist[to] - ylist[from];
        stepx = 0;
        if ( divx > 0 )
        {
            stepx = ( ( xlist[to] - xlist[from] ) << TILE_BITS ) / divx;
        }

        run = ylist[to] >> TILE_BITS;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < PMesh->info.tiles_y )
            {
                fanx = x >> TILE_BITS;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= PMesh->info.tiles_x - 1 )  fanx = PMesh->info.tiles_x - 1;//-2

                fanrowrun[row] = ABS( fanx - fanrowstart[row] ) + 1;
                row++;
            }

            x += stepx;
            fany++;
        }

        cnt++;
    }

    if ( numrow != row )
    {
        log_error( "ROW error (%i, %i)\n", numrow, row );
    }

    // Fill 'em up again
    fany = ylist[0] >> TILE_BITS;
    if ( fany < 0 ) fany = 0;
    if ( fany >= PMesh->info.tiles_y ) fany = PMesh->info.tiles_y - 1;

    for ( row = 0; row < numrow; row++, fany++ )
    {
        cnt = PMesh->mem.tilestart[fany] + fanrowstart[row];

        run = fanrowrun[row];
        for ( fanx = 0; fanx < run && renderlist.all_count < MAXMESHRENDER; fanx++, cnt++ )
        {
            // Put each tile in basic list
            tlist[cnt].inrenderlist = btrue;

            renderlist.all[renderlist.all_count] = cnt;
            renderlist.all_count++;

            // Put each tile in one other list, for shadows and relections
            if ( 0 != mesh_test_fx( PMesh, cnt, MPDFX_SHA ) )
            {
                renderlist.sha[renderlist.sha_count] = cnt;
                renderlist.sha_count++;
            }
            else
            {
                renderlist.ref[renderlist.ref_count] = cnt;
                renderlist.ref_count++;
            }

            if ( 0 != mesh_test_fx( PMesh, cnt, MPDFX_DRAWREF ) )
            {
                renderlist.drf[renderlist.drf_count] = cnt;
                renderlist.drf_count++;
            }
            else
            {
                renderlist.ndr[renderlist.ndr_count] = cnt;
                renderlist.ndr_count++;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_chr_flashing()
{
    int i;

    for( i=0; i<dolist_count; i++)
    {
        Uint16 ichr = dolist[i].ichr;

        if( INVALID_CHR(ichr) ) continue;

        // Do flashing
        if ( 0 == ( frame_all & ChrList[ichr].flashand ) && ChrList[ichr].flashand != DONTFLASH )
        {
            flash_character( ichr, 255 );
        }

        // Do blacking
        if ( 0 == ( frame_all & SEEKURSEAND ) && local_seekurse && ChrList[ichr].iskursed )
        {
            flash_character( ichr, 0 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void flash_character( Uint16 character, Uint8 value )
{
    // ZZ> This function sets a character's lighting

    ChrList[character].inst.color_amb = value;
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    // This function changes the animated tile frame
    if ( ( update_wld & animtileupdateand ) == 0 )
    {
        animtileframeadd = ( animtileframeadd + 1 ) & animtileframeand;
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( const char *modname )
{
    // ZZ> This function loads the standard textures for a module
    char newloadname[256];

    // Particle sprites
    ego_texture_load( TxTexture + TX_PARTICLE_TRANS, "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle_trans", TRANSCOLOR );
    ego_texture_load( TxTexture + TX_PARTICLE_LIGHT, "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle_light", INVALID_KEY );

    // Module background tiles
    make_newloadname( modname, "gamedat" SLASH_STR "tile0", newloadname );
    ego_texture_load( TxTexture + TX_TILE_0, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile1", newloadname );
    ego_texture_load(  TxTexture + TX_TILE_1, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile2", newloadname );
    ego_texture_load( TxTexture + TX_TILE_2, newloadname, TRANSCOLOR);

    make_newloadname( modname, "gamedat" SLASH_STR "tile3", newloadname );
    ego_texture_load( TxTexture + TX_TILE_3, newloadname, TRANSCOLOR );

    // Water textures
    make_newloadname( modname, "gamedat" SLASH_STR "watertop", newloadname );
    ego_texture_load( TxTexture + TX_WATER_TOP, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "waterlow", newloadname );
    ego_texture_load( TxTexture + TX_WATER_LOW, newloadname, TRANSCOLOR);

    // Texture 7 is the phong map
    ego_texture_load( TxTexture + TX_PHONG, "basicdat" SLASH_STR "phong", TRANSCOLOR );

}

//--------------------------------------------------------------------------------------------
void load_bars( const char* szBitmap )
{
    // ZZ> This function loads the status bar bitmap
    if ( INVALID_TX_ID == ego_texture_load( &TxBars, szBitmap, TRANSCOLOR ) )
    {
        log_warning( "Cannot load file! (\"%s\")\n", szBitmap );
    }
}

//--------------------------------------------------------------------------------------------
void load_map( const char* szModule )
{
    // ZZ> This function loads the map bitmap
    char szMap[256];

    // Turn it all off
    mapvalid = bfalse;
    mapon = bfalse;
    youarehereon = bfalse;
    numblip = 0;

    // Load the images
    sprintf( szMap, "%sgamedat" SLASH_STR "plan", szModule );
    if ( INVALID_TX_ID == ego_texture_load( &TxMap, szMap, INVALID_KEY ) )
    {
        log_warning( "Cannot load file! (\"%s\")\n", szMap );
    }
    else
    {
        mapvalid = btrue;
    }

}

//--------------------------------------------------------------------------------------------
void font_init()
{
    //Intitializes the font, ready to use
    GLXtexture_new( &TxFont );

    font_release();
}

//--------------------------------------------------------------------------------------------
void font_release()
{
    // BB > fill in default values

    Uint16 i, ix, iy, cnt;
    float dx, dy;

    GLXtexture_Release( &TxFont );

    // Mark all as unused
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        asciitofont[cnt] = 255;
    }

    dx = 256 / NUMFONTX;
    dy = 256 / NUMFONTY;
    for ( i = 0; i < NUMFONT; i++ )
    {
        ix = i % NUMFONTX;
        iy = i / NUMFONTX;

        fontrect[i].x = ix * dx;
        fontrect[i].w = dx;
        fontrect[i].y = iy * dy;
        fontrect[i].h = dy;
        fontxspacing[i] = 0;
    }
    fontyspacing = dy;
}

//--------------------------------------------------------------------------------------------
void font_load( const char* szBitmap, const char* szSpacing )
{
    // ZZ> This function loads the font bitmap and sets up the coordinates
    //     of each font on that bitmap...  Bitmap must have 16x6 fonts
    int cnt, y, xsize, ysize, xdiv, ydiv;
    int stt_x, stt_y;
    int xspacing, yspacing;
    char cTmp;
    FILE *fileread;

    font_release();
    if ( INVALID_TX_ID == ego_texture_load( &TxFont, szBitmap, TRANSCOLOR ) )
    {
        log_error( "Cannot load file! (\"%s\")\n", szBitmap );
    }

    // Get the size of the bitmap
    xsize = GLXtexture_GetImageWidth( &TxFont );
    ysize = GLXtexture_GetImageHeight( &TxFont );
    if ( xsize == 0 || ysize == 0 )
    {
        log_error( "Bad font size! (%i, %i)\n", xsize, ysize );
    }

    // Figure out the general size of each font
    ydiv = ysize / NUMFONTY;
    xdiv = xsize / NUMFONTX;

    // Figure out where each font is and its spacing
    fileread = fopen( szSpacing, "r" );
    if ( fileread == NULL )
    {
        log_error( "Font spacing not avalible! (%i, %i)\n", xsize, ysize );
    }

    parse_filename = szSpacing;
    y = 0;
    stt_x = 0;
    stt_y = 0;

    // Uniform font height is at the top
    goto_colon( NULL, fileread, bfalse );
    fscanf( fileread, "%d", &yspacing );
    fontoffset = yspacing;

    for ( cnt = 0; cnt < NUMFONT && goto_colon( NULL, fileread, btrue ); cnt++ )
    {
        fscanf( fileread, "%c%d", &cTmp, &xspacing );
        if ( asciitofont[(Uint8)cTmp] == 255 ) asciitofont[(Uint8)cTmp] = (Uint8) cnt;
        if ( stt_x + xspacing + 1 > 255 )
        {
            stt_x = 0;
            stt_y += yspacing;
        }

        fontrect[cnt].x = stt_x;
        fontrect[cnt].w = xspacing;
        fontrect[cnt].y = stt_y;
        fontrect[cnt].h = yspacing - 2;
        fontxspacing[cnt] = xspacing + 1;

        stt_x += xspacing + 1;
    }
    fclose( fileread );

    // Space between lines
    fontyspacing = ( yspacing >> 1 ) + FONTADD;
}

//--------------------------------------------------------------------------------------------
void make_water()
{
    // ZZ> This function sets up water movements
    int layer, frame, point, mode, cnt;
    float temp;
    Uint8 spek;

    for ( layer = 0; layer < numwaterlayer; layer++ )
    {
        if ( waterlight )  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...

        waterlayeru[layer] = 0;
        waterlayerv[layer] = 0;
        
        for ( frame = 0; frame < MAXWATERFRAME; frame++ )
        {
            // Do first mode
            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI/2 * layer / MAXWATERLAYER ) );
                waterlayerzadd[layer][frame][point] = temp * waterlayeramp[layer];
                waterlayercolor[layer][frame][point] = ( waterlightlevel[layer] * ( temp + 1.0f ) ) + waterlightadd[layer];
            }
        }
    }

    // Calculate specular highlights
    spek = 0;
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;
        if ( cnt > waterspekstart )
        {
            temp = cnt - waterspekstart;
            temp = temp / ( 256 - waterspekstart );
            temp = temp * temp;
            spek = temp * waterspeklevel;
        }

        // [claforte] Probably need to replace this with a
        //            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( gfx.shading == GL_FLAT )
            waterspek[cnt] = 0;
        else
            waterspek[cnt] = spek;
    }
}

//--------------------------------------------------------------------------------------------
void read_wawalite( const char *modname )
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float fTmp;
    char cTmp;
    int iTmp;

    make_newloadname( modname, "gamedat" SLASH_STR "wawalite.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread )
    {
        log_error( "Could not read file! (wawalite.txt)\n" );
        return;
    }

    goto_colon( NULL, fileread, bfalse );
    //  !!!BAD!!!
    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    //  !!!BAD!!!

    // Read water data first
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  numwaterlayer = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterspekstart = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterspeklevel = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterdouselevel = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  watersurfacelevel = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'T' || cTmp == 't' )  waterlight = btrue;
    else waterlight = bfalse;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    wateriswater = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  wateriswater = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    water_overlay_req = ('T' == toupper(cTmp));

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    water_background_req = ('T' == toupper(cTmp));

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[0] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[0] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[1] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[1] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  foregroundrepeat = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  backgroundrepeat = iTmp;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayerz[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlightadd[0] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[0] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[0] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[0] = fTmp;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayerz[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  waterlightadd[1] = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[1] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[1] = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[1] = fTmp;

    waterlayeru[0] = 0;
    waterlayerv[0] = 0;
    waterlayeru[1] = 0;
    waterlayerv[1] = 0;
    waterlayerframe[0] = rand() & WATERFRAMEAND;
    waterlayerframe[1] = rand() & WATERFRAMEAND;
    // Read light data second
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_y = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_z = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_a = fTmp;
    // Read tile data third
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  hillslide = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  slippyfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  airfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  noslipfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  gravity = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  animtileupdateand = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  animtileframeand = iTmp;
    biganimtileframeand = ( iTmp << 1 ) + 1;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  damagetileamount = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'S' || cTmp == 's' )  damagetiletype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  damagetiletype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  damagetiletype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  damagetiletype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  damagetiletype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  damagetiletype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  damagetiletype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  damagetiletype = DAMAGE_ZAP;

    // Read weather data fourth
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    weatheroverwater = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  weatheroverwater = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  weathertimereset = iTmp;
    weathertime = weathertimereset;
    weatherplayer = 0;
    // Read extra data
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    PMesh->info.exploremode = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  PMesh->info.exploremode = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    usefaredge = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  usefaredge = btrue;

    gCamera.swing = 0;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  gCamera.swingrate = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  gCamera.swingamp = fTmp;

    // Read unnecessary data...  Only read if it exists...
    fogon = bfalse;
    fogaffectswater = btrue;
    fogtop = 100;
    fogbottom = 0;
    fogdistance = 100;
    fogred = 255;
    foggrn = 255;
    fogblu = 255;
    damagetileparttype = -1;
    damagetilepartand = 255;
    damagetilesound = -1;
    damagetilesoundtime = TILESOUNDTIME;
    damagetilemindistance = 9999;
    if ( goto_colon( NULL, fileread, btrue ) )
    {
        fogon = cfg.fog_allowed;
        fscanf( fileread, "%f", &fTmp );  fogtop = fTmp;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fogbottom = fTmp;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fogred = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  foggrn = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fogblu = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'F' || cTmp == 'f' )  fogaffectswater = bfalse;

        fogdistance = ( fogtop - fogbottom );
        if ( fogdistance < 1.0f )  fogon = bfalse;

        // Read extra stuff for damage tile particles...
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            fscanf( fileread, "%d", &iTmp );  damagetileparttype = iTmp;
            goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );
            damagetilepartand = iTmp;
            goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );
            damagetilesound = CLIP(iTmp, -1, MAX_WAVE);
        }
    }

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && numwaterlayer > 1 )
    {
        numwaterlayer = 1;
        iTmp = waterlayeralpha[0];
        iTmp = FP8_MUL( waterlayeralpha[1], iTmp ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        waterlayeralpha[0] = iTmp;
    }

    fclose( fileread );

    // Do it
    make_lighttable( light_x, light_y, light_z, light_a );
    make_lighttospek();
    make_water();
}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
  // ZZ> This function draws the large background
  GLvertex vtlist[4];
  int i;
  float z0, d, mag0,mag1, Qx,Qy;

  z0 = 1500; // the original height of the gCamera.era
  d = MIN(waterlayerdistx[0], waterlayerdisty[0])/10.0f;
  mag0 = 1.0f/(1.0f + z0*d);
  //mag1 = backgroundrepeat/128.0f/10;
  mag1 = 1.0f/128.0f/5.0f;

  // clip the waterlayer uv offset
  waterlayeru[0] = waterlayeru[0] - (float)floor(waterlayeru[0]);
  waterlayerv[0] = waterlayerv[0] - (float)floor(waterlayerv[0]);

  // Figure out the coordinates of its corners
  Qx = -PMesh->info.edge_x;
  Qy = -PMesh->info.edge_y;
  vtlist[0].pos[XX] = mag0 * gCamera.pos.x + Qx * ( 1.0f - mag0 );
  vtlist[0].pos[YY] = mag0 * gCamera.pos.y + Qy * ( 1.0f - mag0 );
  vtlist[0].pos[ZZ] = 0;
  vtlist[0].tex[SS] = Qx * mag1 + waterlayeru[0];
  vtlist[0].tex[TT] = Qy * mag1 + waterlayerv[0];

  Qx = 2*PMesh->info.edge_x;
  Qy = -PMesh->info.edge_y;
  vtlist[1].pos[XX] = mag0 * gCamera.pos.x + Qx * ( 1.0f - mag0 );
  vtlist[1].pos[YY] = mag0 * gCamera.pos.y + Qy * ( 1.0f - mag0 );
  vtlist[1].pos[ZZ] = 0;
  vtlist[1].tex[SS] = Qx * mag1 + waterlayeru[0];
  vtlist[1].tex[TT] = Qy * mag1 + waterlayerv[0];

  Qx = 2*PMesh->info.edge_x;
  Qy = 2*PMesh->info.edge_y;
  vtlist[2].pos[XX] = mag0 * gCamera.pos.x + Qx * ( 1.0f - mag0 );
  vtlist[2].pos[YY] = mag0 * gCamera.pos.y + Qy * ( 1.0f - mag0 );
  vtlist[2].pos[ZZ] = 0;
  vtlist[2].tex[SS] = Qx * mag1 + waterlayeru[0];
  vtlist[2].tex[TT] = Qy * mag1 + waterlayerv[0];

  Qx = -PMesh->info.edge_x;
  Qy = 2*PMesh->info.edge_y;
  vtlist[3].pos[XX] = mag0 * gCamera.pos.x + Qx * ( 1.0f - mag0 );
  vtlist[3].pos[YY] = mag0 * gCamera.pos.y + Qy * ( 1.0f - mag0 );
  vtlist[3].pos[ZZ] = 0;
  vtlist[3].tex[SS] = Qx * mag1 + waterlayeru[0];
  vtlist[3].tex[TT] = Qy * mag1 + waterlayerv[0];

  {
    GLvector3 intens = VECT3(0.0f, 0.0f, 0.0f);
    float alpha  = 1.0f;
    GLint shading_save, depthfunc_save;
    GLboolean depthmask_save, cullface_save;

    //if(gLight.on)
    //{
    //  float fcos;
    //  GLvector3 updir = VECT3( 0,0,-gPhys.gravity);

    //  VNormalizeEq3( updir.v, 1.0f );
    //  fcos = VDotProduct3(gLight.spekdir, updir);
    //  intens = gLight.ambicol;
    //  if(fcos > 0)
    //  {
    //    intens = VAdd3( VScale3(gLight.spekcol, fcos*fcos), intens);
    //  }
    //}

    alpha = waterlayeralpha[0] / 255.0f;

    GLXtexture_Bind ( TxTexture + texture );

    glGetIntegerv( GL_SHADE_MODEL, &shading_save );
    glShadeModel( GL_FLAT );  // Flat shade this

    depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
    glDepthMask( GL_FALSE );

    glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
    glDepthFunc( GL_ALWAYS );

    cullface_save = glIsEnabled( GL_CULL_FACE );
    glDisable( GL_CULL_FACE );

    glColor4f( 1, 1, 1, alpha );
    glBegin ( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2fv ( vtlist[i].tex );
      glVertex3fv ( vtlist[i].pos );
    }
    glEnd ();

    glDepthFunc( depthfunc_save );
    glDepthMask( depthmask_save );
    glShadeModel(shading_save);
    if(cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
  }
}


//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
  // ZZ> This function draws the large foreground
  GLvertex vtlist[4];
  GLvector3 vforw_wind, vforw_cam;
  int i;
  float size;
  float sinsize, cossize;
  float x, y, z;
  float u, v;
  float loc_foregroundrepeat;

  // Figure out the screen coordinates of its corners
  x = sdl_scr.x << 6;
  y = sdl_scr.y << 6;
  z = 0;
  u = waterlayeru[1];
  v = waterlayerv[1];
  size = x + y + 1;
  sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
  cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
  loc_foregroundrepeat = foregroundrepeat * MIN( x / sdl_scr.x, y / sdl_scr.x );

  vtlist[0].pos[XX] = x + cossize;
  vtlist[0].pos[YY] = y - sinsize;
  vtlist[0].pos[ZZ] = z;
  vtlist[0].tex[SS] = 0 + u;
  vtlist[0].tex[TT] = 0 + v;

  vtlist[1].pos[XX] = x + sinsize;
  vtlist[1].pos[YY] = y + cossize;
  vtlist[1].pos[ZZ] = z;
  vtlist[1].tex[SS] = loc_foregroundrepeat + u;
  vtlist[1].tex[TT] = 0 + v;

  vtlist[2].pos[XX] = x - cossize;
  vtlist[2].pos[YY] = y + sinsize;
  vtlist[2].pos[ZZ] = z;
  vtlist[2].tex[SS] = loc_foregroundrepeat + u;
  vtlist[2].tex[TT] = loc_foregroundrepeat + v;

  vtlist[3].pos[XX] = x - sinsize;
  vtlist[3].pos[YY] = y - cossize;
  vtlist[3].pos[ZZ] = z;
  vtlist[3].tex[SS] = 0 + u;
  vtlist[3].tex[TT] = loc_foregroundrepeat + v;

  vforw_wind.x = waterlayeruadd[1];
  vforw_wind.y = waterlayervadd[1];
  vforw_wind.z = 0;
  vforw_wind = VNormalize( vforw_wind );
  vforw_cam  = mat_getCamForward( gCamera.mView );

  {
    float alpha;
    GLint shading_save, depthfunc_save, smoothhint_save;
    GLboolean depthmask_save, cullface_save, alphatest_save;

    GLint alphatestfunc_save, alphatestref_save, alphablendsrc_save, alphablenddst_save;
    GLboolean alphablend_save;

    glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &smoothhint_save);
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );             // make sure that the texture is as smooth as possible

    GLXtexture_Bind ( TxTexture + texture );

    glGetIntegerv( GL_SHADE_MODEL, &shading_save );
    glShadeModel( GL_FLAT );  // Flat shade this

    depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
    glDepthMask( GL_FALSE );

    glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
    glDepthFunc( GL_ALWAYS );

    cullface_save = glIsEnabled( GL_CULL_FACE );
    glDisable( GL_CULL_FACE );

    alphatest_save = glIsEnabled( GL_ALPHA_TEST );
    glEnable( GL_ALPHA_TEST );

    glGetIntegerv( GL_ALPHA_TEST_FUNC, &alphatestfunc_save );
    glGetIntegerv( GL_ALPHA_TEST_REF, &alphatestref_save );
    glAlphaFunc( GL_GREATER, 0 );

    alphablend_save = glIsEnabled( GL_BLEND );
    glEnable( GL_BLEND );

    glGetIntegerv( GL_BLEND_SRC, &alphablendsrc_save );
    glGetIntegerv( GL_BLEND_DST, &alphablenddst_save );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // make the texture a filter

    // make the texture begin to disappear if you are not looking straight down
    alpha = VDotProduct( vforw_wind, vforw_cam );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f - ABS(alpha) );
    glBegin ( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2fv ( vtlist[i].tex );
      glVertex3fv ( vtlist[i].pos );
    }
    glEnd ();

    glHint( GL_POLYGON_SMOOTH_HINT, smoothhint_save );
    glShadeModel( shading_save );
    glDepthMask( depthmask_save );
    glDepthFunc( depthfunc_save );

    if(cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
    if(alphatest_save) glEnable( GL_ALPHA_TEST ); else glDisable( GL_ALPHA_TEST );
    glAlphaFunc( alphatestfunc_save, alphatestref_save );

    if(alphablend_save) glEnable( GL_BLEND ); else glDisable( GL_BLEND );
    glBlendFunc( alphablendsrc_save, alphablenddst_save );
  }
}//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_shadow_sprite( float intensity, GLvertex v[] )
{
    int i;

    if ( intensity*255.0f < 1.0f ) return;

    glColor4f( intensity, intensity, intensity, 1.0f );

    glBegin( GL_TRIANGLE_FAN );
    {
        for ( i = 0; i < 4; i++ )
        {
            glTexCoord2fv ( v[i].tex );
            glVertex3fv ( v[i].pos );
        }
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void render_shadow( Uint16 character )
{
    // ZZ> This function draws a NIFTY shadow
    GLvertex v[4];

    float x, y;
    float level;
    float height, size_umbra, size_penumbra;
    float alpha, alpha_umbra, alpha_penumbra;
    chr_t * pchr;

    if ( character >= MAX_CHR || !ChrList[character].on || ChrList[character].inpack ) return;
    pchr = ChrList + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden ) return;

    // no shadow if off the mesh
    if ( INVALID_TILE == pchr->onwhichfan || FANOFF == PMesh->mem.tile_list[pchr->onwhichfan].img ) return;

    // no shadow if completely transparent
    alpha = (pchr->inst.alpha * INV_FF) * (pchr->inst.light * INV_FF);
    if ( alpha * 255 < 1.0f ) return;

    // much resuced shadow if on a reflective tile
    if ( 0 != mesh_test_fx(PMesh, pchr->onwhichfan, MPDFX_DRAWREF) )
    {
        alpha *= 0.1f;
    }
    if ( alpha * 255 < 1.0f ) return;

    // Original points
    level = pchr->floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    if ( height < 0 ) height = 0;

    size_umbra    = 1.5f * ( pchr->bumpsize - height / 30.0f );
    size_penumbra = 1.5f * ( pchr->bumpsize + height / 30.0f );

    alpha *= 0.3f;
    alpha_umbra = alpha_penumbra = alpha;
    if ( height > 0 )
    {
        float factor_penumbra = ( 1.5f ) * ( ( pchr->bumpsize ) / size_penumbra );
        float factor_umbra    = ( 1.5f ) * ( ( pchr->bumpsize ) / size_umbra );

        factor_umbra    = MAX(1.0f, factor_umbra);
        factor_penumbra = MAX(1.0f, factor_penumbra);

        alpha_umbra    *= 1.0f / factor_umbra / factor_umbra / 1.5f;
        alpha_penumbra *= 1.0f / factor_penumbra / factor_penumbra / 1.5f;

        alpha_umbra    = CLIP(alpha_umbra,    0.0f, 1.0f);
        alpha_penumbra = CLIP(alpha_penumbra, 0.0f, 1.0f);
    }

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    // Choose texture.
    GLXtexture_Bind( TxTexture + TX_PARTICLE_LIGHT );

    // GOOD SHADOW
    v[0].tex[SS] = sprite_list_u[238][0];
    v[0].tex[TT] = sprite_list_v[238][0];

    v[1].tex[SS] = sprite_list_u[255][1];
    v[1].tex[TT] = sprite_list_v[238][0];

    v[2].tex[SS] = sprite_list_u[255][1];
    v[2].tex[TT] = sprite_list_v[255][1];

    v[3].tex[SS] = sprite_list_u[238][0];
    v[3].tex[TT] = sprite_list_v[255][1];

    if ( size_penumbra > 0 )
    {
        v[0].pos[XX] = x + size_penumbra;
        v[0].pos[YY] = y - size_penumbra;
        v[0].pos[ZZ] = level;

        v[1].pos[XX] = x + size_penumbra;
        v[1].pos[YY] = y + size_penumbra;
        v[1].pos[ZZ] = level;

        v[2].pos[XX] = x - size_penumbra;
        v[2].pos[YY] = y + size_penumbra;
        v[2].pos[ZZ] = level;

        v[3].pos[XX] = x - size_penumbra;
        v[3].pos[YY] = y - size_penumbra;
        v[3].pos[ZZ] = level;

        render_shadow_sprite(alpha_penumbra, v );
    };

    if ( size_umbra > 0 )
    {
        v[0].pos[XX] = x + size_umbra;
        v[0].pos[YY] = y - size_umbra;
        v[0].pos[ZZ] = level + 0.1f;

        v[1].pos[XX] = x + size_umbra;
        v[1].pos[YY] = y + size_umbra;
        v[1].pos[ZZ] = level + 0.1f;

        v[2].pos[XX] = x - size_umbra;
        v[2].pos[YY] = y + size_umbra;
        v[2].pos[ZZ] = level + 0.1f;

        v[3].pos[XX] = x - size_umbra;
        v[3].pos[YY] = y - size_umbra;
        v[3].pos[ZZ] = level + 0.1f;

        render_shadow_sprite( alpha_umbra, v );
    };

}

//--------------------------------------------------------------------------------------------
void render_bad_shadow( Uint16 character )
{
    // ZZ> This function draws a sprite shadow
    GLvertex v[4];
    float size, x, y;
    float level, height, height_factor, alpha;
    chr_t * pchr;

    if ( character >= MAX_CHR || !ChrList[character].on || ChrList[character].inpack ) return;
    pchr = ChrList + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden ) return;

    // no shadow if off the mesh
    if ( INVALID_TILE == pchr->onwhichfan || FANOFF == PMesh->mem.tile_list[pchr->onwhichfan].img ) return;

    // no shadow if completely transparent or completely glowing
    alpha = (pchr->inst.alpha * INV_FF) * (pchr->inst.light * INV_FF);
    if ( alpha < INV_FF ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != mesh_test_fx(PMesh, pchr->onwhichfan, MPDFX_DRAWREF) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    height_factor = 1.0f - height / ( pchr->shadowsize * 5.0f );
    if ( height_factor <= 0.0f ) return;

    // how much transparency from height
    alpha *= height_factor * 0.5f + 0.25f;
    if ( alpha < INV_FF ) return;

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    size = pchr->shadowsize * height_factor;

    v[0].pos[XX] = ( float ) x + size;
    v[0].pos[YY] = ( float ) y - size;
    v[0].pos[ZZ] = ( float ) level;

    v[1].pos[XX] = ( float ) x + size;
    v[1].pos[YY] = ( float ) y + size;
    v[1].pos[ZZ] = ( float ) level;

    v[2].pos[XX] = ( float ) x - size;
    v[2].pos[YY] = ( float ) y + size;
    v[2].pos[ZZ] = ( float ) level;

    v[3].pos[XX] = ( float ) x - size;
    v[3].pos[YY] = ( float ) y - size;
    v[3].pos[ZZ] = ( float ) level;

    // Choose texture and matrix
    GLXtexture_Bind( TxTexture + TX_PARTICLE_LIGHT );

    v[0].tex[SS] = sprite_list_u[236][0];
    v[0].tex[TT] = sprite_list_v[236][0];

    v[1].tex[SS] = sprite_list_u[253][1];
    v[1].tex[TT] = sprite_list_v[236][0];

    v[2].tex[SS] = sprite_list_u[253][1];
    v[2].tex[TT] = sprite_list_v[253][1];

    v[3].tex[SS] = sprite_list_u[236][0];
    v[3].tex[TT] = sprite_list_v[253][1];

    render_shadow_sprite( alpha, v );
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float get_mix(float u0, float u, float v0, float v)
{
    float wt_u, wt_v;
    float du = u - u0;
    float dv = v - v0;

    if( ABS(du) > 1.5 || ABS(dv) > 1.5 ) return 0;

    du *= 4.0f / 9.0f;
    wt_u = (1.0f-du)*(1.0f+du);

    dv *= 4.0f / 9.0f;
    wt_v = (1.0f-dv)*(1.0f+dv);

    return wt_u * wt_v;
}

//--------------------------------------------------------------------------------------------
void light_fans()
{
    int entry, cnt;
    Uint8 type;

    for ( entry = 0; entry < renderlist.all_count; entry++ )
    {
        int vertex,numvertices;
        int ix,iy, dx,dy;
        int fan;
        float min_x, max_x, min_y, max_y, min_z, max_z;
        float offset_x[4] = {0,1,1,0}, offset_y[4] = {0,0,1,1};

        fan = renderlist.all[entry];
        if ( INVALID_TILE == fan || fan > PMesh->info.tiles_count ) continue;

        ix = fan % PMesh->info.tiles_x;
        iy = fan / PMesh->info.tiles_x;

        min_x = ix * TILE_SIZE;
        max_x = min_x + TILE_SIZE;
        min_y = iy * TILE_SIZE;
        max_y = min_y + TILE_SIZE;

        type   = PMesh->mem.tile_list[fan].type;
        vertex = PMesh->mem.tile_list[fan].vrtstart;

        numvertices = tile_dict[type].numvertices;

        // find the actual extent of the tile
        //min_x = max_x = PMesh->mem.vrt_x[vertex];
        //min_y = max_y = PMesh->mem.vrt_y[vertex];
        //min_z = max_z = PMesh->mem.vrt_z[vertex];
        //for(cnt=1; cnt<numvertices; cnt++)
        //{
        //    min_x = MIN(min_x, PMesh->mem.vrt_x[vertex + cnt]);
        //    min_y = MIN(min_y, PMesh->mem.vrt_y[vertex + cnt]);
        //    min_z = MIN(min_z, PMesh->mem.vrt_z[vertex + cnt]);

        //    max_x = MAX(max_x, PMesh->mem.vrt_x[vertex + cnt]);
        //    max_y = MAX(max_y, PMesh->mem.vrt_y[vertex + cnt]);
        //    max_z = MAX(max_z, PMesh->mem.vrt_z[vertex + cnt]);
        //};

        // do the 4 corners
        for(cnt=0; cnt<4; cnt++)
        {
            float u,v;
            float light;

            // find the normal
            u = offset_x[cnt];
            v = offset_y[cnt];

            dx = u < 0.5f ? 0 : 1;
            dy = v < 0.5f ? 0 : 1;

            // get the vertex normal
            fan = mesh_get_tile_int( PMesh, ix + dx, iy + dy );

            light = 0;
            if( VALID_TILE(PMesh, fan) )
            {
                GLvector3 * pnrm;
                light_cache_t * cache;

                cache = PMesh->mem.cache + fan;
                pnrm  = PMesh->mem.nrm   + fan;

                if( cache->max_light > 0 )
                {
                    if ( pnrm->x > 0 )
                    {
                        light += ABS(pnrm->x) * cache->lighting[0];
                    }
                    else if ( pnrm->x < 0 )
                    {
                        light += ABS(pnrm->x) * cache->lighting[1];
                    }

                    if ( pnrm->y > 0 )
                    {
                        light += ABS(pnrm->y) * cache->lighting[2];
                    }
                    else if ( pnrm->y < 0 )
                    {
                        light += ABS(pnrm->y) * cache->lighting[3];
                    }

                    if ( pnrm->z > 0 )
                    {
                        light += ABS(pnrm->z) * cache->lighting[4];
                    }
                    else if ( pnrm->z < 0 )
                    {
                        light += ABS(pnrm->z) * cache->lighting[5];
                    }
                }
            }
            light = CLIP(light, 0, 255);

            PMesh->mem.vrt_l[vertex + cnt] = PMesh->mem.vrt_l[vertex + cnt] * 0.9f + light * 0.1f;
        }

        // linearly interpolate the other vertices
        for( /* nothing */ ; cnt<numvertices; cnt++)
        {
            // sum up the lighting
            int   tnc;
            float u,v;
            float light;
            float weight_sum;

            u = (PMesh->mem.vrt_x[vertex + cnt] - min_x) / (max_x - min_x);
            v = (PMesh->mem.vrt_y[vertex + cnt] - min_y) / (max_y - min_y);

            weight_sum = 0;
            light      = 0;
            for( tnc=0; tnc<4; tnc++ )
            {
                float u2, v2;
                float wt;

                u2 = (PMesh->mem.vrt_x[vertex + tnc] - min_x) / (max_x - min_x);
                v2 = (PMesh->mem.vrt_y[vertex + tnc] - min_y) / (max_y - min_y);

                wt = get_mix(u2, u, v2, v);

                weight_sum += wt;
                light      += wt * PMesh->mem.vrt_l[vertex + tnc];
            }

            if( light > 0 && weight_sum > 0.0 )
            {
                light /= weight_sum;
            }
            else
            {
                light = 0;
            }
            light = CLIP(light, 0, 255);

            PMesh->mem.vrt_l[vertex + cnt] = PMesh->mem.vrt_l[vertex + cnt] * 0.9f + light * 0.1f;
        }
    }

}

////--------------------------------------------------------------------------------------------
//void light_fans()
//{
//    int entry, cnt;
//    Uint8 type;
//    float light;
//
//    for ( entry = 0; entry < renderlist.all_count; entry++ )
//    {
//        int vertex,numvertices;
//        int ix,iy, dx,dy;
//        int fan;
//        float min_x, max_x, min_y, max_y, min_z, max_z;
//
//        fan = renderlist.all[entry];
//        if ( INVALID_TILE == fan || fan > PMesh->info.tiles_count ) continue;
//
//        ix = fan % PMesh->info.tiles_x;
//        iy = fan / PMesh->info.tiles_x;
//
//        type   = PMesh->mem.tile_list[fan].type;
//        vertex = PMesh->mem.tile_list[fan].vrtstart;
//
//        numvertices = tile_dict[type].numvertices;
//
//        // find the actual extent of the tile
//        min_x = max_x = PMesh->mem.vrt_x[vertex];
//        min_y = max_y = PMesh->mem.vrt_y[vertex];
//        min_z = max_z = PMesh->mem.vrt_z[vertex];
//        for(cnt=1; cnt<numvertices; cnt++)
//        {
//            min_x = MIN(min_x, PMesh->mem.vrt_x[vertex + cnt]);
//            min_y = MIN(min_y, PMesh->mem.vrt_y[vertex + cnt]);
//            min_z = MIN(min_z, PMesh->mem.vrt_z[vertex + cnt]);
//
//            max_x = MAX(max_x, PMesh->mem.vrt_x[vertex + cnt]);
//            max_y = MAX(max_y, PMesh->mem.vrt_y[vertex + cnt]);
//            max_z = MAX(max_z, PMesh->mem.vrt_z[vertex + cnt]);
//        };
//
//        // do the 4 corners
//        for(cnt=0; cnt<4; cnt++)
//        {
//            float u,v;
//            float light;
//            int weight_sum;
//            GLvector3 nrm;
//
//            // find the normal
//            u = (PMesh->mem.vrt_x[vertex + cnt] - min_x) / (max_x - min_x);
//            v = (PMesh->mem.vrt_y[vertex + cnt] - min_y) / (max_y - min_y);
//
//            dx = u < 0.5f ? 0 : 1;
//            dy = v < 0.5f ? 0 : 1;
//
//            // get the vertex normal
//            fan = mesh_get_tile_int( PMesh, ix + dx, iy + dy );
//            if( INVALID_TILE == fan )
//            {
//                nrm.x = nrm.y = 0.0f;
//                nrm.z = 1.0f;
//            }
//            else
//            {
//                nrm = PMesh->mem.nrm[fan];
//            }
//
//            light = 0;
//            weight_sum = 0;
//            for(dx = -1; dx <= 0; dx++)
//            {
//                for(dy = -1; dy<= 0; dy++)
//                {
//                    fan = mesh_get_tile_int( PMesh, ix + dx, iy + dy );
//                    if( VALID_TILE(PMesh, fan) )
//                    {
//                        float wt;
//                        GLvector3 * pnrm;
//                        light_cache_t * cache;
//                        float loc_light;
//
//                        cache = PMesh->mem.cache + fan;
//
//                        loc_light = 0;
//                        if( cache->max_light > 0 )
//                        {
//                            if ( nrm.x > 0 )
//                            {
//                                loc_light += ABS(nrm.x) * cache->lighting[0];
//                            }
//                            else if ( nrm.x < 0 )
//                            {
//                                loc_light += ABS(nrm.x) * cache->lighting[1];
//                            }
//
//                            if ( nrm.y > 0 )
//                            {
//                                loc_light += ABS(nrm.y) * cache->lighting[2];
//                            }
//                            else if ( nrm.y < 0 )
//                            {
//                                loc_light += ABS(nrm.y) * cache->lighting[3];
//                            }
//
//                            if ( nrm.z > 0 )
//                            {
//                                loc_light += ABS(nrm.z) * cache->lighting[4];
//                            }
//                            else if ( nrm.z < 0 )
//                            {
//                                loc_light += ABS(nrm.z) * cache->lighting[5];
//                            }
//                        }
//                        weight_sum++;
//                        light += loc_light;
//                    }
//                }
//            }
//
//
//            if( light > 0 && weight_sum > 0 )
//            {
//                light /= weight_sum;
//            }
//            else
//            {
//                light = 0;
//            }
//            light = CLIP(light, 0, 255);
//
//            PMesh->mem.vrt_l[vertex + cnt] = PMesh->mem.vrt_l[vertex + cnt] * 0.9f + light * 0.1f;
//        }
//
//        // linearly interpolate the other vertices
//        for( /* nothing */ ; cnt<numvertices; cnt++)
//        {
//            // sum up the lighting
//            int   tnc;
//            float u,v;
//            float light;
//            float weight_sum;
//
//            u = (PMesh->mem.vrt_x[vertex + cnt] - min_x) / (max_x - min_x);
//            v = (PMesh->mem.vrt_y[vertex + cnt] - min_y) / (max_y - min_y);
//
//            weight_sum = 0;
//            light      = 0;
//            for( tnc=0; tnc<4; tnc++ )
//            {
//                float u2, v2;
//                float wt;
//
//                u2 = (PMesh->mem.vrt_x[vertex + tnc] - min_x) / (max_x - min_x);
//                v2 = (PMesh->mem.vrt_y[vertex + tnc] - min_y) / (max_y - min_y);
//
//                wt = get_mix(u2, u, v2, v);
//
//                weight_sum += wt;
//                light      += wt * PMesh->mem.vrt_l[vertex + tnc];
//            }
//
//            if( light > 0 && weight_sum > 0.0 )
//            {
//                light /= weight_sum;
//            }
//            else
//            {
//                light = 0;
//            }
//            light = CLIP(light, 0, 255);
//
//            PMesh->mem.vrt_l[vertex + cnt] = PMesh->mem.vrt_l[vertex + cnt] * 0.9f + light * 0.1f;
//        }
//    }
//
//}
//--------------------------------------------------------------------------------------------
void light_characters()
{
    // ZZ> This function figures out character lighting

    int cnt, tnc;
    Uint16 light_max, light_min;
    Uint16 tl, tr, bl, br;
    mesh_mem_t * pmem = &(PMesh->mem);

    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        int istart;
        chr_t * pchr;
        chr_instance_t * pinst;

        tnc = dolist[cnt].ichr;

        if( INVALID_CHR(tnc) ) continue;
        pchr = ChrList + tnc;
        pinst = &(pchr->inst);

        if ( INVALID_TILE == pchr->onwhichfan )
        {
            pinst->light_turn_z   = 0;
            pinst->lightlevel_amb = 0;
            pinst->lightlevel_dir = 0;
            continue;
        }

        istart = pmem->tile_list[pchr->onwhichfan].vrtstart;

        // grab the corner intensities
        tl = pmem->vrt_l[ istart + 0 ];
        tr = pmem->vrt_l[ istart + 1 ];
        br = pmem->vrt_l[ istart + 2 ];
        bl = pmem->vrt_l[ istart + 3 ];

        // determine the amount of directionality
        light_min = MIN(MIN(tl, tr), MIN(bl, br));
        light_max = MAX(MAX(tl, tr), MAX(bl, br));

        if (light_max == 0 && light_min == 0 )
        {
            pinst->light_turn_z = 0;
            pinst->lightlevel_amb = 0;
            pinst->lightlevel_dir = 0;
            continue;
        }
        else if ( light_max == light_min )
        {
            pinst->light_turn_z = 0;
            pinst->lightlevel_amb = light_min;
            pinst->lightlevel_dir = 0;
        }
        else
        {
            int ix, iy;
            Uint16 itop, ibot;
            Uint32 light;

            // Interpolate lighting level using tile corners
            ix = ((int)pchr->pos.x) & 127;
            iy = ((int)pchr->pos.y) & 127;

            itop = tl * (128 - ix) + tr * ix;
            ibot = bl * (128 - ix) + br * ix;
            light = (128 - iy) * itop + iy * ibot;
            light >>= 14;

            pinst->lightlevel_dir = ( light * (light_max - light_min) ) / light_max;
            pinst->lightlevel_amb = light - pinst->lightlevel_dir;

            if ( !PMesh->info.exploremode && pinst->lightlevel_dir > 0 )
            {
                Uint32 lookup;

                // Look up light direction using corners again
                lookup  = ( tl & 0xf0 ) << 8;
                lookup |= ( tr & 0xf0 ) << 4;
                lookup |= ( br & 0xf0 );
                lookup |= ( bl & 0xf0 ) >> 4;

                pinst->light_turn_z = lightdirectionlookup[tl] << 8;
            }
            else
            {
                pinst->light_turn_z = 0;
            }
        }
    }

    // do character flashing
    do_chr_flashing();
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
    // ZZ> This function figures out particle lighting
    int iprt;
    int character;

    mesh_mem_t * pmem = &(PMesh->mem);

    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if( INVALID_PRT(iprt) ) continue;
        pprt = PrtList + iprt;
        pinst = &(pprt->inst);

        character = pprt->attachedtocharacter;
        if ( VALID_CHR( character ) )
        {
            pprt->inst.light = ChrList[character].inst.lightlevel_amb;
        }
        else if ( INVALID_TILE == pprt->onwhichfan )
        {
            pprt->inst.light = 0;
        }
        else
        {
            Uint32 istart;
            Uint16 tl, tr, br, bl;
            Uint16 light_min, light_max;

            istart = pmem->tile_list[pprt->onwhichfan].vrtstart;

            // grab the corner intensities
            tl = pmem->vrt_l[ istart + 0 ];
            tr = pmem->vrt_l[ istart + 1 ];
            br = pmem->vrt_l[ istart + 2 ];
            bl = pmem->vrt_l[ istart + 3 ];

            // determine the amount of directionality
            light_min = MIN(MIN(tl, tr), MIN(bl, br));
            light_max = MAX(MAX(tl, tr), MAX(bl, br));

            if (light_max == 0 && light_min == 0 )
            {
                pinst->light = 0;
                continue;
            }
            else if ( light_max == light_min )
            {
                pinst->light = light_min;
            }
            else
            {
                int ix, iy;
                Uint16 itop, ibot;
                Uint32 light;

                // Interpolate lighting level using tile corners
                ix = ((int)pprt->pos.x) & 127;
                iy = ((int)pprt->pos.y) & 127;

                itop = tl * (128 - ix) + tr * ix;
                ibot = bl * (128 - ix) + br * ix;
                light = (128 - iy) * itop + iy * ibot;
                light >>= 14;

                pprt->inst.light = light;
            }
        }
    }
}


////--------------------------------------------------------------------------------------------
//void set_fan_light( int fanx, int fany, Uint16 particle )
//{
//    // ZZ> This function is a little helper, lighting the selected fan
//    //     with the chosen particle
//    float x, y;
//    int fan, vertex, lastvertex;
//    float level;
//    float light;
//
//    if ( fanx >= 0 && fanx < PMesh->info.tiles_x && fany >= 0 && fany < PMesh->info.tiles_y )
//    {
//        // allow raw access because we were careful
//        fan = mesh_get_tile_int( PMesh, fanx, fany );
//
//        if ( VALID_TILE(PMesh, fan) )
//        {
//            Uint8 ttype = PMesh->mem.tile_list[fan].type;
//
//            if ( ttype < MAXMESHTYPE )
//            {
//                vertex = PMesh->mem.tile_list[fan].vrtstart;
//                lastvertex = vertex + tile_dict[ttype].numvertices;
//
//                while ( vertex < lastvertex )
//                {
//                    light = PMesh->mem.vrt_a[vertex];
//                    x = PrtList[particle].pos.x - PMesh->mem.vrt_x[vertex];
//                    y = PrtList[particle].pos.y - PMesh->mem.vrt_y[vertex];
//                    level = ( x * x + y * y ) / PrtList[particle].dynalightfalloff;
//                    level = 255 - level;
//                    level = level * PrtList[particle].dynalightlevel;
//                    if ( level > light )
//                    {
//                        if ( level > 255 ) level = 255;
//
//                        PMesh->mem.vrt_l[vertex] = level;
//                        PMesh->mem.vrt_a[vertex] = level;
//                    }
//
//                    vertex++;
//                }
//            }
//        }
//    }
//}

//--------------------------------------------------------------------------------------------
void do_mpd_lighting( camera_t * pcam )
{
    // ZZ> This function does dynamic lighting of visible fans

    int cnt, tnc, vertex, fan, entry;
    float level;
    bool_t do_global = btrue;

    // refresh the dynamic light list
    make_dynalist( pcam );

    // Add to base light level in normal mode
    for ( entry = 0; entry < renderlist.all_count; entry++ )
    {
        float x0,y0,z0, dx,dy,dz;
        light_cache_t * cache;
        float local_lighting[6];
        int ix, iy;
        int vrtstart;

        fan = renderlist.all[entry];
        if ( INVALID_TILE == fan ) continue;

        vrtstart = PMesh->mem.tile_list[fan].vrtstart;

        ix = fan % PMesh->info.tiles_x;
        iy = fan / PMesh->info.tiles_x;

        x0 = ix * TILE_SIZE;
        y0 = iy * TILE_SIZE;
        z0 = PMesh->mem.vrt_z[ vrtstart ];

        cache = PMesh->mem.cache + fan;

        // blank the lighting
        for(tnc=0; tnc<6; tnc++)
        {
            local_lighting[tnc] = 0;
        };

        if ( gfx.shading != GL_FLAT )
        {
            vertex = PMesh->mem.tile_list[fan].vrtstart;

            // add in the dynamic lighting
            cnt = 0;
            for ( cnt = 0; cnt < dyna_list_count; cnt++ )
            {
                dx = dyna_list[cnt].x - x0;
                dy = dyna_list[cnt].y - y0;
                dz = dyna_list[cnt].z - z0;

                if( ABS(dx) + ABS(dy) + ABS(dz) == 0 )
                {
                    level = dyna_list[cnt].level / 2;

                    for(tnc=0; tnc<6; tnc++)
                    {
                        local_lighting[tnc] += level;
                    };
                }
                else
                {
                    float mag2 = dx*dx + dy*dy + dz*dz;

                    level = 255 - mag2 / dyna_list[cnt].falloff;
                    level = CLIP( level, 0, 255 );
                    level *= dyna_list[cnt].level;
                    if( (int)level > 0 )
                    {
                        float mag  = SQRT( mag2 );

                        dx /= mag;
                        dy /= mag;
                        dz /= mag;

                        if( dx > 0 )
                        {
                            local_lighting[0] += ABS(dx) * level;
                        }
                        else if (dx < 0)
                        {
                            local_lighting[1] += ABS(dx) * level;                            
                        }

                        if( dy > 0 )
                        {
                            local_lighting[2] += ABS(dy) * level;
                        }
                        else if (dy < 0)
                        {
                            local_lighting[3] += ABS(dy) * level;                            
                        }

                        if( dz > 0 )
                        {
                            local_lighting[4] += ABS(dz) * level;
                        }
                        else if (dz < 0)
                        {
                            local_lighting[5] += ABS(dz) * level;                            
                        }
                    }

                }
            }

            if( do_global )
            {
                // do global lighting
                if( light_x > 0 )
                {
                    local_lighting[0] += ABS(light_x) * light_a * 255;
                }
                else if (light_x < 0)
                {
                    local_lighting[1] += ABS(light_x) * light_a * 255;                            
                }

                if( light_y > 0 )
                {
                    local_lighting[2] += ABS(light_y) * light_a * 255;
                }
                else if (light_y < 0)
                {
                    local_lighting[3] += ABS(light_y) * light_a * 255;                            
                }

                if( light_z > 0 )
                {
                    local_lighting[4] += ABS(light_z) * light_a * 255;
                }
                else if (light_z < 0)
                {
                    local_lighting[5] += ABS(light_z) * light_a * 255;                            
                }
            }
        }

        // average this in with the existing lighting
        cache->max_light = 0;
        for( tnc=0; tnc<6; tnc++ )
        {
            cache->lighting[tnc] = cache->lighting[tnc] * 0.9f + local_lighting[tnc] * 0.1f;
            cache->max_light = MAX(cache->max_light, cache->lighting[tnc]);
        }
    }


}

//--------------------------------------------------------------------------------------------
void render_water()
{
    // ZZ> This function draws all of the water fans

    int cnt;

    // Set the transformation thing
    glLoadMatrixf( gCamera.mView.v );

    // Bottom layer first
    if ( gfx.draw_water_1 && waterlayerz[1] > -waterlayeramp[1] )
    {
        for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
        {
            if ( 0 != mesh_test_fx( PMesh, renderlist.all[cnt], MPDFX_WATER ) )
            {
                render_water_fan( renderlist.all[cnt], 1 );
            }
        }
    }

    // Top layer second
    if ( gfx.draw_water_0 && waterlayerz[0] > -waterlayeramp[0] )
    {
        for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
        {
            if ( 0 != mesh_test_fx( PMesh, renderlist.all[cnt], MPDFX_WATER ) )
            {
                render_water_fan( renderlist.all[cnt], 0 );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_init( camera_t * pcam )
{
    // Which tiles can be displayed
    make_renderlist( pcam );

    // determine which objects are visible
    dolist_make();
    dolist_sort( pcam );

    // figure out the terrain lighting
    do_mpd_lighting( pcam );

    // apply the lighting to the characters and particles
    light_fans();
    light_characters();
    light_particles();

    // update the particle instances
    update_all_prt_instance( pcam );
}

//--------------------------------------------------------------------------------------------
void draw_scene_mesh()
{
    int cnt, tnc;

    // BB> draw the mesh and any reflected objects

    //---------------------------------------------
    // draw all tiles that do not reflect characters
    glDisable( GL_BLEND );             // no transparency
    glDisable( GL_CULL_FACE );

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glEnable( GL_ALPHA_TEST );         // use alpha test to allow the thatched roof tiles to look like thatch
    glAlphaFunc( GL_GREATER, 0 );

    meshlasttexture = (Uint16)(~0);
    for ( cnt = 0; cnt < renderlist.ndr_count; cnt++ )
    {
        render_fan( renderlist.ndr[cnt] );
    }

    //--------------------------------
    // draw the reflective tiles and any reflected objects
    if ( gfx.refon )
    {
        //------------------------------
        // draw the reflective tiles, but turn off the depth buffer
        // this blanks out any background that might've been drawn

        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }

        //------------------------------
        // Render all reflected objects
        glEnable( GL_BLEND );
        glDepthMask( GL_TRUE );
        glDepthFunc( GL_LEQUAL );
        for ( cnt = dolist_count-1; cnt >= 0; cnt-- )
        {
            tnc = dolist[cnt].ichr;

            if( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
            {
                Uint32 itile;

                glEnable( GL_CULL_FACE );
                glFrontFace( GL_CCW );

                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                tnc = dolist[cnt].ichr;
                itile = ChrList[tnc].onwhichfan;

                if ( VALID_TILE(PMesh, itile) && (0 != mesh_test_fx( PMesh, itile, MPDFX_DRAWREF )) )
                {
                    render_one_mad_ref( tnc, 255 );
                }
            }
            else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT( dolist[cnt].iprt ) )
            {
                Uint32 itile;
                tnc = dolist[cnt].iprt;
                itile = PrtList[tnc].onwhichfan;

                glDisable( GL_CULL_FACE );

                // render_one_prt_ref() actually sets its own blend function, but just to be safe
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                if ( VALID_TILE(PMesh, itile) && (0 != mesh_test_fx( PMesh, itile, MPDFX_DRAWREF )) )
                {
                    render_one_prt_ref( tnc );
                }
            }
        }

        //------------------------------
        // Render the shadow floors ( let everything show through )
        // turn on the depth mask, so that no objects under the floor will show through
        // this assumes that the floor is not partially transparent...
        glDepthMask( GL_TRUE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glDisable( GL_CULL_FACE );

        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );

        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }
    }
    else
    {
        //------------------------------
        // Render the shadow floors as normal solid floors
        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }
    }

#if defined(RENDER_HMAP)
    //------------------------------
    // render the heighmap
    for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
    {
        render_hmap_fan( renderlist.all[cnt] );
    }
#endif

    //------------------------------
    // Render the shadows
    if ( gfx.shaon )
    {
        glDepthMask( GL_FALSE );
        glEnable( GL_DEPTH_TEST );

        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

        if ( gfx.shasprite )
        {
            // Bad shadows
            for ( cnt = 0; cnt < dolist_count; cnt++ )
            {
                tnc = dolist[cnt].ichr;
                if ( 0 == ChrList[tnc].shadowsize ) continue;

                render_bad_shadow( tnc );
            }
        }
        else
        {
            // Good shadows for me
            for ( cnt = 0; cnt < dolist_count; cnt++ )
            {
                tnc = dolist[cnt].ichr;
                if ( 0 == ChrList[tnc].shadowsize ) continue;

                render_shadow( tnc );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_solid()
{
    int cnt, tnc;

    // ------------------------------
    // Render all solid objects
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        glDepthMask( GL_TRUE );

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LEQUAL );

        glEnable( GL_ALPHA_TEST );
        glAlphaFunc( GL_GREATER, 0 );

        glDisable( GL_BLEND );

        if( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
        {
            tnc = dolist[cnt].ichr;

            glEnable( GL_CULL_FACE );
            glFrontFace( GL_CW );

            if ( ChrList[tnc].inst.alpha == 255 && ChrList[tnc].inst.light == 255 )
            {
                render_one_mad( tnc, 255 );
            }
        }
        else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT( dolist[cnt].iprt ) )
        {
			glDisable( GL_CULL_FACE );

            render_one_prt_solid( dolist[cnt].iprt );
        }
    }
}


//--------------------------------------------------------------------------------------------
void draw_scene_water()
{

    // set the the transparency parameters
    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );

    // And transparent water floors
    if ( !waterlight )
    {
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        render_water();
    }
    else
    {
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
        render_water();
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_trans()
{
    // BB > draw transparent objects

    int cnt;
    Uint8 trans;

    // set the the transparency parameters
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Now render all transparent and light objects
    for ( cnt = dolist_count-1; cnt >= 0; cnt-- )
    {
        if( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
        {
            Uint16  ichr = dolist[cnt].ichr;
            chr_t * pchr = ChrList + ichr;
            chr_instance_t * pinst = &(pchr->inst);

            glEnable( GL_CULL_FACE );
            glFrontFace( GL_CW );

            if (pinst->alpha != 255 && pinst->light == 255)
            {
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                trans = pinst->alpha;
                if ( trans < SEEINVISIBLE && ( local_seeinvisible || pchr->islocalplayer ) ) trans = SEEINVISIBLE;

                render_one_mad( ichr, trans );
            }

            if( pinst->light != 255 )
            {
                glEnable( GL_BLEND );
                glBlendFunc( GL_ONE, GL_ONE );

                trans = pinst->light == 255 ? 0 : pinst->light;

                if ( trans < SEEINVISIBLE && ( local_seeinvisible || pchr->islocalplayer ) )  trans = SEEINVISIBLE;

                render_one_mad( ichr, trans );
            }

            if ( gfx.phongon && pinst->sheen > 0 )
            {
                Uint16 trans;
                Uint16 save_texture, save_enviro;

                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );

                trans = pinst->sheen << 4;
                trans = CLIP(trans, 0, 255);

                save_texture = pinst->texture;
                save_enviro  = pinst->enviro;

                pinst->enviro  = btrue;
                pinst->texture = TX_PHONG;  // The phong map texture...

                render_one_mad( ichr, trans );

                pinst->texture = save_texture;
                pinst->enviro  = save_enviro;
            }
        }
        else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT( dolist[cnt].iprt ) )
        {
            render_one_prt_trans( dolist[cnt].iprt ); 
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_zreflection( camera_t * pcam )
{
    // ZZ> This function draws 3D objects

    if( NULL == pcam ) pcam = &gCamera;

    draw_scene_init( pcam );

    // do the render pass for the mesh
    draw_scene_mesh();

    // do the render pass for solid objects
    draw_scene_solid();

    // draw the water
    draw_scene_water();

    // do the render pass for transparent objects
    draw_scene_trans();
}

//--------------------------------------------------------------------------------------------
void draw_blip( float sizeFactor, Uint8 color, int x, int y )

{
    float xl, xr, yt, yb;
    int width, height;

    // ZZ> This function draws a blip
    if ( x > 0 && y > 0 )
    {
        EnableTexturing();
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glNormal3f( 0.0f, 0.0f, 1.0f );

        GLXtexture_Bind( &TxBlip );

        xl = ( float )bliprect[color].left   / (float)GLXtexture_GetTextureWidth( &TxBlip );
        xr = ( float )bliprect[color].right  / (float)GLXtexture_GetTextureWidth( &TxBlip );
        yt = ( float )bliprect[color].top    / (float)GLXtexture_GetTextureHeight( &TxBlip );
        yb = ( float )bliprect[color].bottom / (float)GLXtexture_GetTextureHeight( &TxBlip );
        width = bliprect[color].right - bliprect[color].left;
        height = bliprect[color].bottom - bliprect[color].top;

        width *= sizeFactor; height *= sizeFactor;
        glBegin( GL_QUADS );
        {
            glTexCoord2f( xl, yb ); glVertex2i( x - 1 - (width / 2), y + 1 + (height / 2) );
            glTexCoord2f( xr, yb ); glVertex2i( x - 1 + (width / 2), y + 1 + (height / 2) );
            glTexCoord2f( xr, yt ); glVertex2i( x - 1 + (width / 2), y + 1 - (height / 2) );
            glTexCoord2f( xl, yt ); glVertex2i( x - 1 - (width / 2), y + 1 - (height / 2) );
        }
        glEnd();

    }
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle )
{
    // ZZ> This function draws an icon
    int position, blipx, blipy;
    float xl, xr, yt, yb;
    int width, height;
    if ( icontype >= 0 && icontype < MAX_ICON )
    {
        EnableTexturing();    // Enable texture mapping
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        GLXtexture_Bind( TxIcon + icontype );

        xl = ( ( float )iconrect.left ) / 32;
        xr = ( ( float )iconrect.right ) / 32;
        yt = ( ( float )iconrect.top ) / 32;
        yb = ( ( float )iconrect.bottom ) / 32;
        width = iconrect.right - iconrect.left; height = iconrect.bottom - iconrect.top;
        glBegin( GL_QUADS );
        {
            glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
            glTexCoord2f( xl, yt );   glVertex2i( x,         y );
        }
        glEnd();
    }
    if ( sparkle != NOSPARKLE )
    {
        position = update_wld & 31;
        position = ( SPARKLESIZE * position >> 5 );

        blipx = x + SPARKLEADD + position;
        blipy = y + SPARKLEADD;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = x + SPARKLEADD + SPARKLESIZE;
        blipy = y + SPARKLEADD + position;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = blipx - position;
        blipy = y + SPARKLEADD + SPARKLESIZE;
        draw_blip(0.5f, sparkle, blipx, blipy );

        blipx = x + SPARKLEADD;
        blipy = blipy - position;
        draw_blip(0.5f, sparkle, blipx, blipy );
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_font( int fonttype, int x, int y )
{
    // ZZ> This function draws a letter or number
    // GAC> Very nasty version for starters.  Lots of room for improvement.
    GLfloat dx, dy, fx1, fx2, fy1, fy2, border;
    GLuint x2, y2;

    y  += fontoffset;
    x2  = x + fontrect[fonttype].w;
    y2  = y - fontrect[fonttype].h;

    dx = 2.0f / 512.0f;
    dy = 1.0f / 256.0f;
    border = 1.0f / 512.0f;

    fx1 = fontrect[fonttype].x * dx + border;
    fx2 = ( fontrect[fonttype].x + fontrect[fonttype].w ) * dx - border;
    fy1 = fontrect[fonttype].y * dy + border;
    fy2 = ( fontrect[fonttype].y + fontrect[fonttype].h ) * dy - border;

    glBegin( GL_QUADS );
    {
        glTexCoord2f( fx1, fy2 );   glVertex2i( x, y );
        glTexCoord2f( fx2, fy2 );   glVertex2i( x2, y );
        glTexCoord2f( fx2, fy1 );   glVertex2i( x2, y2 );
        glTexCoord2f( fx1, fy1 );   glVertex2i( x, y2 );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_map( int x, int y )
{
    // ZZ> This function draws the map
    EnableTexturing();
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    GLXtexture_Bind( &TxMap );

    glBegin( GL_QUADS );
    {
        glTexCoord2f ( 0.0f, 1.0f ); glVertex2i ( x,           y + MAPSIZE );
        glTexCoord2f ( 1.0f, 1.0f ); glVertex2i ( x + MAPSIZE, y + MAPSIZE );
        glTexCoord2f ( 1.0f, 0.0f ); glVertex2i ( x + MAPSIZE, y );
        glTexCoord2f ( 0.0f, 0.0f ); glVertex2i ( x,           y );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks )
{
    // ZZ> This function draws a bar and returns the y position for the next one
    int noticks;
    float xl, xr, yt, yb;
    int width, height;

    EnableTexturing();               // Enable texture mapping
    glColor4f( 1, 1, 1, 1 );
    if ( maxticks > 0 && ticks >= 0 )
    {
        // Draw the tab
        GLXtexture_Bind( &TxBars );

        xl = ( ( float )tabrect[bartype].left ) / 128;
        xr = ( ( float )tabrect[bartype].right ) / 128;
        yt = ( ( float )tabrect[bartype].top ) / 128;
        yb = ( ( float )tabrect[bartype].bottom ) / 128;
        width = tabrect[bartype].right - tabrect[bartype].left; height = tabrect[bartype].bottom - tabrect[bartype].top;
        glBegin( GL_QUADS );
        {
            glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
            glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
            glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
            glTexCoord2f( xl, yt );   glVertex2i( x,         y );
        }
        glEnd();

        // Error check
        if ( maxticks > MAXTICK ) maxticks = MAXTICK;
        if ( ticks > maxticks ) ticks = maxticks;

        // Draw the full rows of ticks
        x += TABX;

        while ( ticks >= NUMTICK )
        {
            barrect[bartype].right = BARX;
            GLXtexture_Bind( &TxBars );

            xl = ( ( float )barrect[bartype].left ) / 128;
            xr = ( ( float )barrect[bartype].right ) / 128;
            yt = ( ( float )barrect[bartype].top ) / 128;
            yb = ( ( float )barrect[bartype].bottom ) / 128;
            width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
            glBegin( GL_QUADS );
            {
                glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
                glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
                glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
                glTexCoord2f( xl, yt );   glVertex2i( x,         y );
            }
            glEnd();
            y += BARY;
            ticks -= NUMTICK;
            maxticks -= NUMTICK;
        }

        // Draw any partial rows of ticks
        if ( maxticks > 0 )
        {
            // Draw the filled ones
            barrect[bartype].right = ( ticks << 3 ) + TABX;
            GLXtexture_Bind( &TxBars );

            xl = ( ( float )barrect[bartype].left ) / 128;
            xr = ( ( float )barrect[bartype].right ) / 128;
            yt = ( ( float )barrect[bartype].top ) / 128;
            yb = ( ( float )barrect[bartype].bottom ) / 128;
            width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
            glBegin( GL_QUADS );
            {
                glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
                glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
                glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
                glTexCoord2f( xl, yt );   glVertex2i( x,         y );
            }
            glEnd();

            // Draw the empty ones
            noticks = maxticks - ticks;
            if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );

            barrect[0].right = ( noticks << 3 ) + TABX;
            GLXtexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            {
                glTexCoord2f( xl, yb );   glVertex2i( ( ticks << 3 ) + x,         y + height );
                glTexCoord2f( xr, yb );   glVertex2i( ( ticks << 3 ) + x + width, y + height );
                glTexCoord2f( xr, yt );   glVertex2i( ( ticks << 3 ) + x + width, y );
                glTexCoord2f( xl, yt );   glVertex2i( ( ticks << 3 ) + x,         y );
            }
            glEnd();
            maxticks -= NUMTICK;
            y += BARY;
        }

        // Draw full rows of empty ticks
        while ( maxticks >= NUMTICK )
        {
            barrect[0].right = BARX;
            GLXtexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            {
                glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
                glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
                glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
                glTexCoord2f( xl, yt );   glVertex2i( x,         y );
            }
            glEnd();
            y += BARY;
            maxticks -= NUMTICK;
        }

        // Draw the last of the empty ones
        if ( maxticks > 0 )
        {
            barrect[0].right = ( maxticks << 3 ) + TABX;
            GLXtexture_Bind( &TxBars );

            xl = ( ( float )barrect[0].left ) / 128;
            xr = ( ( float )barrect[0].right ) / 128;
            yt = ( ( float )barrect[0].top ) / 128;
            yb = ( ( float )barrect[0].bottom ) / 128;
            width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
            glBegin( GL_QUADS );
            {
                glTexCoord2f( xl, yb );   glVertex2i( x,         y + height );
                glTexCoord2f( xr, yb );   glVertex2i( x + width, y + height );
                glTexCoord2f( xr, yt );   glVertex2i( x + width, y );
                glTexCoord2f( xl, yt );   glVertex2i( x,         y );
            }
            glEnd();
            y += BARY;
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void BeginText()
{
    EnableTexturing();    // Enable texture mapping

    GLXtexture_Bind( &TxFont );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );

    glColor4f( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void EndText()
{
    glDisable( GL_BLEND );
    glDisable( GL_ALPHA_TEST );
}

//--------------------------------------------------------------------------------------------
void draw_string( const char *szText, int x, int y )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer
    Uint8 cTmp = szText[0];
    int cnt = 1;
    int x_stt;

    BeginText();

    x_stt = x;
    cnt = 0;
    cTmp = szText[cnt];
    while ( '\0' != cTmp )
    {
        // Convert ASCII to our own little font
        if ( cTmp == '~' )
        {
            // Use squiggle for tab
            x = (x + TABAND) & ( ~TABAND );
        }
        else if ( cTmp == '\n' )
        {
            x  = x_stt;
            y += fontyspacing;
        }
        else
        {
            // Normal letter
            cTmp = asciitofont[cTmp];
            draw_one_font( cTmp, x, y );
            x += fontxspacing[cTmp];
        }

        cnt++;
        cTmp = szText[cnt];
    }

    EndText();
}

//--------------------------------------------------------------------------------------------
int length_of_word( const char *szText )
{
    // ZZ> This function returns the number of pixels the
    //     next word will take on screen in the x direction

    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    Uint8 cTmp = szText[cnt];

    while ( cTmp == ' ' || cTmp == '~' || cTmp == '\n' )
    {
        if ( cTmp == ' ' )
        {
            x += fontxspacing[asciitofont[cTmp]];
        }
        else if ( cTmp == '~' )
        {
            x += TABAND + 1;
        }

        cnt++;
        cTmp = szText[cnt];
    }

    while ( cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0 )
    {
        x += fontxspacing[asciitofont[cTmp]];
        cnt++;
        cTmp = szText[cnt];
    }

    return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( const char *szText, int x, int y, int maxx )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer,
    //     wrapping over the right side and returning the new y value
    int stt_x = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = btrue;
    int cnt = 1;

    BeginText();

    maxx = maxx + stt_x;

    while ( cTmp != 0 )
    {
        // Check each new word for wrapping
        if ( newword )
        {
            int endx = x + length_of_word( szText + cnt - 1 );

            newword = bfalse;
            if ( endx > maxx )
            {
                // Wrap the end and cut off spaces and tabs
                x = stt_x + fontyspacing;
                y += fontyspacing;
                newy += fontyspacing;

                while ( cTmp == ' ' || cTmp == '~' )
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            if ( cTmp == '~' )
            {
                // Use squiggle for tab
                x = x & ( ~TABAND );
                x += TABAND + 1;
            }
            else if ( cTmp == '\n' )
            {
                x = stt_x;
                y += fontyspacing;
                newy += fontyspacing;
            }
            else
            {
                // Normal letter
                cTmp = asciitofont[cTmp];
                draw_one_font( cTmp, x, y );
                x += fontxspacing[cTmp];
            }

            cTmp = szText[cnt];
            if ( cTmp == '~' || cTmp == ' ' )
            {
                newword = btrue;
            }

            cnt++;
        }
    }

    EndText();
    return newy;
}

//--------------------------------------------------------------------------------------------
int draw_status( Uint16 character, int x, int y )
{
    // ZZ> This function shows a character's icon, status and inventory
    //     The x,y coordinates are the top left point of the image to draw
    Uint16 item;
    char cTmp;
    char *readtext;
    STRING generictext;

    int life     = FP8_TO_INT( ChrList[character].life    );
    int lifemax  = FP8_TO_INT( ChrList[character].lifemax );
    int mana     = FP8_TO_INT( ChrList[character].mana    );
    int manamax  = FP8_TO_INT( ChrList[character].manamax );
    int cnt = lifemax;

    // Write the character's first name
    if ( ChrList[character].nameknown )
        readtext = ChrList[character].name;
    else
        readtext = CapList[ChrList[character].model].classname;

    for ( cnt = 0; cnt < 6; cnt++ )
    {
        cTmp = readtext[cnt];
        if ( cTmp == ' ' || cTmp == '\0' )
        {
            generictext[cnt] = '\0';
            break;
        }
        else
            generictext[cnt] = cTmp;
    }

    generictext[6] = '\0';
    draw_string( generictext, x + 8, y ); y += fontyspacing;

    // Write the character's money
    sprintf( generictext, "$%4d", ChrList[character].money );
    draw_string( generictext, x + 8, y ); y += fontyspacing + 8;

    // Draw the icons
    draw_one_icon( skintoicon[ChrList[character].inst.texture], x + 40, y, ChrList[character].sparkle );

    item = ChrList[character].holdingwhich[SLOT_LEFT];
    if ( item != MAX_CHR )
    {
        if ( ChrList[item].icon )
        {
            draw_one_icon( skintoicon[ChrList[item].inst.texture], x + 8, y, ChrList[item].sparkle );
            if ( ChrList[item].ammomax != 0 && ChrList[item].ammoknown )
            {
                if ( !CapList[ChrList[item].model].isstackable || ChrList[item].ammo > 1 )
                {
                    // Show amount of ammo left
                    sprintf( generictext, "%2d", ChrList[item].ammo );
                    draw_string( generictext, x + 8, y - 8 );
                }
            }
        }
        else if ( bookicon_count > 0 )
        {
            Uint16 icon = ChrList[item].money;
            if (icon > bookicon_count) icon = bookicon_count;
            draw_one_icon( bookicon[ icon ], x + 8, y, ChrList[item].sparkle );
        }
        else
        {
            draw_one_icon( nullicon, x + 8, y, ChrList[item].sparkle );
        }
    }
    else
    {
        draw_one_icon( nullicon, x + 8, y, NOSPARKLE );
    }

    item = ChrList[character].holdingwhich[SLOT_RIGHT];
    if ( item != MAX_CHR )
    {
        if ( ChrList[item].icon )
        {
            draw_one_icon( skintoicon[ChrList[item].inst.texture], x + 72, y, ChrList[item].sparkle );
            if ( ChrList[item].ammomax != 0 && ChrList[item].ammoknown )
            {
                if ( !CapList[ChrList[item].model].isstackable || ChrList[item].ammo > 1 )
                {
                    // Show amount of ammo left
                    sprintf( generictext, "%2d", ChrList[item].ammo );
                    draw_string( generictext, x + 72, y - 8 );
                }
            }
        }
        else if ( bookicon_count > 0 )
        {
            Uint16 icon = ChrList[item].money;
            if (icon > bookicon_count) icon = bookicon_count;
            draw_one_icon( bookicon[ icon ], x + 72, y, ChrList[item].sparkle );
        }
        else
        {
            draw_one_icon( nullicon, x + 72, y, ChrList[item].sparkle );
        }
    }
    else
    {
        draw_one_icon( nullicon, x + 72, y, NOSPARKLE );
    }

    y += 32;

    // Draw the bars
    if ( ChrList[character].alive )
        y = draw_one_bar( ChrList[character].lifecolor, x, y, life, lifemax );
    else
        y = draw_one_bar( 0, x, y, 0, lifemax );  // Draw a black bar

    y = draw_one_bar( ChrList[character].manacolor, x, y, mana, manamax );
    return y;
}

//--------------------------------------------------------------------------------------------
void draw_text()
{
    // ZZ> This function spits out some words
    char text[512];
    int y, cnt, tnc, fifties, seconds, minutes;

    Begin2DMode();
    // Status bars
    y = 0;
    if ( staton )
    {
        for ( cnt = 0; cnt < numstat && y < sdl_scr.y; cnt++ )
        {
            y = draw_status( statlist[cnt], sdl_scr.x - BARX, y );
        }
    }

    // Map display
    if ( mapvalid && mapon )
    {
        draw_map( 0, sdl_scr.y - MAPSIZE );

        //If one of the players can sense enemies via EMP, draw them as blips on the map
        if ( MAX_CHR != local_senseenemies )
        {
            Uint16 iTmp;

            for ( iTmp = 0; numblip < MAXBLIP && iTmp < MAX_CHR; iTmp++ )
            {
                Uint16 icap;

                if( !ChrList[iTmp].on ) continue;

                icap = ChrList[iTmp].model;
                if( !VALID_CAP(icap) ) continue;

                //Show only hated team
                if ( TeamList[ChrList[local_senseenemies].team].hatesteam[ChrList[iTmp].team] )
                {
                    // Only if they match the required IDSZ ([NONE] always works)
                    if ( local_senseenemiesID == Make_IDSZ("NONE")
                            || CapList[icap].idsz[IDSZ_PARENT] == local_senseenemiesID
                            || CapList[icap].idsz[IDSZ_TYPE] == local_senseenemiesID)
                    {
                        //Inside the map?
                        if ( ChrList[iTmp].pos.x < PMesh->info.edge_x && ChrList[iTmp].pos.y < PMesh->info.edge_y )
                        {
                            //Valid colors only
                            blipx[numblip] = ChrList[iTmp].pos.x * MAPSIZE / PMesh->info.edge_x;
                            blipy[numblip] = ChrList[iTmp].pos.y * MAPSIZE / PMesh->info.edge_y;
                            blipc[numblip] = 0; //Red blips
                            numblip++;
                        }
                    }
                }
            }
        }

        for ( cnt = 0; cnt < numblip; cnt++ )
        {
            draw_blip(0.75f, blipc[cnt], blipx[cnt], blipy[cnt] + sdl_scr.y - MAPSIZE );
        }

        if ( youarehereon && ( update_wld&8 ) )
        {
            for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
            {
                if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
                {
                    tnc = PlaList[cnt].index;
                    if ( ChrList[tnc].alive )
                    {
                        draw_blip( 0.75f, 0, ChrList[tnc].pos.x*MAPSIZE / PMesh->info.edge_x, ( ChrList[tnc].pos.y*MAPSIZE / PMesh->info.edge_y ) + sdl_scr.y - MAPSIZE );
                    }
                }
            }
        }
    }

    // FPS text
    y = 0;
    if ( outofsync )
    {
        sprintf( text, "OUT OF SYNC" );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( parseerror )
    {
        sprintf( text, "SCRIPT ERROR ( SEE LOG.TXT )" );
        draw_string( text, 0, y );
        y += fontyspacing;
    }
    if ( fpson )
    {
        draw_string( szfpstext, 0, y );
        y += fontyspacing;
    }
    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
        // In-Game help
        sprintf( text, "!!!MOUSE HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Go to input settings to change" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Left Click to use an item" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Left and Right Click to grab" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Middle Click to jump" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  A and S keys do stuff" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Right Drag to move camera" );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
        // In-Game help
        sprintf( text, "!!!JOYSTICK HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Go to input settings to change." );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Hit the buttons" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  You'll figure it out" );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
        // In-Game help
        sprintf( text, "!!!KEYBOARD HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Go to input settings to change." );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  TGB control one hand" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  YHN control the other" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Keypad to move and jump" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  Number keys for stats" );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F5 ) )
    {
        // Debug information
        sprintf( text, "!!!DEBUG MODE-5!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  CAM %f %f %f", gCamera.pos.x, gCamera.pos.y, gCamera.pos.z );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = PlaList[0].index;
        sprintf( text, "  PLA0DEF %d %d %d %d %d %d %d %d",
                 ChrList[tnc].damagemodifier[0]&3,
                 ChrList[tnc].damagemodifier[1]&3,
                 ChrList[tnc].damagemodifier[2]&3,
                 ChrList[tnc].damagemodifier[3]&3,
                 ChrList[tnc].damagemodifier[4]&3,
                 ChrList[tnc].damagemodifier[5]&3,
                 ChrList[tnc].damagemodifier[6]&3,
                 ChrList[tnc].damagemodifier[7]&3 );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = PlaList[0].index;
        sprintf( text, "  PLA0 %5.1f %5.1f", ChrList[tnc].pos.x / 128.0f, ChrList[tnc].pos.y / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = PlaList[1].index;
        sprintf( text, "  PLA1 %5.1f %5.1f", ChrList[tnc].pos.x / 128.0f, ChrList[tnc].pos.y / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( cfg.dev_mode &&  SDLKEYDOWN( SDLK_F6 ) )
    {
        // More debug information
        sprintf( text, "!!!DEBUG MODE-6!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FREEPRT %d", prt_count_free() );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FREECHR %d", chr_count_free() );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  MACHINE %d", local_machine );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  EXPORT %d", exportvalid );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  FOGAFF %d", fogaffectswater );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  PASS %d/%d", numshoppassage, numpassage );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  NETPLAYERS %d", numplayer );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  DAMAGEPART %d", damagetileparttype );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) )
    {
        // White debug mode
        sprintf( text, "!!!DEBUG MODE-7!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", gCamera.mView.CNV( 0, 0 ), gCamera.mView.CNV( 1, 0 ), gCamera.mView.CNV( 2, 0 ), gCamera.mView.CNV( 3, 0 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", gCamera.mView.CNV( 0, 1 ), gCamera.mView.CNV( 1, 1 ), gCamera.mView.CNV( 2, 1 ), gCamera.mView.CNV( 3, 1 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", gCamera.mView.CNV( 0, 2 ), gCamera.mView.CNV( 1, 2 ), gCamera.mView.CNV( 2, 2 ), gCamera.mView.CNV( 3, 2 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", gCamera.mView.CNV( 0, 3 ), gCamera.mView.CNV( 1, 3 ), gCamera.mView.CNV( 2, 3 ), gCamera.mView.CNV( 3, 3 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "x %f", gCamera.center.x );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "y %f", gCamera.center.y );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "turn %d %d", gCamera.turn_mode, gCamera.turn_time );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    // Draw paused text
    /*if ( gamepaused && !SDLKEYDOWN( SDLK_F11 ) )
    {
        sprintf( text, "GAME PAUSED" );
        draw_string( text, -90 + sdl_scr.x / 2, 0 + sdl_scr.y / 2 );
    }*/

    // Pressed panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
        gameactive = bfalse;
        moduleactive = bfalse;
    }
    if ( timeron )
    {
        fifties = ( timervalue % 50 ) << 1;
        seconds = ( ( timervalue / 50 ) % 60 );
        minutes = ( timervalue / 3000 );
        sprintf( text, "=%d:%02d:%02d=", minutes, seconds, fifties );
        draw_string( text, 0, y );
        y += fontyspacing;
    }
    if ( waitingforplayers )
    {
        sprintf( text, "Waiting for players... " );
        draw_string( text, 0, y );
        y += fontyspacing;
    }
    if ( local_allpladead || respawnanytime )
    {
        if ( respawnvalid && cfg.difficulty < GAME_HARD )
        {
            draw_string( "PRESS SPACE TO RESPAWN", 0, y );
            y += fontyspacing;
        }
        else
        {
            draw_string( "PRESS ESCAPE TO QUIT", 0, y );
            y += fontyspacing;
        }
    }
    else if ( beatmodule )
    {
        sprintf( text, "VICTORY!  PRESS ESCAPE" );
        draw_string( text, 0, y );
        y += fontyspacing;
    }

    // Network message input
    if ( console_mode )
    {
        char buffer[KEYB_BUFFER_SIZE + 128];

        snprintf( buffer, sizeof(buffer), "%s > %s%s", cfg.network_messagename, keyb.buffer, (0 == (update_wld & 8)) ? "x" : "+" );

        y = draw_wrap_string( buffer, 0, y, sdl_scr.x - wraptolerance );
    }

    // Messages
    if ( messageon )
    {
        // Display the messages
        tnc = msgstart;

        for ( cnt = 0; cnt < maxmessage; cnt++ )
        {
            if ( msgtime[tnc] > 0 )
            {
                y = draw_wrap_string( msgtextdisplay[tnc], 0, y, sdl_scr.x - wraptolerance );
                if (msgtime[tnc] > msgtimechange)
                {
                    msgtime[tnc] -= msgtimechange;
                }
                else
                {
                    msgtime[tnc] = 0;
                }
            }

            tnc++;
            tnc = tnc % maxmessage;
        }

        msgtimechange = 0;
    }

    End2DMode();
}

//--------------------------------------------------------------------------------------------
void do_flip_pages()
{
    if ( gfx_page_flip_requested )
    {
        gfx_page_flip_requested = bfalse;
        flip_pages();
    }
}

//--------------------------------------------------------------------------------------------
void request_flip_pages()
{
    gfx_page_flip_requested = btrue;
}

//--------------------------------------------------------------------------------------------
void flip_pages()
{
    glFlush();

    egoboo_console_draw_all();

    frame_all++;
    frame_fps++;

    SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void draw_scene( camera_t * pcam )
{
    Begin3DMode( pcam );
    {
        // clear the depth buffer
        glDepthMask( GL_TRUE );
        glClear( GL_DEPTH_BUFFER_BIT );

        // Clear the image if need be
        if ( gfx.clearson )
        {
            glClear( GL_COLOR_BUFFER_BIT );
        }

        if( gfx.draw_background )
        {
            // Render the background
            render_background( TX_WATER_LOW );  // TX_WATER_LOW for waterlow.bmp
        }

        draw_scene_zreflection( pcam );

        // Foreground overlay
        if ( gfx.draw_overlay )
        {
            render_foreground_overlay( TX_WATER_TOP );  // TX_WATER_TOP is watertop.bmp
        }
    }
    End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_main()
{
    // ZZ> This function does all the drawing stuff

    draw_scene( &gCamera );
    draw_text();

    request_flip_pages();
}

//--------------------------------------------------------------------------------------------
Uint32 load_one_title_image( const char *szLoadName )
{
    // ZZ> This function loads a title in the specified image slot, forcing it into
    //     system memory.  Returns btrue if it worked
    Uint32 index;

    index = (Uint32)(~0);
    if ( INVALID_TX_ID != ego_texture_load( TxTitleImage + TxTitleImage_count, szLoadName, INVALID_KEY ) )
    {
        index = TxTitleImage_count;
        TxTitleImage_count++;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
bool_t load_blip_bitmap()
{
    // This function loads the blip bitmaps
    if ( INVALID_TX_ID == ego_texture_load( &TxBlip, "basicdat" SLASH_STR "blip", INVALID_KEY ) )
    {
        log_warning( "Blip bitmap not loaded! (\"%s\")\n", "basicdat" SLASH_STR "blip" );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//void draw_titleimage( int image, int x, int y )
//{
//  // ZZ> This function draws a title image on the backbuffer
//  GLfloat  txWidth, txHeight;
//
//  if ( INVALID_TX_ID != GLXtexture_GetTextureID( TxTitleImage + image ) )
//  {
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 );  // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLXtexture_GetImageWidth( TxTitleImage + image ) / GLXtexture_GetDimensions( TxTitleImage + image ) );
//    txHeight = ( GLfloat )( GLXtexture_GetImageHeight( TxTitleImage + image ) / GLXtexture_GetDimensions( TxTitleImage + image ) );
//
//    /* Bind the texture */
//    GLXtexture_Bind( TxTitleImage + image );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    {
//        glTexCoord2f( 0, 1 );  glVertex2f( x, y + GLXtexture_GetImageHeight( TxTitleImage + image ) );
//        glTexCoord2f( txWidth, 1 );  glVertex2f( x + GLXtexture_GetImageWidth( TxTitleImage + image ), y + GLXtexture_GetImageHeight( TxTitleImage + image ) );
//        glTexCoord2f( txWidth, 1 - txHeight );  glVertex2f( x + GLXtexture_GetImageWidth( TxTitleImage + image ), y );
//        glTexCoord2f( 0, 1 - txHeight );  glVertex2f( x, y );
//    }
//    glEnd();
//
//    End2DMode();
//  }
//}

//--------------------------------------------------------------------------------------------
void do_cursor()
{
    // This function implements a mouse cursor
    input_read();

    cursorx = mous.x;  if ( cursorx < 6 )  cursorx = 6;  if ( cursorx > sdl_scr.x - 16 )  cursorx = sdl_scr.x - 16;

    cursory = mous.y;  if ( cursory < 8 )  cursory = 8;  if ( cursory > sdl_scr.y - 24 )  cursory = sdl_scr.y - 24;

    clicked = bfalse;
    if ( mous.button[0] && !pressed )
    {
        clicked = btrue;
    }

    pressed = mous.button[0];
    BeginText();  // Needed to setup text mode
    // draw_one_font(11, cursorx-5, cursory-7);
    draw_one_font( 95, cursorx - 5, cursory - 7 );
    EndText();    // Needed when done with text mode
}

/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
    glViewport( 0, 0, w, h );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int ogl_init()
{
    if( !_sdl_initialized_graphics )
    {
        sdlinit_graphics();
    }

    //SDL_GL_set_gl_mode(&ogl_vparam);

    // glClear() stuff
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set the background black
    glClearDepth( 1.0 );

    // depth buffer stuff
    glClearDepth( 1.0f );
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // alpha stuff
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    // backface culling
    //glEnable(GL_CULL_FACE);
    //glFrontFace(GL_CW); // TODO: This prevents the mesh from getting rendered
    //glCullFace(GL_BACK);

    // disable OpenGL lighting
    glDisable( GL_LIGHTING );

    // fill mode
    glPolygonMode( GL_FRONT, GL_FILL );
    glPolygonMode( GL_BACK,  GL_FILL );

    // ?Need this for color + lighting?
    glEnable( GL_COLOR_MATERIAL );  // Need this for color + lighting

    // set up environment mapping
    glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
    glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

    // Load the current graphical settings
    //load_graphics();

    _ogl_initialized = btrue;

    return _ogl_initialized && _sdl_initialized_base && _sdl_initialized_graphics;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int sdl_init()
{
    sdlinit_base();
    sdlinit_graphics();

    input_init();

#if defined(USE_LUA_CONSOLE)
    {
        SDL_Rect blah = {0, 0, scrx, scry / 4};
        lua_console_new(NULL, blah);
    };
#endif

    return _sdl_initialized_base && _sdl_initialized_graphics;
}

//---------------------------------------------------------------------------------------------
void sdlinit_base()
{
    log_info ( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init(0) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    if ( !_sdl_atexit_registered )
    {
        atexit( SDL_Quit );
        _sdl_atexit_registered = bfalse;
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    _sdl_initialized_base = btrue;
}

//---------------------------------------------------------------------------------------------
void sdlinit_graphics()
{
    if( !_sdl_initialized_base )
    {
        sdlinit_base();
    }

    log_info( "Intializing SDL Video... " );
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

#ifndef __APPLE__
    {
        SDL_Surface *theSurface;

        /* Setup the cute windows manager icon */
        theSurface = IMG_Load( "basicdat" SLASH_STR "icon.bmp" );
        if ( theSurface == NULL )
        {
            log_error( "Unable to load icon (basicdat" SLASH_STR "icon.bmp)\n" );
        }

        SDL_WM_SetIcon( theSurface, NULL );
    }
#endif

    // Set the window name
    SDL_WM_SetCaption( "Egoboo " VERSION, "Egoboo" );

#ifdef __unix__

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // "Unable to set video mode: Couldn't find matching GLX visual"
    if ( cfg.scrd_req == 32 ) cfg.scrd_req = 24;

#endif

    // the flags to pass to SDL_SetVideoMode
    sdl_vparam.width                     = cfg.scrx_req;
    sdl_vparam.height                    = cfg.scry_req;
    sdl_vparam.depth                     = cfg.scrd_req;

    sdl_vparam.flags.opengl              = SDL_TRUE;
    sdl_vparam.flags.double_buf          = SDL_TRUE;
    sdl_vparam.flags.full_screen         = cfg.fullscreen_req;

    sdl_vparam.gl_att.depth_size         = cfg.scrz_req;
    sdl_vparam.gl_att.multi_buffers      = (cfg.multisamples > 1) ? 1 : 0;
    sdl_vparam.gl_att.multi_samples      = cfg.multisamples;
    sdl_vparam.gl_att.accelerated_visual = GL_FALSE;

    ogl_vparam.dither         = GL_FALSE;
    ogl_vparam.antialiasing   = GL_TRUE;
    ogl_vparam.perspective    = GL_FASTEST;
    ogl_vparam.shading        = GL_SMOOTH;
    ogl_vparam.userAnisotropy = MAX(0, cfg.texturefilter_req - TX_TRILINEAR_2);

    log_info("Opening SDL Video Mode... ");
    if ( NULL == SDL_GL_set_mode(NULL, &sdl_vparam, &ogl_vparam) )
    {
        log_message( "Failed!\n" );
        log_info( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
        exit(-1);
    }
    else
    {
        log_message( "Success!\n" );
    }

    _sdl_initialized_graphics = btrue;


//    /* Set the OpenGL Attributes */
//#ifndef __unix__
//    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   colordepth );
//    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth  );
//    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
//    if ( cfg.scrd_req > colordepth * 3)
//    {
//        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, cfg.scrd_req - colordepth * 3 );
//    }
//#endif
//
//	//Some bug causes non 32-bit to crash on windows, so send them a warning
//#ifdef WIN32
//	if (cfg.scrd_req != 32) log_warning( "Color depth is not 32! This can cause the game to crash upon startup. See setup.txt\n" );
//#endif
//
//    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, btrue );
//    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, cfg.scrz_req );
//
//    //Check if antialiasing is enabled
//    if ( gfx.antialiasing && gfx.multisamples > 1 )
//    {
//		Sint8 success = 0;
//        success += SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, GL_TRUE);
//		success += SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gfx.multisamples);
//        if(success != 0) log_message("Could not set X%i antialiasing. (Not supported by your gfx card?)\n");
//    }
}



//---------------------------------------------------------------------------------------------
//void sdlinit()
//{
//    int     colordepth;
//
//    log_info ( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
//    if ( SDL_Init(0) < 0 )
//    {
//        log_message( "Failure!\n" );
//        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Success!\n" );
//    }
//
//    if ( !_sdl_atexit_registered )
//    {
//        atexit( SDL_Quit );
//        _sdl_atexit_registered = bfalse;
//    }
//
//    log_info( "Intializing SDL Video... " );
//    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Succeess!\n" );
//    }
//
//    log_info( "Intializing SDL Timing Services... " );
//    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Succeess!\n" );
//    }
//
//    log_info( "Intializing SDL Event Threading... " );
//    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Succeess!\n" );
//    }
//
//#ifndef __APPLE__
//    {
//        SDL_Surface *theSurface;
//
//        /* Setup the cute windows manager icon */
//        theSurface = IMG_Load( "basicdat" SLASH_STR "icon.bmp" );
//        if ( theSurface == NULL )
//        {
//            log_error( "Unable to load icon (basicdat" SLASH_STR "icon.bmp)\n" );
//        }
//
//        SDL_WM_SetIcon( theSurface, NULL );
//    }
//#endif
//
//#ifdef __unix__
//
//    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
//    // will cause SDL_SetVideoMode to fail with:
//    // Unable to set video mode: Couldn't find matching GLX visual
//    if ( scrd == 32 ) scrd = 24;
//
//#endif
//
//    // the flags to pass to SDL_SetVideoMode
//    sdl_vparam.flags          = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL;
//    sdl_vparam.opengl         = SDL_TRUE;
//    sdl_vparam.doublebuffer   = SDL_TRUE;
//    sdl_vparam.glacceleration = GL_FALSE;
//    sdl_vparam.width          = cfg.scrx_req;
//    sdl_vparam.height         = cfg.scry_req;
//    sdl_vparam.depth          = cfg.scrd_req;
//
//    ogl_vparam.dither         = GL_FALSE;
//    ogl_vparam.antialiasing   = GL_TRUE;
//    ogl_vparam.perspective    = GL_FASTEST;
//    ogl_vparam.shading        = GL_SMOOTH;
//    ogl_vparam.userAnisotropy = cfg.texturefilter_req > TX_TRILINEAR_2;
//
//
//    log_info("Opening SDL Video Mode... ");
//    if ( NULL == SDL_GL_set_mode(NULL, &sdl_vparam, &ogl_vparam) )
//    {
//        log_message( "Failed!\n" );
//        log_info( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
//        exit(-1);
//    }
//    else
//    {
//        log_message( "Success!\n" );
//    }
//
//
//    /* Set the OpenGL Attributes */
//#ifndef __unix__
//    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   colordepth );
//    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth  );
//    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
//    if ( cfg.scrd_req > colordepth * 3)
//    {
//        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, cfg.scrd_req - colordepth * 3 );
//    }
//#endif
//
//	//Some bug causes non 32-bit to crash on windows, so send them a warning
//#ifdef WIN32
//	if (cfg.scrd_req != 32) log_warning( "Color depth is not 32! This can cause the game to crash upon startup. See setup.txt\n" );
//#endif
//
//    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, btrue );
//    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, cfg.scrz_req );
//
//    //Check if antialiasing is enabled
//    if ( gfx.antialiasing && gfx.multisamples > 1 )
//    {
//		Sint8 success = 0;
//        success += SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, GL_TRUE);
//		success += SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, gfx.multisamples);
//        if(success != 0) log_message("Could not set X%i antialiasing. (Not supported by your gfx card?)\n");
//    }
//
//
//    // Set the window name
//    SDL_WM_SetCaption( "Egoboo " VERSION, "Egoboo" );
//
//    input_init();
//
//#if defined(USE_LUA_CONSOLE)
//    {
//        SDL_Rect blah = {0, 0, scrx, scry / 4};
//        lua_console_new(NULL, blah);
//    };
//#endif
//
//}
//
/*struct s_packing_test
{
    Uint8 val1;
    Uint8 val2;

    Uint8 ary1[3];
    Uint8 ary2[3];
};

static struct s_packing_test packing_test;*/

//---------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    // dumps the current screen (GL context) to a new bitmap file
    // right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    int i;
    FILE *test;
    bool_t savefound = bfalse;
    bool_t saved     = bfalse;
    char szFilename[100];

    // find a valid file name
    savefound = bfalse;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        sprintf( szFilename, "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        test = fopen( szFilename, "rb" );
        if ( test != NULL )
        {
            fclose( test );
            i++;
        }
        else
        {
            savefound = btrue;
        }
    }
    if ( !savefound ) return bfalse;

    // if we are not using OpenGl, jsut dump the screen
    if ( 0 == (sdl_scr.pscreen->flags & SDL_OPENGL) )
    {
        SDL_SaveBMP(sdl_scr.pscreen, szFilename);
        return bfalse;
    }

    // we ARE using OpenGL
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;
        char buff2[100];

        // create a SDL surface
        temp = SDL_CreateRGBSurface( SDL_SWSURFACE, sdl_scr.x, sdl_scr.y, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                     0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                                     0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                                   );
        if ( temp == NULL ) return bfalse;
        if ( -1 != SDL_LockSurface( temp ) )
        {
            SDL_Rect rect;

            memcpy( &rect, &(sdl_scr.pscreen->clip_rect), sizeof(SDL_Rect) );
            if ( 0 == rect.w && 0 == rect.h )
            {
                rect.w = sdl_scr.x;
                rect.h = sdl_scr.y;
            }
            if ( rect.w > 0 && rect.h > 0 )
            {
                int y;
                Uint8 * pixels;

                glGetError();

                //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                //// stolen from SDL ;)
                //glPixelStorei( GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                //assert( GL_NO_ERROR == glGetError() );

                //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                //// it is not necesssaty to mess with the alignment
                //glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                //assert( GL_NO_ERROR == glGetError() );

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = (Uint8 *)temp->pixels;
                for (y = rect.y; y < rect.y + rect.h; y++)
                {
                    glReadPixels(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                    pixels += temp->pitch;
                }
                assert( GL_NO_ERROR == glGetError() );
            }

            SDL_UnlockSurface( temp );

            // Save the file as a .bmp
            saved = ( -1 != SDL_SaveBMP( temp, szFilename ) );
        }

        // free the SDL surface
        SDL_FreeSurface( temp );
        if ( saved )
        {
            // tell the user what we did
            sprintf( buff2, "Saved to %s", szFilename );
            debug_message( buff2 );
        }
    }
    glPopClientAttrib();

    return savefound;
}

//---------------------------------------------------------------------------------------------------
void load_graphics()
{
    // ZF> This function loads all the graphics based on the game settings
    GLenum quality;

    // Check if the computer graphic driver supports anisotropic filtering
    if ( tex_params.texturefilter == TX_ANISOTROPIC )
    {
        glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ogl_caps.maxAnisotropy );
        if ( !strstr( ( char* )glGetString( GL_EXTENSIONS ), "GL_EXT_texture_filter_anisotropic" ) )
        {
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
            tex_params.texturefilter = TX_TRILINEAR_2; // Set filtering to trillienar instead
        }
    }

    // Enable prespective correction?
    if ( gfx.perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering?
    if ( gfx.dither )
	{
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
		glEnable( GL_DITHER );
	}
    else
	{
		glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
		glDisable( GL_DITHER );
	}

    // Enable gourad shading? (Important!)
    glShadeModel( gfx.shading );

    // Enable antialiasing?
    if ( gfx.antialiasing > 0 )
    {
        glEnable(GL_MULTISAMPLE_ARB);

        glEnable( GL_LINE_SMOOTH );
        glHint( GL_LINE_SMOOTH_HINT,    GL_NICEST );

        glEnable( GL_POINT_SMOOTH );
        glHint( GL_POINT_SMOOTH_HINT,   GL_NICEST );

        glDisable( GL_POLYGON_SMOOTH );
        glHint( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        //  glEnable (GL_BLEND);
        //  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        //glEnable(GL_POLYGON_SMOOTH);
        //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        glDisable(GL_MULTISAMPLE_ARB);
        glDisable( GL_POINT_SMOOTH );
        glDisable( GL_LINE_SMOOTH );
        glDisable( GL_POLYGON_SMOOTH );
    }

}

//---------------------------------------------------------------------------------------------
float calc_light_rotation( int rotation, int normal )
{
    // ZZ> This function helps make_lighttable
    GLvector3 nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = kMd2Normals[normal][0];
    nrm.y = kMd2Normals[normal][1];
    nrm.z = kMd2Normals[normal][2];

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;
    
    return (nrm2.x < 0) ? 0 : (nrm2.x * nrm2.x);
}

//---------------------------------------------------------------------------------------------
float calc_light_global( int rotation, int normal, float lx, float ly, float lz )
{
    // ZZ> This function helps make_lighttable
    float fTmp;
    GLvector3 nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = kMd2Normals[normal][0];
    nrm.y = kMd2Normals[normal][1];
    nrm.z = kMd2Normals[normal][2];

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;

    fTmp = nrm2.x * lx + nrm2.y * ly + nrm2.z * lz;
    if( fTmp < 0 ) fTmp = 0;
    
    return fTmp * fTmp;
}

//---------------------------------------------------------------------------------------------
void make_lighttable( float lx, float ly, float lz, float ambi )
{
    // ZZ> This function makes a light table to fake directional lighting
    Uint32 cnt, tnc;

    // Build a lookup table for sin/cos
    for ( cnt = 0; cnt < MAXLIGHTROTATION; cnt++ )
    {
        sinlut[cnt] = SIN( TWO_PI * cnt / MAXLIGHTROTATION );
        coslut[cnt] = COS( TWO_PI * cnt / MAXLIGHTROTATION );
    }

    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
        for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
        {
            lighttable_local[tnc][cnt]  = calc_light_rotation( tnc, cnt );
            lighttable_global[tnc][cnt] = ambi * calc_light_global( tnc, cnt, lx, ly, lz );
        }
    }

    // Fill in index number 162 for the spike mace
    for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
    {
        lighttable_local[tnc][cnt] = 0;
        lighttable_global[tnc][cnt] = 0;
    }
}

//--------------------------------------------------------------------------------------------
void make_enviro( void )
{
    // ZZ> This function sets up the environment mapping table
    int cnt;
    float x, y, z;

    // Find the environment map positions
    for ( cnt = 0; cnt < MD2LIGHTINDICES; cnt++ )
    {
        x = kMd2Normals[cnt][0];
        y = kMd2Normals[cnt][1];
        indextoenvirox[cnt] = ATAN2( y, x ) / TWO_PI;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / 255.0f;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void project_view( camera_t * pcam )
{
    // ZZ> This function figures out where the corners of the view area
    //     go when projected onto the plane of the PMesh->  Used later for
    //     determining which mesh fans need to be rendered

    int cnt, tnc, extra[4];
    float ztemp;
    float numstep;
    float zproject;
    float xfin, yfin, zfin;
    GLmatrix mTemp;

    // Range
    ztemp = ( pcam->pos.z );

    // Topleft
    mTemp = MatrixMult( RotateY( -rotmeshtopside * PI / 360 ), gCamera.mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[0] = xfin;
        cornery[0] = yfin;
    }

    // Topright
    mTemp = MatrixMult( RotateY( rotmeshtopside * PI / 360 ), gCamera.mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[1] = xfin;
        cornery[1] = yfin;
    }

    // Bottomright
    mTemp = MatrixMult( RotateY( rotmeshbottomside * PI / 360 ), gCamera.mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[2] = xfin;
        cornery[2] = yfin;
    }

    // Bottomleft
    mTemp = MatrixMult( RotateY( -rotmeshbottomside * PI / 360 ), gCamera.mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[3] = xfin;
        cornery[3] = yfin;
    }

    // Get the extreme values
    cornerlowx = cornerx[0];
    cornerlowy = cornery[0];
    cornerhighx = cornerx[0];
    cornerhighy = cornery[0];
    cornerlistlowtohighy[0] = 0;
    cornerlistlowtohighy[3] = 0;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        if ( cornerx[cnt] < cornerlowx )
            cornerlowx = cornerx[cnt];
        if ( cornery[cnt] < cornerlowy )
        {
            cornerlowy = cornery[cnt];
            cornerlistlowtohighy[0] = cnt;
        }
        if ( cornerx[cnt] > cornerhighx )
            cornerhighx = cornerx[cnt];
        if ( cornery[cnt] > cornerhighy )
        {
            cornerhighy = cornery[cnt];
            cornerlistlowtohighy[3] = cnt;
        }
    }

    // Figure out the order of points
    tnc = 0;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        if ( cnt != cornerlistlowtohighy[0] && cnt != cornerlistlowtohighy[3] )
        {
            extra[tnc] = cnt;
            tnc++;
        }
    }

    cornerlistlowtohighy[1] = extra[1];
    cornerlistlowtohighy[2] = extra[0];
    if ( cornery[extra[0]] < cornery[extra[1]] )
    {
        cornerlistlowtohighy[1] = extra[0];
        cornerlistlowtohighy[2] = extra[1];
    }
}

//--------------------------------------------------------------------------------------------
void clear_messages()
{
    // ZZ> This function empties the message buffer
    int cnt;

    cnt = 0;

    while ( cnt < MAXMESSAGE )
    {
        msgtime[cnt] = 0;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_dynalist( camera_t * pcam )
{
    // ZZ> This function figures out which particles are visible, and it sets up dynamic
    //     lighting
    int cnt, tnc, slot;
    float disx, disy, disz, distance;

    // Don't really make a list, just set to visible or not
    dyna_list_count = 0;
    dyna_distancetobeat = MAXDYNADIST * MAXDYNADIST;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PrtList[cnt].inview = bfalse;
        if ( !PrtList[cnt].on ) continue;
        
        if( INVALID_TILE == PrtList[cnt].onwhichfan || PrtList[cnt].onwhichfan >= PMesh->info.tiles_count ) continue;

        PrtList[cnt].inview = PMesh->mem.tile_list[PrtList[cnt].onwhichfan].inrenderlist;

        // Set up the lights we need
        if ( !PrtList[cnt].dynalighton ) continue;

        disx = PrtList[cnt].pos.x - pcam->track_pos.x;
        disy = PrtList[cnt].pos.y - pcam->track_pos.y;
        disz = PrtList[cnt].pos.z - pcam->track_pos.z;

        distance = disx*disx + disy*disy + disz*disz;
        if ( distance < dyna_distancetobeat )
        {
            bool_t found = bfalse;
            if ( dyna_list_count < dyna_list_max )
            {
                // Just add the light
                slot = dyna_list_count;
                dyna_list[slot].distance = distance;
                dyna_list_count++;
                found = btrue;
            }
            else
            {
                // Overwrite the worst one
                slot = 0;
                dyna_distancetobeat = dyna_list[0].distance;
                for ( tnc = 1; tnc < dyna_list_max; tnc++ )
                {
                    if ( dyna_list[tnc].distance > dyna_distancetobeat )
                    {
                        slot = tnc;
                        found = btrue;
                    }
                }

                if( found )
                {
                    dyna_list[slot].distance = distance;

                    // Find the new distance to beat
                    dyna_distancetobeat = dyna_list[0].distance;
                    for ( tnc = 1; tnc < dyna_list_max; tnc++ )
                    {
                        if ( dyna_list[tnc].distance > dyna_distancetobeat )
                        {
                            dyna_distancetobeat = dyna_list[tnc].distance;
                        }
                    }
                }
            }

            if( found )
            {
                dyna_list[slot].x       = PrtList[cnt].pos.x;
                dyna_list[slot].y       = PrtList[cnt].pos.y;
                dyna_list[slot].z       = PrtList[cnt].pos.z;
                dyna_list[slot].level   = PrtList[cnt].dynalightlevel;
                dyna_list[slot].falloff = PrtList[cnt].dynalightfalloff;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t dolist_add_chr( Uint16 ichr )
{
    // This function puts a character in the list
    int itile;
    chr_t * pchr;
    cap_t * pcap;
    chr_instance_t * pinst;

    if( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( INVALID_CHR(ichr) ) return bfalse;
    pchr  = ChrList + ichr;
    pinst = &(pchr->inst);

    if( pinst->indolist ) return btrue;

    if( INVALID_CAP( pchr->model ) ) return bfalse;
    pcap = CapList + pchr->model;

    itile = pchr->onwhichfan;
    if ( INVALID_TILE == itile ) return bfalse;

    if ( PMesh->mem.tile_list[itile].inrenderlist )
    {
        if ( 0 == ( 0xFF00 & PMesh->mem.tile_list[itile].img ) )
        {
            int vrtstart;
            int itmp, imin, isum;

            vrtstart = PMesh->mem.tile_list[itile].vrtstart;

            itmp = PMesh->mem.vrt_l[vrtstart + 0];
            imin = itmp;
            isum = itmp;

            itmp = PMesh->mem.vrt_l[vrtstart + 1];
            imin  = MIN(imin, itmp);
            isum += itmp;

            itmp = PMesh->mem.vrt_l[vrtstart + 2];
            imin  = MIN(imin, itmp);
            isum += itmp;

            itmp = PMesh->mem.vrt_l[vrtstart + 3];
            imin  = MIN(imin, itmp);
            isum += itmp;

            pchr->inst.lightlevel_amb = imin;
            pchr->inst.lightlevel_dir = (isum - 4 * imin) / 4;
        }

        dolist[dolist_count].ichr = ichr;
        dolist[dolist_count].iprt = TOTAL_MAX_PRT;
        dolist_count++;

        pinst->indolist = btrue;
    }
    else if ( pcap->alwaysdraw )
    {
        // Double check for large/special objects

        dolist[dolist_count].ichr = ichr;
        dolist[dolist_count].iprt = TOTAL_MAX_PRT;
        dolist_count++;

        pinst->indolist = btrue;
    }

    if ( pinst->indolist )
    {
        // Add its weapons too
        dolist_add_chr( pchr->holdingwhich[SLOT_LEFT] );
        dolist_add_chr( pchr->holdingwhich[SLOT_RIGHT] );
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t dolist_add_prt( Uint16 iprt )
{
    // This function puts a character in the list
    prt_t * pprt;
    prt_instance_t * pinst;

    if( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( INVALID_PRT(iprt) ) return bfalse;
    pprt = PrtList + iprt;
    pinst = &(pprt->inst);

    if( pinst->indolist ) return btrue;

    if( 0 == pinst->size || pprt->is_hidden || INVALID_TILE == pprt->onwhichfan ) return bfalse;

    dolist[dolist_count].ichr = MAX_CHR;
    dolist[dolist_count].iprt = iprt;
    dolist_count++;

    pinst->indolist = btrue;

    return btrue;
}


//--------------------------------------------------------------------------------------------
void dolist_make()
{
    // ZZ> This function finds the characters that need to be drawn and puts them in the list

    int cnt;

    // Remove everyone from the dolist
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        if( TOTAL_MAX_PRT == dolist[cnt].iprt && MAX_CHR != dolist[cnt].ichr )
        {
            ChrList[ dolist[cnt].ichr ].inst.indolist = bfalse;
        }
        else if( MAX_CHR == dolist[cnt].ichr && TOTAL_MAX_PRT != dolist[cnt].iprt )
        {
            PrtList[ dolist[cnt].iprt ].inst.indolist = bfalse;
        }
    }
    dolist_count = 0;

    // Now fill it up again
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( ChrList[cnt].on && !ChrList[cnt].inpack )
        {
            // Add the character
            dolist_add_chr( cnt );
        }
    }

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        if ( PrtList[cnt].on && VALID_TILE(PMesh, PrtList[cnt].onwhichfan) )
        {
            // Add the character
            dolist_add_prt( cnt );
        }
    }
}


//--------------------------------------------------------------------------------------------
void dolist_sort( camera_t * pcam )
{
    // ZZ> This function orders the dolist based on distance from camera,
    //     which is needed for reflections to properly clip themselves.
    //     Order from closest to farthest

    int       cnt, tnc;
    GLvector3 vcam;
    size_t    count;

    vcam = mat_getCamForward(pcam->mView);

    // Figure the distance of each
    count = 0;
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        GLvector3 vtmp;
        float dist;

        if( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR(dolist[cnt].ichr) )
        { 
            tnc = dolist[cnt].ichr;
            vtmp = VSub( ChrList[tnc].pos, pcam->pos );
        }
        else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT(dolist[cnt].iprt) )
        {
            tnc = dolist[cnt].iprt;
            vtmp = VSub( PrtList[tnc].pos, pcam->pos );
        }
        else
        {
            continue;
        }

        dist = VDotProduct( vtmp, vcam );
        if( dist > 0 )
        {
            dolist[count].ichr = dolist[cnt].ichr;
            dolist[count].iprt = dolist[cnt].iprt;
            dolist[count].dist = VDotProduct( vtmp, vcam );
            count++;
        }
    }
    dolist_count = count;

    // use qsort to sort the list in-place
    qsort( dolist, dolist_count, sizeof(obj_registry_entity_t), obj_registry_entity_cmp );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int obj_registry_entity_cmp( const void * pleft, const void * pright )
{
    obj_registry_entity_t * dleft  = (obj_registry_entity_t *) pleft;
    obj_registry_entity_t * dright = (obj_registry_entity_t *) pright;

    return dleft->dist - dright->dist;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t gfx_config_synch(gfx_config_t * pgfx, egoboo_config_t * pcfg )
{
    // call gfx_config_init(), even if the config data is invalid
    if( !gfx_config_init( pgfx ) ) return bfalse;

    // if there is no config data, do not proceed
    if( NULL == pcfg ) return bfalse;

    pgfx->antialiasing = pcfg->multisamples > 0;
    pgfx->multisamples = pcfg->multisamples;

    pgfx->refon        = pcfg->reflect_allowed;
    pgfx->reffadeor    = pcfg->reflect_fade ? 0 : 255;

    pgfx->shaon        = pcfg->shadow_allowed;
    pgfx->shasprite    = pcfg->shadow_sprite;

    pgfx->shading      = pcfg->gourard_req ? GL_SMOOTH : GL_FLAT;
    pgfx->dither       = pcfg->use_dither;
    pgfx->perspective  = pcfg->use_perspective;
    pgfx->phongon      = pcfg->use_phong;

    pgfx->draw_background = pcfg->background_allowed && water_background_req;
    pgfx->draw_overlay    = pcfg->overlay_allowed && water_overlay_req;

    pgfx->draw_water_0 = !pgfx->draw_overlay && (numwaterlayer > 0);
    pgfx->clearson     = !pgfx->draw_background;
    pgfx->draw_water_1 = !pgfx->draw_background && (numwaterlayer > 1);

    return btrue;
}


//--------------------------------------------------------------------------------------------
bool_t gfx_config_init ( gfx_config_t * pgfx )
{
    if(NULL == pgfx) return bfalse;

    pgfx->shading          = GL_SMOOTH;
    pgfx->refon            = btrue;
    pgfx->reffadeor        = 0;
    pgfx->antialiasing     = bfalse;
    pgfx->multisamples     = 0;
    pgfx->dither           = bfalse;
    pgfx->perspective      = bfalse;
    pgfx->phongon          = btrue;
    pgfx->shaon            = btrue;
    pgfx->shasprite        = btrue;

    pgfx->clearson         = btrue;   // Do we clear every time?
    pgfx->draw_background  = bfalse;   // Do we draw the background image?
    pgfx->draw_overlay     = bfalse;   // Draw overlay?
    pgfx->draw_water_0     = btrue;   // Do we draw water layer 1 (TX_WATER_LOW)
    pgfx->draw_water_1     = btrue;   // Do we draw water layer 2 (TX_WATER_TOP)

    return btrue;
};