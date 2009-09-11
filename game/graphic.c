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
#include "network.h"
#include "passage.h"
#include "menu.h"
#include "enchant.h"
#include "mad.h"
#include "script_compile.h"
#include "game.h"
#include "ui.h"
#include "texture.h"

#include "SDL_extensions.h"
#include "SDL_GL_extensions.h"

#include "egoboo_vfs.h"
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

#define GET_MAP_X(PMESH, POS_X) ( (POS_X)*MAPSIZE / PMESH->info.edge_x )
#define GET_MAP_Y(PMESH, POS_Y) ( (POS_Y)*MAPSIZE / PMESH->info.edge_y ) + sdl_scr.y - MAPSIZE

//--------------------------------------------------------------------------------------------
// Lightning effects

#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTAL_MAX_DYNA                    64          // Absolute max number of dynamic lights

struct s_dynalight
{
    float distance;      // The distances
    float x;             // Light position
    float y;
    float z;
    float level;         // Light intensity
    float falloff;       // Light radius
};

typedef struct s_dynalight dynalight_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t _sdl_atexit_registered    = bfalse;
static bool_t _sdl_initialized_graphics = bfalse;
static bool_t _sdl_initialized_base     = bfalse;
static bool_t _ogl_initialized          = bfalse;

static Uint8 asciitofont[256];                                   // Conversion table

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

static float       dyna_distancetobeat;           // The number to beat
static int         dyna_list_count = 0;           // Number of dynamic lights
static dynalight_t dyna_list[TOTAL_MAX_DYNA];

// Interface stuff
static rect_t             iconrect;                   // The 32x32 icon rectangle

static int                fontoffset;                 // Line up fonts from top of screen
static SDL_Rect           fontrect[NUMFONT];          // The font rectangles
static Uint8              fontxspacing[NUMFONT];      // The spacing stuff
static Uint8              fontyspacing;

static rect_t             tabrect[NUMBAR];            // The tab rectangles
static rect_t             barrect[NUMBAR];            // The bar rectangles
static rect_t             bliprect[COLOR_MAX];        // The blip rectangles
static rect_t             maprect;                    // The map rectangle

static bool_t             gfx_page_flip_requested  = bfalse;
static bool_t             gfx_page_clear_requested = btrue;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

gfx_config_t     gfx;

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

size_t                dolist_count = 0;
obj_registry_entity_t dolist[DOLIST_SIZE];

bool_t           meshnotexture   = bfalse;
Uint16           meshlasttexture = (Uint16)(~0);

renderlist_t     renderlist = {0, 0, 0, 0, 0};

float            light_a = 0.0f, light_d = 0.0f, light_x = 0.0f, light_y = 0.0f, light_z = 0.0f;
Uint8            lightdirectionlookup[65536];
float            lighttable_local[MAXLIGHTROTATION][MADLIGHTINDICES];
float            lighttable_global[MAXLIGHTROTATION][MADLIGHTINDICES];
float            indextoenvirox[MADLIGHTINDICES];
float            lighttoenviroy[256];
Uint32           lighttospek[MAXSPEKLEVEL][256];

int rotmeshtopside;
int rotmeshbottomside;
int rotmeshup;
int rotmeshdown;

Uint8   mapon         = bfalse;
Uint8   mapvalid      = bfalse;
Uint8   youarehereon  = bfalse;
Uint16  numblip       = 0;
Uint16  blipx[MAXBLIP];
Uint16  blipy[MAXBLIP];
Uint8   blipc[MAXBLIP];

Uint16  msgtimechange = 0;

DECLARE_STACK( extern, msg_t, DisplayMsg );

line_data_t line_list[LINE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void sdlinit_base();
static void sdlinit_graphics();

static void flip_pages();

static void project_view( camera_t * pcam );
static void make_dynalist( camera_t * pcam );

static int _draw_string_raw( int x, int y, const char *format, ...  );

static bool_t sum_dyna_lighting( dynalight_t * pdyna, float lighting[], float dx, float dy, float dz );

static void init_icon_data();
static void init_bar_data();
static void init_blip_data();
static void init_map_data();

DECLARE_LIST ( ACCESS_TYPE_NONE, billboard_data_t, BillboardList );

static bool_t render_billboard( struct s_camera * pcam, billboard_data_t * pbb, float scale );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EnableTexturing()
{
    if ( !GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D ) )
    {
        GL_DEBUG(glEnable)(GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void DisableTexturing()
{
    if ( GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D ) )
    {
        GL_DEBUG(glDisable)(GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
void move_water( void )
{
    // ZZ> This function animates the water overlays
    int layer;

    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
    {
        water.layer[layer].tx.x += water.layer[layer].tx_add.x;
        water.layer[layer].tx.y += water.layer[layer].tx_add.y;

        if ( water.layer[layer].tx.x >  1.0f )  water.layer[layer].tx.x -= 1.0f;
        if ( water.layer[layer].tx.y >  1.0f )  water.layer[layer].tx.y -= 1.0f;
        if ( water.layer[layer].tx.x < -1.0f )  water.layer[layer].tx.x += 1.0f;
        if ( water.layer[layer].tx.y < -1.0f )  water.layer[layer].tx.y += 1.0f;

        water.layer[layer].frame = ( water.layer[layer].frame + water.layer[layer].frame_add ) & WATERFRAMEAND;
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
    //    The table is used to find which direction the light is coming
    //    from, based on the four corner vertices of a mesh tile.
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
int DisplayMsg_get_free()
{
    // This function finds the best message to use
    // Pick the first one

    int tnc = DisplayMsg.count;

    DisplayMsg.count++;
    DisplayMsg.count = DisplayMsg.count % maxmessage;

    return tnc;
}

//--------------------------------------------------------------------------------------------
void display_message( script_state_t * pstate, int message, Uint16 character )
{
    // ZZ> This function sticks a message in the display queue and sets its timer

    int slot, read;

    if ( message < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        slot = DisplayMsg_get_free();
        DisplayMsg.lst[slot].time = cfg.message_duration;

        // Copy the message
        read = MessageOffset.lst[message];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = DisplayMsg.lst[slot].textdisplay;
        dst_end = DisplayMsg.lst[slot].textdisplay + MESSAGESIZE - 1;

        expand_escape_codes( character, pstate, src, src_end, dst, dst_end );

        *dst_end = '\0';
    }
}

//--------------------------------------------------------------------------------------------
// This needs work
void Begin3DMode( camera_t * pcam )
{
    GL_DEBUG(glMatrixMode)(GL_PROJECTION );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glLoadMatrixf)(pcam->mProjection.v );

    GL_DEBUG(glMatrixMode)(GL_MODELVIEW );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glLoadMatrixf)(pcam->mView.v );
}

void End3DMode()
{
    GL_DEBUG(glMatrixMode)(GL_MODELVIEW );
    GL_DEBUG(glPopMatrix)();

    GL_DEBUG(glMatrixMode)(GL_PROJECTION );
    GL_DEBUG(glPopMatrix)();
}

/********************> Begin2DMode() <*****/
void Begin2DMode( void )
{
    GL_DEBUG(glMatrixMode)(GL_PROJECTION );
    GL_DEBUG(glLoadIdentity)();                  // Reset The Projection Matrix
    GL_DEBUG(glOrtho)(0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );        // Set up an orthogonal projection

    GL_DEBUG(glMatrixMode)(GL_MODELVIEW );
    GL_DEBUG(glLoadIdentity)();

    GL_DEBUG(glDisable)(GL_DEPTH_TEST );
    GL_DEBUG(glDisable)(GL_CULL_FACE );
}

/********************> End2DMode() <*****/
void End2DMode( void )
{
    GL_DEBUG(glEnable)(GL_CULL_FACE );
    GL_DEBUG(glEnable)(GL_DEPTH_TEST );
}

//--------------------------------------------------------------------------------------------
// ZF> Load all the global icons used in all modules
bool_t load_all_global_icons()
{
    // Setup
    bool_t result = bfalse;

    // Now load every icon
    result = TxTexture_load_one( "basicdat" SLASH_STR "nullicon", ICON_NULL, INVALID_KEY );
    result = TxTexture_load_one( "basicdat" SLASH_STR "keybicon", ICON_KEYB, INVALID_KEY );
    result = TxTexture_load_one( "basicdat" SLASH_STR "mousicon", ICON_MOUS, INVALID_KEY );
    result = TxTexture_load_one( "basicdat" SLASH_STR "joyaicon", ICON_JOYA, INVALID_KEY );
    result = TxTexture_load_one( "basicdat" SLASH_STR "joybicon", ICON_JOYB, INVALID_KEY );

    return result;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_icon_data()
{
    // ZZ> This function sets the icon pointers to NULL
    int cnt;

    iconrect.left = 0;
    iconrect.right = 32;
    iconrect.top = 0;
    iconrect.bottom = 32;

    for ( cnt = 0; cnt < MAXSKIN; cnt++ )
    {
        bookicon_ref[cnt] = INVALID_TEXTURE;
    }
    bookicon_count = 0;
}

//---------------------------------------------------------------------------------------------
void init_bar_data()
{
    Uint8 cnt;

    // Initialize the life and mana bars
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
void init_blip_data()
{
    int cnt;

    // Set up the rectangles
    for ( cnt = 0; cnt < COLOR_MAX; cnt++ )
    {
        bliprect[cnt].left   = cnt * BLIPSIZE;
        bliprect[cnt].right  = cnt * BLIPSIZE + BLIPSIZE;
        bliprect[cnt].top    = 0;
        bliprect[cnt].bottom = BLIPSIZE;
    }

    youarehereon = bfalse;
    numblip      = 0;
}

//---------------------------------------------------------------------------------------------
void init_map_data()
{
    // ZZ> This function releases all the map images

    // Set up the rectangles
    maprect.left   = 0;
    maprect.right  = MAPSIZE;
    maprect.top    = 0;
    maprect.bottom = MAPSIZE;

    mapvalid = bfalse;
    mapon    = bfalse;
}

//---------------------------------------------------------------------------------------------
void init_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();
    font_init();

    BillboardList_free_all();
    TxTexture_init_all();
}

//---------------------------------------------------------------------------------------------
void release_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();

    BillboardList_free_all();
    TxTexture_release_all();
}

//--------------------------------------------------------------------------------------------
void delete_all_graphics()
{
    init_icon_data();
    init_bar_data();
    init_blip_data();
    init_map_data();

    BillboardList_free_all();
    TxTexture_delete_all();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void _debug_print( const char *text )
{
    // ZZ> This function sticks a message in the display queue and sets its timer

    int          slot;
    const char * src;
    char       * dst, * dst_end;
    msg_t      * pmsg;

    if( INVALID_CSTR(text) ) return;

    // Get a "free" message
    slot = DisplayMsg_get_free();
    pmsg = DisplayMsg.lst + slot;

    // Copy the message
    for ( src = text, dst = pmsg->textdisplay, dst_end = dst + MESSAGESIZE;
          '\0' != *src && dst < dst_end;
          src++, dst++)
    {
        *dst = *src;
    }
    if( dst < dst_end ) *dst = '\0';

    // Set the time
    pmsg->time = cfg.message_duration;
}

//--------------------------------------------------------------------------------------------
int _debug_vprintf( const char *format, va_list args )
{
    int retval = 0;

    if( VALID_CSTR(format) )
    {
        STRING szTmp;

        retval = vsnprintf( szTmp, SDL_arraysize(szTmp), format, args );
        _debug_print( szTmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int debug_printf( const char *format, ... )
{
    va_list args;
    int retval;

    va_start( args, format );
    retval = _debug_vprintf( format, args );
    va_end( args );

    return retval;
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
void reset_renderlist()
{
    // BB> Clear old render lists

    if ( NULL != renderlist.pmesh )
    {
        int cnt;

        // clear out the inrenderlist flag for the old mesh
        tile_info_t * tlist = renderlist.pmesh->mmem.tile_list;

        for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
        {
            Uint32 fan = renderlist.all[cnt];
            if ( fan < renderlist.pmesh->info.tiles_count )
            {
                tlist[fan].inrenderlist = bfalse;
            }
        }

        renderlist.pmesh = NULL;
    }

    renderlist.all_count = 0;
    renderlist.ref_count = 0;
    renderlist.sha_count = 0;
    renderlist.drf_count = 0;
    renderlist.ndr_count = 0;
}

//--------------------------------------------------------------------------------------------
void make_renderlist( ego_mpd_t * pmesh, camera_t * pcam )
{
    // ZZ> This function figures out which mesh fans to draw
    int cnt, fanx, fany;
    int row, run, numrow;
    int xlist[4], ylist[4];
    int leftnum, leftlist[4];
    int rightnum, rightlist[4];
    int fanrowstart[128], fanrowrun[128];
    int x, stepx, divx, basex;
    int from, to;

    tile_info_t * tlist;

    // reset the current renderlist
    reset_renderlist();

    // Make sure it doesn't die ugly !!!BAD!!!
    if ( NULL == pcam ) return;

    // Find the render area corners
    project_view( pcam );

    if ( NULL == pmesh ) return;

    renderlist.pmesh = pmesh;
    tlist = pmesh->mmem.tile_list;

    // It works better this way...
    cornery[cornerlistlowtohighy[3]] += 256;

    // Make life simpler
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        xlist[cnt] = cornerx[cornerlistlowtohighy[cnt]];
        ylist[cnt] = cornery[cornerlistlowtohighy[cnt]];
    }

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
            if ( fany >= 0 && fany < pmesh->info.tiles_y )
            {
                fanx = x >> TILE_BITS;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= pmesh->info.tiles_x )  fanx = pmesh->info.tiles_x - 1;

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
            if ( fany >= 0 && fany < pmesh->info.tiles_y )
            {
                fanx = x >> TILE_BITS;
                if ( fanx < 0 )  fanx = 0;
                if ( fanx >= pmesh->info.tiles_x - 1 )  fanx = pmesh->info.tiles_x - 1;// -2

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
    if ( fany >= pmesh->info.tiles_y ) fany = pmesh->info.tiles_y - 1;

    for ( row = 0; row < numrow; row++, fany++ )
    {
        cnt = pmesh->gmem.tilestart[fany] + fanrowstart[row];

        run = fanrowrun[row];
        for ( fanx = 0; fanx < run && renderlist.all_count < MAXMESHRENDER; fanx++, cnt++ )
        {
            // Put each tile in basic list
            tlist[cnt].inrenderlist = btrue;

            renderlist.all[renderlist.all_count] = cnt;
            renderlist.all_count++;

            // Put each tile in one other list, for shadows and relections
            if ( 0 != mesh_test_fx( pmesh, cnt, MPDFX_SHA ) )
            {
                renderlist.sha[renderlist.sha_count] = cnt;
                renderlist.sha_count++;
            }
            else
            {
                renderlist.ref[renderlist.ref_count] = cnt;
                renderlist.ref_count++;
            }

            if ( 0 != mesh_test_fx( pmesh, cnt, MPDFX_DRAWREF ) )
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
    Uint32 i;

    for ( i = 0; i < dolist_count; i++)
    {
        Uint16 ichr = dolist[i].ichr;

        if ( INVALID_CHR(ichr) ) continue;

        // Do flashing
        if ( HAS_NO_BITS( frame_all, ChrList.lst[ichr].flashand ) && ChrList.lst[ichr].flashand != DONTFLASH )
        {
            flash_character( ichr, 255 );
        }

        // Do blacking
        if ( HAS_NO_BITS( frame_all, SEEKURSEAND ) && local_seekurse && ChrList.lst[ichr].iskursed )
        {
            flash_character( ichr, 0 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void flash_character( Uint16 character, Uint8 value )
{
    // ZZ> This function sets a character's lighting

    ChrList.lst[character].inst.color_amb = value;
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
    // This function changes the animated tile frame
    if ( ( update_wld & animtile_update_and ) == 0 )
    {
        animtile[0].frame_add = ( animtile[0].frame_add + 1 ) & animtile[0].frame_and;
        animtile[1].frame_add = ( animtile[1].frame_add + 1 ) & animtile[1].frame_and;
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( const char *modname )
{
    // ZZ> This function loads the standard textures for a module
    STRING newloadname;

    // Particle sprites
    TxTexture_load_one( "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle_trans", TX_PARTICLE_TRANS, TRANSCOLOR );
    TxTexture_load_one( "basicdat" SLASH_STR "globalparticles" SLASH_STR "particle_light", TX_PARTICLE_LIGHT, INVALID_KEY );

    // Module background tiles
    make_newloadname( modname, "gamedat" SLASH_STR "tile0", newloadname );
    TxTexture_load_one( newloadname, TX_TILE_0, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile1", newloadname );
    TxTexture_load_one( newloadname, TX_TILE_1, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "tile2", newloadname );
    TxTexture_load_one( newloadname, TX_TILE_2, TRANSCOLOR);

    make_newloadname( modname, "gamedat" SLASH_STR "tile3", newloadname );
    TxTexture_load_one( newloadname, TX_TILE_3, TRANSCOLOR );

    // Water textures
    make_newloadname( modname, "gamedat" SLASH_STR "watertop", newloadname );
    TxTexture_load_one( newloadname, TX_WATER_TOP, TRANSCOLOR );

    make_newloadname( modname, "gamedat" SLASH_STR "waterlow", newloadname );
    TxTexture_load_one( newloadname, TX_WATER_LOW, TRANSCOLOR);

    // Texture 7 is the phong map
    TxTexture_load_one( "basicdat" SLASH_STR "phong", TX_PHONG, TRANSCOLOR );
}

//--------------------------------------------------------------------------------------------
void load_bars()
{
    // ZZ> This function loads the status bar bitmap

    const char * pname;

    pname = "basicdat" SLASH_STR "bars";
    if ( INVALID_TEXTURE == TxTexture_load_one( pname, TX_BARS, TRANSCOLOR ) )
    {
        log_warning( "load_bars() - Cannot load file! (\"%s\")\n", pname );
    }

    pname = "basicdat" SLASH_STR "xpbar";
    if ( INVALID_TEXTURE == TxTexture_load_one( pname, TX_XP_BAR, TRANSCOLOR ) )
    {
        log_warning( "load_bars() - Cannot load file! (\"%s\")\n", pname );
    }
}

//--------------------------------------------------------------------------------------------
void load_map( const char* szModule )
{
    // ZZ> This function loads the map bitmap
    STRING szMap;

    // Turn it all off
    mapvalid = bfalse;
    mapon = bfalse;
    youarehereon = bfalse;
    numblip = 0;

    // Load the images
    snprintf( szMap, SDL_arraysize( szMap), "%sgamedat" SLASH_STR "plan", szModule );

    if ( INVALID_TEXTURE == TxTexture_load_one( szMap, TX_MAP, INVALID_KEY ) )
    {
        log_warning( "load_map() - Cannot load file! (\"%s\")\n", szMap );
    }
    else
    {
        mapvalid = btrue;
    }

}

//--------------------------------------------------------------------------------------------
void font_init()
{
    // BB > fill in default values

    Uint16 i, ix, iy, cnt;
    float dx, dy;

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
    //    of each font on that bitmap...  Bitmap must have 16x6 fonts
    int cnt, y, xsize, ysize, xdiv, ydiv;
    int stt_x, stt_y;
    int xspacing, yspacing;
    char cTmp;
    vfs_FILE *fileread;

    font_init();
    if ( INVALID_TEXTURE == TxTexture_load_one( szBitmap, TX_FONT, TRANSCOLOR ) )
    {
        log_error( "load_font() - Cannot load file! (\"%s\")\n", szBitmap );
    }

    // Get the size of the bitmap
    xsize = oglx_texture_GetImageWidth( TxTexture_get_ptr( TX_FONT ) );
    ysize = oglx_texture_GetImageHeight( TxTexture_get_ptr( TX_FONT ) );
    if ( xsize == 0 || ysize == 0 )
    {
        log_error( "Bad font size! (%i, %i)\n", xsize, ysize );
    }

    // Figure out the general size of each font
    ydiv = ysize / NUMFONTY;
    xdiv = xsize / NUMFONTX;

    // Figure out where each font is and its spacing
    parse_filename = "";
    fileread = vfs_openRead( szSpacing );
    if ( fileread == NULL )
    {
        log_error( "Font spacing not avalible! (%i, %i)\n", xsize, ysize );
    }
    parse_filename = szSpacing;

    y = 0;
    stt_x = 0;
    stt_y = 0;

    // Uniform font height is at the top
    yspacing = fget_next_int( fileread );
    fontoffset = yspacing;
    for ( cnt = 0; cnt < NUMFONT && goto_colon( NULL, fileread, btrue ); cnt++ )
    {
        vfs_scanf( fileread, "%c", &cTmp );
        xspacing = fget_int( fileread );
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
    vfs_close( fileread );
    parse_filename = "";

    // Space between lines
    fontyspacing = ( yspacing >> 1 ) + FONTADD;
}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
    // ZZ> This function draws the large background
    GLvertex vtlist[4];
    int i;
    float z0, d, mag0, mag1, Qx, Qy;
    float light = 1.0f, intens = 1.0f, alpha = 1.0f;

    ego_mpd_info_t * pinfo;
    oglx_texture   * ptex;

    water_instance_layer_t * ilayer = water.layer      + 0;

    z0 = 1500; // the original height of the camera
    d = MIN(ilayer->dist.x, ilayer->dist.y) / 10.0f;
    mag0 = 1.0f / (1.0f + z0 * d);
    // mag1 = backgroundrepeat/128.0f/10;
    mag1 = 1.0f / 128.0f / 5.0f;

    // clip the waterlayer uv offset
    ilayer->tx.x = ilayer->tx.x - (float)floor(ilayer->tx.x);
    ilayer->tx.y = ilayer->tx.y - (float)floor(ilayer->tx.y);

    pinfo = &(PMesh->info);

    // Figure out the coordinates of its corners
    Qx = -pinfo->edge_x;
    Qy = -pinfo->edge_y;
    vtlist[0].pos[XX] = mag0 * PCamera->pos.x + Qx * ( 1.0f - mag0 );
    vtlist[0].pos[YY] = mag0 * PCamera->pos.y + Qy * ( 1.0f - mag0 );
    vtlist[0].pos[ZZ] = 0;
    vtlist[0].tex[SS] = Qx * mag1 + ilayer->tx.x;
    vtlist[0].tex[TT] = Qy * mag1 + ilayer->tx.y;

    Qx = 2 * pinfo->edge_x;
    Qy = -pinfo->edge_y;
    vtlist[1].pos[XX] = mag0 * PCamera->pos.x + Qx * ( 1.0f - mag0 );
    vtlist[1].pos[YY] = mag0 * PCamera->pos.y + Qy * ( 1.0f - mag0 );
    vtlist[1].pos[ZZ] = 0;
    vtlist[1].tex[SS] = Qx * mag1 + ilayer->tx.x;
    vtlist[1].tex[TT] = Qy * mag1 + ilayer->tx.y;

    Qx = 2 * pinfo->edge_x;
    Qy = 2 * pinfo->edge_y;
    vtlist[2].pos[XX] = mag0 * PCamera->pos.x + Qx * ( 1.0f - mag0 );
    vtlist[2].pos[YY] = mag0 * PCamera->pos.y + Qy * ( 1.0f - mag0 );
    vtlist[2].pos[ZZ] = 0;
    vtlist[2].tex[SS] = Qx * mag1 + ilayer->tx.x;
    vtlist[2].tex[TT] = Qy * mag1 + ilayer->tx.y;

    Qx = -pinfo->edge_x;
    Qy = 2 * pinfo->edge_y;
    vtlist[3].pos[XX] = mag0 * PCamera->pos.x + Qx * ( 1.0f - mag0 );
    vtlist[3].pos[YY] = mag0 * PCamera->pos.y + Qy * ( 1.0f - mag0 );
    vtlist[3].pos[ZZ] = 0;
    vtlist[3].tex[SS] = Qx * mag1 + ilayer->tx.x;
    vtlist[3].tex[TT] = Qy * mag1 + ilayer->tx.y;

    light = water.light ? 1.0f : 0.0f;
    alpha = ilayer->alpha * INV_FF;

    if ( gfx.usefaredge )
    {
        float fcos;

        intens = light_a * ilayer->light_add;

        fcos = light_z;
        if (fcos > 0.0f)
        {
            intens += fcos * fcos * light_d * ilayer->light_dir;
        }

        intens = CLIP(intens, 0.0f, 1.0f);
    }

    ptex = TxTexture_get_ptr( texture );

    ATTRIB_PUSH( "render_background()", GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT );
    {
        oglx_texture_Bind ( ptex );

        GL_DEBUG(glShadeModel)( GL_FLAT );   // GL_LIGHTING_BIT - Flat shade this
        GL_DEBUG(glDepthMask)( GL_FALSE );   // GL_DEPTH_BUFFER_BIT
        GL_DEBUG(glDepthFunc)( GL_ALWAYS );  // GL_DEPTH_BUFFER_BIT
        GL_DEBUG(glDisable)( GL_CULL_FACE ); // GL_POLYGON_BIT GL_ENABLE_BIT

        GL_DEBUG(glColor4f)( intens, intens, intens, alpha );
        GL_DEBUG(glBegin)( GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG(glTexCoord2fv)(vtlist[i].tex );
                GL_DEBUG(glVertex3fv)(vtlist[i].pos );
            }
        }
        GL_DEBUG_END();

        if ( light > 0.0f )
        {
            ATTRIB_PUSH( "render_background() - glow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG(glEnable)( GL_BLEND );                             // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
                GL_DEBUG(glBlendFunc)( GL_ONE_MINUS_SRC_ALPHA, GL_ONE );    // GL_COLOR_BUFFER_BIT

                GL_DEBUG(glColor4f)( light, light, light, 1.0f );

                GL_DEBUG(glBegin)( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        GL_DEBUG(glTexCoord2fv)(vtlist[i].tex );
                        GL_DEBUG(glVertex3fv)(vtlist[i].pos );
                    }
                }
                GL_DEBUG_END();
            }
            ATTRIB_POP( "render_background() - glow" );
        }
    }
    ATTRIB_POP("render_background()");
}

//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
    // ZZ> This function draws the large foreground

    float alpha, ftmp;
    GLvector3 vforw_wind, vforw_cam;

    oglx_texture           * ptex;

    water_instance_layer_t * ilayer = water.layer      + 1;

    vforw_wind.x = ilayer->tx_add.x;
    vforw_wind.y = ilayer->tx_add.y;
    vforw_wind.z = 0;
    vforw_wind = VNormalize( vforw_wind );

    vforw_cam  = mat_getCamForward( PCamera->mView );

    // make the texture begin to disappear if you are not looking straight down
    ftmp = VDotProduct( vforw_wind, vforw_cam );

    alpha = (1.0f - ftmp * ftmp) * (ilayer->alpha * INV_FF);

    if ( alpha != 0.0f )
    {
        GLvertex vtlist[4];
        int i;
        float size;
        float sinsize, cossize;
        float x, y, z;
        float loc_foregroundrepeat;

        // Figure out the screen coordinates of its corners
        x = sdl_scr.x << 6;
        y = sdl_scr.y << 6;
        z = 0;
        size = x + y + 1;
        sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
        cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
        loc_foregroundrepeat = water.foregroundrepeat * MIN( x / sdl_scr.x, y / sdl_scr.x );

        vtlist[0].pos[XX] = x + cossize;
        vtlist[0].pos[YY] = y - sinsize;
        vtlist[0].pos[ZZ] = z;
        vtlist[0].tex[SS] = ilayer->tx.x;
        vtlist[0].tex[TT] = ilayer->tx.y;

        vtlist[1].pos[XX] = x + sinsize;
        vtlist[1].pos[YY] = y + cossize;
        vtlist[1].pos[ZZ] = z;
        vtlist[1].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[1].tex[TT] = ilayer->tx.y;

        vtlist[2].pos[XX] = x - cossize;
        vtlist[2].pos[YY] = y + sinsize;
        vtlist[2].pos[ZZ] = z;
        vtlist[2].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[2].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        vtlist[3].pos[XX] = x - sinsize;
        vtlist[3].pos[YY] = y - cossize;
        vtlist[3].pos[ZZ] = z;
        vtlist[3].tex[SS] = ilayer->tx.x;
        vtlist[3].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        ptex = TxTexture_get_ptr( texture );

        ATTRIB_PUSH( "render_foreground_overlay()", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST );             // GL_HINT_BIT make sure that the texture is as smooth as possible

            GL_DEBUG(glShadeModel)( GL_FLAT );      // GL_LIGHTING_BIT - Flat shade this

            GL_DEBUG(glDepthMask)( GL_FALSE );     // GL_DEPTH_BUFFER_BIT
            GL_DEBUG(glDepthFunc)( GL_ALWAYS );     // GL_DEPTH_BUFFER_BIT

            GL_DEBUG(glDisable)( GL_CULL_FACE );   // GL_POLYGON_BIT GL_ENABLE_BIT

            GL_DEBUG(glEnable)( GL_ALPHA_TEST );   // GL_COLOR_BUFFER_BIT GL_ENABLE_BIT
            GL_DEBUG(glAlphaFunc)( GL_GREATER, 0 ); // GL_COLOR_BUFFER_BIT

            GL_DEBUG(glEnable)( GL_BLEND );                                 // GL_COLOR_BUFFER_BIT GL_ENABLE_BIT
            GL_DEBUG(glBlendFunc)( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // GL_COLOR_BUFFER_BIT - make the texture a filter

            oglx_texture_Bind ( ptex );

            GL_DEBUG(glColor4f)( 1.0f, 1.0f, 1.0f, 1.0f - ABS(alpha) );
            GL_DEBUG(glBegin)( GL_TRIANGLE_FAN );
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG(glTexCoord2fv)( vtlist[i].tex );
                GL_DEBUG(glVertex3fv)( vtlist[i].pos );
            }
            GL_DEBUG_END();
        }
        ATTRIB_POP( "render_foreground_overlay()" );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_shadow_sprite( float intensity, GLvertex v[] )
{
    int i;

    if ( intensity*255.0f < 1.0f ) return;

    GL_DEBUG(glColor4f)(intensity, intensity, intensity, 1.0f );

    GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
    {
        for ( i = 0; i < 4; i++ )
        {
            GL_DEBUG(glTexCoord2fv)(v[i].tex );
            GL_DEBUG(glVertex3fv)(v[i].pos );
        }
    }
    GL_DEBUG_END();
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

    if ( character >= MAX_CHR || !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked ) return;
    pchr = ChrList.lst + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden ) return;

    // no shadow if off the mesh
    if ( !VALID_TILE(PMesh, pchr->onwhichfan) ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF(PMesh->mmem.tile_list[pchr->onwhichfan]) ) return;

    // no shadow if completely transparent
    alpha = (pchr->inst.alpha * INV_FF) * (pchr->inst.light * INV_FF);
    if ( alpha * 255 < 1.0f ) return;

    // much resuced shadow if on a reflective tile
    if ( 0 != mesh_test_fx(PMesh, pchr->onwhichfan, MPDFX_DRAWREF) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

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
    oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_LIGHT ) );

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

    if ( character >= MAX_CHR || !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked ) return;
    pchr = ChrList.lst + character;

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden ) return;

    // no shadow if off the mesh
    if ( !VALID_TILE(PMesh, pchr->onwhichfan) ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF(PMesh->mmem.tile_list[pchr->onwhichfan]) ) return;

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
    oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_LIGHT ) );

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

//--------------------------------------------------------------------------------------------
void light_fans( renderlist_t * prlist )
{
    int   entry;
    Uint8 type;
    float light;
    int   numvertices, vertex;

    ego_mpd_t      * pmesh;
    ego_mpd_info_t * pinfo;
    mesh_mem_t     * pmem;
    grid_mem_t     * pgmem;

    if ( NULL == prlist ) return;

    pmesh = prlist->pmesh;
    if (NULL == pmesh) return;
    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mmem);
    pgmem = &(pmesh->gmem);

    // cache the grid lighting
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        int fan;

        fan = prlist->all[entry];
        if ( !VALID_TILE(pmesh, fan) ) continue;

        mesh_light_corners( prlist->pmesh, fan );
    }

    // use the grid to light the tiles
    for ( entry = 0; entry < prlist->all_count; entry++ )
    {
        int ivrt;
        Uint32 fan;

        fan = prlist->all[entry];
        if ( !VALID_TILE(pmesh, fan) ) continue;

        type   = pmem->tile_list[fan].type;

        numvertices = tile_dict[type].numvertices;

        vertex = pmem->tile_list[fan].vrtstart;

        // copy the 1st 4 vertices
        for ( ivrt = 0; ivrt < 4; ivrt++, vertex++ )
        {
            light = pmem->lcache[fan][ivrt];

            pmem->clst[vertex][RR] =
                pmem->clst[vertex][GG] =
                    pmem->clst[vertex][BB] = light * INV_FF;
        };

        for ( /* nothing */ ; ivrt < numvertices; ivrt++, vertex++ )
        {
            light = 0;
            mesh_interpolate_vertex( pmem, fan, pmem->plst[vertex], &light );

            pmem->clst[vertex][RR] =
                pmem->clst[vertex][GG] =
                    pmem->clst[vertex][BB] = light * INV_FF;
        };
    }
}

//--------------------------------------------------------------------------------------------
void light_particles( ego_mpd_t * pmesh )
{
    // ZZ> This function figures out particle lighting
    int iprt;

    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if ( !PrtList.lst[iprt].on ) continue;
        pprt = PrtList.lst + iprt;
        pinst = &(pprt->inst);

        pprt->inst.light = 0;
        if ( VALID_CHR( pprt->attachedtocharacter ) )
        {
            chr_t * pchr = ChrList.lst + pprt->attachedtocharacter;
            Uint16  imad = pchr->inst.imad;

            // grab the lighting from the vertex that the particle is attached to
            if ( 0 == pprt->vrt_off )
            {
                // not sure what to do here, since it is attached to the object's origin
                pprt->inst.light = 0.5f * (pchr->inst.max_light + pchr->inst.min_light);
            }
            else if ( VALID_MAD(imad) )
            {
                int vertex = MAX(0, MadList[imad].md2_data.vertices - pprt->vrt_off);
                int light  = pchr->inst.color_amb + pchr->inst.vlst[vertex].color_dir;

                pprt->inst.light = CLIP(light, 0, 255);
            }
        }
        else if ( VALID_TILE(pmesh, pprt->onwhichfan) )
        {
            Uint32 istart;
            Uint16 tl, tr, br, bl;
            Uint16 light_min, light_max;
            mesh_mem_t * pmem;
            grid_mem_t * pgmem;

            pmem  = &(pmesh->mmem);
            pgmem = &(pmesh->gmem);

            istart = pmem->tile_list[pprt->onwhichfan].vrtstart;

            // grab the corner intensities
            tl = pgmem->light[ pprt->onwhichfan + 0 ].l;
            tr = pgmem->light[ pprt->onwhichfan + 1 ].l;
            br = pgmem->light[ pprt->onwhichfan + 2 ].l;
            bl = pgmem->light[ pprt->onwhichfan + 3 ].l;

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
                ix = ((int)pprt->pos.x) & TILE_MASK;
                iy = ((int)pprt->pos.y) & TILE_MASK;

                itop = tl * (TILE_ISIZE - ix) + tr * ix;
                ibot = bl * (TILE_ISIZE - ix) + br * ix;
                light = (TILE_ISIZE - iy) * itop + iy * ibot;
                light >>= 2 * TILE_BITS;

                pprt->inst.light = light;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t sum_dyna_lighting( dynalight_t * pdyna, float lighting[], float dx, float dy, float dz )
{
    float level;
    float maxval   = 255;
    float rho_sqr  = dx * dx + dy * dy;
    float rho0_sqr = 0.1f * maxval * pdyna->falloff;

    level = maxval / ( 1.0f + rho_sqr / rho0_sqr );
    level *= pdyna->level;

    if ( (int)level > 0 )
    {
        float rad_sqr = rho_sqr + dz * dz;
        float rad = SQRT( rad_sqr );

        dx /= rad;
        dy /= rad;
        dz /= rad;

        if ( dx > 0 )
        {
            lighting[0] += ABS(dx) * level;
        }
        else if (dx < 0)
        {
            lighting[1] += ABS(dx) * level;
        }

        if ( dy > 0 )
        {
            lighting[2] += ABS(dy) * level;
        }
        else if (dy < 0)
        {
            lighting[3] += ABS(dy) * level;
        }

        if ( dz > 0 )
        {
            lighting[4] += ABS(dz) * level;
        }
        else if (dz < 0)
        {
            lighting[5] += ABS(dz) * level;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t sum_global_lighting( float lighting[] )
{
    int cnt;
    float glob_amb;

    if ( NULL == lighting ) return bfalse;

    glob_amb = gfx.usefaredge ? (light_a * 255.0f) : MAX( 32.0f * light_a, INVISIBLE );
    for ( cnt = 0; cnt < 6; cnt++ )
    {
        lighting[cnt] = glob_amb * 0.362f;
    }

    if ( !gfx.usefaredge ) return btrue;

    // do global lighting
    if ( light_x > 0 )
    {
        lighting[0] += ABS(light_x) * light_d * 255;
    }
    else if (light_x < 0)
    {
        lighting[1] += ABS(light_x) * light_d * 255;
    }

    if ( light_y > 0 )
    {
        lighting[2] += ABS(light_y) * light_d * 255;
    }
    else if (light_y < 0)
    {
        lighting[3] += ABS(light_y) * light_d * 255;
    }

    if ( light_z > 0 )
    {
        lighting[4] += ABS(light_z) * light_d * 255;
    }
    else if (light_z < 0)
    {
        lighting[5] += ABS(light_z) * light_d * 255;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_grid_dynalight( ego_mpd_t * pmesh, camera_t * pcam )
{
    // ZZ> This function does dynamic lighting of visible fans

    int   cnt, tnc, fan, entry;
    lighting_vector_t global_lighting;

    ego_mpd_info_t * pinfo;
    grid_mem_t     * pgmem;

    if ( NULL == pmesh ) return;
    pinfo = &(pmesh->info);
    pgmem = &(pmesh->gmem);

    // refresh the dynamic light list
    make_dynalist( pcam );

    // sum up the lighting from global sources
    sum_global_lighting( global_lighting );

    // Add to base light level in normal mode
    for ( entry = 0; entry < renderlist.all_count; entry++ )
    {
        float x0, y0, dx, dy;
        lighting_cache_t * cache;
        lighting_vector_t local_lighting_low, local_lighting_hgh;
        int ix, iy;

        fan = renderlist.all[entry];
        if ( !VALID_TILE(pmesh, fan) ) continue;

        ix = fan % pinfo->tiles_x;
        iy = fan / pinfo->tiles_x;

        x0 = ix * TILE_SIZE;
        y0 = iy * TILE_SIZE;

        cache = &(pgmem->light[fan].cache);

        // blank the lighting
        for (tnc = 0; tnc < 6; tnc++)
        {
            local_lighting_low[tnc] = 0.0f;
            local_lighting_hgh[tnc] = 0.0f;
        };

        if ( gfx.shading != GL_FLAT )
        {
            // add in the dynamic lighting
            for ( cnt = 0; cnt < dyna_list_count; cnt++ )
            {
                dx = dyna_list[cnt].x - x0;
                dy = dyna_list[cnt].y - y0;

                sum_dyna_lighting( dyna_list + cnt, local_lighting_low, dx, dy, dyna_list[cnt].z - pmesh->mmem.bbox.mins[ZZ] );
                sum_dyna_lighting( dyna_list + cnt, local_lighting_hgh, dx, dy, dyna_list[cnt].z - pmesh->mmem.bbox.maxs[ZZ] );
            }

            for ( cnt = 0; cnt < 6; cnt++ )
            {
                local_lighting_low[cnt] += global_lighting[cnt];
                local_lighting_hgh[cnt] += global_lighting[cnt];
            }
        }

        // average this in with the existing lighting
        cache->max_light = 0;
        for ( tnc = 0; tnc < 6; tnc++ )
        {
            cache->lighting_low[tnc] = cache->lighting_low[tnc] * 0.9f + local_lighting_low[tnc] * 0.1f;
            cache->max_light = MAX(cache->max_light, cache->lighting_low[tnc]);

            cache->lighting_hgh[tnc] = cache->lighting_hgh[tnc] * 0.9f + local_lighting_hgh[tnc] * 0.1f;
            cache->max_light = MAX(cache->max_light, cache->lighting_hgh[tnc]);
        }
    }
}

//--------------------------------------------------------------------------------------------
void render_water( renderlist_t * prlist )
{
    // ZZ> This function draws all of the water fans

    int cnt;

    // Bottom layer first
    if ( gfx.draw_water_1 && water.layer[1].z > -water.layer[1].amp )
    {
        for ( cnt = 0; cnt < prlist->all_count; cnt++ )
        {
            if ( 0 != mesh_test_fx( PMesh, prlist->all[cnt], MPDFX_WATER ) )
            {
                render_water_fan( prlist->pmesh, prlist->all[cnt], 1 );
            }
        }
    }

    // Top layer second
    if ( gfx.draw_water_0 && water.layer[0].z > -water.layer[0].amp )
    {
        for ( cnt = 0; cnt < prlist->all_count; cnt++ )
        {
            if ( 0 != mesh_test_fx( PMesh, prlist->all[cnt], MPDFX_WATER ) )
            {
                render_water_fan( prlist->pmesh, prlist->all[cnt], 0 );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_init( ego_mpd_t * pmesh, camera_t * pcam )
{
    // Which tiles can be displayed
    make_renderlist( pmesh, pcam );

    // determine which objects are visible
    dolist_make( renderlist.pmesh );
    dolist_sort( pcam );

    // figure out the terrain lighting
    do_grid_dynalight( renderlist.pmesh, pcam );

    // apply the lighting to the characters and particles
    light_fans( &renderlist );

    light_particles( renderlist.pmesh );

    // update the particle instances
    update_all_prt_instance( pcam );
}

//--------------------------------------------------------------------------------------------
void draw_scene_mesh( renderlist_t * prlist )
{
    // BB> draw the mesh and any reflected objects

    int cnt, tnc;
    ego_mpd_t * pmesh;

    if ( NULL == prlist ) return;

    if ( NULL == prlist->pmesh ) return;
    pmesh = prlist->pmesh;

    //---------------------------------------------
    // draw all tiles that do not reflect characters
    GL_DEBUG(glDisable)(GL_BLEND );             // no transparency
    GL_DEBUG(glDisable)(GL_CULL_FACE );

    GL_DEBUG(glEnable)(GL_DEPTH_TEST );
    GL_DEBUG(glDepthMask)(GL_TRUE );

    GL_DEBUG(glEnable)(GL_ALPHA_TEST );         // use alpha test to allow the thatched roof tiles to look like thatch
    GL_DEBUG(glAlphaFunc)(GL_GREATER, 0 );

    meshlasttexture = (Uint16)(~0);
    for ( cnt = 0; cnt < prlist->ndr_count; cnt++ )
    {
        render_fan( pmesh, prlist->ndr[cnt] );
    }

    //--------------------------------
    // draw the reflective tiles and any reflected objects
    if ( gfx.refon )
    {
        //------------------------------
        // draw the reflective tiles, but turn off the depth buffer
        // this blanks out any background that might've been drawn

        GL_DEBUG(glEnable)(GL_DEPTH_TEST );
        GL_DEBUG(glDepthMask)(GL_FALSE );

        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < prlist->drf_count; cnt++ )
        {
            render_fan( pmesh, prlist->drf[cnt] );
        }

        //------------------------------
        // Render all reflected objects
        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glDepthMask)(GL_TRUE );
        GL_DEBUG(glDepthFunc)(GL_LEQUAL );
        for ( cnt = dolist_count - 1; cnt >= 0; cnt-- )
        {
            tnc = dolist[cnt].ichr;

            if ( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
            {
                Uint32 itile;

                GL_DEBUG(glEnable)(GL_CULL_FACE );
                GL_DEBUG(glFrontFace)(GL_CCW );

                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE );

                tnc = dolist[cnt].ichr;
                itile = ChrList.lst[tnc].onwhichfan;

                if ( VALID_TILE(pmesh, itile) && (0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF )) )
                {
                    render_one_mad_ref( tnc, 255 );
                }
            }
            else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT( dolist[cnt].iprt ) )
            {
                Uint32 itile;
                tnc = dolist[cnt].iprt;
                itile = PrtList.lst[tnc].onwhichfan;

                GL_DEBUG(glDisable)(GL_CULL_FACE );

                // render_one_prt_ref() actually sets its own blend function, but just to be safe
                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE );

                if ( VALID_TILE(pmesh, itile) && (0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF )) )
                {
                    render_one_prt_ref( tnc );
                }
            }
        }

        //------------------------------
        // Render the shadow floors ( let everything show through )
        // turn on the depth mask, so that no objects under the floor will show through
        // this assumes that the floor is not partially transparent...
        GL_DEBUG(glDepthMask)(GL_TRUE );

        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE );

        GL_DEBUG(glDisable)(GL_CULL_FACE );

        GL_DEBUG(glEnable)(GL_DEPTH_TEST );
        GL_DEBUG(glDepthMask)(GL_TRUE );

        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < prlist->drf_count; cnt++ )
        {
            render_fan( pmesh, prlist->drf[cnt] );
        }
    }
    else
    {
        //------------------------------
        // Render the shadow floors as normal solid floors
        meshlasttexture = (Uint16)(~0);
        for ( cnt = 0; cnt < prlist->drf_count; cnt++ )
        {
            render_fan( pmesh, prlist->drf[cnt] );
        }
    }

#if defined(RENDER_HMAP)
    //------------------------------
    // render the heighmap
    for ( cnt = 0; cnt < prlist->all_count; cnt++ )
    {
        render_hmap_fan( pmesh, prlist->all[cnt] );
    }
#endif

    //------------------------------
    // Render the shadows
    if ( gfx.shaon )
    {
        GL_DEBUG(glDepthMask)(GL_FALSE );
        GL_DEBUG(glEnable)(GL_DEPTH_TEST );

        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

        if ( gfx.shasprite )
        {
            // Bad shadows
            for ( cnt = 0; cnt < dolist_count; cnt++ )
            {
                tnc = dolist[cnt].ichr;
                if ( 0 == ChrList.lst[tnc].shadowsize ) continue;

                render_bad_shadow( tnc );
            }
        }
        else
        {
            // Good shadows for me
            for ( cnt = 0; cnt < dolist_count; cnt++ )
            {
                tnc = dolist[cnt].ichr;
                if ( 0 == ChrList.lst[tnc].shadowsize ) continue;

                render_shadow( tnc );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_solid()
{
    Uint32 cnt, tnc;

    //------------------------------
    // Render all solid objects
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        GL_DEBUG(glDepthMask)( GL_TRUE );

        GL_DEBUG(glEnable)( GL_DEPTH_TEST );
        GL_DEBUG(glDepthFunc)( GL_LEQUAL );

        GL_DEBUG(glEnable)( GL_ALPHA_TEST );
        GL_DEBUG(glAlphaFunc)( GL_GREATER, 0 );

        GL_DEBUG(glDisable)( GL_BLEND );

        GL_DEBUG(glDisable)( GL_CULL_FACE );

        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
        {
            tnc = dolist[cnt].ichr;

            if ( ChrList.lst[tnc].inst.alpha == 255 && ChrList.lst[tnc].inst.light == 255 )
            {
                render_one_mad( tnc, 255 );
            }
        }
        else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT( dolist[cnt].iprt ) )
        {
            GL_DEBUG(glDisable)( GL_CULL_FACE );

            render_one_prt_solid( dolist[cnt].iprt );
        }
    }

    // Render the solid billboards
    render_all_billboards( PCamera );

    // daw some debugging lines
    draw_all_lines( PCamera );

}

//--------------------------------------------------------------------------------------------
void draw_scene_water( renderlist_t * prlist )
{
    // set the the transparency parameters
    GL_DEBUG(glDepthMask)(GL_FALSE );
    GL_DEBUG(glEnable)(GL_DEPTH_TEST );
    GL_DEBUG(glDepthFunc)(GL_LEQUAL );

    GL_DEBUG(glEnable)(GL_CULL_FACE );
    GL_DEBUG(glFrontFace)(GL_CW );

    // And transparent water floors
    if ( !water.light )
    {
        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        render_water( prlist );
    }
    else
    {
        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE );
        render_water( prlist );
    }
}

//--------------------------------------------------------------------------------------------
void draw_scene_trans()
{
    // BB > draw transparent objects

    int cnt;
    Uint8 trans;

    // set the the transparency parameters
    GL_DEBUG(glDepthMask)(GL_FALSE );

    GL_DEBUG(glEnable)(GL_DEPTH_TEST );
    GL_DEBUG(glDepthFunc)(GL_LEQUAL );

    // Now render all transparent and light objects
    for ( cnt = dolist_count - 1; cnt >= 0; cnt-- )
    {
        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR( dolist[cnt].ichr ) )
        {
            Uint16  ichr = dolist[cnt].ichr;
            chr_t * pchr = ChrList.lst + ichr;
            chr_instance_t * pinst = &(pchr->inst);

            GL_DEBUG(glEnable)(GL_CULL_FACE );
            GL_DEBUG(glFrontFace)(GL_CW );

            if (pinst->alpha != 255 && pinst->light == 255)
            {
                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                trans = pinst->alpha;
                if ( trans < SEEINVISIBLE && ( local_seeinvisible || pchr->islocalplayer ) ) trans = SEEINVISIBLE;

                render_one_mad( ichr, trans );
            }

            if ( pinst->light != 255 )
            {
                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE );

                trans = pinst->light == 255 ? 0 : pinst->light;

                if ( trans < SEEINVISIBLE && ( local_seeinvisible || pchr->islocalplayer ) )  trans = SEEINVISIBLE;

                render_one_mad( ichr, trans );
            }

            if ( gfx.phongon && pinst->sheen > 0 )
            {
                Uint16 trans;
                Uint16 save_texture, save_enviro;

                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE );

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
void draw_scene_zreflection( ego_mpd_t * pmesh, camera_t * pcam )
{
    // ZZ> This function draws 3D objects

    if ( NULL == pcam  ) pcam = PCamera;
    if ( NULL == pmesh ) pmesh = PMesh;

    draw_scene_init( pmesh, pcam );

    // do the render pass for the mesh
    draw_scene_mesh( &renderlist );

    // do the render pass for solid objects
    draw_scene_solid();

    // draw the water
    draw_scene_water( &renderlist );

    // do the render pass for transparent objects
    draw_scene_trans();

}

//--------------------------------------------------------------------------------------------
void draw_blip( float sizeFactor, Uint8 color, int x, int y )
{
    float xl, xr, yt, yb;
    float width, height;

    // ZZ> This function draws a blip
    if ( x > 0 && y > 0 )
    {
        oglx_texture * ptex = TxTexture_get_ptr( TX_BLIP );

        EnableTexturing();
        GL_DEBUG(glColor4f)(1.0f, 1.0f, 1.0f, 1.0f );
        GL_DEBUG(glNormal3f)(0.0f, 0.0f, 1.0f );

        oglx_texture_Bind( ptex );

        xl = ( float )bliprect[color].left   / (float)oglx_texture_GetTextureWidth ( ptex );
        xr = ( float )bliprect[color].right  / (float)oglx_texture_GetTextureWidth ( ptex );
        yt = ( float )bliprect[color].top    / (float)oglx_texture_GetTextureHeight( ptex );
        yb = ( float )bliprect[color].bottom / (float)oglx_texture_GetTextureHeight( ptex );

        width  = bliprect[color].right  - bliprect[color].left;
        height = bliprect[color].bottom - bliprect[color].top;

        width  *= sizeFactor;
        height *= sizeFactor;

        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)( xl, yb ); GL_DEBUG(glVertex2f)( x - (width / 2), y + (height / 2) );
            GL_DEBUG(glTexCoord2f)( xr, yb ); GL_DEBUG(glVertex2f)( x + (width / 2), y + (height / 2) );
            GL_DEBUG(glTexCoord2f)( xr, yt ); GL_DEBUG(glVertex2f)( x + (width / 2), y - (height / 2) );
            GL_DEBUG(glTexCoord2f)( xl, yt ); GL_DEBUG(glVertex2f)( x - (width / 2), y - (height / 2) );
        }
        GL_DEBUG_END();
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle )
{
    // ZZ> This function draws an icon
    int   position, blipx, blipy;
    float xl, xr, yt, yb;
    int   width, height;

    oglx_texture * ptex = TxTexture_get_ptr( icontype );

    EnableTexturing();    // Enable texture mapping
    GL_DEBUG(glColor4f)(1.0f, 1.0f, 1.0f, 1.0f );

    oglx_texture_Bind( ptex );

    xl = ( ( float )iconrect.left   ) / 32.0f;
    xr = ( ( float )iconrect.right  ) / 32.0f;
    yt = ( ( float )iconrect.top    ) / 32.0f;
    yb = ( ( float )iconrect.bottom ) / 32.0f;

    width  = iconrect.right  - iconrect.left;
    height = iconrect.bottom - iconrect.top;

    GL_DEBUG(glBegin)(GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
        GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
        GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
        GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
    }
    GL_DEBUG_END();

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

    GL_DEBUG(glBegin)( GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(fx1, fy2 );   GL_DEBUG(glVertex2i)(x, y );
        GL_DEBUG(glTexCoord2f)(fx2, fy2 );   GL_DEBUG(glVertex2i)(x2, y );
        GL_DEBUG(glTexCoord2f)(fx2, fy1 );   GL_DEBUG(glVertex2i)(x2, y2 );
        GL_DEBUG(glTexCoord2f)(fx1, fy1 );   GL_DEBUG(glVertex2i)(x, y2 );
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
void draw_map_texture( int x, int y )
{
    // ZZ> This function draws the map
    EnableTexturing();

    oglx_texture_Bind( TxTexture_get_ptr( TX_MAP ) );

    GL_DEBUG(glBegin)(GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(0.0f, 1.0f ); GL_DEBUG(glVertex2i)(x,           y + MAPSIZE );
        GL_DEBUG(glTexCoord2f)(1.0f, 1.0f ); GL_DEBUG(glVertex2i)(x + MAPSIZE, y + MAPSIZE );
        GL_DEBUG(glTexCoord2f)(1.0f, 0.0f ); GL_DEBUG(glVertex2i)(x + MAPSIZE, y );
        GL_DEBUG(glTexCoord2f)(0.0f, 0.0f ); GL_DEBUG(glVertex2i)(x,           y );
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
int draw_one_xp_bar( int x, int y, Uint8 ticks )
{
    // ZF> This function draws a xp bar and returns the y position for the next one
    float xl, xr, yt, yb;
    int width, height;
    Uint8 cnt;

    if ( ticks < 0 ) return y;
    ticks = MIN(ticks, NUMTICK);

    EnableTexturing();               // Enable texture mapping
    GL_DEBUG(glColor4f)(1, 1, 1, 1 );

    // Draw the tab (always colored)
    oglx_texture_Bind( TxTexture_get_ptr( TX_XP_BAR ) );
    xl = 0;
    xr = 32.00f / 128;
    yt = XPTICK / 16;
    yb = XPTICK * 2 / 16;
    width = 16;
    height = XPTICK;
    GL_DEBUG(glBegin)(GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
        GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
        GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
        GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
    }
    GL_DEBUG_END();
    x += 16;

    // Draw the filled ones
    xl = 0;
    xr = 32.00f / 128;
    yt = XPTICK / 16;
    yb = XPTICK * 2 / 16;
    width = XPTICK;
    height = XPTICK;
    for ( cnt = 0; cnt < ticks; cnt++)
    {
        oglx_texture_Bind( TxTexture_get_ptr( TX_XP_BAR ) );
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(( cnt * width ) + x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(( cnt * width ) + x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(( cnt * width ) + x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(( cnt * width ) + x,         y );
        }
        GL_DEBUG_END();
    }

    // Draw the remaining empty ones
    xl = 0;
    xr = 32.00f / 128;
    yt = 0;
    yb = XPTICK / 16;
    width = XPTICK;
    height = XPTICK;
    for ( /*nothing*/; cnt < NUMTICK; cnt++)
    {
        oglx_texture_Bind( TxTexture_get_ptr( TX_XP_BAR ) );
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(( cnt * width ) + x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(( cnt * width ) + x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(( cnt * width ) + x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(( cnt * width ) + x,         y );
        }
        GL_DEBUG_END();
    }

    return y + XPTICK;
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( Uint8 bartype, int x, int y, int ticks, int maxticks )
{
    // ZZ> This function draws a bar and returns the y position for the next one
    int noticks;
    float xl, xr, yt, yb;
    int width, height;

    if ( maxticks <= 0 || ticks < 0 || bartype > NUMBAR ) return y;

    EnableTexturing();               // Enable texture mapping
    GL_DEBUG(glColor4f)(1, 1, 1, 1 );

    // Draw the tab
    oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

    xl = ( ( float )tabrect[bartype].left ) / 128;
    xr = ( ( float )tabrect[bartype].right ) / 128;
    yt = ( ( float )tabrect[bartype].top ) / 128;
    yb = ( ( float )tabrect[bartype].bottom ) / 128;
    width = tabrect[bartype].right - tabrect[bartype].left; height = tabrect[bartype].bottom - tabrect[bartype].top;
    GL_DEBUG(glBegin)(GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
        GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
        GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
        GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
    }
    GL_DEBUG_END();

    // Error check
    if ( maxticks > MAXTICK ) maxticks = MAXTICK;
    if ( ticks > maxticks ) ticks = maxticks;

    // Draw the full rows of ticks
    x += TABX;

    while ( ticks >= NUMTICK )
    {
        barrect[bartype].right = BARX;
        oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

        xl = ( ( float )barrect[bartype].left ) / 128;
        xr = ( ( float )barrect[bartype].right ) / 128;
        yt = ( ( float )barrect[bartype].top ) / 128;
        yb = ( ( float )barrect[bartype].bottom ) / 128;
        width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
        }
        GL_DEBUG_END();
        y += BARY;
        ticks -= NUMTICK;
        maxticks -= NUMTICK;
    }

    // Draw any partial rows of ticks
    if ( maxticks > 0 )
    {
        // Draw the filled ones
        barrect[bartype].right = ( ticks << 3 ) + TABX;
        oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

        xl = ( ( float )barrect[bartype].left ) / 128;
        xr = ( ( float )barrect[bartype].right ) / 128;
        yt = ( ( float )barrect[bartype].top ) / 128;
        yb = ( ( float )barrect[bartype].bottom ) / 128;
        width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
        }
        GL_DEBUG_END();

        // Draw the empty ones
        noticks = maxticks - ticks;
        if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );

        barrect[0].right = ( noticks << 3 ) + TABX;
        oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

        xl = ( ( float )barrect[0].left ) / 128;
        xr = ( ( float )barrect[0].right ) / 128;
        yt = ( ( float )barrect[0].top ) / 128;
        yb = ( ( float )barrect[0].bottom ) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(( ticks << 3 ) + x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(( ticks << 3 ) + x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(( ticks << 3 ) + x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(( ticks << 3 ) + x,         y );
        }
        GL_DEBUG_END();
        maxticks -= NUMTICK;
        y += BARY;
    }

    // Draw full rows of empty ticks
    while ( maxticks >= NUMTICK )
    {
        barrect[0].right = BARX;
        oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

        xl = ( ( float )barrect[0].left ) / 128;
        xr = ( ( float )barrect[0].right ) / 128;
        yt = ( ( float )barrect[0].top ) / 128;
        yb = ( ( float )barrect[0].bottom ) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
        }
        GL_DEBUG_END();
        y += BARY;
        maxticks -= NUMTICK;
    }

    // Draw the last of the empty ones
    if ( maxticks > 0 )
    {
        barrect[0].right = ( maxticks << 3 ) + TABX;
        oglx_texture_Bind( TxTexture_get_ptr( TX_BARS ) );

        xl = ( ( float )barrect[0].left ) / 128;
        xr = ( ( float )barrect[0].right ) / 128;
        yt = ( ( float )barrect[0].top ) / 128;
        yb = ( ( float )barrect[0].bottom ) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;
        GL_DEBUG(glBegin)(GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)(xl, yb );   GL_DEBUG(glVertex2i)(x,         y + height );
            GL_DEBUG(glTexCoord2f)(xr, yb );   GL_DEBUG(glVertex2i)(x + width, y + height );
            GL_DEBUG(glTexCoord2f)(xr, yt );   GL_DEBUG(glVertex2i)(x + width, y );
            GL_DEBUG(glTexCoord2f)(xl, yt );   GL_DEBUG(glVertex2i)(x,         y );
        }
        GL_DEBUG_END();
        y += BARY;
    }

    return y;

}

//--------------------------------------------------------------------------------------------
void BeginText()
{
    EnableTexturing();    // Enable texture mapping

    oglx_texture_Bind( TxTexture_get_ptr( TX_FONT ) );

    GL_DEBUG(glEnable)(GL_ALPHA_TEST );
    GL_DEBUG(glAlphaFunc)(GL_GREATER, 0 );

    GL_DEBUG(glEnable)(GL_BLEND );
    GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    GL_DEBUG(glDisable)(GL_DEPTH_TEST );
    GL_DEBUG(glDisable)(GL_CULL_FACE );

    GL_DEBUG(glColor4f)(1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void EndText()
{
    GL_DEBUG(glDisable)(GL_BLEND );
    GL_DEBUG(glDisable)(GL_ALPHA_TEST );
}

//--------------------------------------------------------------------------------------------
int write_draw_string( int x, int y, const char *format, va_list args  )
{
    int cnt = 1;
    int x_stt;
    STRING szText;
    Uint8 cTmp;

    if ( vsnprintf( szText, SDL_arraysize(szText) - 1, format, args ) <= 0 )
    {
        return y;
    }

    BeginText();
    {
        x_stt = x;
        cnt = 0;
        cTmp = szText[cnt];
        while ( '\0' != cTmp )
        {
            // Convert ASCII to our own little font
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = (x & TABAND) + TABADD;
            }
            else if ( '\n' == cTmp )
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
    }
    EndText();

    y += fontyspacing;

    return y;
}

//--------------------------------------------------------------------------------------------
int _draw_string_raw( int x, int y, const char *format, ...  )
{
    // BB> the same as draw string, but it does not use the Begin2DMode() ... End2DMode()
    //    bookends.

    va_list args;

    va_start( args, format );
    y = write_draw_string( x, y, format, args );
    va_end( args );

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_string( int x, int y, const char *format, ...  )
{
    // ZZ> This function spits a line of null terminated text onto the backbuffer
    //
    // BB> Uses Begin2DMode() ... End2DMode() so that the function can basically be called from anywhere
    //    The way they are currently implemented, this breaks the icon drawing in draw_status() if
    //    you use draw_string() and then draw_icon(). Use _draw_string_raw(), instead.

    va_list args;

    Begin2DMode();
    {
        va_start( args, format );
        y = write_draw_string( x, y, format, args );
        va_end( args );
    }
    End2DMode();

    return y;
}

//--------------------------------------------------------------------------------------------
int length_of_word( const char *szText )
{
    // ZZ> This function returns the number of pixels the
    //    next word will take on screen in the x direction

    // Count all preceeding spaces
    int x = 0;
    int cnt = 0;
    Uint8 cTmp = szText[cnt];

    while ( ' ' == cTmp || '~' == cTmp || '\n' == cTmp )
    {
        if ( ' ' == cTmp )
        {
            x += fontxspacing[asciitofont[cTmp]];
        }
        else if ( '~' == cTmp )
        {
            x = (x & TABAND) + TABADD;
        }

        cnt++;
        cTmp = szText[cnt];
    }

    while ( ' ' != cTmp && '~' != cTmp && '\n' != cTmp && cTmp != 0 )
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
    //    wrapping over the right side and returning the new y value
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

                while ( ' ' == cTmp || '~' == cTmp )
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = (x & TABAND) + TABADD;
            }
            else if ( '\n' == cTmp )
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
            if ( '~' == cTmp || ' ' == cTmp )
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
    //    The x,y coordinates are the top left point of the image to draw
    Uint16 item, imad, iskin;
    char cTmp;
    char *readtext;
    STRING generictext;

    int life     = FP8_TO_INT( ChrList.lst[character].life    );
    int lifemax  = FP8_TO_INT( ChrList.lst[character].lifemax );
    int mana     = FP8_TO_INT( ChrList.lst[character].mana    );
    int manamax  = FP8_TO_INT( ChrList.lst[character].manamax );
    int cnt = lifemax;

    // Write the character's first name
    if ( ChrList.lst[character].nameknown )
        readtext = ChrList.lst[character].name;
    else
        readtext = CapList[ChrList.lst[character].model].classname;

    for ( cnt = 0; cnt < 6; cnt++ )
    {
        cTmp = readtext[cnt];
        if ( ' ' == cTmp || '\0' == cTmp )
        {
            generictext[cnt] = '\0';
            break;
        }
        else
            generictext[cnt] = cTmp;
    }

    generictext[6] = '\0';
    y = _draw_string_raw( x + 8, y, generictext );

    // Write the character's money
    y = _draw_string_raw( x + 8, y, "$%4d", ChrList.lst[character].money ) + 8;

    // Draw the icons
    imad  = ChrList.lst[character].inst.imad;
    iskin = ChrList.lst[character].skin;
    if ( VALID_MAD(imad) )
    {
        draw_one_icon( MadList[imad].ico_ref[iskin], x + 40, y, ChrList.lst[character].sparkle );
    }

    item = ChrList.lst[character].holdingwhich[SLOT_LEFT];
    if ( VALID_CHR(item) )
    {
        chr_t * pitem = ChrList.lst + item;
        bool_t is_spell_fx, is_book, draw_book;

        is_spell_fx = CapList[pitem->model].is_spelleffect;
        is_book     = SPELLBOOK == pitem->model;

        imad  = pitem->inst.imad;
        iskin = pitem->skin;

        draw_book = (is_book || (is_spell_fx && !pitem->icon)) && (bookicon_count > 0);

        if ( VALID_MAD(imad) && !draw_book )
        {
            draw_one_icon( MadList[imad].ico_ref[iskin], x + 8, y, pitem->sparkle );
            if ( pitem->ammomax != 0 && pitem->ammoknown )
            {
                if ( !CapList[pitem->model].isstackable || pitem->ammo > 1 )
                {
                    // Show amount of ammo left
                    _draw_string_raw( x + 8, y - 8, "%2d", pitem->ammo );
                }
            }
        }
        else if ( VALID_MAD(imad) && draw_book )
        {
            Uint16 icon = pitem->money;
            if (icon > bookicon_count) icon = bookicon_count;
            draw_one_icon( bookicon_ref[ icon ], x + 8, y, pitem->sparkle );
        }
        else
        {
            draw_one_icon( ICON_NULL, x + 8, y, pitem->sparkle );
        }
    }
    else
    {
        draw_one_icon( ICON_NULL, x + 8, y, NOSPARKLE );
    }

    item = ChrList.lst[character].holdingwhich[SLOT_RIGHT];
    if ( VALID_CHR(item) )
    {
        chr_t * pitem = ChrList.lst + item;
        bool_t is_spell_fx, is_book, draw_book;

        is_spell_fx = CapList[pitem->model].is_spelleffect;
        is_book     = SPELLBOOK == pitem->model;

        draw_book = (is_book || (is_spell_fx && !pitem->icon)) && (bookicon_count > 0);

        imad  = pitem->inst.imad;
        iskin = pitem->skin;

        if ( VALID_MAD(imad) && !draw_book )
        {
            draw_one_icon( MadList[imad].ico_ref[iskin], x + 72, y, pitem->sparkle );
            if ( pitem->ammomax != 0 && pitem->ammoknown )
            {
                if ( !CapList[pitem->model].isstackable || pitem->ammo > 1 )
                {
                    // Show amount of ammo left
                    _draw_string_raw( x + 72, y - 8, "%2d", pitem->ammo );
                }
            }
        }
        else if ( VALID_MAD(imad) && draw_book )
        {
            Uint16 icon = pitem->money;
            if (icon > bookicon_count) icon = bookicon_count;
            draw_one_icon( bookicon_ref[ icon ], x + 72, y, pitem->sparkle );
        }
        else
        {
            draw_one_icon( ICON_NULL, x + 72, y, pitem->sparkle );
        }
    }
    else
    {
        draw_one_icon( ICON_NULL, x + 72, y, NOSPARKLE );
    }

    y += 32;

    //Draw the small XP progress bar
    if ( ChrList.lst[character].experiencelevel < MAXLEVEL)
    {
        Uint16 profile = ChrList.lst[character].model;
        Uint8 curlevel = ChrList.lst[character].experiencelevel + 1;
        Uint32 xplastlevel = CapList[profile].experienceforlevel[curlevel-1];
        Uint32 xpneed = CapList[profile].experienceforlevel[curlevel];

        y = draw_one_xp_bar( x + 16, y, ( ( (float)MAX(ChrList.lst[character].experience - xplastlevel, 0) / MAX( xpneed - xplastlevel, 1 ) ) * NUMTICK) );
    }

    // Draw the status bars
    if ( ChrList.lst[character].alive )
        y = draw_one_bar( ChrList.lst[character].lifecolor, x, y, life, lifemax );
    else
        y = draw_one_bar( 0, x, y, 0, lifemax );  // Draw a black bar

    y = draw_one_bar( ChrList.lst[character].manacolor, x, y, mana, manamax );

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_all_status( int y )
{
    int cnt;

    if ( staton )
    {
        for ( cnt = 0; cnt < numstat && y < sdl_scr.y; cnt++ )
        {
            y = draw_status( statlist[cnt], sdl_scr.x - BARX, y );
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_map()
{
    int cnt, tnc;

    // Map display
    if ( !mapvalid || !mapon ) return;

    ATTRIB_PUSH( "draw_map()", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
    {

        GL_DEBUG(glEnable)( GL_BLEND );                                 // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
        GL_DEBUG(glBlendFunc)( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        GL_DEBUG(glColor4f)(1.0f, 1.0f, 1.0f, 1.0f );
        draw_map_texture( 0, sdl_scr.y - MAPSIZE );

        GL_DEBUG(glBlendFunc)( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        // If one of the players can sense enemies via EMP, draw them as blips on the map
        if ( TEAM_MAX != local_senseenemiesTeam )
        {
            Uint16 iTmp;

            for ( iTmp = 0; numblip < MAXBLIP && iTmp < MAX_CHR; iTmp++ )
            {
                Uint16 icap;

                if ( !ChrList.lst[iTmp].on ) continue;

                icap = ChrList.lst[iTmp].model;
                if ( !VALID_CAP(icap) ) continue;

                // Show only teams that will attack the player
                if ( TeamList[ChrList.lst[iTmp].team].hatesteam[local_senseenemiesTeam] )
                {
                    // Only if they match the required IDSZ ([NONE] always works)
                    if ( local_senseenemiesID == IDSZ_NONE ||
                            local_senseenemiesID == CapList[icap].idsz[IDSZ_PARENT] ||
                            local_senseenemiesID == CapList[icap].idsz[IDSZ_TYPE  ])
                    {
                        // Inside the map?
                        if ( ChrList.lst[iTmp].pos.x < PMesh->info.edge_x && ChrList.lst[iTmp].pos.y < PMesh->info.edge_y )
                        {
                            // Valid colors only
                            blipx[numblip] = GET_MAP_X(PMesh, ChrList.lst[iTmp].pos.x);
                            blipy[numblip] = ChrList.lst[iTmp].pos.y * MAPSIZE / PMesh->info.edge_y;
                            blipc[numblip] = COLOR_RED; // Red blips
                            numblip++;
                        }
                    }
                }
            }
        }

        // draw all the blips
        for ( cnt = 0; cnt < numblip; cnt++ )
        {
            draw_blip(0.75f, blipc[cnt], blipx[cnt], blipy[cnt] + sdl_scr.y - MAPSIZE );
        }
        numblip = 0;

        // Show local player position(s)
        if ( youarehereon && ( update_wld & 8 ) )
        {
            for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
            {
                if ( !PlaList[cnt].valid ) continue;

                if ( INPUT_BITS_NONE != PlaList[cnt].device )
                {
                    tnc = PlaList[cnt].index;
                    if ( VALID_CHR(tnc) && ChrList.lst[tnc].alive )
                    {
                        draw_blip( 0.75f, COLOR_WHITE, GET_MAP_X(PMesh, ChrList.lst[tnc].pos.x), GET_MAP_Y(PMesh, ChrList.lst[tnc].pos.y));
                    }
                }
            }
        }

        //// draw the camera
        // if ( update_wld & 2 )
        // {
        //   draw_blip( 0.75f, COLOR_PURPLE, GET_MAP_X(PMesh, PCamera->pos.x), GET_MAP_Y(PMesh, PCamera->pos.y));
        // }
    }
    ATTRIB_POP("draw_map()")

}

//--------------------------------------------------------------------------------------------
int draw_fps( int y )
{
    // FPS text

    if ( outofsync )
    {
        y = _draw_string_raw( 0, y, "OUT OF SYNC" );
    }

    if ( parseerror )
    {
        y = _draw_string_raw( 0, y, "SCRIPT ERROR ( SEE LOG.TXT )" );
    }

    if ( fpson )
    {
        y = _draw_string_raw( 0, y, szfpstext );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_help( int y )
{
    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!MOUSE HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change" );
        y = _draw_string_raw( 0, y, "Default settings" );
        y = _draw_string_raw( 0, y, "~~Left Click to use an item" );
        y = _draw_string_raw( 0, y, "~~Left and Right Click to grab" );
        y = _draw_string_raw( 0, y, "~~Middle Click to jump" );
        y = _draw_string_raw( 0, y, "~~A and S keys do stuff" );
        y = _draw_string_raw( 0, y, "~~Right Drag to move camera" );
    }
    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!JOYSTICK HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = _draw_string_raw( 0, y, "~~Hit the buttons" );
        y = _draw_string_raw( 0, y, "~~You'll figure it out" );
    }
    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
        // In-Game help
        y = _draw_string_raw( 0, y, "!!!KEYBOARD HELP!!!" );
        y = _draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = _draw_string_raw( 0, y, "Default settings" );
        y = _draw_string_raw( 0, y, "~~TGB control left hand" );
        y = _draw_string_raw( 0, y, "~~YHN control right hand" );
        y = _draw_string_raw( 0, y, "~~Keypad to move and jump" );
        y = _draw_string_raw( 0, y, "~~Number keys for stats" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_debug( int y )
{
    int tnc;

    if ( !cfg.dev_mode ) return y;

    if ( SDLKEYDOWN( SDLK_F5 ) )
    {
        // Debug information
        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-5!!!" );
        y = _draw_string_raw( 0, y, "~~CAM %f %f %f", PCamera->pos.x, PCamera->pos.y, PCamera->pos.z );

        tnc = PlaList[0].index;
        y = _draw_string_raw( 0, y, "~~PLA0DEF %d %d %d %d %d %d %d %d",
                              ChrList.lst[tnc].damagemodifier[0] & 3,
                              ChrList.lst[tnc].damagemodifier[1] & 3,
                              ChrList.lst[tnc].damagemodifier[2] & 3,
                              ChrList.lst[tnc].damagemodifier[3] & 3,
                              ChrList.lst[tnc].damagemodifier[4] & 3,
                              ChrList.lst[tnc].damagemodifier[5] & 3,
                              ChrList.lst[tnc].damagemodifier[6] & 3,
                              ChrList.lst[tnc].damagemodifier[7] & 3  );

        tnc = PlaList[0].index;
        y = _draw_string_raw( 0, y, "~~PLA0 %5.1f %5.1f", ChrList.lst[tnc].pos.x / TILE_SIZE, ChrList.lst[tnc].pos.y / TILE_SIZE );

        tnc = PlaList[1].index;
        y = _draw_string_raw( 0, y, "~~PLA1 %5.1f %5.1f", ChrList.lst[tnc].pos.x / TILE_SIZE, ChrList.lst[tnc].pos.y / TILE_SIZE );
    }

    if ( SDLKEYDOWN( SDLK_F6 ) )
    {
        // More debug information
        STRING text;

        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-6!!!" );
        y = _draw_string_raw( 0, y, "~~FREEPRT %d", prt_count_free() );
        y = _draw_string_raw( 0, y, "~~FREECHR %d", chr_count_free() );
        y = _draw_string_raw( 0, y, "~~MACHINE %d", local_machine );
        if ( PMod->exportvalid ) snprintf( text, SDL_arraysize( text), "~~EXPORT: TRUE" );
        else                    snprintf( text, SDL_arraysize( text), "~~EXPORT: FALSE" );
        y = _draw_string_raw( 0, y, text, PMod->exportvalid );
        y = _draw_string_raw( 0, y, "~~PASS %d/%d", ShopStack.count, PassageStack.count );
        y = _draw_string_raw( 0, y, "~~NETPLAYERS %d", numplayer );
        y = _draw_string_raw( 0, y, "~~DAMAGEPART %d", damagetile.parttype );

        // y = _draw_string_raw( 0, y, "~~FOGAFF %d", fog_data.affects_water );
    }

    if ( SDLKEYDOWN( SDLK_F7 ) )
    {
        // White debug mode
        y = _draw_string_raw( 0, y, "!!!DEBUG MODE-7!!!" );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 0 ), PCamera->mView.CNV( 1, 0 ), PCamera->mView.CNV( 2, 0 ), PCamera->mView.CNV( 3, 0 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 1 ), PCamera->mView.CNV( 1, 1 ), PCamera->mView.CNV( 2, 1 ), PCamera->mView.CNV( 3, 1 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 2 ), PCamera->mView.CNV( 1, 2 ), PCamera->mView.CNV( 2, 2 ), PCamera->mView.CNV( 3, 2 ) );
        y = _draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", PCamera->mView.CNV( 0, 3 ), PCamera->mView.CNV( 1, 3 ), PCamera->mView.CNV( 2, 3 ), PCamera->mView.CNV( 3, 3 ) );
        y = _draw_string_raw( 0, y, "CAM center <%f, %f>", PCamera->center.x, PCamera->center.y );
        y = _draw_string_raw( 0, y, "CAM turn %d %d", PCamera->turn_mode, PCamera->turn_time );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_timer( int y )
{
    int fifties, seconds, minutes;

    if ( timeron )
    {
        fifties = ( timervalue % 50 ) << 1;
        seconds = ( ( timervalue / 50 ) % 60 );
        minutes = ( timervalue / 3000 );
        y = _draw_string_raw( 0, y, "=%d:%02d:%02d=", minutes, seconds, fifties );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_game_status( int y )
{

    if ( PNet->waitingforplayers )
    {
        y = _draw_string_raw( 0, y, "Waiting for players... " );
    }

    if ( local_allpladead || PMod->respawnanytime )
    {
        if ( PMod->respawnvalid && cfg.difficulty < GAME_HARD )
        {
            y = _draw_string_raw( 0, y, "PRESS SPACE TO RESPAWN" );
        }
        else
        {
            y = _draw_string_raw( 0, y, "PRESS ESCAPE TO QUIT" );
        }
    }
    else if ( PMod->beat )
    {
        y = _draw_string_raw( 0, y, "VICTORY!  PRESS ESCAPE" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_messages( int y )
{
    int cnt, tnc;

    // Messages
    if ( messageon )
    {
        // Display the messages
        tnc = DisplayMsg.count;
        for ( cnt = 0; cnt < maxmessage; cnt++ )
        {
            if ( DisplayMsg.lst[tnc].time > 0 )
            {
                y = draw_wrap_string( DisplayMsg.lst[tnc].textdisplay, 0, y, sdl_scr.x - wraptolerance );
                if (DisplayMsg.lst[tnc].time > msgtimechange)
                {
                    DisplayMsg.lst[tnc].time -= msgtimechange;
                }
                else
                {
                    DisplayMsg.lst[tnc].time = 0;
                }
            }

            tnc = (tnc + 1) % maxmessage;
        }

        msgtimechange = 0;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_text()
{
    // ZZ> draw in-game heads up display

    int y;

    Begin2DMode();
    {
        draw_map();

        y = draw_all_status( 0 );

        y = draw_fps( 0 );
        y = draw_help( y );
        y = draw_debug( y );
        y = draw_timer( y );
        y = draw_game_status( y );

        // Network message input
        if ( console_mode )
        {
            char buffer[KEYB_BUFFER_SIZE + 128];

            snprintf( buffer, SDL_arraysize(buffer), "%s > %s%s", cfg.network_messagename, keyb.buffer, HAS_NO_BITS( update_wld, 8 ) ? "x" : "+" );

            y = draw_wrap_string( buffer, 0, y, sdl_scr.x - wraptolerance );
        }

        y = draw_messages( y );
    }
    End2DMode();
}

//--------------------------------------------------------------------------------------------
void request_clear_screen()
{
    gfx_page_clear_requested = btrue;
}

//--------------------------------------------------------------------------------------------
void do_clear_screen()
{
    bool_t try_clear;

    try_clear = bfalse;
    if ( process_instance_running( PROC_PBASE(GProc) ) && PROC_PBASE(GProc)->state > proc_begin )
    {
        try_clear = gfx_page_clear_requested;
    }
    else if ( process_instance_running( PROC_PBASE(MProc) ) && PROC_PBASE(MProc)->state > proc_begin )
    {
        try_clear = gfx_page_clear_requested;
    }

    if ( try_clear )
    {
        bool_t game_needs_clear, menu_needs_clear;

        gfx_page_clear_requested = bfalse;

        // clear the depth buffer no matter what
        GL_DEBUG(glDepthMask)( GL_TRUE );
        GL_DEBUG(glClear)( GL_DEPTH_BUFFER_BIT );

        // clear the color buffer only if necessary
        game_needs_clear = gfx.clearson && process_instance_running( PROC_PBASE(GProc) );
        menu_needs_clear = mnu_draw_background && process_instance_running( PROC_PBASE(MProc) );

        if ( game_needs_clear || menu_needs_clear )
        {
            GL_DEBUG(glClear)( GL_COLOR_BUFFER_BIT );
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_flip_pages()
{
    bool_t try_flip;

    try_flip = bfalse;
    if ( process_instance_running( PROC_PBASE(GProc) ) && PROC_PBASE(GProc)->state > proc_begin )
    {
        try_flip = gfx_page_flip_requested;
    }
    else if ( process_instance_running( PROC_PBASE(MProc) ) && PROC_PBASE(MProc)->state > proc_begin )
    {
        try_flip = gfx_page_flip_requested;
    }

    if ( try_flip )
    {
        gfx_page_flip_requested = bfalse;
        flip_pages();

        gfx_page_clear_requested = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void request_flip_pages()
{
    gfx_page_flip_requested = btrue;
}

//--------------------------------------------------------------------------------------------
bool_t flip_pages_requested()
{
    return gfx_page_flip_requested;
}

//--------------------------------------------------------------------------------------------
void flip_pages()
{
    GL_DEBUG(glFlush)();

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
        if ( gfx.draw_background )
        {
            // Render the background
            render_background( TX_WATER_LOW );  // TX_WATER_LOW for waterlow.bmp
        }

        draw_scene_zreflection( PMesh, pcam );

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

    draw_scene( PCamera );
    draw_text();

    request_flip_pages();
}

//--------------------------------------------------------------------------------------------
bool_t load_blip_bitmap()
{
    // This function loads the blip bitmaps
    if ( INVALID_TEXTURE == TxTexture_load_one( "basicdat" SLASH_STR "blip", TX_BLIP, INVALID_KEY ) )
    {
        log_warning( "Blip bitmap not loaded! (\"%s\")\n", "basicdat" SLASH_STR "blip" );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_cursor()
{
    // This function implements a mouse cursor
    input_read();

    cursor_x = mous.x;  if ( cursor_x < 6 )  cursor_x = 6;  if ( cursor_x > sdl_scr.x - 16 )  cursor_x = sdl_scr.x - 16;

    cursor_y = mous.y;  if ( cursor_y < 8 )  cursor_y = 8;  if ( cursor_y > sdl_scr.y - 24 )  cursor_y = sdl_scr.y - 24;

    cursor_clicked = bfalse;
    if ( mous.button[0] && !cursor_pressed )
    {
        cursor_clicked = btrue;
    }

    cursor_pressed = mous.button[0];
    BeginText();  // Needed to setup text mode
    // draw_one_font(11, cursor_x-5, cursor_y-7);
    draw_one_font( 95, cursor_x - 5, cursor_y - 7 );
    EndText();    // Needed when done with text mode
}

/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
    GL_DEBUG(glViewport)(0, 0, w, h );
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int ogl_init()
{
    if ( !_sdl_initialized_graphics )
    {
        sdlinit_graphics();
    }

    // GL_DEBUG(glClear)) stuff
    GL_DEBUG(glClearColor)(0.0f, 0.0f, 0.0f, 0.0f); // Set the background black
    GL_DEBUG(glClearDepth)( 1.0f );

    // depth buffer stuff
    GL_DEBUG(glClearDepth)( 1.0f );
    GL_DEBUG(glDepthMask)(GL_TRUE);
    GL_DEBUG(glEnable)(GL_DEPTH_TEST);
    GL_DEBUG(glDepthFunc)(GL_LESS);

    // alpha stuff
    GL_DEBUG(glDisable)(GL_BLEND);
    GL_DEBUG(glEnable)(GL_ALPHA_TEST);
    GL_DEBUG(glAlphaFunc)(GL_GREATER, 0);

    // backface culling
    // GL_DEBUG(glEnable)(GL_CULL_FACE);
    // GL_DEBUG(glFrontFace)(GL_CW); // TODO: This prevents the mesh from getting rendered
    // GL_DEBUG(glCullFace)(GL_BACK);

    // disable OpenGL lighting
    GL_DEBUG(glDisable)(GL_LIGHTING );

    // fill mode
    GL_DEBUG(glPolygonMode)(GL_FRONT, GL_FILL );
    GL_DEBUG(glPolygonMode)(GL_BACK,  GL_FILL );

    // ?Need this for color + lighting?
    GL_DEBUG(glEnable)(GL_COLOR_MATERIAL );  // Need this for color + lighting

    // set up environment mapping
    GL_DEBUG(glTexGeni)(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
    GL_DEBUG(glTexGeni)(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

    // Load the current graphical settings
    // load_graphics();

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
    if ( !_sdl_initialized_base )
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

    sdl_vparam.gl_att.buffer_size        = cfg.scrd_req;
    sdl_vparam.gl_att.depth_size         = cfg.scrz_req;
    sdl_vparam.gl_att.multi_buffers      = (cfg.multisamples > 1) ? 1 : 0;
    sdl_vparam.gl_att.multi_samples      = cfg.multisamples;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;

    ogl_vparam.dither         = GL_FALSE;
    ogl_vparam.antialiasing   = GL_TRUE;
    ogl_vparam.perspective    = GL_FASTEST;
    ogl_vparam.shading        = GL_SMOOTH;
    ogl_vparam.userAnisotropy = 16.0f * MAX(0, cfg.texturefilter_req - TX_TRILINEAR_2);

    log_info("Opening SDL Video Mode... ");

    // redirect the output of the SDL_GL_* debug functions
    SDL_GL_set_stdout( log_get_file() );

    // actually set the video mode
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

}

//---------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    // BB> dumps the current screen (GL context) to a new bitmap file
    // right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    int i;
    bool_t savefound = bfalse;
    bool_t saved     = bfalse;
    STRING szFilename;

    // find a valid file name
    savefound = bfalse;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        snprintf( szFilename, SDL_arraysize( szFilename), "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        savefound = !vfs_exists( szFilename );
        if ( !savefound )
        {
            i++;
        }
    }
    if ( !savefound ) return bfalse;

    // if we are not using OpenGl, jsut dump the screen
    if ( HAS_NO_BITS( sdl_scr.pscreen->flags, SDL_OPENGL) )
    {
        SDL_SaveBMP(sdl_scr.pscreen, szFilename);
        return bfalse;
    }

    // we ARE using OpenGL
    GL_DEBUG(glPushClientAttrib)( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;

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

                GL_DEBUG(glGetError)();

                //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                //// stolen from SDL ;)
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                // assert( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                //// it is not necesssaty to mess with the alignment
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1 );
                // assert( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = (Uint8 *)temp->pixels;
                for (y = rect.y; y < rect.y + rect.h; y++)
                {
                    GL_DEBUG(glReadPixels)(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                    pixels += temp->pitch;
                }
                assert( GL_NO_ERROR == GL_DEBUG(glGetError)() );
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
            debug_printf( "Saved to %s", szFilename );
        }
    }
    GL_DEBUG(glPopClientAttrib)();

    return savefound;
}

//---------------------------------------------------------------------------------------------------
void load_graphics()
{
    // ZF> This function loads all the graphics based on the game settings
    GLenum quality;

    // Check if the computer graphic driver supports anisotropic filtering

    if ( !ogl_caps.anisotropic_supported )
    {
        if ( tex_params.texturefilter >= TX_ANISOTROPIC )
        {
            tex_params.texturefilter = TX_TRILINEAR_2;
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
        }
    }

    // Enable prespective correction?
    if ( gfx.perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;
    GL_DEBUG(glHint)(GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering?
    if ( gfx.dither )
    {
        GL_DEBUG(glHint)(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        GL_DEBUG(glEnable)(GL_DITHER );
    }
    else
    {
        GL_DEBUG(glHint)(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
        GL_DEBUG(glDisable)(GL_DITHER );
    }

    // Enable gourad shading? (Important!)
    GL_DEBUG(glShadeModel)( gfx.shading );

    // Enable antialiasing?
    if ( gfx.antialiasing )
    {
        GL_DEBUG(glEnable)(GL_MULTISAMPLE_ARB);

        GL_DEBUG(glEnable)( GL_LINE_SMOOTH );
        GL_DEBUG(glHint)(GL_LINE_SMOOTH_HINT,    GL_NICEST );

        GL_DEBUG(glEnable)( GL_POINT_SMOOTH );
        GL_DEBUG(glHint)(GL_POINT_SMOOTH_HINT,   GL_NICEST );

        GL_DEBUG(glDisable)( GL_POLYGON_SMOOTH );
        GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        // GL_DEBUG(glEnable)(GL_BLEND);
        // GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        // GL_DEBUG(glEnable)(GL_POLYGON_SMOOTH);
        // GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        GL_DEBUG(glDisable)(GL_MULTISAMPLE_ARB);
        GL_DEBUG(glDisable)(GL_POINT_SMOOTH );
        GL_DEBUG(glDisable)(GL_LINE_SMOOTH );
        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH );
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
    if ( fTmp < 0 ) fTmp = 0;

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

    for ( cnt = 0; cnt < MADLIGHTINDICES - 1; cnt++ )  // Spikey mace
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
    for ( cnt = 0; cnt < MADLIGHTINDICES; cnt++ )
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
    //    go when projected onto the plane of the PMesh->  Used later for
    //    determining which mesh fans need to be rendered

    int cnt, tnc, extra[4];
    float ztemp;
    float numstep;
    float zproject;
    float xfin, yfin, zfin;
    GLmatrix mTemp;

    // Range
    ztemp = ( pcam->pos.z );

    // Topleft
    mTemp = MatrixMult( RotateY( -rotmeshtopside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[0] = xfin;
        cornery[0] = yfin;
    }

    // Topright
    mTemp = MatrixMult( RotateY( rotmeshtopside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[1] = xfin;
        cornery[1] = yfin;
    }

    // Bottomright
    mTemp = MatrixMult( RotateY( rotmeshbottomside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
        zfin = 0;
        cornerx[2] = xfin;
        cornery[2] = yfin;
    }

    // Bottomleft
    mTemp = MatrixMult( RotateY( -rotmeshbottomside * PI / 360 ), PCamera->mView );
    mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
    zproject = mTemp.CNV( 2, 2 );             // 2,2
    // Camera must look down
    if ( zproject < 0 )
    {
        numstep = -ztemp / zproject;
        xfin = pcam->pos.x + ( numstep * mTemp.CNV( 0, 2 ) );  // xgg      // 0,2
        yfin = pcam->pos.y + ( numstep * mTemp.CNV( 1, 2 ) );    // 1,2
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

    while ( cnt < MAX_MESSAGE )
    {
        DisplayMsg.lst[cnt].time = 0;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_dynalist( camera_t * pcam )
{
    // ZZ> This function figures out which particles are visible, and it sets up dynamic
    //    lighting
    int cnt, tnc, slot;
    float disx, disy, disz, distance;

    // Don't really make a list, just set to visible or not
    dyna_list_count = 0;
    dyna_distancetobeat = MAXDYNADIST * MAXDYNADIST;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PrtList.lst[cnt].inview = bfalse;
        if ( !PrtList.lst[cnt].on ) continue;

        if ( !VALID_TILE(PMesh, PrtList.lst[cnt].onwhichfan) ) continue;

        PrtList.lst[cnt].inview = PMesh->mmem.tile_list[PrtList.lst[cnt].onwhichfan].inrenderlist;

        // Set up the lights we need
        if ( !PrtList.lst[cnt].dynalighton ) continue;

        disx = PrtList.lst[cnt].pos.x - pcam->track_pos.x;
        disy = PrtList.lst[cnt].pos.y - pcam->track_pos.y;
        disz = PrtList.lst[cnt].pos.z - pcam->track_pos.z;

        distance = disx * disx + disy * disy + disz * disz;
        if ( distance < dyna_distancetobeat )
        {
            bool_t found = bfalse;
            if ( dyna_list_count < gfx.dyna_list_max )
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
                for ( tnc = 1; tnc < gfx.dyna_list_max; tnc++ )
                {
                    if ( dyna_list[tnc].distance > dyna_distancetobeat )
                    {
                        slot = tnc;
                        found = btrue;
                    }
                }

                if ( found )
                {
                    dyna_list[slot].distance = distance;

                    // Find the new distance to beat
                    dyna_distancetobeat = dyna_list[0].distance;
                    for ( tnc = 1; tnc < gfx.dyna_list_max; tnc++ )
                    {
                        if ( dyna_list[tnc].distance > dyna_distancetobeat )
                        {
                            dyna_distancetobeat = dyna_list[tnc].distance;
                        }
                    }
                }
            }

            if ( found )
            {
                dyna_list[slot].x       = PrtList.lst[cnt].pos.x;
                dyna_list[slot].y       = PrtList.lst[cnt].pos.y;
                dyna_list[slot].z       = PrtList.lst[cnt].pos.z;
                dyna_list[slot].level   = PrtList.lst[cnt].dynalightlevel;
                dyna_list[slot].falloff = PrtList.lst[cnt].dynalightfalloff;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t dolist_add_chr( ego_mpd_t * pmesh, Uint16 ichr )
{
    // This function puts a character in the list
    Uint32 itile;
    chr_t * pchr;
    cap_t * pcap;
    chr_instance_t * pinst;

    if ( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( INVALID_CHR(ichr) ) return bfalse;
    pchr  = ChrList.lst + ichr;
    pinst = &(pchr->inst);

    if ( pinst->indolist ) return btrue;

    if ( INVALID_CAP( pchr->model ) ) return bfalse;
    pcap = CapList + pchr->model;

    itile = pchr->onwhichfan;
    if ( !VALID_TILE(pmesh, itile) ) return bfalse;

    if ( pmesh->mmem.tile_list[itile].inrenderlist )
    {
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
        dolist_add_chr( pmesh, pchr->holdingwhich[SLOT_LEFT] );
        dolist_add_chr( pmesh, pchr->holdingwhich[SLOT_RIGHT] );
    }

    return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t dolist_add_prt( ego_mpd_t * pmesh, Uint16 iprt )
{
    // This function puts a character in the list
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( dolist_count >= DOLIST_SIZE ) return bfalse;

    if ( INVALID_PRT(iprt) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &(pprt->inst);

    if ( pinst->indolist ) return btrue;

    if ( 0 == pinst->size || pprt->is_hidden || !VALID_TILE(pmesh, pprt->onwhichfan) ) return bfalse;

    dolist[dolist_count].ichr = MAX_CHR;
    dolist[dolist_count].iprt = iprt;
    dolist_count++;

    pinst->indolist = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void dolist_make( ego_mpd_t * pmesh )
{
    // ZZ> This function finds the characters that need to be drawn and puts them in the list

    Uint32 cnt;

    // Remove everyone from the dolist
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && MAX_CHR != dolist[cnt].ichr )
        {
            ChrList.lst[ dolist[cnt].ichr ].inst.indolist = bfalse;
        }
        else if ( MAX_CHR == dolist[cnt].ichr && TOTAL_MAX_PRT != dolist[cnt].iprt )
        {
            PrtList.lst[ dolist[cnt].iprt ].inst.indolist = bfalse;
        }
    }
    dolist_count = 0;

    // Now fill it up again
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( ChrList.lst[cnt].on && !ChrList.lst[cnt].pack_ispacked )
        {
            // Add the character
            dolist_add_chr( pmesh, cnt );
        }
    }

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        if ( PrtList.lst[cnt].on && VALID_TILE(pmesh, PrtList.lst[cnt].onwhichfan) )
        {
            // Add the character
            dolist_add_prt( pmesh, cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void dolist_sort( camera_t * pcam )
{
    // ZZ> This function orders the dolist based on distance from camera,
    //    which is needed for reflections to properly clip themselves.
    //    Order from closest to farthest

    Uint32    cnt, tnc;
    GLvector3 vcam;
    size_t    count;

    vcam = mat_getCamForward(pcam->mView);

    // Figure the distance of each
    count = 0;
    for ( cnt = 0; cnt < dolist_count; cnt++ )
    {
        GLvector3 vtmp;
        float dist;

        if ( TOTAL_MAX_PRT == dolist[cnt].iprt && VALID_CHR(dolist[cnt].ichr) )
        {
            tnc = dolist[cnt].ichr;
            vtmp = VSub( ChrList.lst[tnc].pos, pcam->pos );
        }
        else if ( MAX_CHR == dolist[cnt].ichr && VALID_PRT(dolist[cnt].iprt) )
        {
            tnc = dolist[cnt].iprt;
            vtmp = VSub( PrtList.lst[tnc].pos, pcam->pos );
        }
        else
        {
            continue;
        }

        dist = VDotProduct( vtmp, vcam );
        if ( dist > 0 )
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

    int   rv;
    float diff;

    diff = dleft->dist - dright->dist;

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t gfx_config_synch(gfx_config_t * pgfx, egoboo_config_t * pcfg )
{
    float kx, ky;

    // call gfx_config_init(), even if the config data is invalid
    if ( !gfx_config_init( pgfx ) ) return bfalse;

    // if there is no config data, do not proceed
    if ( NULL == pcfg ) return bfalse;

    pgfx->antialiasing = pcfg->multisamples > 0;

    pgfx->refon        = pcfg->reflect_allowed;
    pgfx->reffadeor    = pcfg->reflect_fade ? 0 : 255;

    pgfx->shaon        = pcfg->shadow_allowed;
    pgfx->shasprite    = pcfg->shadow_sprite;

    pgfx->shading      = pcfg->gourard_req ? GL_SMOOTH : GL_FLAT;
    pgfx->dither       = pcfg->use_dither;
    pgfx->perspective  = pcfg->use_perspective;
    pgfx->phongon      = pcfg->use_phong;

    pgfx->draw_background = pcfg->background_allowed && water.background_req;
    pgfx->draw_overlay    = pcfg->overlay_allowed && water.overlay_req;

    pgfx->dyna_list_max = CLIP(pcfg->dyna_count_req, 0, TOTAL_MAX_DYNA);

    pgfx->draw_water_0 = !pgfx->draw_overlay && (water.layer_count > 0);
    pgfx->clearson     = !pgfx->draw_background;
    pgfx->draw_water_1 = !pgfx->draw_background && (water.layer_count > 1);

    kx = (float)GFX_WIDTH  / (float)sdl_scr.x;
    ky = (float)GFX_HEIGHT / (float)sdl_scr.y;

    if ( kx == ky )
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y;
    }
    else if ( kx > ky )
    {
        pgfx->vw = sdl_scr.x * kx / ky;
        pgfx->vh = sdl_scr.y;
    }
    else
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y * ky / kx;
    }

    pgfx->vdw = (GFX_WIDTH  - pgfx->vw) * 0.5f;
    pgfx->vdh = (GFX_HEIGHT - pgfx->vh) * 0.5f;

    ui_set_virtual_screen(pgfx->vw, pgfx->vh, GFX_WIDTH, GFX_HEIGHT);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_config_init ( gfx_config_t * pgfx )
{
    if (NULL == pgfx) return bfalse;

    pgfx->shading          = GL_SMOOTH;
    pgfx->refon            = btrue;
    pgfx->reffadeor        = 0;
    pgfx->antialiasing     = bfalse;
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

    pgfx->dyna_list_max    = 8;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t oglx_texture_parameters_synch( oglx_texture_parameters_t * ptex, egoboo_config_t * pcfg )
{
    /// BB > synch the texture parameters with the video mode

    if ( NULL == ptex || NULL == pcfg ) return GL_FALSE;

    if ( ogl_caps.maxAnisotropy == 0.0f )
    {
        ptex->userAnisotropy = 0.0f;
        ptex->texturefilter  = (TX_FILTERS)MIN( pcfg->texturefilter_req, TX_TRILINEAR_2 );
    }
    else
    {
        ptex->texturefilter  = (TX_FILTERS)MIN( pcfg->texturefilter_req, TX_FILTER_COUNT );
        ptex->userAnisotropy = ogl_caps.maxAnisotropy * MAX(0, (int)ptex->texturefilter - (int)TX_TRILINEAR_2);
    }

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
bool_t interpolate_mesh_lighting( ego_mpd_t * pmesh, lighting_cache_t * dst, GLvector3 pos )
{
    lighting_cache_t * cache_list[4];
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v, min_x, max_x, min_y, max_y;

    fan[0] = mesh_get_tile( pmesh, pos.x,             pos.y             );
    fan[1] = mesh_get_tile( pmesh, pos.x + TILE_SIZE, pos.y             );
    fan[2] = mesh_get_tile( pmesh, pos.x,             pos.y + TILE_SIZE );
    fan[3] = mesh_get_tile( pmesh, pos.x + TILE_SIZE, pos.y + TILE_SIZE );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        cache_list[cnt] = VALID_TILE(pmesh, fan[cnt]) ? &(pmesh->gmem.light[fan[cnt]].cache) : NULL;
    }

    ix    = floor( pos.x / TILE_SIZE );
    min_x = ix * TILE_SIZE;
    max_x = (ix + 1) * TILE_SIZE;

    iy    = floor( pos.y / TILE_SIZE );
    min_y = iy * TILE_SIZE;
    max_y = (iy + 1) * TILE_SIZE;

    u = (pos.x - min_x) / (max_x - min_x);
    v = (pos.y - min_y) / (max_y - min_y);

    return interpolate_lighting( dst, cache_list, u, v );
}

//--------------------------------------------------------------------------------------------
bool_t project_lighting( lighting_cache_t * dst, lighting_cache_t * src, GLmatrix mat )
{
    int cnt;
    GLvector3 fwd, right, up;

    // blank the destination lighting
    if ( NULL == dst ) return bfalse;

    dst->max_light = 0.0f;
    for ( cnt = 0; cnt < 6; cnt++)
    {
        dst->lighting_low[cnt] = 0.0f;
        dst->lighting_hgh[cnt] = 0.0f;
    }

    if ( NULL == src ) return bfalse;
    if ( src->max_light <= 0.0f ) return btrue;

    // grab the character directions
    fwd   = VNormalize( mat_getChrForward( mat ) );         // along body-fixed +y-axis
    right = VNormalize( mat_getChrRight( mat ) );        // along body-fixed +x-axis
    up    = VNormalize( mat_getChrUp( mat ) );            // along body-fixed +z axis

    // split the lighting cache up
    project_sum_lighting( dst, src, right, 0 );
    project_sum_lighting( dst, src, fwd,   2 );
    project_sum_lighting( dst, src, up,    4 );

    // determine the maximum lighting amount
    dst->max_light = 0.0f;
    for ( cnt = 0; cnt < 6; cnt++ )
    {
        dst->max_light = MAX(dst->max_light, dst->lighting_low[cnt]);
        dst->max_light = MAX(dst->max_light, dst->lighting_hgh[cnt]);
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t interpolate_lighting( lighting_cache_t * dst, lighting_cache_t * src[], float u, float v )
{
    int   cnt, tnc;
    float wt_sum;

    if ( NULL == dst ) return bfalse;

    dst->max_light = 0.0f;
    for ( cnt = 0; cnt < 6; cnt++ )
    {
        dst->lighting_low[cnt] = 0.0f;
        dst->lighting_hgh[cnt] = 0.0f;
    }

    if ( NULL == src ) return bfalse;

    u = CLIP(u, 0, 1);
    v = CLIP(v, 0, 1);

    wt_sum = 0.0f;
    if ( NULL != src[0] )
    {
        float wt = (1 - u) * (1 - v);
        for (tnc = 0; tnc < 6; tnc++)
        {
            dst->lighting_low[tnc] += src[0]->lighting_low[tnc] * wt;
            dst->lighting_hgh[tnc] += src[0]->lighting_hgh[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[1] )
    {
        float wt = u * (1 - v);
        for (tnc = 0; tnc < 6; tnc++)
        {
            dst->lighting_low[tnc] += src[1]->lighting_low[tnc] * wt;
            dst->lighting_hgh[tnc] += src[1]->lighting_hgh[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[2] )
    {
        float wt = (1 - u) * v;
        for (tnc = 0; tnc < 6; tnc++)
        {
            dst->lighting_low[tnc] += src[2]->lighting_low[tnc] * wt;
            dst->lighting_hgh[tnc] += src[2]->lighting_hgh[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( NULL != src[3] )
    {
        float wt = u * v;
        for (tnc = 0; tnc < 6; tnc++)
        {
            dst->lighting_low[tnc] += src[3]->lighting_low[tnc] * wt;
            dst->lighting_hgh[tnc] += src[3]->lighting_hgh[tnc] * wt;
        }
        wt_sum += wt;
    }

    if ( wt_sum > 0.0f )
    {
        for (tnc = 0; tnc < 6; tnc++)
        {
            dst->lighting_low[tnc] /= wt_sum;
            dst->lighting_hgh[tnc] /= wt_sum;

            dst->max_light = MAX(dst->max_light, dst->lighting_low[tnc]);
            dst->max_light = MAX(dst->max_light, dst->lighting_hgh[tnc]);
        }
    }

    return wt_sum > 0.0f;
}

//--------------------------------------------------------------------------------------------
bool_t project_sum_lighting( lighting_cache_t * dst, lighting_cache_t * src, GLvector3 vec, int dir )
{
    if ( NULL == src || NULL == dst ) return bfalse;

    if ( dir < 0 || dir > 4 || 0 != (dir&1) )
        return bfalse;

    if ( vec.x > 0 )
    {
        dst->lighting_low[dir+0] += ABS(vec.x) * src->lighting_low[0];
        dst->lighting_hgh[dir+1] += ABS(vec.x) * src->lighting_hgh[1];

        dst->lighting_low[dir+0] += ABS(vec.x) * src->lighting_low[0];
        dst->lighting_hgh[dir+1] += ABS(vec.x) * src->lighting_hgh[1];
    }
    else if (vec.x < 0)
    {
        dst->lighting_low[dir+0] += ABS(vec.x) * src->lighting_low[1];
        dst->lighting_hgh[dir+1] += ABS(vec.x) * src->lighting_hgh[0];

        dst->lighting_low[dir+0] += ABS(vec.x) * src->lighting_low[1];
        dst->lighting_hgh[dir+1] += ABS(vec.x) * src->lighting_hgh[0];
    }

    if ( vec.y > 0 )
    {
        dst->lighting_low[dir+0] += ABS(vec.y) * src->lighting_low[2];
        dst->lighting_hgh[dir+1] += ABS(vec.y) * src->lighting_hgh[3];

        dst->lighting_low[dir+0] += ABS(vec.y) * src->lighting_low[2];
        dst->lighting_hgh[dir+1] += ABS(vec.y) * src->lighting_hgh[3];
    }
    else if (vec.y < 0)
    {
        dst->lighting_low[dir+0] += ABS(vec.y) * src->lighting_low[3];
        dst->lighting_hgh[dir+1] += ABS(vec.y) * src->lighting_hgh[2];

        dst->lighting_low[dir+0] += ABS(vec.y) * src->lighting_low[3];
        dst->lighting_hgh[dir+1] += ABS(vec.y) * src->lighting_hgh[2];
    }

    if ( vec.z > 0 )
    {
        dst->lighting_low[dir+0] += ABS(vec.z) * src->lighting_low[4];
        dst->lighting_hgh[dir+1] += ABS(vec.z) * src->lighting_hgh[5];

        dst->lighting_low[dir+0] += ABS(vec.z) * src->lighting_low[4];
        dst->lighting_hgh[dir+1] += ABS(vec.z) * src->lighting_hgh[5];
    }
    else if (vec.z < 0)
    {
        dst->lighting_low[dir+0] += ABS(vec.z) * src->lighting_low[5];
        dst->lighting_hgh[dir+1] += ABS(vec.z) * src->lighting_hgh[4];

        dst->lighting_low[dir+0] += ABS(vec.z) * src->lighting_low[5];
        dst->lighting_hgh[dir+1] += ABS(vec.z) * src->lighting_hgh[4];
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bbox_gl_draw(aabb_t * pbbox)
{
    GLXvector3f * pmin, * pmax;

    if (NULL == pbbox) return bfalse;

    GL_DEBUG(glPushMatrix)();
    {
        pmin = &(pbbox->mins);
        pmax = &(pbbox->maxs);

        // !!!! there must be an optimized way of doing this !!!!

        GL_DEBUG(glBegin)(GL_QUADS);
        {
            // Front Face
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmax)[ZZ]);

            // Back Face
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmin)[ZZ]);

            // Top Face
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmin)[ZZ]);

            // Bottom Face
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmax)[ZZ]);

            // Right face
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmax)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmax)[XX], (*pmin)[YY], (*pmax)[ZZ]);

            // Left Face
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmin)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmin)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmax)[ZZ]);
            glVertex3f((*pmin)[XX], (*pmax)[YY], (*pmin)[ZZ]);
        }
        GL_DEBUG_END();
    }
    GL_DEBUG(glPopMatrix)();

    return btrue;
}

//--------------------------------------------------------------------------------------------
void release_all_object_textures()
{
    int cnt, tnc;
    mad_t  * pmad;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        if ( !MadList[cnt].loaded ) continue;

        pmad = MadList + cnt;

        for ( tnc = 0; tnc < MAXSKIN; tnc++ )
        {
            int itex;

            itex = pmad->tex_ref[tnc] ;
            if ( itex > TX_LAST )
            {
                TxTexture_free_one( itex );
            }

            itex = pmad->ico_ref[tnc] ;
            if ( itex > TX_LAST )
            {
                TxTexture_free_one( itex );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
billboard_data_t * billboard_data_init(billboard_data_t * pbb)
{
    if ( NULL == pbb ) return pbb;

    memset( pbb, 0, sizeof(billboard_data_t) );

    pbb->tex_ref = INVALID_TEXTURE;
    pbb->ichr    = MAX_CHR;

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_free(billboard_data_t * pbb)
{
    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // free any allocated texture
    TxTexture_free_one( pbb->tex_ref );

    billboard_data_init(pbb);

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_update( billboard_data_t * pbb )
{
    GLvector3   vup, pos_new;
    chr_t     * pchr;
    float       height, offset;

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    if ( INVALID_CHR(pbb->ichr) ) return bfalse;
    pchr = ChrList.lst + pbb->ichr;

    // determine where the new position should be
    if ( pchr->inst.matrixvalid )
    {
        vup = mat_getChrUp( pchr->inst.matrix );
    }
    else
    {
        vup.x = vup.y = 0.0f;
        vup.z = 1.0f;
    }

    height = pchr->bumpheight;
    offset = MIN(pchr->bumpheight * 0.5f, pchr->bumpsize);

    pos_new.x = pchr->pos.x + vup.x * (height + offset);
    pos_new.y = pchr->pos.y + vup.y * (height + offset);
    pos_new.z = pchr->pos.z + vup.z * (height + offset);

    // allow the billboards to be a bit bouncy
    pbb->pos.x = pbb->pos.x * 0.5f + pos_new.x * 0.5f;
    pbb->pos.y = pbb->pos.y * 0.5f + pos_new.y * 0.5f;
    pbb->pos.z = pbb->pos.z * 0.5f + pos_new.z * 0.5f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t billboard_data_printf_ttf( billboard_data_t * pbb, Font *font, SDL_Color color, const char * format, ... )
{
    va_list args;
    int rv;
    oglx_texture * ptex;
    float texCoords[4];

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // release any existing texture in case there is an error
    ptex = TxTexture_get_ptr(pbb->tex_ref);
    oglx_texture_Release( ptex );

    va_start( args, format );
    rv = fnt_vprintf( font, color, &(ptex->surface), ptex->base.binding, texCoords, format, args );
    va_end( args );

    ptex->base_valid = bfalse;
    oglx_grab_texture_state( GL_TEXTURE_2D, 0, ptex );

    ptex->alpha = 1.0f;
    ptex->imgW  = ptex->surface->w;
    ptex->imgH  = ptex->surface->h;
    strncpy( ptex->name, "billboard text", SDL_arraysize(ptex->name) );

    return ( rv >= 0 );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void BillboardList_clear_data()
{
    // BB> reset the free billboard list.

    int cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        BillboardList.free_ref[cnt] = cnt;
    }
    BillboardList.free_count = cnt;

}

//--------------------------------------------------------------------------------------------
void BillboardList_init_all()
{
    int cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        billboard_data_init( BillboardList.lst + cnt );
    }

    BillboardList_clear_data();
}

//--------------------------------------------------------------------------------------------
void BillboardList_update_all()
{
    Uint32 cnt, ticks;

    ticks = SDL_GetTicks();

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        bool_t is_invalid;

        billboard_data_t * pbb = BillboardList.lst + cnt;

        if ( !pbb->valid ) continue;

        is_invalid = bfalse;
        if ( ticks >= pbb->time || NULL == TxTexture_get_ptr( pbb->tex_ref ) )
        {
            is_invalid = btrue;
        }

        if( INVALID_CHR(pbb->ichr) || VALID_CHR(ChrList.lst[pbb->ichr].attachedto) )
        {
            is_invalid = btrue;
        }

        if ( is_invalid )
        {
            // the billboard has expired

            // unlink it from the character
            if ( VALID_CHR(pbb->ichr) )
            {
                ChrList.lst[pbb->ichr].ibillboard = INVALID_BILLBOARD;
            }

            // deallocate the billboard
            BillboardList_free_one(cnt);
        }
        else
        {
            billboard_data_update( BillboardList.lst + cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void BillboardList_free_all()
{
    int cnt;

    for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
    {
        if ( !BillboardList.lst[cnt].valid ) continue;

        billboard_data_update( BillboardList.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
int BillboardList_get_free( Uint32 lifetime_secs )
{
    int                itex = INVALID_TEXTURE;
    int                ibb  = INVALID_BILLBOARD;
    billboard_data_t * pbb  = NULL;

    if ( BillboardList.free_count <= 0 ) return INVALID_BILLBOARD;

    if ( 0 == lifetime_secs ) return INVALID_BILLBOARD;

    itex = TxTexture_get_free( INVALID_TEXTURE );
    if ( INVALID_TEXTURE == itex ) return INVALID_BILLBOARD;

    // grab the top index
    BillboardList.free_count--;
    ibb = BillboardList.free_ref[BillboardList.free_count];

    if ( VALID_BILLBOARD_RANGE(ibb) )
    {
        pbb = BillboardList.lst + ibb;
        billboard_data_init( pbb );

        pbb->tex_ref = itex;
        pbb->time    = SDL_GetTicks() + lifetime_secs * TICKS_PER_SEC;
        pbb->valid   = btrue;
    }
    else
    {
        // the billboard allocation returned an ivaild value
        // deallocate the texture
        TxTexture_free_one( itex );

        ibb = INVALID_BILLBOARD;
    }

    return ibb;
}

//--------------------------------------------------------------------------------------------
bool_t BillboardList_free_one(int ibb)
{
    billboard_data_t * pbb;

    if ( !VALID_BILLBOARD_RANGE(ibb) ) return bfalse;

    pbb = BillboardList.lst + ibb;

    billboard_data_free( pbb );

#if defined(DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < BillboardList.free_count; cnt++ )
        {
            if ( ibb == BillboardList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    if ( BillboardList.free_count >= BILLBOARD_COUNT )
        return bfalse;

    // do not put anything below TX_LAST back onto the SDL_free stack
    BillboardList.free_ref[BillboardList.free_count] = ibb;
    BillboardList.free_count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
billboard_data_t * BillboardList_get_ptr( int ibb )
{
    if ( !VALID_BILLBOARD(ibb) ) return NULL;

    return BillboardList.lst + ibb;
}
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t render_billboard( camera_t * pcam, billboard_data_t * pbb, float scale )
{
    int i;
    GLvertex vtlist[4];
    float x1, y1;
    float w, h;
    GLvector4 vector_up, vector_right;

    oglx_texture     * ptex;

    if ( NULL == pbb || !pbb->valid ) return bfalse;

    // do not display for objects that are mounted or being held
    if( VALID_CHR(pbb->ichr) && VALID_CHR(ChrList.lst[pbb->ichr].attachedto) ) return bfalse;

    ptex = TxTexture_get_ptr( pbb->tex_ref );

    oglx_texture_Bind( ptex );

    w = oglx_texture_GetImageWidth ( ptex );
    h = oglx_texture_GetImageHeight ( ptex );

    x1 = w  / ( float ) oglx_texture_GetTextureWidth ( ptex );
    y1 = h  / ( float ) oglx_texture_GetTextureHeight( ptex );

    vector_right.x =  pcam->mView.CNV(0, 0) * w * scale;
    vector_right.y =  pcam->mView.CNV(1, 0) * w * scale;
    vector_right.z =  pcam->mView.CNV(2, 0) * w * scale;

    vector_up.x    = -pcam->mView.CNV(0, 1) * h * scale;
    vector_up.y    = -pcam->mView.CNV(1, 1) * h * scale;
    vector_up.z    = -pcam->mView.CNV(2, 1) * h * scale;

    // bottom left
    vtlist[0].pos[XX] = pbb->pos.x + ( -vector_right.x - 0 * vector_up.x );
    vtlist[0].pos[YY] = pbb->pos.y + ( -vector_right.y - 0 * vector_up.y );
    vtlist[0].pos[ZZ] = pbb->pos.z + ( -vector_right.z - 0 * vector_up.z );
    vtlist[0].tex[SS] = x1;
    vtlist[0].tex[TT] = y1;

    // top left
    vtlist[1].pos[XX] = pbb->pos.x + ( -vector_right.x + 2 * vector_up.x );
    vtlist[1].pos[YY] = pbb->pos.y + ( -vector_right.y + 2 * vector_up.y );
    vtlist[1].pos[ZZ] = pbb->pos.z + ( -vector_right.z + 2 * vector_up.z );
    vtlist[1].tex[SS] = x1;
    vtlist[1].tex[TT] = 0;

    // top right
    vtlist[2].pos[XX] = pbb->pos.x + ( vector_right.x + 2 * vector_up.x );
    vtlist[2].pos[YY] = pbb->pos.y + ( vector_right.y + 2 * vector_up.y );
    vtlist[2].pos[ZZ] = pbb->pos.z + ( vector_right.z + 2 * vector_up.z );
    vtlist[2].tex[SS] = 0;
    vtlist[2].tex[TT] = 0;

    // bottom right
    vtlist[3].pos[XX] = pbb->pos.x + ( vector_right.x - 0 * vector_up.x );
    vtlist[3].pos[YY] = pbb->pos.y + ( vector_right.y - 0 * vector_up.y );
    vtlist[3].pos[ZZ] = pbb->pos.z + ( vector_right.z - 0 * vector_up.z );
    vtlist[3].tex[SS] = 0;
    vtlist[3].tex[TT] = y1;

    // Go on and draw it
    GL_DEBUG(glBegin)( GL_QUADS );
    {
        for ( i = 0; i < 4; i++ )
        {
            GL_DEBUG(glTexCoord2fv)( vtlist[i].tex );
            GL_DEBUG(glVertex3fv)  ( vtlist[i].pos );
        }
    }
    GL_DEBUG_END();

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_billboards( camera_t * pcam )
{
    int cnt;

    if ( NULL == pcam ) pcam = PCamera;
    if ( NULL == pcam ) return;

    Begin3DMode( pcam );
    {
        ATTRIB_PUSH( "render_all_billboards()", GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG(glShadeModel)( GL_FLAT );      // GL_LIGHTING_BIT - Flat shade this
            GL_DEBUG(glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT
            GL_DEBUG(glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT
            GL_DEBUG(glDisable   )( GL_CULL_FACE ); // GL_POLYGON_BIT | GL_ENABLE_BIT

            GL_DEBUG(glDisable   )( GL_CULL_FACE ); // GL_POLYGON_BIT | GL_ENABLE_BIT

            GL_DEBUG(glEnable)( GL_BLEND );                                       // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
            GL_DEBUG(glBlendFunc)( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );        // GL_COLOR_BUFFER_BIT

            GL_DEBUG(glColor4f)( 1.0f, 1.0f, 1.0f, 1.0f );

            for ( cnt = 0; cnt < BILLBOARD_COUNT; cnt++ )
            {
                billboard_data_t * pbb = BillboardList.lst + cnt;

                if ( !pbb->valid ) continue;

                render_billboard( pcam, pbb, 0.75 );
            }
        }
        ATTRIB_POP( "render_all_billboards()" );
    }
    End3DMode();

}

int get_free_line()
{
    int cnt;

    for( cnt = 0; cnt < LINE_COUNT; cnt++)
    {
        if( line_list[cnt].time < 0 )
        {
            break;
        }
    }

    return cnt < LINE_COUNT ? cnt : -1;
}

void draw_all_lines( camera_t * pcam )
{
    // BB> draw some lines for debugging purposes

    int cnt, ticks;

    GL_DEBUG(glDisable)( GL_TEXTURE_2D );

    Begin3DMode( pcam );
    {
        ATTRIB_PUSH( "render_all_billboards()", GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
        {
            GL_DEBUG(glShadeModel)( GL_FLAT );      // GL_LIGHTING_BIT - Flat shade this
            GL_DEBUG(glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT
            GL_DEBUG(glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT
            GL_DEBUG(glDisable   )( GL_CULL_FACE ); // GL_POLYGON_BIT | GL_ENABLE_BIT

            GL_DEBUG(glDisable)( GL_BLEND );                                       // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
            //GL_DEBUG(glBlendFunc)( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );        // GL_COLOR_BUFFER_BIT

            ticks = SDL_GetTicks();

            for( cnt = 0; cnt < LINE_COUNT; cnt++)
            {
                if( line_list[cnt].time < 0 ) continue;

                if( line_list[cnt].time < ticks )
                {
                    line_list[cnt].time = -1;
                    continue;
                }

                GL_DEBUG(glColor4fv)( line_list[cnt].color.v );
                GL_DEBUG(glBegin)( GL_LINES );
                {
                    glVertex3fv( line_list[cnt].src.v );
                    glVertex3fv( line_list[cnt].dst.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( "render_all_billboards()" );
    }
    End3DMode();

    GL_DEBUG(glEnable)( GL_TEXTURE_2D );

}