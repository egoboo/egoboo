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

#include "egoboo.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"

#include <SDL_image.h>

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define BLIPSIZE 6

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint8           maxformattypes = 0;
STRING          TxFormatSupported[50]; // OpenGL icon surfaces

GLTexture       TxIcon[MAXTEXTURE+1];       // OpenGL icon surfaces
GLTexture       TxTitleImage[MAXMODULE];    // OpenGL title image surfaces
GLTexture       TxFont;                     // OpenGL font surface
GLTexture       TxBars;                     // OpenGL status bar surface
GLTexture       TxBlip;                     // OpenGL you are here surface
GLTexture       TxMap;                      // OpenGL map surface
GLTexture       txTexture[MAXTEXTURE];      // All textures

SDL_Surface *    displaySurface = NULL;

Uint16           numdolist = 0;
Uint16           dolist[MAXCHR];

bool_t           meshnotexture = bfalse;
Uint16           meshlasttexture = ~0;

renderlist_t     renderlist = {0, 0, 0, 0, 0};

Uint8            lightdirectionlookup[65536];
Uint8            lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
float            indextoenvirox[MD2LIGHTINDICES];
float            lighttoenviroy[256];
Uint32           lighttospek[MAXSPEKLEVEL][256];

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

glMatrix mWorld;                       // World Matrix
glMatrix mView;                        // View Matrix
glMatrix mViewSave;                    // View Matrix initial state
glMatrix mProjection;                  // Projection Matrix

int         dyna_distancetobeat;           // The number to beat
int         dyna_list_max   = 8;           // Max number of lights to draw
int         dyna_list_count = 0;           // Number of dynamic lights
dynalight_t dyna_list[TOTALMAXDYNA];

// Interface stuff
static rect_t             iconrect;                   // The 32x32 icon rectangle

static int                fontoffset;                 // Line up fonts from top of screen

static SDL_Rect           fontrect[NUMFONT];          // The font rectangles
static Uint8              fontxspacing[NUMFONT];      // The spacing stuff
static Uint8              fontyspacing;               //
static rect_t             tabrect[NUMBAR];            // The tab rectangles
static rect_t             barrect[NUMBAR];            // The bar rectangles
static rect_t             bliprect[NUMBAR];           // The blip rectangles
static rect_t             maprect;                    // The map rectangle

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void font_init();
static void font_release();

static void project_view(camera_t * pcam);
static void make_prtlist( void );

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
void load_mesh_fans()
{
    // ZZ> This function loads fan types for the terrain
    int cnt, entry;
    int numfantype, fantype, bigfantype, vertices;
    int numcommand, command, commandsize;
    int itmp;
    float ftmp;
    FILE* fileread;
    float offx, offy;

    // Initialize all mesh types to 0
    entry = 0;

    while ( entry < MAXMESHTYPE )
    {
        meshcommandnumvertices[entry] = 0;
        meshcommands[entry] = 0;
        entry++;
    }

    // Open the file and go to it
    fileread = fopen( "basicdat" SLASH_STR "fans.txt", "r" );
    if ( fileread )
    {
        goto_colon( fileread );
        fscanf( fileread, "%d", &numfantype );
        fantype = 0;
        bigfantype = MAXMESHTYPE / 2; // Duplicate for 64x64 tiles

        while ( fantype < numfantype )
        {
            goto_colon( fileread );
            fscanf( fileread, "%d", &vertices );
            meshcommandnumvertices[fantype] = vertices;
            meshcommandnumvertices[bigfantype] = vertices;  // Dupe
            cnt = 0;

            while ( cnt < vertices )
            {
                goto_colon( fileread );
                fscanf( fileread, "%d", &itmp );
                goto_colon( fileread );
                fscanf( fileread, "%f", &ftmp );
                meshcommandu[fantype][cnt] = ftmp;
                meshcommandu[bigfantype][cnt] = ftmp;  // Dupe
                goto_colon( fileread );
                fscanf( fileread, "%f", &ftmp );
                meshcommandv[fantype][cnt] = ftmp;
                meshcommandv[bigfantype][cnt] = ftmp;  // Dupe
                cnt++;
            }

            goto_colon( fileread );
            fscanf( fileread, "%d", &numcommand );
            meshcommands[fantype] = numcommand;
            meshcommands[bigfantype] = numcommand;  // Dupe
            entry = 0;
            command = 0;

            while ( command < numcommand )
            {
                goto_colon( fileread );
                fscanf( fileread, "%d", &commandsize );
                meshcommandsize[fantype][command] = commandsize;
                meshcommandsize[bigfantype][command] = commandsize;  // Dupe
                cnt = 0;

                while ( cnt < commandsize )
                {
                    goto_colon( fileread );
                    fscanf( fileread, "%d", &itmp );
                    meshcommandvrt[fantype][entry] = itmp;
                    meshcommandvrt[bigfantype][entry] = itmp;  // Dupe
                    entry++;
                    cnt++;
                }

                command++;
            }

            fantype++;
            bigfantype++;  // Dupe
        }

        fclose( fileread );
    }

    // Correct all of them silly texture positions for seamless tiling
    entry = 0;

    while ( entry < MAXMESHTYPE / 2 )
    {
        cnt = 0;

        while ( cnt < meshcommandnumvertices[entry] )
        {
//            meshcommandu[entry][cnt] = ((0.5f/32)+(meshcommandu[entry][cnt]*31/32))/8;
//            meshcommandv[entry][cnt] = ((0.5f/32)+(meshcommandv[entry][cnt]*31/32))/8;
            meshcommandu[entry][cnt] = ( ( 0.6f / 32 ) + ( meshcommandu[entry][cnt] * 30.8f / 32 ) ) / 8;
            meshcommandv[entry][cnt] = ( ( 0.6f / 32 ) + ( meshcommandv[entry][cnt] * 30.8f / 32 ) ) / 8;
            cnt++;
        }

        entry++;
    }

    // Do for big tiles too
    while ( entry < MAXMESHTYPE )
    {
        cnt = 0;

        while ( cnt < meshcommandnumvertices[entry] )
        {
//            meshcommandu[entry][cnt] = ((0.5f/64)+(meshcommandu[entry][cnt]*63/64))/4;
//            meshcommandv[entry][cnt] = ((0.5f/64)+(meshcommandv[entry][cnt]*63/64))/4;
            meshcommandu[entry][cnt] = ( ( 0.6f / 64 ) + ( meshcommandu[entry][cnt] * 62.8f / 64 ) ) / 4;
            meshcommandv[entry][cnt] = ( ( 0.6f / 64 ) + ( meshcommandv[entry][cnt] * 62.8f / 64 ) ) / 4;
            cnt++;
        }

        entry++;
    }

    // Make tile texture offsets
    entry = 0;

    while ( entry < MAXTILETYPE )
    {
        offx = ( entry & 7 ) / 8.0f;
        offy = ( entry >> 3 ) / 8.0f;
        meshtileoffu[entry] = offx;
        meshtileoffv[entry] = offy;
        entry++;
    }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    // do the fanstart
    for ( cnt = 0; cnt < meshtilesy; cnt++ )
    {
        meshfanstart[cnt] = meshtilesx * cnt;
    }

    // calculate some of the block info
    meshblocksx = (meshtilesx >> 2);
    if ( 0 != (meshtilesx & 0x03) ) meshblocksx++;
    if ( meshblocksx >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", meshblocksx, MAXMESHBLOCKY );
    }

    meshblocksy = (meshtilesy >> 2);
    if ( 0 != (meshtilesy & 0x03) ) meshblocksy++;
    if ( meshblocksy >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", meshblocksy, MAXMESHBLOCKY );
    }

    meshblocks = meshblocksx * meshblocksy;

    // do the blockstart
    for ( cnt = 0; cnt < meshblocksy; cnt++ )
    {
        meshblockstart[cnt] = meshblocksx * cnt;
    }

}

//--------------------------------------------------------------------------------------------
void make_vrtstart()
{
    int x, y, vert;
    Uint32 fan;

    vert = 0;
    for ( y = 0; y < meshtilesy; y++ )
    {
        for ( x = 0; x < meshtilesx; x++ )
        {
            // allow raw access because we are careful
            fan = meshfanstart[y] + x;
            meshvrtstart[fan] = vert;
            vert += meshcommandnumvertices[meshtype[fan]];
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
        msgtime[slot] = messagetime;
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
void Begin3DMode()
{
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( mProjection.v );

    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( mView.v );
    glMultMatrixf( mWorld.v );
}

void End3DMode()
{

}

/********************> Begin2DMode() <*****/
void Begin2DMode( void )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();                  // Reset The Projection Matrix
    glOrtho( 0, displaySurface->w, displaySurface->h, 0, -1, 1 );        // Set up an orthogonal projection

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

//---------------------------------------------------------------------------------------------
float get_level( float x, float y, bool_t waterwalk )
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
    //     level of the water.

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_tile(x, y);
    if ( INVALID_TILE == tile ) return 0;

    ix = x;
    iy = y;

    ix &= 127;
    iy &= 127;

    z0 = meshvrtz[ meshvrtstart[tile] + 0 ];
    z1 = meshvrtz[ meshvrtstart[tile] + 1 ];
    z2 = meshvrtz[ meshvrtstart[tile] + 2 ];
    z3 = meshvrtz[ meshvrtstart[tile] + 3 ];

    zleft = ( z0 * ( 128 - iy ) + z3 * iy ) / (float)(1 << 7);
    zright = ( z1 * ( 128 - iy ) + z2 * iy ) / (float)(1 << 7);
    zdone = ( zleft * ( 128 - ix ) + zright * ix ) / (float)(1 << 7);

    if ( waterwalk )
    {
        if ( watersurfacelevel > zdone && 0 != ( meshfx[tile] & MESHFX_WATER ) && wateriswater )
        {
            return watersurfacelevel;
        }
    }

    return zdone;
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
bool_t load_one_icon(  const char *szLoadName )
{
    // ZZ> This function is used to load an icon.  Most icons are loaded
    //     without this function though...
    if ( INVALID_TX_ID == GLTexture_Load( GL_TEXTURE_2D,  TxIcon + globalicon_count,  szLoadName, INVALID_KEY ) )
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

    for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
    {
        GLTexture_new( TxIcon + cnt );
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
        GLTexture_new( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void init_bars()
{
    int cnt;

    GLTexture_new( &TxBars );

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

    GLTexture_new( &TxBlip );

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
    GLTexture_new( &TxMap );

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

    for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
    {
        GLTexture_new( txTexture + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_models()
{
    // ZZ> This function initializes all of the models

    Uint16 cnt;

    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        CapList[cnt].classname[0] = 0;

        MadList[cnt].used = bfalse;
        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
    }

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
    // ZZ> This function clears out all of the icons
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE + 1; cnt++ )
    {
        GLTexture_Release( TxIcon + cnt );
        skintoicon[cnt] = 0;
    }

    bookicon_count = 0;
    for ( cnt = 0; cnt < MAXSKIN; cnt++ )
    {
        bookicon[cnt] = MAXTEXTURE + 1;
    }

    globalicon_count = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
    // ZZ> This function clears out all of the title images
    int cnt;

    for ( cnt = 0; cnt < MAXMODULE; cnt++ )
    {
        GLTexture_Release( TxTitleImage + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_bars()
{
    GLTexture_Release( &TxBars );
}

//---------------------------------------------------------------------------------------------
void release_blip()
{
    GLTexture_Release( &TxBlip );

    youarehereon = bfalse;
    numblip      = 0;
}

//---------------------------------------------------------------------------------------------
void release_map()
{
    GLTexture_Release( &TxMap );

    mapvalid = bfalse;
    mapon    = bfalse;
}

//---------------------------------------------------------------------------------------------
void release_all_textures()
{
    // ZZ> This function releases all of the textures
    int cnt;

    for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
    {
        GLTexture_Release( txTexture + cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_models()
{
    // ZZ> This function clears out all of the models
    Uint16 cnt;

    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        CapList[cnt].classname[0] = 0;

        MadList[cnt].used = bfalse;
        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
    }

    md2_loadframe = 0;
}

//--------------------------------------------------------------------------------------------
void debug_message(  const char *text )
{
    // ZZ> This function sticks a message in the display queue and sets its timer
    int slot = get_free_message();
    // Copy the message
    int write = 0;
    int read = 0;
    char cTmp = text[read];  read++;
    msgtime[slot] = messagetime;

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
    if ( numpla > 1 )
    {
        sprintf( endtext, "Sadly, they were never heard from again..." );
        endtextwrite = 42;  // Where to append further text
    }
    else
    {
        if ( numpla == 0 )
        {
            // No players???
            sprintf( endtext, "The game has ended..." );
            endtextwrite = 21;
        }
        else
        {
            // One player
            sprintf( endtext, "Sadly, no trace was ever found..." );
            endtextwrite = 33;  // Where to append further text
        }
    }
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
void make_renderlist()
{
    // ZZ> This function figures out which mesh fans to draw
    int cnt, fan, fanx, fany;
    int row, run, numrow;
    int xlist[4], ylist[4];
    int leftnum, leftlist[4];
    int rightnum, rightlist[4];
    int fanrowstart[128], fanrowrun[128];
    int x, stepx, divx, basex;
    int from, to;

    // Clear old render lists
    for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
    {
        fan = renderlist.all[cnt];
        meshinrenderlist[fan] = btrue;
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
    fany = ylist[0] >> 7;
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
            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
        }

        x -= 256;
        run = ylist[to] >> 7;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < meshtilesy )
            {
                fanx = x >> 7;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= meshtilesx )  fanx = meshtilesx - 1;

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
    fany = ylist[0] >> 7;
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
            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
        }

        run = ylist[to] >> 7;

        while ( fany < run )
        {
            if ( fany >= 0 && fany < meshtilesy )
            {
                fanx = x >> 7;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= meshtilesx - 1 )  fanx = meshtilesx - 1;//-2

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
    fany = ylist[0] >> 7;
    if ( fany < 0 ) fany = 0;
    if ( fany >= meshtilesy ) fany = meshtilesy - 1;

    row = 0;
    while ( row < numrow )
    {
        // allow raw access because we have no choice
        cnt = meshfanstart[fany] + fanrowstart[row];

        run = fanrowrun[row];
        fanx = 0;

        while ( fanx < run )
        {
            if ( renderlist.all_count < MAXMESHRENDER )
            {
                // Put each tile in basic list
                meshinrenderlist[cnt] = btrue;
                renderlist.all[renderlist.all_count] = cnt;
                renderlist.all_count++;

                // Put each tile in one other list, for shadows and relections
                if ( 0 != ( meshfx[cnt] & MESHFX_SHA ) )
                {
                    renderlist.sha[renderlist.sha_count] = cnt;
                    renderlist.sha_count++;
                }
                else
                {
                    renderlist.ref[renderlist.ref_count] = cnt;
                    renderlist.ref_count++;
                }

                if ( 0 != ( meshfx[cnt] & MESHFX_DRAWREF ) )
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

            cnt++;
            fanx++;
        }

        row++;
        fany++;
    }
}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw()
{
    // ZZ> This function determines the things that need to be drawn

    // Find the render area corners
    project_view(&gCamera);

    // Make the render list for the mesh
    make_renderlist();

    gCamera.turnleftrightone = ( gCamera.turnleftright ) / ( TWO_PI );
    gCamera.turnleftrightshort = gCamera.turnleftrightone * 65536;

    // Request matrices needed for local machine
    make_dolist();
    order_dolist();
}

//--------------------------------------------------------------------------------------------
void order_dolist( void )
{
    // ZZ> This function orders the dolist based on distance from camera,
    //     which is needed for reflections to properly clip themselves.
    //     Order from closest to farthest
    int tnc, cnt, character, order;
    int dist[MAXCHR];
    Uint16 olddolist[MAXCHR];

    // Figure the distance of each
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        character = dolist[cnt];  olddolist[cnt] = character;
        if ( ChrList[character].light != 255 || ChrList[character].alpha != 255 )
        {
            // This makes stuff inside an invisible character visible...
            // A key inside a Jellcube, for example
            dist[cnt] = 0x7fffffff;
        }
        else
        {
            dist[cnt] = (int) (ABS( ChrList[character].xpos - gCamera.x ) + ABS( ChrList[character].ypos - gCamera.y ));
        }
    }

    // Put em in the right order
    for ( cnt = 0; cnt < numdolist; cnt++  )
    {
        character = olddolist[cnt];
        order = 0;  // Assume this character is closest

        for ( tnc = 0; tnc < numdolist; tnc++ )
        {
            // For each one closer, increment the order
            order += ( dist[cnt] > dist[tnc] );
            order += ( dist[cnt] == dist[tnc] ) && ( cnt < tnc );
        }

        dolist[order] = character;
    }
}

//--------------------------------------------------------------------------------------------
void flash_character( Uint16 character, Uint8 value )
{
    // ZZ> This function sets a character's lighting
    int cnt;

    for ( cnt = 0; cnt < MadList[ChrList[character].model].transvertices; cnt++  )
    {
        ChrList[character].vrta[cnt] = value;
    }
}

//--------------------------------------------------------------------------------------------
void add_to_dolist( Uint16 ichr )
{
    // This function puts a character in the list
    int itile;

    if ( ichr >= MAXCHR || ChrList[ichr].indolist ) return;

    itile = ChrList[ichr].onwhichfan;
    if ( INVALID_TILE == itile ) return;

    if ( meshinrenderlist[itile] )
    {
        if ( 0 == ( 0xFF00 & meshtile[itile] ) )
        {
            int itmp, imin, isum;

            itmp = meshvrtl[meshvrtstart[itile] + 0];
            imin = itmp;
            isum = itmp;

            itmp = meshvrtl[meshvrtstart[itile] + 1];
            imin  = MIN(imin, itmp);
            isum += itmp;

            itmp = meshvrtl[meshvrtstart[itile] + 2];
            imin  = MIN(imin, itmp);
            isum += itmp;

            itmp = meshvrtl[meshvrtstart[itile] + 3];
            imin  = MIN(imin, itmp);
            isum += itmp;

            ChrList[ichr].lightlevel_amb = imin;
            ChrList[ichr].lightlevel_dir = (isum - 4 * imin) / 4;
        }

        dolist[numdolist] = ichr;
        ChrList[ichr].indolist = btrue;
        numdolist++;
    }
    else if ( CapList[ChrList[ichr].model].alwaysdraw )
    {
        // Double check for large/special objects
        dolist[numdolist] = ichr;
        ChrList[ichr].indolist = btrue;
        numdolist++;
    }

    if ( ChrList[ichr].indolist )
    {
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

        // Add its weapons too
        add_to_dolist( ChrList[ichr].holdingwhich[0] );
        add_to_dolist( ChrList[ichr].holdingwhich[1] );
    }

}

//--------------------------------------------------------------------------------------------
void make_dolist()
{
    // ZZ> This function finds the characters that need to be drawn and puts them in the list

    int cnt, character;

    // Remove everyone from the dolist
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        character = dolist[cnt];
        ChrList[character].indolist = bfalse;
    }
    numdolist = 0;

    // Now fill it up again
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( ChrList[cnt].on && !ChrList[cnt].inpack )
        {
            // Add the character
            add_to_dolist( cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    // This function changes the animated tile frame
    if ( ( frame_wld & animtileupdateand ) == 0 )
    {
        animtileframeadd = ( animtileframeadd + 1 ) & animtileframeand;
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures(  const char *modname )
{
    // ZZ> This function loads the standard textures for a module
    char newloadname[256];

    // Particle sprites
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_PARTICLE, "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle", TRANSCOLOR );

    // Module background tiles
    make_newloadname( modname, "gamedat" SLASH_STR "tile0", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_0, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile1", newloadname );
    GLTexture_Load(GL_TEXTURE_2D,  txTexture + TX_TILE_1, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile2", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_2, newloadname, TRANSCOLOR);

    make_newloadname( modname, "gamedat" SLASH_STR "tile3", newloadname );
    GLTexture_Load(GL_TEXTURE_2D, txTexture + TX_TILE_3, newloadname, TRANSCOLOR );

    // Water textures
    make_newloadname( modname, "gamedat" SLASH_STR "watertop", newloadname );
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_WATER_TOP, newloadname, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "waterlow", newloadname );
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_WATER_LOW, newloadname, TRANSCOLOR);

    // Texture 7 is the phong map
    GLTexture_Load( GL_TEXTURE_2D,  txTexture + TX_PHONG, "basicdat" SLASH_STR "phong", TRANSCOLOR );

}

//--------------------------------------------------------------------------------------------
void load_bars(  const char* szBitmap )
{
    // ZZ> This function loads the status bar bitmap
    if ( INVALID_TX_ID == GLTexture_Load(GL_TEXTURE_2D, &TxBars, szBitmap, TRANSCOLOR ) )
    {
        log_warning( "Cannot load file! (\"%s\")\n", szBitmap );
    }
}

//--------------------------------------------------------------------------------------------
void load_map(  const char* szModule )
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
    if ( INVALID_TX_ID == GLTexture_Load(GL_TEXTURE_2D, &TxMap, szMap, INVALID_KEY ) )
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
    GLTexture_new( &TxFont );

    font_release();
}

//--------------------------------------------------------------------------------------------
void font_release()
{
    // BB > fill in default values

    Uint16 i, ix, iy, cnt;
    float dx, dy;

    GLTexture_Release( &TxFont );

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
void font_load(  const char* szBitmap,  const char* szSpacing )
{
    // ZZ> This function loads the font bitmap and sets up the coordinates
    //     of each font on that bitmap...  Bitmap must have 16x6 fonts
    int cnt, y, xsize, ysize, xdiv, ydiv;
    int xstt, ystt;
    int xspacing, yspacing;
    char cTmp;
    FILE *fileread;

    font_release();
    if ( INVALID_TX_ID == GLTexture_Load( GL_TEXTURE_2D, &TxFont, szBitmap, TRANSCOLOR ) )
    {
        log_error( "Cannot load file! (\"%s\")\n", szBitmap );
    }

    // Get the size of the bitmap
    xsize = GLTexture_GetImageWidth( &TxFont );
    ysize = GLTexture_GetImageHeight( &TxFont );
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
    xstt = 0;
    ystt = 0;

    // Uniform font height is at the top
    goto_colon( fileread );
    fscanf( fileread, "%d", &yspacing );
    fontoffset = yspacing;

    for ( cnt = 0; cnt < NUMFONT && goto_colon_yesno( fileread ); cnt++ )
    {
        fscanf( fileread, "%c%d", &cTmp, &xspacing );
        if ( asciitofont[(Uint8)cTmp] == 255 ) asciitofont[(Uint8)cTmp] = (Uint8) cnt;
        if ( xstt + xspacing + 1 > 255 )
        {
            xstt = 0;
            ystt += yspacing;
        }

        fontrect[cnt].x = xstt;
        fontrect[cnt].w = xspacing;
        fontrect[cnt].y = ystt;
        fontrect[cnt].h = yspacing - 2;
        fontxspacing[cnt] = xspacing + 1;

        xstt += xspacing + 1;
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

    layer = 0;

    while ( layer < numwaterlayer )
    {
        if ( waterlight )  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...

        waterlayeru[layer] = 0;
        waterlayerv[layer] = 0;
        frame = 0;

        while ( frame < MAXWATERFRAME )
        {
            // Do first mode
            mode = 0;

            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( TWO_PI * layer / MAXWATERLAYER ) );
                waterlayerzadd[layer][frame][mode][point] = temp * waterlayeramp[layer];
                waterlayercolor[layer][frame][mode][point] = ( waterlightlevel[layer] * ( temp + 1.0f ) ) + waterlightadd[layer];
            }

            // Now mirror and copy data to other three modes
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
            frame++;
        }

        layer++;
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
        if ( shading == GL_FLAT )
            waterspek[cnt] = 0;
        else
            waterspek[cnt] = spek;
    }
}

//--------------------------------------------------------------------------------------------
void read_wawalite(  const char *modname )
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float lx, ly, lz, la;
    float fTmp;
    char cTmp;
    int iTmp;

    make_newloadname( modname, "gamedat" SLASH_STR "wawalite.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( fileread )
    {
        goto_colon( fileread );
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
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  numwaterlayer = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspekstart = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspeklevel = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterdouselevel = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  watersurfacelevel = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  waterlight = btrue;
        else waterlight = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        wateriswater = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  wateriswater = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( ( cTmp == 'T' || cTmp == 't' ) && overlayvalid )  overlayon = btrue;
        else overlayon = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( ( cTmp == 'T' || cTmp == 't' ) && backgroundvalid )  clearson = bfalse;
        else clearson = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  foregroundrepeat = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  backgroundrepeat = iTmp;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[0] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[0] = fTmp;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[1] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[1] = fTmp;

        waterlayeru[0] = 0;
        waterlayerv[0] = 0;
        waterlayeru[1] = 0;
        waterlayerv[1] = 0;
        waterlayerframe[0] = rand() & WATERFRAMEAND;
        waterlayerframe[1] = rand() & WATERFRAMEAND;
        // Read light data second
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lx = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  ly = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lz = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  la = fTmp;
        // Read tile data third
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  hillslide = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  slippyfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  airfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  noslipfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  gravity = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileupdateand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileframeand = iTmp;
        biganimtileframeand = ( iTmp << 1 ) + 1;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  damagetileamount = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'S' || cTmp == 's' )  damagetiletype = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' )  damagetiletype = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' )  damagetiletype = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' )  damagetiletype = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' )  damagetiletype = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' )  damagetiletype = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' )  damagetiletype = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' )  damagetiletype = DAMAGE_ZAP;

        // Read weather data fourth
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        weatheroverwater = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  weatheroverwater = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  weathertimereset = iTmp;
        weathertime = weathertimereset;
        weatherplayer = 0;
        // Read extra data
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        meshexploremode = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  meshexploremode = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        usefaredge = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  usefaredge = btrue;

        gCamera.swing = 0;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  gCamera.swingrate = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  gCamera.swingamp = fTmp;

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
        if ( goto_colon_yesno( fileread ) )
        {
            fogon = fogallowed;
            fscanf( fileread, "%f", &fTmp );  fogtop = fTmp;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogbottom = fTmp;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogred = fTmp * 255;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  foggrn = fTmp * 255;
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogblu = fTmp * 255;
            goto_colon( fileread );  cTmp = fget_first_letter( fileread );
            if ( cTmp == 'F' || cTmp == 'f' )  fogaffectswater = bfalse;

            fogdistance = ( fogtop - fogbottom );
            if ( fogdistance < 1.0f )  fogon = bfalse;

            // Read extra stuff for damage tile particles...
            if ( goto_colon_yesno( fileread ) )
            {
                fscanf( fileread, "%d", &iTmp );  damagetileparttype = iTmp;
                goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
                damagetilepartand = iTmp;
                goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
                damagetilesound = CLIP(iTmp, -1, MAXWAVE);
            }
        }

        // Allow slow machines to ignore the fancy stuff
        if ( !twolayerwateron && numwaterlayer > 1 )
        {
            numwaterlayer = 1;
            iTmp = waterlayeralpha[0];
            iTmp = FP8_MUL( waterlayeralpha[1], iTmp ) + iTmp;
            if ( iTmp > 255 ) iTmp = 255;

            waterlayeralpha[0] = iTmp;
        }

        fclose( fileread );
        // Do it
        make_lighttable( lx, ly, lz, la );
        make_lighttospek();
        make_water();
    }
    else
    {
        log_error( "Could not read file! (wawalite.txt)\n" );
    }
}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
    // ZZ> This function draws the large background
    GLVERTEX vtlist[4];
    float size;
    float sinsize, cossize;
    float x, y, z, u, v;
    float loc_backgroundrepeat;
    Uint8 i;

    // Figure out the coordinates of its corners
    x = displaySurface->w << 6;
    y = displaySurface->h << 6;
    z = 0.99999f;
    size = x + y + 1;
    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
    u = waterlayeru[1];
    v = waterlayerv[1];
    loc_backgroundrepeat = backgroundrepeat * MIN( x / displaySurface->w, y / displaySurface->h );

    vtlist[0].x = x + cossize;
    vtlist[0].y = y - sinsize;
    vtlist[0].z = z;
    vtlist[0].s = 0 + u;
    vtlist[0].t = 0 + v;

    vtlist[1].x = x + sinsize;
    vtlist[1].y = y + cossize;
    vtlist[1].z = z;
    vtlist[1].s = loc_backgroundrepeat + u;
    vtlist[1].t = 0 + v;

    vtlist[2].x = x - cossize;
    vtlist[2].y = y + sinsize;
    vtlist[2].z = z;
    vtlist[2].s = loc_backgroundrepeat + u;
    vtlist[2].t = loc_backgroundrepeat + v;

    vtlist[3].x = x - sinsize;
    vtlist[3].y = y - cossize;
    vtlist[3].z = z;
    vtlist[3].s = 0 + u;
    vtlist[3].t = loc_backgroundrepeat + v;

    {
        GLint shading_save, depthfunc_save;
        GLboolean depthmask_save, cullface_save;

        glGetIntegerv( GL_SHADE_MODEL, &shading_save );
        glShadeModel( GL_FLAT );  // Flat shade this

        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
        glDepthMask( GL_FALSE );

        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
        glDepthFunc( GL_ALWAYS );

        cullface_save = glIsEnabled( GL_CULL_FACE );
        glDisable( GL_CULL_FACE );

        GLTexture_Bind( txTexture + texture );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin ( GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
            }
        }
        glEnd ();

        glDepthFunc( depthfunc_save );
        glDepthMask( depthmask_save );
        glShadeModel(shading_save);
        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
    }
}

//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
    // ZZ> This function draws the large foreground
    GLVERTEX vtlist[4];
    int i;
    float size;
    float sinsize, cossize;
    float x, y, z;
    float u, v;
    float loc_foregroundrepeat;

    // Figure out the screen coordinates of its corners
    x = displaySurface->w << 6;
    y = displaySurface->h << 6;
    z = 0;
    u = waterlayeru[1];
    v = waterlayerv[1];
    size = x + y + 1;
    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
    loc_foregroundrepeat = foregroundrepeat * MIN( x / displaySurface->w, y / displaySurface->h );

    vtlist[0].x = x + cossize;
    vtlist[0].y = y - sinsize;
    vtlist[0].z = z;
    vtlist[0].s = 0 + u;
    vtlist[0].t = 0 + v;

    vtlist[1].x = x + sinsize;
    vtlist[1].y = y + cossize;
    vtlist[1].z = z;
    vtlist[1].s = loc_foregroundrepeat + u;
    vtlist[1].t = v;

    vtlist[2].x = x - cossize;
    vtlist[2].y = y + sinsize;
    vtlist[2].z = z;
    vtlist[2].s = loc_foregroundrepeat + u;
    vtlist[2].t = loc_foregroundrepeat + v;

    vtlist[3].x = x - sinsize;
    vtlist[3].y = y - cossize;
    vtlist[3].z = z;
    vtlist[3].s = 0 + u;
    vtlist[3].t = loc_foregroundrepeat + v;

    {
        GLint shading_save, depthfunc_save, smoothhint_save;
        GLboolean depthmask_save, cullface_save, alphatest_save;

        GLint alphatestfunc_save, alphatestref_save, alphablendsrc_save, alphablenddst_save;
        GLboolean alphablend_save;

        glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &smoothhint_save);
        glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );             // make sure that the texture is as smooth as possible

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

        GLTexture_Bind(txTexture + texture);

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin ( GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
            }
        }
        glEnd ();

        glHint( GL_POLYGON_SMOOTH_HINT, smoothhint_save );
        glShadeModel( shading_save );
        glDepthMask( depthmask_save );
        glDepthFunc( depthfunc_save );
        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
        if (alphatest_save) glEnable( GL_ALPHA_TEST ); else glDisable( GL_ALPHA_TEST );

        glAlphaFunc( alphatestfunc_save, alphatestref_save );
        if (alphablend_save) glEnable( GL_BLEND ); else glDisable( GL_BLEND );

        glBlendFunc( alphablendsrc_save, alphablenddst_save );
    }
}

//--------------------------------------------------------------------------------------------
void render_shadow_sprite( float intensity, GLVERTEX v[] )
{
    int i;

    if ( intensity*255.0f < 1.0f ) return;

    glColor4f( intensity, intensity, intensity, 1.0f );

    glBegin( GL_TRIANGLE_FAN );
    {
        for ( i = 0; i < 4; i++ )
        {
            glTexCoord2fv ( &v[i].s );
            glVertex3fv ( &v[i].x );
        }
    }
    glEnd();
}


//--------------------------------------------------------------------------------------------
void render_shadow( Uint16 character )
{
    // ZZ> This function draws a NIFTY shadow
    GLVERTEX v[4];

    float x, y;
    float level;
    float height, size_umbra, size_penumbra;
    float alpha, alpha_umbra, alpha_penumbra;
    Sint8 hide;
    int i;
    chr_t * pchr;

    if ( character >= MAXCHR || !ChrList[character].on || ChrList[character].inpack ) return;
    pchr = ChrList + character;

    // if the character is hidden, not drawn at all, so no shadow
    hide = CapList[pchr->model].hidestate;
    if ( hide != NOHIDE && hide == pchr->ai.state ) return;

    // no shadow if off the mesh
    if ( INVALID_TILE == pchr->onwhichfan || FANOFF == meshtile[pchr->onwhichfan] ) return;

    // no shadow if completely transparent
    alpha = (pchr->alpha * INV_FF) * (pchr->light * INV_FF);
    if ( alpha * 255 < 1.0f ) return;

    // much resuced shadow if on a reflective tile
    if ( 0 != (meshfx[pchr->onwhichfan] & MESHFX_DRAWREF) )
    {
        alpha *= 0.1f;
    }
    if ( alpha * 255 < 1.0f ) return;

    // Original points
    level = pchr->level;
    level += SHADOWRAISE;
    height = pchr->matrix.CNV( 3, 2 ) - level;
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

    x = pchr->matrix.CNV( 3, 0 );
    y = pchr->matrix.CNV( 3, 1 );

    // Choose texture.
    GLTexture_Bind( txTexture + particletexture );

    // GOOD SHADOW
    v[0].s = particleimageu[238][0];
    v[0].t = particleimagev[238][0];

    v[1].s = particleimageu[255][1];
    v[1].t = particleimagev[238][0];

    v[2].s = particleimageu[255][1];
    v[2].t = particleimagev[255][1];

    v[3].s = particleimageu[238][0];
    v[3].t = particleimagev[255][1];

    if ( size_penumbra > 0 )
    {
        v[0].x = x + size_penumbra;
        v[0].y = y - size_penumbra;
        v[0].z = level;

        v[1].x = x + size_penumbra;
        v[1].y = y + size_penumbra;
        v[1].z = level;

        v[2].x = x - size_penumbra;
        v[2].y = y + size_penumbra;
        v[2].z = level;

        v[3].x = x - size_penumbra;
        v[3].y = y - size_penumbra;
        v[3].z = level;

        render_shadow_sprite(alpha_penumbra, v );
    };

    if ( size_umbra > 0 )
    {
        v[0].x = x + size_umbra;
        v[0].y = y - size_umbra;
        v[0].z = level + 0.1f;

        v[1].x = x + size_umbra;
        v[1].y = y + size_umbra;
        v[1].z = level + 0.1f;

        v[2].x = x - size_umbra;
        v[2].y = y + size_umbra;
        v[2].z = level + 0.1f;

        v[3].x = x - size_umbra;
        v[3].y = y - size_umbra;
        v[3].z = level + 0.1f;

        render_shadow_sprite(alpha_umbra, v );
    };

}

//--------------------------------------------------------------------------------------------
void render_bad_shadow( Uint16 character )
{
    // ZZ> This function draws a sprite shadow
    GLVERTEX v[4];
    float size, x, y;
    Uint8 ambi;
    float level, height, height_factor, alpha;
    Sint8 hide;
    int trans;
    int i;
    chr_t * pchr;

    if ( character >= MAXCHR || !ChrList[character].on || ChrList[character].inpack ) return;
    pchr = ChrList + character;

    // if the character is hidden, not drawn at all, so no shadow
    hide = CapList[pchr->model].hidestate;
    if ( hide != NOHIDE && hide == pchr->ai.state ) return;

    // no shadow if off the mesh
    if ( INVALID_TILE == pchr->onwhichfan || FANOFF == meshtile[pchr->onwhichfan] ) return;

    // no shadow if completely transparent or completely glowing
    alpha = (pchr->alpha * INV_FF) * (pchr->light * INV_FF);
    if ( alpha < INV_FF ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != (meshfx[pchr->onwhichfan] & MESHFX_DRAWREF) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->level;
    level += SHADOWRAISE;
    height = pchr->matrix.CNV( 3, 2 ) - level;
    height_factor = 1.0f - height / ( pchr->shadowsize * 5.0f );
    if ( height_factor <= 0.0f ) return;

    // how much transparency from height
    alpha *= height_factor * 0.5f + 0.25f;
    if ( alpha < INV_FF ) return;

    x = pchr->matrix.CNV( 3, 0 );
    y = pchr->matrix.CNV( 3, 1 );

    size = pchr->shadowsize * height_factor;

    v[0].x = ( float ) x + size;
    v[0].y = ( float ) y - size;
    v[0].z = ( float ) level;

    v[1].x = ( float ) x + size;
    v[1].y = ( float ) y + size;
    v[1].z = ( float ) level;

    v[2].x = ( float ) x - size;
    v[2].y = ( float ) y + size;
    v[2].z = ( float ) level;

    v[3].x = ( float ) x - size;
    v[3].y = ( float ) y - size;
    v[3].z = ( float ) level;

    // Choose texture and matrix
    GLTexture_Bind( txTexture + particletexture );

    v[0].s = particleimageu[236][0];
    v[0].t = particleimagev[236][0];

    v[1].s = particleimageu[253][1];
    v[1].t = particleimagev[236][0];

    v[2].s = particleimageu[253][1];
    v[2].t = particleimagev[253][1];

    v[3].s = particleimageu[236][0];
    v[3].t = particleimagev[253][1];

    render_shadow_sprite(trans * INV_FF, v );
}

//--------------------------------------------------------------------------------------------
void light_characters()
{
    // ZZ> This function figures out character lighting
    int cnt, tnc, ix, iy, light_min, light_max;
    Uint16 tl, tr, bl, br, itop, ibot;
    Uint32 light;

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];

        if ( INVALID_TILE == ChrList[tnc].onwhichfan )
        {
            ChrList[tnc].lightturnleftright = 0;
            ChrList[tnc].lightlevel_amb = 0;
            ChrList[tnc].lightlevel_dir = 0;
            continue;
        }

        // grab the corner intensities
        tl = meshvrtl[ meshvrtstart[ChrList[tnc].onwhichfan] + 0 ];
        tr = meshvrtl[ meshvrtstart[ChrList[tnc].onwhichfan] + 1 ];
        br = meshvrtl[ meshvrtstart[ChrList[tnc].onwhichfan] + 2 ];
        bl = meshvrtl[ meshvrtstart[ChrList[tnc].onwhichfan] + 3 ];

        // determine the amount of directionality
        light_min = MIN(MIN(tl, tr), MIN(bl, br));
        light_max = MAX(MAX(tl, tr), MAX(bl, br));

        if (light_max == 0 && light_min == 0 )
        {
            ChrList[tnc].lightturnleftright = 0;
            ChrList[tnc].lightlevel_amb = 0;
            ChrList[tnc].lightlevel_dir = 0;
            continue;
        }

        // Interpolate lighting level using tile corners
        ix = ((int)ChrList[tnc].xpos) & 127;
        iy = ((int)ChrList[tnc].ypos) & 127;

        itop = tl * (128 - ix) + tr * ix;
        ibot = bl * (128 - ix) + br * ix;
        light = (128 - iy) * itop + iy * ibot;
        light >>= 14;

        ChrList[tnc].lightlevel_dir = ( light * (light_max - light_min) ) / (light_max + light_min);
        ChrList[tnc].lightlevel_amb = light - ChrList[tnc].lightlevel_dir;

        if ( !meshexploremode && ChrList[tnc].lightlevel_dir > 0 )
        {
            // Look up light direction using corners again
            tl = ( tl << 8 ) & 0xf000;
            tr = ( tr << 4 ) & 0x0f00;
            br = ( br ) & 0x00f0;
            bl = bl >> 4;
            tl = tl | tr | br | bl;
            ChrList[tnc].lightturnleftright = ( lightdirectionlookup[tl] << 8 );
        }
        else
        {
            ChrList[tnc].lightturnleftright = 0;
        }
    }
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
    // ZZ> This function figures out particle lighting
    int iprt;
    int character;

    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        if ( !PrtList[iprt].on ) continue;

        character = PrtList[iprt].attachedtocharacter;
        if ( MAXCHR != character )
        {
            PrtList[iprt].light = ChrList[character].lightlevel_amb;
        }
        else if ( INVALID_TILE == PrtList[iprt].onwhichfan )
        {
            PrtList[iprt].light = 0;
        }
        else
        {
            int itmp = 0;
            Uint32 itile = PrtList[iprt].onwhichfan;

            itmp += meshvrtl[meshvrtstart[itile] + 0];
            itmp += meshvrtl[meshvrtstart[itile] + 1];
            itmp += meshvrtl[meshvrtstart[itile] + 2];
            itmp += meshvrtl[meshvrtstart[itile] + 3];

            PrtList[iprt].light = itmp / 4;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_fan_light( int fanx, int fany, Uint16 particle )
{
    // ZZ> This function is a little helper, lighting the selected fan
    //     with the chosen particle
    float x, y;
    int fan, vertex, lastvertex;
    float level;
    float light;

    if ( fanx >= 0 && fanx < meshtilesx && fany >= 0 && fany < meshtilesy )
    {
        // allow raw access because we were careful
        fan = fanx + meshfanstart[fany];

        vertex = meshvrtstart[fan];
        lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];

        while ( vertex < lastvertex )
        {
            light = meshvrta[vertex];
            x = PrtList[particle].xpos - meshvrtx[vertex];
            y = PrtList[particle].ypos - meshvrty[vertex];
            level = ( x * x + y * y ) / PrtList[particle].dynalightfalloff;
            level = 255 - level;
            level = level * PrtList[particle].dynalightlevel;
            if ( level > light )
            {
                if ( level > 255 ) level = 255;

                meshvrtl[vertex] = level;
                meshvrta[vertex] = level;
            }

            vertex++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_dynalight()
{
    // ZZ> This function does dynamic lighting of visible fans

    int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
    float x, y;
    float level;
    float light;

    // Do each floor tile
    if ( meshexploremode )
    {
        // Set base light level in explore mode...  Don't need to do every frame
        if ( ( frame_all & 7 ) == 0 )
        {
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( PrtList[cnt].on && PrtList[cnt].dynalighton )
                {
                    fanx = PrtList[cnt].xpos;
                    fany = PrtList[cnt].ypos;
                    fanx = fanx >> 7;
                    fany = fany >> 7;
                    addy = -DYNAFANS;

                    while ( addy <= DYNAFANS )
                    {
                        addx = -DYNAFANS;

                        while ( addx <= DYNAFANS )
                        {
                            set_fan_light( fanx + addx, fany + addy, cnt );
                            addx++;
                        }

                        addy++;
                    }
                }

                cnt++;
            }
        }
    }
    else if ( shading != GL_FLAT )
    {
        // Add to base light level in normal mode
        for ( entry = 0; entry < renderlist.all_count; entry++ )
        {
            fan = renderlist.all[entry];
            if ( INVALID_TILE == fan ) continue;

            vertex = meshvrtstart[fan];
            lastvertex = vertex + meshcommandnumvertices[meshtype[fan]];
            while ( vertex < lastvertex )
            {
                // Do light particles
                light = meshvrta[vertex];
                cnt = 0;

                while ( cnt < dyna_list_count )
                {
                    x = dyna_list[cnt].x - meshvrtx[vertex];
                    y = dyna_list[cnt].y - meshvrty[vertex];
                    level = ( x * x + y * y ) / dyna_list[cnt].falloff;
                    level = 255 - level;
                    if ( level > 0 )
                    {
                        light += level * dyna_list[cnt].level;
                    }

                    cnt++;
                }
                if ( light > 255 ) light = 255;
                if ( light < 0 ) light = 0;

                meshvrtl[vertex] = light;
                vertex++;
            }

        }
    }

}

//--------------------------------------------------------------------------------------------
void render_water()
{
    // ZZ> This function draws all of the water fans

    int cnt;

    // Set the transformation thing
    glLoadMatrixf( mView.v );
    glMultMatrixf( mWorld.v );

    // Bottom layer first
    if ( clearson && numwaterlayer > 1 && waterlayerz[1] > -waterlayeramp[1] )
    {
        cnt = 0;

        while ( cnt < renderlist.all_count )
        {
            if ( 0 != ( meshfx[renderlist.all[cnt]] & MESHFX_WATER ) )
            {
                render_water_fan( renderlist.all[cnt], 1 );
            }

            cnt++;
        }
    }

    // Top layer second
    if ( !overlayon && numwaterlayer > 0 && waterlayerz[0] > -waterlayeramp[0] )
    {
        cnt = 0;

        while ( cnt < renderlist.all_count )
        {
            if ( 0 != ( meshfx[renderlist.all[cnt]] & MESHFX_WATER ) )
            {
                render_water_fan( renderlist.all[cnt], 0 );
            }

            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_zreflection()
{
    // ZZ> This function draws 3D objects
    Uint16 cnt, tnc;
    Uint8 trans;

    // draw all tiles that do not reflect characters
    glDisable( GL_BLEND );             // no transparency
    glDisable( GL_CULL_FACE );

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glEnable( GL_ALPHA_TEST );         // use alpha test to allow the thatched roof tiles to look like thatch
    glAlphaFunc( GL_GREATER, 0 );

    meshlasttexture = ~0;
    for ( cnt = 0; cnt < renderlist.ndr_count; cnt++ )
    {
        render_fan( renderlist.ndr[cnt] );
    }

    if ( refon )
    {
        // draw the reflective tiles, but turn off the depth buffer
        // this blanks out any background that might've been drawn

        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        meshlasttexture = ~0;
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }

        // Render reflections of characters
        glEnable( GL_CULL_FACE );
        glFrontFace( GL_CCW );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glDepthFunc( GL_LEQUAL );

        for ( cnt = 0; cnt < numdolist; cnt++ )
        {
            tnc = dolist[cnt];
            if ( INVALID_TILE != ChrList[tnc].onwhichfan && (0 != ( meshfx[ChrList[tnc].onwhichfan]&MESHFX_DRAWREF )) )
            {
                render_refmad( tnc, FP8_MUL( ChrList[tnc].alpha, ChrList[tnc].light ) );
            }
        }

        // Render the reflected sprites
        glDisable( GL_CULL_FACE );
        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );
        glFrontFace( GL_CW );
        render_refprt();

        // Render the shadow floors ( let everything show through )
        // turn on the depth mask, so that no objects under the floor will show through
        // this assumes that the floor is not partially transparent...
        glDepthMask( GL_TRUE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        glDisable( GL_CULL_FACE );

        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );

        meshlasttexture = ~0;
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }

        glDisable( GL_BLEND );
        glDepthFunc( GL_LEQUAL );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );
    }
    else
    {
        // Render the shadow floors as normal solid floors
        meshlasttexture = ~0;
        for ( cnt = 0; cnt < renderlist.drf_count; cnt++ )
        {
            render_fan( renderlist.drf[cnt] );
        }
    }

    // Render the shadows
    if ( shaon )
    {
        glDepthMask( GL_FALSE );
        glEnable( GL_DEPTH_TEST );

        glEnable( GL_BLEND );
        glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

        if ( shasprite )
        {
            // Bad shadows
            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];
                if ( 0 == ChrList[tnc].shadowsize ) continue;

                render_bad_shadow( tnc );
            }
        }
        else
        {
            // Good shadows for me
            for ( cnt = 0; cnt < numdolist; cnt++ )
            {
                tnc = dolist[cnt];
                if ( 0 == ChrList[tnc].shadowsize ) continue;

                render_shadow( tnc );
            }
        }
    }

    // Render the normal characters
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];
        if ( ChrList[tnc].alpha == 255 && ChrList[tnc].light == 255 )
            render_mad( tnc, 255 );
    }

    // Now render the transparent characters
    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];
        if ( ChrList[tnc].alpha != 255 && ChrList[tnc].light == 255 )
        {
            trans = ChrList[tnc].alpha;
            if ( trans < SEEINVISIBLE && ( local_seeinvisible || ChrList[tnc].islocalplayer ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }
    }

    // And alpha water floors
    if ( !waterlight )
    {
        render_water();
    }

    // Then do the light characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
        tnc = dolist[cnt];
        if ( ChrList[tnc].light != 255 )
        {
            trans = ChrList[tnc].light;
            if ( trans < SEEINVISIBLE && ( local_seeinvisible || ChrList[tnc].islocalplayer ) )  trans = SEEINVISIBLE;

            render_mad( tnc, trans );
        }

        // Do phong highlights
        if ( phongon && ChrList[tnc].alpha == 255 && ChrList[tnc].light == 255 && !ChrList[tnc].enviro && ChrList[tnc].sheen > 0 )
        {
            Uint16 texturesave;
            ChrList[tnc].enviro = btrue;
            texturesave = ChrList[tnc].texture;
            ChrList[tnc].texture = TX_PHONG;  // The phong map texture...
            render_mad( tnc, ChrList[tnc].sheen << 4 );
            ChrList[tnc].texture = texturesave;
            ChrList[tnc].enviro = bfalse;
        }
    }

    // Do light water
    if ( waterlight )
    {
        render_water();
    }

    // Turn Z buffer back on, alphablend off
    glDepthMask( GL_FALSE );
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );
    render_prt();
    glDisable( GL_ALPHA_TEST );

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    // Done rendering
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x < meshedgex && pos_y >= 0.0f && pos_y < meshedgey )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 9;
        iy >>= 9;

        if ( iy < MAXMESHBLOCKY )
        {
            block = ix + meshblockstart[ iy ];
        }
    }

    return block;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < meshedgex && pos_y >= 0.0f && pos_y < meshedgey )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 7;
        iy >>= 7;

        if ( iy < MAXMESHTILEY )
        {
            tile = ix + meshfanstart[ iy ];
        }
    }

    return tile;
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

        GLTexture_Bind( &TxBlip );

        xl = ( ( float )bliprect[color].left ) / (float)TxBlip.txW;
        xr = ( ( float )bliprect[color].right ) / (float)TxBlip.txW;
        yt = ( ( float )bliprect[color].top ) / (float)TxBlip.txH;
        yb = ( ( float )bliprect[color].bottom ) / (float)TxBlip.txH;
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
    if ( icontype >= 0 && icontype < MAXTEXTURE + 1 )
    {
        EnableTexturing();    // Enable texture mapping
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        GLTexture_Bind( TxIcon + icontype );

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
        position = frame_wld & 31;
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
    //glNormal3f( 0.0f, 0.0f, 1.0f );

    GLTexture_Bind( &TxMap );

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
        GLTexture_Bind( &TxBars );

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
            GLTexture_Bind( &TxBars );

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
            GLTexture_Bind( &TxBars );

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
            GLTexture_Bind( &TxBars );

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
            GLTexture_Bind( &TxBars );

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
            GLTexture_Bind( &TxBars );

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

    GLTexture_Bind( &TxFont );

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
void draw_string(  const char *szText, int x, int y )
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
int length_of_word(  const char *szText )
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
int draw_wrap_string(  const char *szText, int x, int y, int maxx )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer,
    //     wrapping over the right side and returning the new y value
    int sttx = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = btrue;
    int cnt = 1;

    BeginText();

    maxx = maxx + sttx;

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
                x = sttx + fontyspacing;
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
                x = sttx;
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
    draw_one_icon( skintoicon[ChrList[character].texture], x + 40, y, ChrList[character].sparkle );

    item = ChrList[character].holdingwhich[0];
    if ( item != MAXCHR )
    {
        if ( ChrList[item].icon )
        {
            draw_one_icon( skintoicon[ChrList[item].texture], x + 8, y, ChrList[item].sparkle );
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
            draw_one_icon( bookicon[ ChrList[item].money % bookicon_count ], x + 8, y, ChrList[item].sparkle );
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

    item = ChrList[character].holdingwhich[1];
    if ( item != MAXCHR )
    {
        if ( ChrList[item].icon )
        {
            draw_one_icon( skintoicon[ChrList[item].texture], x + 72, y, ChrList[item].sparkle );
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
            draw_one_icon( bookicon[ ChrList[item].money % bookicon_count ], x + 72, y, ChrList[item].sparkle );
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
        for ( cnt = 0; cnt < numstat && y < displaySurface->h; cnt++ )
        {
            y = draw_status( statlist[cnt], displaySurface->w - BARX, y );
        }
    }

    // Map display
    if ( mapvalid && mapon )
    {
        draw_map( 0, displaySurface->h - MAPSIZE );

        //If one of the players can sense enemies via EMP, draw them as blips on the map
        if ( local_senseenemies )
        {
            Uint16 iTmp = 0;

            while ( numblip < MAXBLIP && iTmp < MAXCHR )
            {
                //Show only hated team
                if (ChrList[iTmp].on && TeamList[ChrList[local_senseenemies].team].hatesteam[ChrList[iTmp].team])
                {
                    //Only if they match the required IDSZ ([NONE] always works)
                    if ( local_senseenemiesID == Make_IDSZ("NONE")
                            || CapList[iTmp].idsz[IDSZ_PARENT] == local_senseenemiesID
                            || CapList[iTmp].idsz[IDSZ_TYPE] == local_senseenemiesID)
                    {
                        //Inside the map?
                        if ( ChrList[iTmp].xpos < meshedgex && ChrList[iTmp].ypos < meshedgey )
                        {
                            //Valid colors only
                            if ( numblip < NUMBLIP )
                            {
                                blipx[numblip] = ChrList[iTmp].xpos * MAPSIZE / meshedgex;
                                blipy[numblip] = ChrList[iTmp].ypos * MAPSIZE / meshedgey;
                                blipc[numblip] = 0; //Red blips
                                numblip++;
                            }
                        }
                    }
                }

                iTmp++;
            }
        }

        for ( cnt = 0; cnt < numblip; cnt++ )
        {
            draw_blip(0.75f, blipc[cnt], blipx[cnt], blipy[cnt] + displaySurface->h - MAPSIZE );
        }

        if ( youarehereon && ( frame_wld&8 ) )
        {
            for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
            {
                if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
                {
                    tnc = PlaList[cnt].index;
                    if ( ChrList[tnc].alive )
                    {
                        draw_blip( 0.75f, 0, ChrList[tnc].xpos*MAPSIZE / meshedgex, ( ChrList[tnc].ypos*MAPSIZE / meshedgey ) + displaySurface->h - MAPSIZE );
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
        if ( rtscontrol )
        {
            sprintf( text, "  Left Drag to select units" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left Click to order them" );
            draw_string( text, 0, y );  y += fontyspacing;

        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left Click to use an item" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Left and Right Click to grab" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Middle Click to jump" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  A and S keys do stuff" );
            draw_string( text, 0, y );  y += fontyspacing;
        }

        sprintf( text, "  Right Drag to move camera" );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
        // In-Game help
        sprintf( text, "!!!JOYSTICK HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        if ( rtscontrol )
        {
            sprintf( text, "  Joystick not available" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  Hit the buttons" );
            draw_string( text, 0, y );  y += fontyspacing;
            sprintf( text, "  You'll figure it out" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
    }
    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
        // In-Game help
        sprintf( text, "!!!KEYBOARD HELP!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        if ( rtscontrol )
        {
            sprintf( text, "  Keyboard not available" );
            draw_string( text, 0, y );  y += fontyspacing;
        }
        else
        {
            sprintf( text, "  Edit CONTROLS.TXT to change" );
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
    }
    if ( gDevMode && SDLKEYDOWN( SDLK_F5 ) )
    {
        // Debug information
        sprintf( text, "!!!DEBUG MODE-5!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "  CAM %f %f", gCamera.x, gCamera.y );
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
        sprintf( text, "  PLA0 %5.1f %5.1f", ChrList[tnc].xpos / 128.0f, ChrList[tnc].ypos / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
        tnc = PlaList[1].index;
        sprintf( text, "  PLA1 %5.1f %5.1f", ChrList[tnc].xpos / 128.0f, ChrList[tnc].ypos / 128.0f );
        draw_string( text, 0, y );  y += fontyspacing;
    }
    if ( gDevMode &&  SDLKEYDOWN( SDLK_F6 ) )
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
    if ( gDevMode && SDLKEYDOWN( SDLK_F7 ) )
    {
        // White debug mode
        sprintf( text, "!!!DEBUG MODE-7!!!" );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 0 ), mView.CNV( 1, 0 ), mView.CNV( 2, 0 ), mView.CNV( 3, 0 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 1 ), mView.CNV( 1, 1 ), mView.CNV( 2, 1 ), mView.CNV( 3, 1 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 2 ), mView.CNV( 1, 2 ), mView.CNV( 2, 2 ), mView.CNV( 3, 2 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "CAM %f %f %f %f", mView.CNV( 0, 3 ), mView.CNV( 1, 3 ), mView.CNV( 2, 3 ), mView.CNV( 3, 3 ) );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "x %f", gCamera.centerx );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "y %f", gCamera.centery );
        draw_string( text, 0, y );  y += fontyspacing;
        sprintf( text, "turn %d %d", autoturncamera, doturntime );
        draw_string( text, 0, y );  y += fontyspacing;
    }

    // Draw paused text
    if ( gamepaused && !SDLKEYDOWN( SDLK_F11 ) )
    {
        sprintf( text, "GAME PAUSED" );
        draw_string( text, -90 + displaySurface->w / 2, 0 + displaySurface->h / 2 );
    }

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
        if ( respawnvalid )
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

        snprintf( buffer, sizeof(buffer), "%s > %s%s", netmessagename, keyb.buffer, (0 == (frame_wld & 8)) ? "x" : "+" );

        y = draw_wrap_string( buffer, 0, y, displaySurface->w - wraptolerance );
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
                y = draw_wrap_string( msgtextdisplay[tnc], 0, y, displaySurface->w - wraptolerance );
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
void flip_pages()
{
    glFlush();

    lua_console_draw( our_lua_console );

    SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void draw_scene()
{
    Begin3DMode();

    make_prtlist();
    do_dynalight();
    light_characters();
    light_particles();

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Clear the image if need be
    if ( clearson )
    {
        glClear( GL_COLOR_BUFFER_BIT );
    }
    else
    {
        // Render the background
        render_background( TX_WATER_LOW );  // TX_WATER_LOW for waterlow.bmp
    }

    // no need to have a completely different function just to torn the reflections off and on
    {
        bool_t refon_save = refon;
        refon = refon && zreflect;

        draw_scene_zreflection();

        refon = refon_save;
    }

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Foreground overlay
    if ( overlayon )
    {
        render_foreground_overlay( TX_WATER_TOP );  // TX_WATER_TOP is watertop.bmp
    }

    End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_main()
{
    // ZZ> This function does all the drawing stuff
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    draw_scene();
    draw_text();
    flip_pages();
    frame_all++;
    frame_fps++;
}

//--------------------------------------------------------------------------------------------
int load_one_title_image( int titleimage,  const char *szLoadName )
{
    // ZZ> This function loads a title in the specified image slot, forcing it into
    //     system memory.  Returns btrue if it worked

    Uint32 tx_id;

    tx_id = GLTexture_Load(GL_TEXTURE_2D, TxTitleImage + titleimage, szLoadName, INVALID_KEY );

    return INVALID_TX_ID != tx_id;

}

//--------------------------------------------------------------------------------------------
bool_t load_blip_bitmap()
{
    // This function loads the blip bitmaps
    if ( INVALID_TX_ID == GLTexture_Load(GL_TEXTURE_2D, &TxBlip, "basicdat" SLASH_STR "blip", INVALID_KEY ) )
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
//  if ( INVALID_TX_ID != GLTexture_GetTextureID( TxTitleImage + image ) )
//  {
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 );  // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( TxTitleImage + image ) / GLTexture_GetDimensions( TxTitleImage + image ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( TxTitleImage + image ) / GLTexture_GetDimensions( TxTitleImage + image ) );
//
//    /* Bind the texture */
//    GLTexture_Bind( TxTitleImage + image );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    {
//        glTexCoord2f( 0, 1 );  glVertex2f( x, y + GLTexture_GetImageHeight( TxTitleImage + image ) );
//        glTexCoord2f( txWidth, 1 );  glVertex2f( x + GLTexture_GetImageWidth( TxTitleImage + image ), y + GLTexture_GetImageHeight( TxTitleImage + image ) );
//        glTexCoord2f( txWidth, 1 - txHeight );  glVertex2f( x + GLTexture_GetImageWidth( TxTitleImage + image ), y );
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

    cursorx = mous.x;  if ( cursorx < 6 )  cursorx = 6;  if ( cursorx > displaySurface->w - 16 )  cursorx = displaySurface->w - 16;

    cursory = mous.y;  if ( cursory < 8 )  cursory = 8;  if ( cursory > displaySurface->h - 24 )  cursory = displaySurface->h - 24;

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

int glinit( int argc, char **argv )
{
    /* Depth testing stuff */
    glClearDepth( 1.0f );
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );

    // Load the current graphical settings
    load_graphics();

    // fill mode
    glPolygonMode( GL_FRONT, GL_FILL );
    glPolygonMode( GL_BACK,  GL_FILL );

    /* Enable a single OpenGL light. */
    // glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
    // glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    // glEnable(GL_LIGHT0);
    glDisable( GL_LIGHTING );
    // glLightModelfv(GL_LIGHT_MODEL_AMBIENT,intensity);

    /* Backface culling */
    glEnable ( GL_CULL_FACE ); // This seems implied - DDOI
    glCullFace( GL_BACK );

    glEnable( GL_COLOR_MATERIAL );  // Need this for color + lighting
    EnableTexturing();    // Enable texture mapping

    return btrue;
}

void sdlinit( int argc, char **argv )
{
    Uint32  vflags = 0;
    int     colordepth;

    log_info ( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init(0) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
        atexit( SDL_Quit );
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

#ifdef __unix__

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // Unable to set video mode: Couldn't find matching GLX visual
    if ( scrd == 32 ) scrd = 24;

#endif

    colordepth = scrd / 3;

    /* Set the OpenGL Attributes */
#ifndef __unix__
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   colordepth );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, colordepth  );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  colordepth );
    if (scrd > colordepth * 3)
    {
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, scrd - colordepth * 3 );
    }
#endif
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, scrz );

    //Check if antialiasing is enabled
    if (antialiasing != bfalse)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, antialiasing);
    }

    log_info("Opening SDL Video Mode... ");

    vflags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_OPENGL; // basic flags
    vflags |= SDL_ASYNCBLIT | SDL_OPENGLBLIT;            // flags for the console
    vflags |= ( fullscreen ? SDL_FULLSCREEN : 0 );

    displaySurface = SDL_SetVideoMode( scrx, scry, scrd, vflags );
    if ( displaySurface == NULL )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to set video mode: %s\n", SDL_GetError() );
    }
    else
    {
        int i;

        log_message( "Success!\n" );

        SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &i );
        log_message("\tSDL_GL_DOUBLEBUFFER == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &i );
        log_message("\tSDL_GL_RED_SIZE     == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &i );
        log_message("\tSDL_GL_GREEN_SIZE   == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &i );
        log_message("\tSDL_GL_BLUE_SIZE    == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, &i );
        log_message("\tSDL_GL_ALPHA_SIZE   == %d\n", i);

        SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &i );
        log_message("\tSDL_GL_DEPTH_SIZE   == %d  (z-buffer depth)\n", i);
    }

    // Set the window name
    SDL_WM_SetCaption( "Egoboo", "Egoboo" );
    if ( gGrabMouse )
    {
        SDL_WM_GrabInput ( SDL_GRAB_ON );
    }
    if ( gHideMouse )
    {
        SDL_ShowCursor( 0 );  // Hide the mouse cursor
    }

    input_init();

    {
        SDL_Rect blah = {0, 0, scrx, scry / 4};
        our_lua_console = lua_console_new(NULL, blah);
    };
}

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

    // grab the current screen
    displaySurface = SDL_GetVideoSurface();

    // if we are not using OpenGl, jsut dump the screen
    if ( 0 == (displaySurface->flags & SDL_OPENGL) )
    {
        SDL_SaveBMP(displaySurface, szFilename);
        return bfalse;
    }

    // we ARE using OpenGL
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;
        char buff2[100];

        // create a SDL surface
        temp = SDL_CreateRGBSurface( SDL_SWSURFACE, displaySurface->w, displaySurface->h, 24,
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

            memcpy( &rect, &(displaySurface->clip_rect), sizeof(SDL_Rect) );
            if ( 0 == rect.w && 0 == rect.h )
            {
                rect.w = displaySurface->w;
                rect.h = displaySurface->h;
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
    if ( texturefilter == TX_ANISOTROPIC )
    {
        glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
        if ( !strstr( ( char* )glGetString( GL_EXTENSIONS ), "GL_EXT_texture_filter_anisotropic" ) )
        {
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
            texturefilter = TX_TRILINEAR_2; // Set filtering to trillienar instead
        }
    }

    // Enable prespective correction?
    if ( perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering? (This actually reduces quality but increases preformance)
    if ( dither ) glEnable( GL_DITHER );
    else glDisable( GL_DITHER );

    // Enable gourad shading? (Important!)
    glShadeModel( shading );

    // Enable antialiasing?
    if ( antialiasing != bfalse )
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
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi )
{
    // ZZ> This function helps make_lighttable
    float fTmp;
    float nx, ny, nz;
    float sinrot, cosrot;

    nx = kMd2Normals[normal][0];
    ny = kMd2Normals[normal][1];
    nz = kMd2Normals[normal][2];
    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];
    fTmp = cosrot * nx + sinrot * ny;
    ny = cosrot * ny - sinrot * nx;
    nx = fTmp;
    fTmp = nx * lx + ny * ly + nz * lz + ambi;
    if ( fTmp < ambi ) fTmp = ambi;

    return fTmp;
}


//---------------------------------------------------------------------------------------------
void make_lighttable( float lx, float ly, float lz, float ambi )
{
    // ZZ> This function makes a light table to fake directional lighting
    int lev, cnt, tnc;
    int itmp, itmptwo;

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
            lev = MAXLIGHTLEVEL - 1;
            itmp = ( 255 * light_for_normal( tnc,
                                             cnt,
                                             lx * lev / MAXLIGHTLEVEL,
                                             ly * lev / MAXLIGHTLEVEL,
                                             lz * lev / MAXLIGHTLEVEL,
                                             ambi ) );

            // This creates the light value for each level entry
            while ( lev >= 0 )
            {
                itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );
                if ( itmptwo > 255 )  itmptwo = 255;

                lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
                lev--;
            }
        }
    }

    // Fill in index number 162 for the spike mace
    for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
    {
        lev = MAXLIGHTLEVEL - 1;
        itmp = 255;

        // This creates the light value for each level entry
        while ( lev >= 0 )
        {
            itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );
            if ( itmptwo > 255 )  itmptwo = 255;

            lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
            lev--;
        }
    }
}


//--------------------------------------------------------------------------------------------
void make_enviro( void )
{
    // ZZ> This function sets up the environment mapping table
    int cnt;
    float z;
    float x, y;

    // Find the environment map positions
    for ( cnt = 0; cnt < MD2LIGHTINDICES; cnt++ )
    {
        x = kMd2Normals[cnt][0];
        y = kMd2Normals[cnt][1];
        x = ( ATAN2( y, x ) + PI ) / ( PI );
        x--;
        if ( x < 0 )
            x--;

        indextoenvirox[cnt] = x;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / 256.0f;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void project_view( camera_t * pcam )
{
    // ZZ> This function figures out where the corners of the view area
    //     go when projected onto the plane of the mesh.  Used later for
    //     determining which mesh fans need to be rendered

    int cnt, tnc, extra[2];
    float ztemp;
    float numstep;
    float zproject;
    float xfin, yfin, zfin;
    glMatrix mTemp;

    // make this camera's matricies the global matrices
    CopyMatrix( &mWorld,      &pcam->mWorld);                       // World Matrix
    CopyMatrix( &mView,       &pcam->mView);                         // View Matrix
    CopyMatrix( &mProjection, &pcam->mProjection);             // Projection Matrix

    // Range
    ztemp = ( pcam->z );

    // Topleft
    mTemp = MatrixMult( RotateY( -rotmeshtopside * PI / 360 ), mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[0] = xfin;
        cornery[0] = yfin;
    }

    // Topright
    mTemp = MatrixMult( RotateY( rotmeshtopside * PI / 360 ), mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[1] = xfin;
        cornery[1] = yfin;
    }

    // Bottomright
    mTemp = MatrixMult( RotateY( rotmeshbottomside * PI / 360 ), mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
        zfin = 0;
        cornerx[2] = xfin;
        cornery[2] = yfin;
    }

    // Bottomleft
    mTemp = MatrixMult( RotateY( -rotmeshbottomside * PI / 360 ), mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             //2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      //0,2
        yfin = pcam->y + ( numstep * mTemp.CNV( 1, 2 ) );    //1,2
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
void make_prtlist( void )
{
    // ZZ> This function figures out which particles are visible, and it sets up dynamic
    //     lighting
    int cnt, tnc, disx, disy, distance, slot;

    // Don't really make a list, just set to visible or not
    dyna_list_count = 0;
    dyna_distancetobeat = MAXDYNADIST;
    cnt = 0;

    while ( cnt < maxparticles )
    {
        PrtList[cnt].inview = bfalse;
        if ( PrtList[cnt].on && INVALID_TILE != PrtList[cnt].onwhichfan )
        {
            PrtList[cnt].inview = meshinrenderlist[PrtList[cnt].onwhichfan];

            // Set up the lights we need
            if ( PrtList[cnt].dynalighton )
            {
                disx = PrtList[cnt].xpos - gCamera.trackx;
                disx = ABS( disx );
                disy = PrtList[cnt].ypos - gCamera.tracky;
                disy = ABS( disy );
                distance = disx + disy;
                if ( distance < dyna_distancetobeat )
                {
                    if ( dyna_list_count < dyna_list_max )
                    {
                        // Just add the light
                        slot = dyna_list_count;
                        dyna_list[slot].distance = distance;
                        dyna_list_count++;
                    }
                    else
                    {
                        // Overwrite the worst one
                        slot = 0;
                        tnc = 1;
                        dyna_distancetobeat = dyna_list[0].distance;

                        while ( tnc < dyna_list_max )
                        {
                            if ( dyna_list[tnc].distance > dyna_distancetobeat )
                            {
                                slot = tnc;
                            }

                            tnc++;
                        }

                        dyna_list[slot].distance = distance;

                        // Find the new distance to beat
                        tnc = 1;
                        dyna_distancetobeat = dyna_list[0].distance;

                        while ( tnc < dyna_list_max )
                        {
                            if ( dyna_list[tnc].distance > dyna_distancetobeat )
                            {
                                dyna_distancetobeat = dyna_list[tnc].distance;
                            }

                            tnc++;
                        }
                    }

                    dyna_list[slot].x = PrtList[cnt].xpos;
                    dyna_list[slot].y = PrtList[cnt].ypos;
                    dyna_list[slot].level = PrtList[cnt].dynalightlevel;
                    dyna_list[slot].falloff = PrtList[cnt].dynalightfalloff;
                }
            }
        }

        cnt++;
    }
}

