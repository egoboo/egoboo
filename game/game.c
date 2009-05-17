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

/* Egoboo - game.c
*/

#define DECLARE_GLOBALS

#include "egoboo.h"

#include "clock.h"
#include "link.h"
#include "ui.h"
#include "font.h"
#include "log.h"
#include "System.h"
#include "script.h"
#include "sound.h"
#include "graphic.h"
#include "passage.h"
#include "enchant.h"
#include "input.h"
#include "menu.h"
#include "file_common.h"
#include "network.h"

#include "char.h"
#include "particle.h"
#include "camera.h"
#include "id_md2.h"

#include "script_compile.h"
#include "script.h"

#include "egoboo_endian.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"

#include <SDL_image.h>

#include <time.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ATTACH_NONE                      0
#define ATTACH_INVENTORY                 1
#define ATTACH_LEFT                      2
#define ATTACH_RIGHT                     3

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_chr_setup_info
{
    STRING spawn_name;
    char   *pname;
    Sint32 slot;
    float  x, y, z;
    int    passage, content, money, level, skin;
    bool_t stat;
    Uint8  team;
    Uint16 facing;
    Uint16 attach;
    Uint16 parent;
};
typedef struct s_chr_setup_info chr_setup_info_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void make_randie();
static void reset_teams();
static void reset_messages();
static void reset_timers();
static void quit_game( void );

// looping - stuff called every loop - not accessible by scripts
static void do_enchant_spawn();
static void attach_particles();
static void check_stats();
static void tilt_characters_to_terrain();
static void bump_characters( void );
static void do_weather_spawn();
static void stat_return();
static void update_pits();
static void update_game();
static void update_timers();
static void make_onwhichfan( void );
static void set_local_latches( void );
static void make_onwhichfan( void );
static void let_all_characters_think();

// module initialization / deinitialization - not accessible by scripts
static bool_t load_module( const char *smallname );
static void   release_module();

static void   setup_characters( const char *modname );
static void   setup_alliances( const char *modname );
static void   load_all_messages( const char *loadname, Uint16 object );
static int    load_one_object( const char* tmploadname , int skin );
static int    load_all_objects( const char *modname );
static void   load_all_global_objects(int skin);

static bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo );
static bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo );
static void setup_characters( const char *modname );

// mesh initialization - not accessible by scripts
static void   make_twist();
static void   mesh_make_vrtstart( mesh_t * pmesh );
static void   mesh_make_fanstart( mesh_t * pmesh );

static mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem );
static mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem );
static bool_t       mesh_mem_free( mesh_mem_t * pmem );
static bool_t       mesh_mem_allocate( mesh_mem_t * pmem, mesh_info_t * pinfo );

static mesh_info_t * mesh_info_new( mesh_info_t * pinfo );
static mesh_info_t * mesh_info_delete( mesh_info_t * pinfo );
static void          mesh_info_init( mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  );

// Model stuff
static Uint16 test_frame_name( char letter );

static Uint16 action_number();
static Uint16 action_frame();
static void   action_check_copy( const char* loadname, Uint16 object );
static void   action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb );

static void   mad_get_framefx( int frame );
static void   mad_get_walk_frame( Uint16 object, int lip, int action );
static void   mad_make_equally_lit( int model );
static void   mad_make_framelip( Uint16 object, int action );
static void   mad_rip_actions( Uint16 object );

static void load_action_names( const char* loadname );
static void log_madused( const char *savename );

static bool_t game_begin_menu( int which );
static void   game_end_menu();

static void   memory_cleanUp(void);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

char idsz_string[5] = { '\0' };

static int    gamemenu_depth = -1;
static bool_t game_escape_requested = bfalse;

//--------------------------------------------------------------------------------------------
// Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local )
{
    // ZZ> This function exports a character
    int tnc = 0, profile;
    char fromdir[128];
    char todir[128];
    char fromfile[128];
    char tofile[128];
    char todirname[16];
    char todirfullname[64];

    // Don't export enchants
    disenchant_character( character );

    profile = ChrList[character].model;
    if ( ( CapList[profile].cancarrytonextmodule || !CapList[profile].isitem ) && exportvalid )
    {
        // TWINK_BO.OBJ
        sprintf(todirname, "%s", get_file_path(ChrList[owner].name) );

        // Is it a character or an item?
        if ( owner != character )
        {
            // Item is a subdirectory of the owner directory...
            sprintf( todirfullname, "%s" SLASH_STR "%d.obj", todirname, number );
        }
        else
        {
            // Character directory
            sprintf( todirfullname, "%s", todirname );
        }

        // players/twink.obj or players/twink.obj/sword.obj
        if ( is_local )
        {
            sprintf( todir, "players" SLASH_STR "%s", todirfullname );
        }
        else
        {
            sprintf( todir, "remote" SLASH_STR "%s", todirfullname );
        }

        // modules/advent.mod/objects/advent.obj
        sprintf( fromdir, "%s", MadList[profile].name );

        // Delete all the old items
        if ( owner == character )
        {
            tnc = 0;

            while ( tnc < 8 )
            {
                sprintf( tofile, "%s" SLASH_STR "%d.obj", todir, tnc );  /*.OBJ*/
                fs_removeDirectoryAndContents( tofile );
                tnc++;
            }
        }

        // Make the directory
        fs_createDirectory( todir );

        // Build the DATA.TXT file
        sprintf( tofile, "%s" SLASH_STR "data.txt", todir );  /*DATA.TXT*/
        export_one_character_profile( tofile, character );

        // Build the SKIN.TXT file
        sprintf( tofile, "%s" SLASH_STR "skin.txt", todir );  /*SKIN.TXT*/
        export_one_character_skin( tofile, character );

        // Build the NAMING.TXT file
        sprintf( tofile, "%s" SLASH_STR "naming.txt", todir );  /*NAMING.TXT*/
        export_one_character_name( tofile, character );

        // Copy all of the misc. data files
        sprintf( fromfile, "%s" SLASH_STR "message.txt", fromdir );  /*MESSAGE.TXT*/
        sprintf( tofile, "%s" SLASH_STR "message.txt", todir );  /*MESSAGE.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "tris.md2", fromdir );  /*TRIS.MD2*/
        sprintf( tofile,   "%s" SLASH_STR "tris.md2", todir );  /*TRIS.MD2*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "copy.txt", fromdir );  /*COPY.TXT*/
        sprintf( tofile,   "%s" SLASH_STR "copy.txt", todir );  /*COPY.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "script.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "script.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "enchant.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "enchant.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "credits.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "credits.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "quest.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "quest.txt", todir );
        fs_copyFile( fromfile, tofile );

        // Copy all of the particle files
        tnc = 0;

        while ( tnc < MAXPRTPIPPEROBJECT )
        {
            sprintf( fromfile, "%s" SLASH_STR "part%d.txt", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "part%d.txt", todir,   tnc );
            fs_copyFile( fromfile, tofile );
            tnc++;
        }

        // Copy all of the sound files
        tnc = 0;

        while ( tnc < MAXWAVE )
        {
            sprintf( fromfile, "%s" SLASH_STR "sound%d.wav", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.wav", todir,   tnc );
            fs_copyFile( fromfile, tofile );

            sprintf( fromfile, "%s" SLASH_STR "sound%d.ogg", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.ogg", todir,   tnc );
            fs_copyFile( fromfile, tofile );

            tnc++;
        }

        // Copy all of the image files (try to copy all supported formats too)
        tnc = 0;

        while ( tnc < 4 )
        {
            Uint8 type = 0;

            while (type < maxformattypes)
            {
                sprintf( fromfile, "%s" SLASH_STR "tris%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "tris%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
                sprintf( fromfile, "%s" SLASH_STR "icon%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "icon%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
                type++;
            }

            tnc++;
        }
    }
}

//---------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory
    bool_t is_local;
    int cnt, character, item, number;

    // Check each player
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        is_local = ( 0 != PlaList[cnt].device );
        if ( require_local && !is_local ) continue;
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList[character].on || !ChrList[character].alive ) continue;

        // Export the character
        number = 0;
        export_one_character( character, character, number, is_local );

        // Export the left hand item
        number = 0;
        item = ChrList[character].holdingwhich[number];
        if ( item != MAXCHR && ChrList[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the right hand item
        number = 1;
        item = ChrList[character].holdingwhich[number];
        if ( item != MAXCHR && ChrList[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the inventory
        number = 2;
        item = ChrList[character].nextinpack;
        while ( item != MAXCHR )
        {
            if ( ChrList[item].isitem )
            {
                export_one_character( item, character, number++, is_local );
            }

            item = ChrList[item].nextinpack;
        }
    }

}

//---------------------------------------------------------------------------------------------
void export_all_local_players( void )
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory

    // Check each player
    if ( exportvalid )
    {
        export_all_players( btrue );
    }
}

//---------------------------------------------------------------------------------------------
void quit_module()
{
    // ZZ> This function forces a return to the menu
    moduleactive = bfalse;
    hostactive = bfalse;

    export_all_local_players();

    release_all_icons();
    release_all_titleimages();
    release_bars();
    release_blip();
    release_map();
    release_all_textures();
    release_all_models();
    release_all_ai_scripts();

    gamepaused = bfalse;

    mesh_delete( &mesh );

    if ( mixeron  )
    {
        Mix_FadeOutChannel( -1, 500 );     // Stop all sounds that are playing
    }
}

//--------------------------------------------------------------------------------------------
void quit_game()
{
    // ZZ> This function exits the game entirely
    if ( gameactive )
    {
        gameactive = bfalse;
    }
    if ( moduleactive )
    {
        quit_module();
    }

    empty_import_directory();
}

//--------------------------------------------------------------------------------------------
void getadd( int min, int value, int max, int* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    int newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
void fgetadd( float min, float value, float max, float* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    float newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_action_names( const char* loadname )
{
    // ZZ> This function loads all of the 2 letter action names
    FILE* fileread;
    int cnt;
    char first, second;

    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        cnt = 0;

        while ( cnt < MAXACTION )
        {
            goto_colon( fileread );
            fscanf( fileread, "%c%c", &first, &second );
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cnt++;
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void log_madused( const char *savename )
{
    // ZZ> This is a debug function for checking model loads
    FILE* hFileWrite;
    int cnt;

    hFileWrite = fopen( savename, "w" );
    if ( hFileWrite )
    {
        fprintf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        fprintf( hFileWrite, "%d of %d frames used...\n", md2_loadframe, MAXFRAME );
        cnt = 0;

        while ( cnt < MAXMODEL )
        {
            fprintf( hFileWrite, "%3d %32s %s\n", cnt, CapList[cnt].classname, MadList[cnt].name );
            cnt++;
        }

        fclose( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_add( Uint16 character )
{
    // ZZ> This function adds a status display to the do list
    if ( numstat < MAXSTAT )
    {
        statlist[numstat] = character;
        ChrList[character].staton = btrue;
        numstat++;
    }
}

//--------------------------------------------------------------------------------------------
void statlist_move_to_top( Uint16 character )
{
    // ZZ> This function puts the character on top of the statlist
    int cnt, oldloc;

    // Find where it is
    oldloc = numstat;

    for ( cnt = 0; cnt < numstat; cnt++ )
    {
        if ( statlist[cnt] == character )
        {
            oldloc = cnt;
            cnt = numstat;
        }
    }

    // Change position
    if ( oldloc < numstat )
    {
        // Move all the lower ones up
        while ( oldloc > 0 )
        {
            oldloc--;
            statlist[oldloc+1] = statlist[oldloc];
        }

        // Put the character in the top slot
        statlist[0] = character;
    }
}

//--------------------------------------------------------------------------------------------
void statlist_sort()
{
    // ZZ> This function puts all of the local players on top of the statlist
    int cnt;

    for ( cnt = 0; cnt < numpla; cnt++ )
    {
        if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
        {
            statlist_move_to_top( PlaList[cnt].index );
        }
    }
}
//--------------------------------------------------------------------------------------------
void chr_play_action( Uint16 character, Uint16 action, Uint8 actionready )
{
    // ZZ> This function starts a generic action for a character
    
    chr_t * pchr;
    mad_t * pmad;

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if( pchr->inst.imad > MAXMODEL || !MadList[pchr->inst.imad].used ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        pchr->nextaction = ACTIONDA;
        pchr->action = action;

        pchr->inst.lip = 0;
        pchr->inst.lastframe = pchr->inst.frame;
        pchr->inst.frame     = pmad->actionstart[pchr->action];
        pchr->actionready    = actionready;
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip )
{
    // ZZ> This function sets the frame for a character explicitly...  This is used to
    //     rotate Tank turrets

    chr_t * pchr;
    mad_t * pmad;

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if( pchr->inst.imad > MAXMODEL || !MadList[pchr->inst.imad].used ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        int framesinaction, frame_stt, frame_end;

        pchr->nextaction = ACTIONDA;
        pchr->action = ACTIONDA;
        pchr->actionready = btrue;

        framesinaction = (pmad->actionend[action] - pmad->actionstart[action]) + 1;
        if( framesinaction <= 1 )
        {
            frame_stt = pmad->actionstart[action];
            frame_end = frame_stt;
        }
        else
        {
            frame %= framesinaction;
            frame_stt = pmad->actionstart[action] + frame;

            frame = (frame+1) % framesinaction;
            frame_end = frame_stt + 1;
        }

        pchr->inst.lip = ( lip << 6 );
        pchr->inst.lastframe = frame_stt;
        pchr->inst.frame     = frame_end;
    }
}

//--------------------------------------------------------------------------------------------
int generate_number( int numbase, int numrand )
{
    // ZZ> This function generates a random number
    int tmp = 0;

    tmp = numbase;
    if ( numrand > 0 )
    {
        tmp += ( rand() % numrand );
    }
    else
    {
        log_warning( "One of the data pairs is wrong! (%i and %i) Cannot be 0 or less.\n", numbase, numrand );
        numrand = numbase;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
void setup_alliances( const char *modname )
{
    // ZZ> This function reads the alliance file
    char newloadname[256];
    char szTemp[256];
    Uint8 teama, teamb;
    FILE *fileread;

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "alliance.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%s", szTemp );
            teama = ( szTemp[0] - 'A' ) % MAXTEAM;
            fscanf( fileread, "%s", szTemp );
            teamb = ( szTemp[0] - 'A' ) % MAXTEAM;
            TeamList[teama].hatesteam[teamb] = bfalse;
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
    // ZZ> This function precomputes surface normals and steep hill acceleration for
    //     the mesh
    Uint16 cnt;
    int x, y;
    float xslide, yslide;

    cnt = 0;

    while ( cnt < 256 )
    {
        y = (cnt >> 4) & 0x0F;
        x = (cnt >> 0) & 0x0F;
        y -= 7;  // -7 to 8
        x -= 7;  // -7 to 8

        mapudtwist[cnt] = 32768 + y * SLOPE;
        maplrtwist[cnt] = 32768 + x * SLOPE;

        if ( ABS( y ) >= 7 ) y = y << 1;
        if ( ABS( x ) >= 7 ) x = x << 1;

        xslide = x * SLIDE;
        yslide = y * SLIDE;
        if ( xslide < 0 )
        {
            xslide += SLIDEFIX;
            if ( xslide > 0 )
                xslide = 0;
        }
        else
        {
            xslide -= SLIDEFIX;
            if ( xslide < 0 )
                xslide = 0;
        }
        if ( yslide < 0 )
        {
            yslide += SLIDEFIX;
            if ( yslide > 0 )
                yslide = 0;
        }
        else
        {
            yslide -= SLIDEFIX;
            if ( yslide < 0 )
                yslide = 0;
        }

        veludtwist[cnt] = -yslide * hillslide;
        vellrtwist[cnt] =  xslide * hillslide;
        flattwist[cnt] = bfalse;
        if ( ABS( veludtwist[cnt] ) + ABS( vellrtwist[cnt] ) < SLIDEFIX*4 )
        {
            flattwist[cnt] = btrue;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_create( mesh_t * pmesh, int tiles_x, int tiles_y )
{
    if (NULL == pmesh)
    {
//        pmesh = calloc(1, sizeof(mesh_t));
        pmesh = mesh_new(pmesh);
    }

    if (NULL != pmesh)
    {
        // intitalize the mesh info using the max number of vertices for each tile
        mesh_info_init( &(pmesh->info), -1, tiles_x, tiles_y );

        // allocate the mesh memory
        mesh_mem_allocate( &(pmesh->mem), &(pmesh->info) );

        return pmesh;
    };

    return pmesh;
}

//--------------------------------------------------------------------------------------------
void mesh_info_init( mesh_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y  )
{
    // set the desired number of tiles
    pinfo->tiles_x = tiles_x;
    pinfo->tiles_y = tiles_y;
    pinfo->tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // set the desired number of fertices
    if ( numvert < 0 )
    {
        numvert = MAXMESHVERTICES * pinfo->tiles_count;
    };
    pinfo->vertcount = numvert;

    // set the desired blocknumber of blocks
    pinfo->blocks_x = (pinfo->tiles_x >> 2);
    if ( 0 != (pinfo->tiles_x & 0x03) ) pinfo->blocks_x++;

    pinfo->blocks_y = (pinfo->tiles_y >> 2);
    if ( 0 != (pinfo->tiles_y & 0x03) ) pinfo->blocks_y++;

    pinfo->blocks_count = pinfo->blocks_x * pinfo->blocks_y;

    // set the mesh edge info
    pinfo->edge_x = pinfo->tiles_x << 7;
    pinfo->edge_y = pinfo->tiles_y << 7;
};

//--------------------------------------------------------------------------------------------
mesh_t * mesh_load( const char *modname, mesh_t * pmesh )
{
    // ZZ> This function loads the level.mpd file
    FILE* fileread;
    char newloadname[256];
    int itmp, tiles_x, tiles_y;
    float ftmp;
    Uint32 fan, cnt;
    int numvert;
    Uint8 btemp;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh)
    {
//        pmesh = calloc(1, sizeof(mesh_t));
        pmesh = mesh_new(pmesh);
    }
    if (NULL == pmesh) return pmesh;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    // free any memory that has been allocated
    mesh_renew(pmesh);

    make_newloadname( modname, "gamedat" SLASH_STR "level.mpd", newloadname );
    fileread = fopen( newloadname, "rb" );
    if ( NULL == fileread )
    {
        log_warning( "Cannot find level.mpd!!\n" );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );
    if ( MAPID != ( Uint32 )ENDIAN_INT32( itmp ) )
    {
        log_warning( "This is not a valid level.mpd!!\n" );
        fclose( fileread );
        return NULL;
    }

    // Read the number of vertices
    fread( &itmp, 4, 1, fileread );  numvert   = ( int )ENDIAN_INT32( itmp );

    // grab the tiles in x and y
    fread( &itmp, 4, 1, fileread );  tiles_x = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_x >= MAXMESHTILEY )
    {
        mesh_delete( pmesh );
        log_warning( "Invalid mesh size. Mesh too large in x direction.\n" );
        fclose( fileread );
        return NULL;
    }

    fread( &itmp, 4, 1, fileread );  tiles_y = ( int )ENDIAN_INT32( itmp );
    if ( pinfo->tiles_y >= MAXMESHTILEY )
    {
        mesh_delete( pmesh );
        log_warning( "Invalid mesh size. Mesh too large in y direction.\n" );
        fclose( fileread );
        return NULL;
    }

    // intitalize the mesh info
    mesh_info_init( pinfo, numvert, tiles_x, tiles_y );

    // allocate the mesh memory
    if ( !mesh_mem_allocate( pmem, pinfo ) )
    {
        mesh_delete( pmesh );
        fclose( fileread );
        log_warning( "Could not allocate memory for the mesh!!\n" );
        return NULL;
    }

    // Load fan data
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        fread( &itmp, 4, 1, fileread );
        pmem->tile_list[fan].type = (ENDIAN_INT32( itmp ) >> 24) & 0xFF;
        pmem->tile_list[fan].fx   = (ENDIAN_INT32( itmp ) >> 16) & 0xFF;
        pmem->tile_list[fan].img  = (ENDIAN_INT32( itmp )      ) & 0xFFFF;
    }

    // Load twist data
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        fread( &itmp, 1, 1, fileread );
        pmem->tile_list[fan].twist = ENDIAN_INT32( itmp );
    }

    // Load vertex x data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_x[cnt] = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex y data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_y[cnt] = ENDIAN_FLOAT( ftmp );
    }

    // Load vertex z data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &ftmp, 4, 1, fileread );
        pmem->vrt_z[cnt] = ENDIAN_FLOAT( ftmp ) / 16.0f;  // Cartman uses 4 bit fixed point for Z
    }

    // Load vertex a data
    for ( cnt = 0; cnt < pmem->vertcount; cnt++ )
    {
        fread( &btemp, 1, 1, fileread );
        pmem->vrt_a[cnt] = btemp; //ENDIAN_INT32( itmp );
        pmem->vrt_l[cnt] = 0;
    }

    fclose( fileread );

    mesh_make_fanstart( pmesh );
    mesh_make_vrtstart( pmesh );

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff_u[cnt] = ( cnt & 7 ) / 8.0f;
        pmesh->tileoff_v[cnt] = ( cnt >> 3 ) / 8.0f;
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
void update_game()
{
    // ZZ> This function does several iterations of character movements and such
    //     to keep the game in sync.
    //     This is the main game loop
    int cnt, numdead;

    // Check for all local players being dead
    local_allpladead = bfalse;
    local_seeinvisible = bfalse;
    local_seekurse = bfalse;

    cnt = 0;
    numdead = 0;

    while ( cnt < MAXPLAYER )
    {
        if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
        {
            if ( !ChrList[PlaList[cnt].index].alive )
            {
                numdead++;
                if ( difficulty < GAME_HARD && local_allpladead && SDLKEYDOWN( SDLK_SPACE ) && respawnvalid && 0 == revivetimer )
                {
                    respawn_character( PlaList[cnt].index );
                    ChrList[cnt].experience *= EXPKEEP;  // Apply xp Penality
					ChrList[cnt].money *= EXPKEEP;
                }
            }
            else
            {
                if ( ChrList[PlaList[cnt].index].canseeinvisible )
                {
                    local_seeinvisible = btrue;
                }
                if ( ChrList[PlaList[cnt].index].canseekurse )
                {
                    local_seekurse = btrue;
                }
            }
        }

        cnt++;
    }
    if ( numdead >= local_numlpla )
    {
        local_allpladead = btrue;
    }

    sv_talkToRemotes();

    // [claforte Jan 6th 2001]
    // TODO: Put that back in place once networking is functional.
    while ( clock_wld < clock_all && numplatimes > 0 )
    {
        // Important stuff to keep in sync
        srand( randsave );
        randsave = rand();

        resize_characters();
        keep_weapons_with_holders();
        let_all_characters_think();
        do_weather_spawn();
        do_enchant_spawn();
        unbuffer_player_latches();

        make_onwhichfan();
        move_characters();
        move_particles();
        make_character_matrices();
        attach_particles();
        bump_characters();

        stat_return();
        update_pits();

        // Stuff for which sync doesn't matter
        animate_tiles();
        move_water();

        // Timers
        clock_wld  += UPDATE_SKIP;
        clock_stat += UPDATE_SKIP;

        //Reset the respawn timer
        if ( revivetimer > 0 )
        {
            revivetimer -= UPDATE_SKIP;
        }

        frame_wld++;
    }
    if ( networkon && !rtscontrol )
    {
        if ( numplatimes == 0 )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            clock_wld += 25;
        }
        if ( numplatimes > 3 && !hostactive )
        {
            // The host has too many messages, and is probably experiencing control
            // lag...  Speed it up so it gets closer to sync
            clock_wld -= 5;
        }
    }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
    // ZZ> This function updates the game timers
    clock_lst = clock_all;
    clock_all = SDL_GetTicks() - clock_stt;
    clock_fps += clock_all - clock_lst;
    if ( clock_fps >= TICKS_PER_SEC )
    {
        create_szfpstext( frame_fps );
        clock_fps = 0;
        frame_fps = 0;
    }
}

//--------------------------------------------------------------------------------------------
static void get_message( FILE* fileread )
{
    // ZZ> This function loads a string into the message buffer, making sure it
    //     is null terminated.
    int cnt;
    char cTmp;
    char szTmp[256];
    if ( msgtotal < MAXTOTALMESSAGE )
    {
        if ( msgtotalindex >= MESSAGEBUFFERSIZE )
        {
            msgtotalindex = MESSAGEBUFFERSIZE - 1;
        }

        msgindex[msgtotal] = msgtotalindex;
        fscanf( fileread, "%s", szTmp );
        szTmp[255] = 0;
        cTmp = szTmp[0];
        cnt = 1;

        while ( cTmp != 0 && msgtotalindex < MESSAGEBUFFERSIZE - 1 )
        {
            if ( cTmp == '_' )  cTmp = ' ';

            msgtext[msgtotalindex] = cTmp;
            msgtotalindex++;
            cTmp = szTmp[cnt];
            cnt++;
        }

        msgtext[msgtotalindex] = 0;  msgtotalindex++;
        msgtotal++;
    }
}

//--------------------------------------------------------------------------------------------
void load_all_messages( const char *loadname, Uint16 object )
{
    // ZZ> This function loads all of an objects messages
    FILE *fileread;

    MadList[object].msgstart = 0;
    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        MadList[object].msgstart = msgtotal;

        while ( goto_colon_yesno( fileread ) )
        {
            get_message( fileread );
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    // ZZ> This function makes everyone hate everyone else
    int teama, teamb;

    teama = 0;

    while ( teama < MAXTEAM )
    {
        // Make the team hate everyone
        teamb = 0;

        while ( teamb < MAXTEAM )
        {
            TeamList[teama].hatesteam[teamb] = btrue;
            teamb++;
        }

        // Make the team like itself
        TeamList[teama].hatesteam[teama] = bfalse;
        // Set defaults
        TeamList[teama].leader = NOLEADER;
        TeamList[teama].sissy = 0;
        TeamList[teama].morale = 0;
        teama++;
    }

    // Keep the null team neutral
    teama = 0;

    while ( teama < MAXTEAM )
    {
        TeamList[teama].hatesteam[NULLTEAM] = bfalse;
        TeamList[NULLTEAM].hatesteam[teama] = bfalse;
        teama++;
    }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    // ZZ> This makes messages safe to use
    int cnt;

    msgtotal = 0;
    msgtotalindex = 0;
    msgtimechange = 0;
    msgstart = 0;
    cnt = 0;

    while ( cnt < MAXMESSAGE )
    {
        msgtime[cnt] = 0;
        cnt++;
    }

    cnt = 0;

    while ( cnt < MAXTOTALMESSAGE )
    {
        msgindex[cnt] = 0;
        cnt++;
    }

    msgtext[0] = 0;
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    // ZZ> This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    cnt = 0;

    while ( cnt < MAXRAND )
    {
        randie[cnt] = rand() << 1;
        cnt++;
    }

    // Keep adjusting those values
    tnc = 0;

    while ( tnc < 20 )
    {
        cnt = 0;

        while ( cnt < MAXRAND )
        {
            randie[cnt] += rand();
            cnt++;
        }

        tnc++;
    }

    // All done
    randindex = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    // ZZ> This function resets the timers...
    clock_stt = SDL_GetTicks();
    clock_all = 0;
    clock_lst = 0;
    clock_wld = 0;
    clock_stat = 0;
    pitclock = 0;  pitskill = pitsfall = bfalse;

    frame_wld = 0;
    frame_all = 0;
    frame_fps = 0;
    outofsync = bfalse;
}


//--------------------------------------------------------------------------------------------
int game_do_menu( double frameDuration, bool_t needs_clear )
{
    int menuResult;

    // do menus
    if ( needs_clear )
    {
        glClear( GL_COLOR_BUFFER_BIT );
    };

    ui_beginFrame( frameDuration );
    {
        menuResult = doMenu( ( float )frameDuration );
        request_flip_pages();
    }
    ui_endFrame();

    return menuResult;
}

//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    // ZZ> This is where the program starts and all the high level stuff happens

    double frameDuration;
    int menuResult;
    int frame_next = 0, frame_now = 0;
    bool_t menu_was_active, menu_is_active;

    // Initialize logging first, so that we can use it everywhere.
    log_init();
    log_setLoggingLevel( 2 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n" );

    sys_initialize();
    clk_init();
    fs_init();

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL ); \
    GLSetup_SupportedFormats();

    // read the "setup.txt" file
    if ( !setup_read( "setup.txt" ) )
    {
        log_error( "Could not find Setup.txt\n" );
    }

    // download the "setup.txt" values into game variables
    setup_download();

    scantag_read_all( "basicdat" SLASH_STR "scancode.txt" );
    input_settings_load( "controls.txt" );

    release_all_ai_scripts();
    load_ai_codes( "basicdat" SLASH_STR "aicodes.txt" );
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    sdlinit( argc, argv );
    glinit( argc, argv );
    net_initialize();

    ui_initialize( "basicdat" SLASH_STR "Negatori.ttf", 24 );
    sound_initialize();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // Linking system
    log_info( "Initializing module linking... " );
    empty_import_directory();
    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    // Matrix init stuff (from remove.c)
    rotmeshtopside    = ( ( float )displaySurface->w / displaySurface->h ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = ( ( float )displaySurface->w / displaySurface->h ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = ( ( float )displaySurface->w / displaySurface->h ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = ( ( float )displaySurface->w / displaySurface->h ) * ROTMESHDOWN / ( 1.33333f );

    camera_new( &gCamera );

    // initialize all these structures
    init_all_icons();
    init_all_titleimages();
    init_bars();
    init_blip();
    init_map();
    init_all_textures();
    init_all_models();
    font_init();
    mesh_new( &mesh );

    // Load stuff into memory
    make_textureoffset();
    make_lightdirectionlookup();
    make_turntosin();
    make_enviro();
    tile_dictionary_load( tile_dict, MAXMESHTYPE );
    load_blip_bitmap();
    load_all_music_sounds();
    initMenus();        // Start the game menu

    // Let the normal OS mouse cursor work
    SDL_WM_GrabInput( SDL_GRAB_OFF );
    SDL_ShowCursor( btrue );

    // can't be done globally :(
    local_senseenemiesID = IDSZ_NONE;

    // Network's temporarily disabled
    clk_frameStep();
    frameDuration = clk_getFrameDuration();
    gameactive = btrue;

    // load all module info at game initialization
    // this will not change unless a new module is downloaded for a network game?
    modlist_load_all_info();

    menuactive = btrue;
    gameactive = bfalse;
    menu_was_active = menu_is_active = (menuactive || gamemenuactive);
    while ( gameactive || menuactive )
    {
        menu_was_active = menu_is_active;
        menu_is_active = (menuactive || gamemenuactive);

        if ( menu_is_active )
        {
            SDL_ShowCursor( SDL_ENABLE );
            SDL_WM_GrabInput ( SDL_GRAB_ON );
        }
        else
        {
            // restore mouse defaults
            SDL_WM_GrabInput ( gGrabMouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
            SDL_ShowCursor( gHideMouse ? SDL_DISABLE : SDL_ENABLE );
        }

        // Clock updates each frame
        clk_frameStep();
        frameDuration = clk_getFrameDuration();

        // do the panic button
        input_read();
        if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
        {
            menuactive = bfalse;
            gameactive = bfalse;
        }

        // Either run through the menus, or jump into the game
        if ( menuactive )
        {
            // Play the menu music
            sound_play_song( 0, 0, -1 );

            menuResult = game_do_menu( frameDuration, btrue);

            switch ( menuResult )
            {
                case MENU_SELECT:
                    // Go ahead and start the game
                    menuactive = bfalse;
                    gameactive = btrue;
                    networkon  = bfalse;
                    hostactive = btrue;
                    if ( gGrabMouse )
                    {
                        SDL_WM_GrabInput ( SDL_GRAB_ON );
                    }
                    if ( gHideMouse )
                    {
                        SDL_ShowCursor( 0 );  // Hide the mouse cursor
                    }
                    break;

                case MENU_QUIT:
                    // The user selected "Quit"
                    menuactive = bfalse;
                    gameactive = bfalse;
                    break;
            }
        }
        else if ( gameactive )
        {
            bool_t game_menu_is_active, game_menu_was_active;
            // Start a new module
            seed = time( NULL );

            moduleactive = game_init_module( pickedmodule, seed );

            game_menu_was_active = game_menu_is_active = gamemenuactive;
            game_escape_requested = bfalse;
            while ( moduleactive )
            {
                game_menu_was_active = menu_is_active;
                game_menu_is_active = gamemenuactive;

                if ( game_menu_is_active )
                {
                    SDL_ShowCursor( SDL_ENABLE );
                    SDL_WM_GrabInput ( SDL_GRAB_ON );
                }
                else
                {
                    // restore mouse defaults
                    SDL_WM_GrabInput ( gGrabMouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
                    SDL_ShowCursor( gHideMouse ? SDL_DISABLE : SDL_ENABLE );
                }

                // This is the control loop
                input_read();
                if ( networkon && console_done )
                {
                    net_send_message();
                }

                //Check for screenshots
                if ( !SDLKEYDOWN( SDLK_F11 ) ) screenshotkeyready = btrue;
                if ( SDLKEYDOWN( SDLK_F11 ) && keyb.on && screenshotkeyready )
                {
                    if ( !dump_screenshot() )                // Take the shot, returns bfalse if failed
                    {
                        debug_message( "Error writing screenshot!" );
                        log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
                    }

                    screenshotkeyready = bfalse;
                }

                // Do important things
                if ( !gamepaused || networkon )
                {
                    // start the console mode?
                    if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_MESSAGE ) )
                    {
                        // reset the keyboard buffer
                        SDL_EnableKeyRepeat(20, SDL_DEFAULT_REPEAT_DELAY);
                        console_mode = btrue;
                        console_done = bfalse;
                        keyb.buffer_count = 0;
                        keyb.buffer[0] = '\0';
                    }

                    check_stats();
                    set_local_latches();
                    update_timers();
                    check_passage_music();

                    // NETWORK PORT
                    listen_for_packets();
                    if ( !waitingforplayers )
                    {
                        cl_talkToHost();
                        update_game();
                    }
                    else
                    {
                        clock_wld = clock_all;
                    }
                }
                else
                {
                    update_timers();
                    clock_wld = clock_all;
                }

                // Do the display stuff
                frame_now = SDL_GetTicks();
                if (frame_now > frame_next)
                {
                    float  frameskip = (float)TICKS_PER_SEC / (float)framelimit;
                    frame_next = frame_now + frameskip; //FPS limit

                    camera_move(&gCamera);
                    figure_out_what_to_draw();

                    draw_main();

                    if ( gamemenuactive )
                    {
                        game_do_menu( frameDuration, bfalse );

                        if ( mnu_get_menu_depth() <= gamemenu_depth )
                        {
                            mnu_draw_background = btrue;
                            gamemenuactive = bfalse;
                            game_escape_requested = bfalse;
                            gamemenu_depth = -1;
                        }
                    }

                    msgtimechange++;
                    if ( statdelay > 0 )  statdelay--;
                }

                // Check for quitters
                // :TODO: local_noplayers is not set correctly
                if ( !game_escape_requested && ( SDLKEYDOWN( SDLK_ESCAPE ) /*|| local_noplayers*/ ) )
                {
                    game_escape_requested = btrue;

                    if( beatmodule )
                    {
                        game_begin_menu( ShowEndgame );
                    }
                    else
                    {
                        game_begin_menu( GamePaused );
                    }
                }

                do_flip_pages();
            }

            game_quit_module();
        }


        do_flip_pages();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp(void)
{
    //ZF> This function releases all loaded things in memory and cleans up everything properly

    log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

    // quit any existing game
    quit_game();

    // quit the setup system, making sure that the setup file is written
    setup_upload();
    setup_write();
    setup_quit();

    // make sure that the current control configuration is written
    input_settings_save( "controls.txt" );

    // shut down the ui
    ui_shutdown();

    // shut down the network
    if (networkon)
    {
        net_shutDown();
    }

    // shut down the clock services
    clk_shutdown();

    log_message("Success!\n");
    log_info( "Exiting Egoboo " VERSION " the good way...\n" );

    // shut down the log services
    log_shutdown();
}

//--------------------------------------------------------------------------------------------
int load_one_model_profile( const char* tmploadname, Uint16 object, int skin )
{
    int numskins, numicon;
    STRING newloadname;
    int cnt;
    mad_t * pmad;

    if( object > MAXMODEL ) return 0;
    pmad = MadList + object;

    // clear out the mad
    memset( pmad, 0, sizeof(mad_t) );

    // mark it as used
    pmad->used = btrue;

    // Make up a name for the model...  IMPORT\TEMP0000.OBJ
    strncpy( pmad->name, tmploadname, SDL_arraysize(pmad->name) );
    pmad->name[ SDL_arraysize(pmad->name) - 1 ] = '\0';

    // Load the AI script for this object
    make_newloadname( tmploadname, SLASH_STR "script.txt", newloadname );

    // Create a reference to the one we just loaded
    pmad->ai = load_ai_script( newloadname );

    // Load the object model
    make_newloadname( tmploadname, SLASH_STR "tris.md2", newloadname );

#ifdef __unix__

    // unix is case sensitive, but sometimes this file is called tris.MD2
    if ( access( newloadname, R_OK ) )
    {
        make_newloadname( tmploadname, SLASH_STR "tris.MD2", newloadname );

        // still no luck !
        if ( access( newloadname, R_OK ) )
        {
            log_warning( "Cannot open: %s\n", newloadname );
        }
    }

#endif

    md2_load_one( newloadname, object );
    md2_models[object] = md2_loadFromFile( newloadname );

    // Create the actions table for this object
    mad_rip_actions( object );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, SLASH_STR "copy.txt", newloadname );
    action_check_copy( newloadname, object );

    // Load the messages for this object
    make_newloadname( tmploadname, SLASH_STR "message.txt", newloadname );
    load_all_messages( newloadname, object );

    // Load the particles for this object
    for ( cnt = 0; cnt < MAXPRTPIPPEROBJECT; cnt++ )
    {
        sprintf( newloadname, "%s" SLASH_STR "part%d.txt", tmploadname, cnt );

        // Make sure it's referenced properly
        pmad->prtpip[cnt] = load_one_particle_profile( newloadname );
    }

    // Load the skins and icons
    pmad->skinstart = skin;
    numskins = 0;
    numicon = 0;
    for ( cnt = 0; cnt < MAXSKIN; cnt++)
    {
        snprintf( newloadname, sizeof(newloadname), "%s" SLASH_STR "tris%d", tmploadname, cnt );
        if ( INVALID_TX_ID != GLtexture_Load(GL_TEXTURE_2D, txTexture + (skin + numskins), newloadname, TRANSCOLOR ) )
        {
            numskins++;

            snprintf( newloadname, sizeof(newloadname), "%s" SLASH_STR "icon%d", tmploadname, cnt );
            if ( INVALID_TX_ID != GLtexture_Load(GL_TEXTURE_2D, TxIcon + globalicon_count, newloadname, INVALID_KEY ) )
            {
                for ( /* nothing */ ; numicon < numskins; numicon++ )
                {
                    skintoicon[skin + numicon] = globalicon_count;
                    if ( SPELLBOOK == object )
                    {
                        if ( bookicon_count < MAXSKIN )
                        {
                            bookicon[bookicon_count] = globalicon_count;
                            bookicon_count++;
                        }
                    }
                }

                globalicon_count++;
            }
        }
    }

    if ( 0 == numskins )
    {
        // If we didn't get a skin, set it to the water texture
        pmad->skinstart = TX_WATER_TOP;
        numskins = 1;
        if (gDevMode)
        {
            log_message( "NOTE: Object is missing a skin (%s)!\n", tmploadname );
        }
    }

    pmad->skins = numskins;

    return numskins;
}

//--------------------------------------------------------------------------------------------
int load_one_object( const char* tmploadname, int skin )
{
    // ZZ> This function loads one object and returns the number of skins
    int object;
    int numskins;
    STRING newloadname;

    // Load the object data file and get the object number
    object = load_one_character_profile( tmploadname );

    if( !VALID_CAP(object) ) return 0;  // no skins for an invalid object

    // Load the model for this object
    numskins = load_one_model_profile( tmploadname, object, skin );

    // Load the enchantment for this object
    make_newloadname( tmploadname, SLASH_STR "enchant.txt", newloadname );
    load_one_enchant_profile( newloadname, object );

    // Fix lighting if need be
    if ( CapList[object].uniformlit )
    {
        mad_make_equally_lit( object );
    }

    return numskins;
}

//--------------------------------------------------------------------------------------------
void mad_rip_actions( Uint16 object )
{
    // ZZ> This function creates the frame lists for each action based on the
    //     name of each md2 frame in the model

    int frame, framesinaction;
    int action, lastaction;

    // Clear out all actions and reset to invalid
    action = 0;

    while ( action < MAXACTION )
    {
        MadList[object].actionvalid[action] = bfalse;
        action++;
    }

    // Set the primary dance action to be the first frame, just as a default
    MadList[object].actionvalid[ACTIONDA] = btrue;
    MadList[object].actionstart[ACTIONDA] = MadList[object].framestart;
    MadList[object].actionend[ACTIONDA] = MadList[object].framestart + 1;

    // Now go huntin' to see what each frame is, look for runs of same action
    md2_rip_frame_name( 0 );
    lastaction = action_number();  framesinaction = 0;
    frame = 0;

    while ( frame < MadList[object].frames )
    {
        md2_rip_frame_name( frame );
        action = action_number();
        if ( lastaction == action )
        {
            framesinaction++;
        }
        else
        {
            // Write the old action
            if ( lastaction < MAXACTION )
            {
                MadList[object].actionvalid[lastaction] = btrue;
                MadList[object].actionstart[lastaction] = MadList[object].framestart + frame - framesinaction;
                MadList[object].actionend[lastaction] = MadList[object].framestart + frame;
            }

            framesinaction = 1;
            lastaction = action;
        }

        mad_get_framefx( MadList[object].framestart + frame );
        frame++;
    }

    // Write the old action
    if ( lastaction < MAXACTION )
    {
        MadList[object].actionvalid[lastaction] = btrue;
        MadList[object].actionstart[lastaction] = MadList[object].framestart + frame - framesinaction;
        MadList[object].actionend[lastaction]   = MadList[object].framestart + frame;
    }

    // Make sure actions are made valid if a similar one exists
    action_copy_correct( object, ACTIONDA, ACTIONDB );  // All dances should be safe
    action_copy_correct( object, ACTIONDB, ACTIONDC );
    action_copy_correct( object, ACTIONDC, ACTIONDD );
    action_copy_correct( object, ACTIONDB, ACTIONDC );
    action_copy_correct( object, ACTIONDA, ACTIONDB );
    action_copy_correct( object, ACTIONUA, ACTIONUB );
    action_copy_correct( object, ACTIONUB, ACTIONUC );
    action_copy_correct( object, ACTIONUC, ACTIONUD );
    action_copy_correct( object, ACTIONTA, ACTIONTB );
    action_copy_correct( object, ACTIONTC, ACTIONTD );
    action_copy_correct( object, ACTIONCA, ACTIONCB );
    action_copy_correct( object, ACTIONCC, ACTIONCD );
    action_copy_correct( object, ACTIONSA, ACTIONSB );
    action_copy_correct( object, ACTIONSC, ACTIONSD );
    action_copy_correct( object, ACTIONBA, ACTIONBB );
    action_copy_correct( object, ACTIONBC, ACTIONBD );
    action_copy_correct( object, ACTIONLA, ACTIONLB );
    action_copy_correct( object, ACTIONLC, ACTIONLD );
    action_copy_correct( object, ACTIONXA, ACTIONXB );
    action_copy_correct( object, ACTIONXC, ACTIONXD );
    action_copy_correct( object, ACTIONFA, ACTIONFB );
    action_copy_correct( object, ACTIONFC, ACTIONFD );
    action_copy_correct( object, ACTIONPA, ACTIONPB );
    action_copy_correct( object, ACTIONPC, ACTIONPD );
    action_copy_correct( object, ACTIONZA, ACTIONZB );
    action_copy_correct( object, ACTIONZC, ACTIONZD );
    action_copy_correct( object, ACTIONWA, ACTIONWB );
    action_copy_correct( object, ACTIONWB, ACTIONWC );
    action_copy_correct( object, ACTIONWC, ACTIONWD );
    action_copy_correct( object, ACTIONDA, ACTIONWD );  // All walks should be safe
    action_copy_correct( object, ACTIONWC, ACTIONWD );
    action_copy_correct( object, ACTIONWB, ACTIONWC );
    action_copy_correct( object, ACTIONWA, ACTIONWB );
    action_copy_correct( object, ACTIONJA, ACTIONJB );
    action_copy_correct( object, ACTIONJB, ACTIONJC );
    action_copy_correct( object, ACTIONDA, ACTIONJC );  // All jumps should be safe
    action_copy_correct( object, ACTIONJB, ACTIONJC );
    action_copy_correct( object, ACTIONJA, ACTIONJB );
    action_copy_correct( object, ACTIONHA, ACTIONHB );
    action_copy_correct( object, ACTIONHB, ACTIONHC );
    action_copy_correct( object, ACTIONHC, ACTIONHD );
    action_copy_correct( object, ACTIONHB, ACTIONHC );
    action_copy_correct( object, ACTIONHA, ACTIONHB );
    action_copy_correct( object, ACTIONKA, ACTIONKB );
    action_copy_correct( object, ACTIONKB, ACTIONKC );
    action_copy_correct( object, ACTIONKC, ACTIONKD );
    action_copy_correct( object, ACTIONKB, ACTIONKC );
    action_copy_correct( object, ACTIONKA, ACTIONKB );
    action_copy_correct( object, ACTIONMH, ACTIONMI );
    action_copy_correct( object, ACTIONDA, ACTIONMM );
    action_copy_correct( object, ACTIONMM, ACTIONMN );

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < MadList[object].frames; frame++ )
    {
        Md2FrameList[frame+MadList[object].framestart].framelip = 0;
    }

    // Need to figure out how far into action each frame is
    mad_make_framelip( object, ACTIONWA );
    mad_make_framelip( object, ACTIONWB );
    mad_make_framelip( object, ACTIONWC );

    // Now do the same, in reverse, for walking animations
    mad_get_walk_frame( object, LIPDA, ACTIONDA );
    mad_get_walk_frame( object, LIPWA, ACTIONWA );
    mad_get_walk_frame( object, LIPWB, ACTIONWB );
    mad_get_walk_frame( object, LIPWC, ACTIONWC );
}

//--------------------------------------------------------------------------------------------
/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
Uint8 onlyfriends, Uint8 anyone, Uint8 team,
Uint16 donttarget, Uint16 oldtarget )
{
// ZZ> This function helps find a target, returning btrue if it found a decent target
int cnt;
Uint16 angle;
Uint16 charb;
Uint8 enemies, returncode;
Uint32 fanblock;
int distance;

returncode = bfalse;

// Current fanblock
if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
{
fanblock = mesh_get_block_int(&mesh, x,y);

enemies = bfalse;
if ( !onlyfriends ) enemies = btrue;

charb = bumplist[fanblock].chr;
cnt = 0;
while ( cnt < bumplist[fanblock].chrnum )
{
if ( ChrList[charb].alive && !ChrList[charb].invictus && charb != donttarget && charb != oldtarget )
{
if ( anyone || ( ChrList[charb].team == team && onlyfriends ) || ( TeamList[team].hatesteam[ChrList[charb].team] && enemies ) )
{
distance = ABS( ChrList[charb].xpos - chrx ) + ABS( ChrList[charb].ypos - chry );
if ( distance < globestdistance )
{
angle = ( ATAN2( ChrList[charb].ypos - chry, ChrList[charb].xpos - chrx ) + PI ) * 0xFFFF / ( TWO_PI );
angle = facing - angle;
if ( angle < globestangle || angle > ( 0xFFFF - globestangle ) )
{
returncode = btrue;
globesttarget = charb;
globestdistance = distance;
glouseangle = angle;
if ( angle  > 32767 )
globestangle = -angle;
else
globestangle = angle;
}
}
}
}
charb = ChrList[charb].bumpnext;
cnt++;
}
}
return returncode;
}*/

//--------------------------------------------------------------------------------------------
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget )
{
    //ZF> This is the new improved targeting system for particles. Also includes distance in the Z direction.
    Uint16 besttarget = MAXCHR, cnt;
    Uint16 longdist = WIDE;

    for (cnt = 0; cnt < MAXCHR; cnt++)
    {
        if (ChrList[cnt].on && ChrList[cnt].alive && !ChrList[cnt].isitem && ChrList[cnt].attachedto == MAXCHR
                && !ChrList[cnt].invictus)
        {
            if ((PipList[particletype].onlydamagefriendly && team == ChrList[cnt].team) || (!PipList[particletype].onlydamagefriendly && TeamList[team].hatesteam[ChrList[cnt].team]) )
            {
                //Don't retarget someone we already had or not supposed to target
                if (cnt != oldtarget && cnt != donttarget)
                {
                    Uint16 angle = (ATAN2( ChrList[cnt].ypos - ypos, ChrList[cnt].xpos - xpos ) * 0xFFFF / ( TWO_PI ))
                                   + BEHIND - facing;

                    //Only proceed if we are facing the target
                    if (angle < PipList[particletype].targetangle || angle > ( 0xFFFF - PipList[particletype].targetangle ) )
                    {
                        Uint32 dist = ( Uint32 ) SQRT(ABS( pow(ChrList[cnt].xpos - xpos, 2))
                                                      + ABS( pow(ChrList[cnt].ypos - ypos, 2))
                                                      + ABS( pow(ChrList[cnt].zpos - zpos, 2)) );
                        if (dist < longdist && dist <= WIDE )
                        {
                            besttarget = cnt;
                            longdist = dist;
                        }
                    }
                }
            }
        }

    }

    //All done
    return besttarget;
}
//--------------------------------------------------------------------------------------------
/*Uint16 find_target( float chrx, float chry, Uint16 facing,
Uint16 targetangle, Uint8 onlyfriends, Uint8 anyone,
Uint8 team, Uint16 donttarget, Uint16 oldtarget )
{
// This function finds the best target for the given parameters
Uint8 done;
int x, y;

x = chrx;
y = chry;
x = x >> 9;
y = y >> 9;
globestdistance = 9999;
globestangle = targetangle;
done = find_target_in_block( x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x + 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
if ( done ) return globesttarget;

done = find_target_in_block( x + 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x + 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
if ( done ) return globesttarget;

return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz )
{
    //ZF> This is the new improved AI targeting system. Also includes distance in the Z direction.
    //If maxdistance is 0 then it searches without a max limit.
    Uint16 besttarget = MAXCHR, cnt = 0;
    float longdist = TILESIZE * MAX(mesh.info.tiles_x, mesh.info.tiles_y); //pow(2, 31);
    float longdist2 = longdist * longdist;
    float maxdistance2 = maxdistance * maxdistance;

    if (team == NONE) return MAXCHR;

    for (/*nothing*/; cnt != MAXCHR; cnt++)
    {
        //Skip non-existing objects, held objects and self
		if ( !ChrList[cnt].on || ChrList[cnt].attachedto != MAXCHR || ChrList[cnt].inpack || cnt == character ) continue;

        //Target items
        if ( !targetitems && ( ChrList[cnt].isitem && ChrList[cnt].invictus ) ) continue;

        //Target dead stuff
        if ( !targetdead && !ChrList[cnt].alive ) continue;

        //Dont target hostile invisible stuff, unless we can actually see them
        if ( TeamList[ChrList[character].team].hatesteam[ChrList[cnt].team] && (!ChrList[character].canseeinvisible
                && ( ChrList[cnt].inst.alpha < INVISIBLE && ChrList[cnt].inst.light < INVISIBLE )) ) continue;

        //Which team to target
        if ( team == ALL || team != TeamList[ChrList[character].team].hatesteam[ChrList[cnt].team] )
        {
            //Check for specific IDSZ too?
            if ( idsz != IDSZ_NONE && excludeidsz == (CapList[ChrList[cnt].model].idsz[IDSZ_PARENT] == idsz)
                    && excludeidsz == (CapList[ChrList[cnt].model].idsz[IDSZ_TYPE] == idsz )) continue;
            {
                float dx, dy, dz;
                float dist2;

                dx = ChrList[cnt].xpos - ChrList[character].xpos;
                dy = ChrList[cnt].ypos - ChrList[character].ypos;
                dz = ChrList[cnt].zpos - ChrList[character].zpos;

                dist2 = dx * dx + dy * dy + dz * dz;
                if (dist2 < longdist2 && (maxdistance == 0 || dist2 <= maxdistance2) )
                {
                    besttarget = cnt;
                    longdist2 = dist2;
                }
            }
        }
    }

    //Now set the target
    if (besttarget != MAXCHR)
    {
        ChrList[character].ai.target = besttarget;
    }

    return besttarget;
}

//--------------------------------------------------------------------------------------------
Uint16 action_number()
{
    // ZZ> This function returns the number of the action in cFrameName, or
    //     it returns NOACTION if it could not find a match
    int cnt;
    char first, second;

    first = cFrameName[0];
    second = cFrameName[1];

    for ( cnt = 0; cnt < MAXACTION; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
    // ZZ> This function returns the frame number in the third and fourth characters
    //     of cFrameName
    int number;
    sscanf( &cFrameName[2], "%d", &number );
    return number;
}

//--------------------------------------------------------------------------------------------
Uint16 test_frame_name( char letter )
{
    // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
    //     of the frame name matches the input argument
    if ( cFrameName[4] == letter ) return btrue;
    if ( cFrameName[4] == 0 ) return bfalse;
    if ( cFrameName[5] == letter ) return btrue;
    if ( cFrameName[5] == 0 ) return bfalse;
    if ( cFrameName[6] == letter ) return btrue;
    if ( cFrameName[6] == 0 ) return bfalse;
    if ( cFrameName[7] == letter ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb )
{
    // ZZ> This function makes sure both actions are valid if either of them
    //     are valid.  It will copy start and ends to mirror the valid action.

    if( object > MAXMODEL || !MadList[object].used ) return;

    if ( MadList[object].actionvalid[actiona] == MadList[object].actionvalid[actionb] )
    {
        // They are either both valid or both invalid, in either case we can't help
        return;
    }
    else
    {
        // Fix the invalid one
        if ( !MadList[object].actionvalid[actiona] )
        {
            // Fix actiona
            MadList[object].actionvalid[actiona] = btrue;
            MadList[object].actionstart[actiona] = MadList[object].actionstart[actionb];
            MadList[object].actionend[actiona] = MadList[object].actionend[actionb];
        }
        else
        {
            // Fix actionb
            MadList[object].actionvalid[actionb] = btrue;
            MadList[object].actionstart[actionb] = MadList[object].actionstart[actiona];
            MadList[object].actionend[actionb] = MadList[object].actionend[actiona];
        }
    }
}

//--------------------------------------------------------------------------------------------
void mad_get_walk_frame( Uint16 object, int lip, int action )
{
    // ZZ> This helps make walking look right
    int frame = 0;
    int framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];

    while ( frame < 16 )
    {
        int framealong = 0;
        if ( framesinaction > 0 )
        {
            framealong = ( ( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        MadList[object].frameliptowalkframe[lip][frame] = MadList[object].actionstart[action] + framealong;
        frame++;
    }
}

//--------------------------------------------------------------------------------------------
void mad_get_framefx( int frame )
{
    // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
    //     Drop timings
    Uint16 fx = 0;
    if ( test_frame_name( 'I' ) )
        fx = fx | MADFXINVICTUS;
    if ( test_frame_name( 'L' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFXACTLEFT;
        if ( test_frame_name( 'G' ) )
            fx = fx | MADFXGRABLEFT;
        if ( test_frame_name( 'D' ) )
            fx = fx | MADFXDROPLEFT;
        if ( test_frame_name( 'C' ) )
            fx = fx | MADFXCHARLEFT;
    }
    if ( test_frame_name( 'R' ) )
    {
        if ( test_frame_name( 'A' ) )
            fx = fx | MADFXACTRIGHT;
        if ( test_frame_name( 'G' ) )
            fx = fx | MADFXGRABRIGHT;
        if ( test_frame_name( 'D' ) )
            fx = fx | MADFXDROPRIGHT;
        if ( test_frame_name( 'C' ) )
            fx = fx | MADFXCHARRIGHT;
    }
    if ( test_frame_name( 'S' ) )
        fx = fx | MADFXSTOP;
    if ( test_frame_name( 'F' ) )
        fx = fx | MADFXFOOTFALL;
    if ( test_frame_name( 'P' ) )
        fx = fx | MADFXPOOF;

    Md2FrameList[frame].framefx = fx;
}

//--------------------------------------------------------------------------------------------
void mad_make_framelip( Uint16 object, int action )
{
    // ZZ> This helps make walking look right
    int frame, framesinaction;
    if ( MadList[object].actionvalid[action] )
    {
        framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];
        frame = MadList[object].actionstart[action];

        while ( frame < MadList[object].actionend[action] )
        {
            Md2FrameList[frame].framelip = ( frame - MadList[object].actionstart[action] ) * 15 / framesinaction;
            Md2FrameList[frame].framelip = ( Md2FrameList[frame].framelip ) & 15;
            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit( int model )
{
    // ZZ> This function makes ultra low poly models look better
    int frame, cnt, vert;
    if ( MadList[model].used )
    {
        frame = MadList[model].framestart;

        for ( cnt = 0; cnt < MadList[model].frames; cnt++ )
        {
            vert = 0;

            while ( vert < MAXVERTICES )
            {
                Md2FrameList[frame].vrta[vert] = EQUALLIGHTINDEX;
                vert++;
            }

            frame++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void action_check_copy( const char* loadname, Uint16 object )
{
    // ZZ> This function copies a model's actions
    FILE *fileread;
    int actiona, actionb;
    char szOne[16], szTwo[16];

    if( object > MAXMODEL || !MadList[object].used ) return;

    MadList[object].msgstart = 0;
    fileread = fopen( loadname, "r" );
    if ( fileread )
    {
        while ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%s%s", szOne, szTwo );

            actiona = action_which( szOne[0] );
            actionb = action_which( szTwo[0] );

            action_copy_correct( object, actiona + 0, actionb + 0 );
            action_copy_correct( object, actiona + 1, actionb + 1 );
            action_copy_correct( object, actiona + 2, actionb + 2 );
            action_copy_correct( object, actiona + 3, actionb + 3 );
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    // ZZ> This function changes a letter into an action code
    int action;
    action = ACTIONDA;
    if ( cTmp == 'U' || cTmp == 'u' )  action = ACTIONUA;
    if ( cTmp == 'T' || cTmp == 't' )  action = ACTIONTA;
    if ( cTmp == 'S' || cTmp == 's' )  action = ACTIONSA;
    if ( cTmp == 'C' || cTmp == 'c' )  action = ACTIONCA;
    if ( cTmp == 'B' || cTmp == 'b' )  action = ACTIONBA;
    if ( cTmp == 'L' || cTmp == 'l' )  action = ACTIONLA;
    if ( cTmp == 'X' || cTmp == 'x' )  action = ACTIONXA;
    if ( cTmp == 'F' || cTmp == 'f' )  action = ACTIONFA;
    if ( cTmp == 'P' || cTmp == 'p' )  action = ACTIONPA;
    if ( cTmp == 'Z' || cTmp == 'z' )  action = ACTIONZA;

    return action;
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
    // ZZ> This function figures out which fan characters are on and sets their level
    Uint16 character, distance;
    int ripand;
    // int volume;
    float level;

    // First figure out which fan each character is in
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !ChrList[character].on ) continue;

        ChrList[character].onwhichfan   = mesh_get_tile ( ChrList[character].xpos, ChrList[character].ypos );
        ChrList[character].onwhichblock = mesh_get_block( ChrList[character].xpos, ChrList[character].ypos );
    }

    // Get levels every update
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !ChrList[character].on || ChrList[character].inpack ) continue;

        level = get_level( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].waterwalk ) + RAISE;

        if ( ChrList[character].alive )
        {
            if ( ( INVALID_TILE != ChrList[character].onwhichfan ) && ( 0 != ( mesh.mem.tile_list[ChrList[character].onwhichfan].fx & MESHFX_DAMAGE ) ) && ( ChrList[character].zpos <= ChrList[character].floor_level + DAMAGERAISE ) && ( MAXCHR == ChrList[character].attachedto ) )
            {
                if ( ( ChrList[character].damagemodifier[damagetiletype]&DAMAGESHIFT ) != 3 && !ChrList[character].invictus ) // 3 means they're pretty well immune
                {
                    distance = ABS( gCamera.trackx - ChrList[character].xpos ) + ABS( gCamera.tracky - ChrList[character].ypos );
                    if ( distance < damagetilemindistance )
                    {
                        damagetilemindistance = distance;
                    }
                    if ( distance < damagetilemindistance + 256 )
                    {
                        damagetilesoundtime = 0;
                    }
                    if ( ChrList[character].damagetime == 0 )
                    {
                        damage_character( character, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, ChrList[character].ai.bumplast, DAMFX_BLOC | DAMFX_ARMO, bfalse );
                        ChrList[character].damagetime = DAMAGETILETIME;
                    }
                    if ( (damagetileparttype != ((Sint16)~0)) && ( frame_wld&damagetilepartand ) == 0 )
                    {
                        spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                                            0, MAXMODEL, damagetileparttype, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    }
                }
                if ( ChrList[character].reaffirmdamagetype == damagetiletype )
                {
                    if ( ( frame_wld&TILEREAFFIRMAND ) == 0 )
                        reaffirm_attached_particles( character );
                }
            }
        }

        if ( ChrList[character].zpos < watersurfacelevel && INVALID_TILE != ChrList[character].onwhichfan && 0 != ( mesh.mem.tile_list[ChrList[character].onwhichfan].fx & MESHFX_WATER ) )
        {
            if ( !ChrList[character].inwater )
            {
                // Splash
                if ( ChrList[character].attachedto == MAXCHR )
                {
                    spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, watersurfacelevel + RAISE,
                                        0, MAXMODEL, SPLASH, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                }

                ChrList[character].inwater = btrue;
                if ( wateriswater )
                {
                    ChrList[character].ai.alert |= ALERTIF_INWATER;
                }
            }
            else
            {
                if ( ChrList[character].zpos > watersurfacelevel - RIPPLETOLERANCE && CapList[ChrList[character].model].ripple )
                {
                    // Ripples
                    ripand = ( ( int )ChrList[character].xvel != 0 ) | ( ( int )ChrList[character].yvel != 0 );
                    ripand = RIPPLEAND >> ripand;
                    if ( ( frame_wld&ripand ) == 0 && ChrList[character].zpos < watersurfacelevel && ChrList[character].alive )
                    {
                        spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, watersurfacelevel,
                                            0, MAXMODEL, RIPPLE, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    }
                }
                if ( wateriswater && ( frame_wld&7 ) == 0 )
                {
                    ChrList[character].jumpready = btrue;
                    ChrList[character].jumpnumber = 1; // ChrList[character].jumpnumberreset;
                }
            }

            ChrList[character].xvel = ChrList[character].xvel * waterfriction;
            ChrList[character].yvel = ChrList[character].yvel * waterfriction;
            ChrList[character].zvel = ChrList[character].zvel * waterfriction;
        }
        else
        {
            ChrList[character].inwater = bfalse;
        }

        ChrList[character].floor_level = level;
    }

    // Play the damage tile sound
    if ( damagetilesound >= 0 )
    {
        if ( ( frame_wld & 3 ) == 0 )
        {
            // Change the volume...
            /*PORT
            volume = -(damagetilemindistance + (damagetilesoundtime<<8));
            volume = volume<<VOLSHIFT;
            if(volume > VOLMIN)
            {
            lpDSBuffer[damagetilesound]->SetVolume(volume);
            }
            if(damagetilesoundtime < TILESOUNDTIME)  damagetilesoundtime++;
            else damagetilemindistance = 9999;
            */
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
    minordir += majordir;
    return minordir;
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major, but not by much.  Makes turning faster.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
    minordir += majordir;
    return minordir;
}

//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
{
    // ZZ> This function lets enchantments spawn particles

    int cnt, tnc;
    Uint16 facing, eve, character;

    for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
    {
        if ( !EncList[cnt].on ) continue;

        eve = EncList[cnt].eve;
        if ( EveList[eve].contspawnamount > 0 )
        {
            EncList[cnt].spawntime--;
            if ( EncList[cnt].spawntime == 0 )
            {
                character = EncList[cnt].target;
                EncList[cnt].spawntime = EveList[eve].contspawntime;
                facing = ChrList[character].turnleftright;
                for ( tnc = 0; tnc < EveList[eve].contspawnamount; tnc++ )
                {
                    spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                                        facing, eve, EveList[eve].contspawnpip,
                                        MAXCHR, GRIP_LAST, ChrList[EncList[cnt].owner].team, EncList[cnt].owner, tnc, MAXCHR );
                    facing += EveList[eve].contspawnfacingadd;
                }
            }
        }
    }
}
//--------------------------------------------------------------------------------------------
void update_pits()
{
    // ZZ> This function kills any character in a deep pit...
    int cnt;
    if ( pitskill || pitsfall )
    {
        if ( pitclock > 19 )
        {
            pitclock = 0;

            // Kill any particles that fell in a pit, if they die in water...
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( PrtList[cnt].on )
                {
                    if ( PrtList[cnt].zpos < PITDEPTH && PipList[PrtList[cnt].pip].endwater )
                    {
                        PrtList[cnt].time = 1;
                    }
                }

                cnt++;
            }

            // Kill or teleport any characters that fell in a pit...
            cnt = 0;

            while ( cnt < MAXCHR )
            {
                if ( ChrList[cnt].on && ChrList[cnt].alive && !ChrList[cnt].inpack )
                {
                    if ( !ChrList[cnt].invictus && ChrList[cnt].zpos < PITDEPTH && ChrList[cnt].attachedto == MAXCHR )
                    {
                        //Do we kill it?
                        if (pitskill)
                        {
                            // Got one!
                            kill_character( cnt, MAXCHR );
                            ChrList[cnt].xvel = 0;
                            ChrList[cnt].yvel = 0;

                            //Play sound effect
                            sound_play_chunk( ChrList[cnt].xpos, ChrList[cnt].ypos, g_wavelist[GSND_PITFALL] );
                        }

                        //Do we teleport it?
                        if (pitsfall && ChrList[cnt].zpos < PITDEPTH*8)
                        {
                            // Yeah!  It worked!
                            detach_character_from_mount( cnt, btrue, bfalse );
                            ChrList[cnt].oldx = ChrList[cnt].xpos;
                            ChrList[cnt].oldy = ChrList[cnt].ypos;
                            ChrList[cnt].xpos = pitx;
                            ChrList[cnt].ypos = pity;
                            ChrList[cnt].zpos = pitz;
                            if ( __chrhitawall( cnt ) )
                            {
                                // No it didn't...
                                ChrList[cnt].xpos = ChrList[cnt].oldx;
                                ChrList[cnt].ypos = ChrList[cnt].oldy;
                                ChrList[cnt].zpos = ChrList[cnt].oldz;

                                // Kill it instead
                                kill_character( cnt, MAXCHR );
                                ChrList[cnt].xvel = 0;
                                ChrList[cnt].yvel = 0;
                            }
                            else
                            {
                                ChrList[cnt].oldx = ChrList[cnt].xpos;
                                ChrList[cnt].oldy = ChrList[cnt].ypos;
                                ChrList[cnt].oldz = ChrList[cnt].zpos;

                                //Stop movement
                                ChrList[cnt].zvel = 0;
                                ChrList[cnt].xvel = 0;
                                ChrList[cnt].yvel = 0;

                                //Play sound effect
                                if (ChrList[cnt].isplayer)
                                {
                                    sound_play_chunk( gCamera.trackx, gCamera.tracky, g_wavelist[GSND_PITFALL] );
                                }
                                else
                                {
                                    sound_play_chunk( ChrList[cnt].xpos, ChrList[cnt].ypos, g_wavelist[GSND_PITFALL] );
                                }

                                //Do some damage (same as damage tile)
                                damage_character( cnt, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, ChrList[cnt].ai.bumplast, DAMFX_BLOC | DAMFX_ARMO, btrue );
                            }
                        }
                    }

                }

                cnt++;
            }
        }
        else
        {
            pitclock++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn()
{
    // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
    int particle, cnt;
    float x, y, z;
    bool_t foundone;
    if ( weathertime > 0 )
    {
        weathertime--;
        if ( weathertime == 0 )
        {
            weathertime = weathertimereset;

            // Find a valid player
            foundone = bfalse;
            cnt = 0;

            while ( cnt < MAXPLAYER )
            {
                weatherplayer = ( weatherplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( PlaList[weatherplayer].valid )
                {
                    foundone = btrue;
                    break;
                }
                cnt++;
            }

            // Did we find one?
            if ( foundone )
            {
                // Yes, but is the character valid?
                cnt = PlaList[weatherplayer].index;
                if ( ChrList[cnt].on && !ChrList[cnt].inpack )
                {
                    // Yes, so spawn over that character
                    x = ChrList[cnt].xpos;
                    y = ChrList[cnt].ypos;
                    z = ChrList[cnt].zpos;
                    particle = spawn_one_particle( x, y, z, 0, MAXMODEL, WEATHER4, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    
					if(particle != TOTALMAXPRT)
					{
						if(__prthitawall( particle ) ) free_one_particle_no_sound( particle );
						else if ( weatheroverwater )
						{
							if ( !prt_is_over_water( particle ) )
							{
								free_one_particle_no_sound( particle );
							}
						}
					}
					
                }
            }
        }
    }

    gCamera.swing = ( gCamera.swing + gCamera.swingrate ) & 16383;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( Uint16 player )
{
    // ZZ> This function converts input readings to latch settings, so players can
    //     move around
    float newx, newy;
    Uint16 turnsin, turncos, character;
    Uint8 device;
    float dist, scale;
    float inputx, inputy;

    // Check to see if we need to bother
    if ( PlaList[player].valid && PlaList[player].device != INPUT_BITS_NONE )
    {
        // Make life easier
        character = PlaList[player].index;
        device = PlaList[player].device;

        // Clear the player's latch buffers
        PlaList[player].latchbutton = 0;
        PlaList[player].latchx = 0;
        PlaList[player].latchy = 0;

        // Mouse routines
        if ( ( device & INPUT_BITS_MOUSE ) && mous.on )
        {
            // Movement
            newx = 0;
            newy = 0;
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
            {
                dist = SQRT( mous.x * mous.x + mous.y * mous.y );
                if ( dist > 0 )
                {
                    scale = mous.sense / dist;
                    if ( dist < mous.sense )
                    {
                        scale = dist / mous.sense;
                    }

                    scale = scale / mous.sense;
                    if ( ChrList[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = mous.x * ChrList[ChrList[character].attachedto].maxaccel * scale;
                        inputy = mous.y * ChrList[ChrList[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = mous.x * ChrList[character].maxaccel * scale;
                        inputy = mous.y * ChrList[character].maxaccel * scale;
                    }

                    turnsin = ( gCamera.turnleftrightone * 16383 );
                    turnsin = turnsin & 16383;
                    turncos = ( turnsin + 4096 ) & 16383;
                    if ( autoturncamera == 255 &&
                            local_numlpla == 1 &&
                            control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                    newx = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                    newy = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                    //                    PlaList[player].latchx+=newx;
                    //                    PlaList[player].latchy+=newy;
                }
            }

            PlaList[player].latchx += newx * mous.cover + mous.latcholdx * mous.sustain;
            PlaList[player].latchy += newy * mous.cover + mous.latcholdy * mous.sustain;
            mous.latcholdx = PlaList[player].latchx;
            mous.latcholdy = PlaList[player].latchy;

            // Sustain old movements to ease mouse play
            //            PlaList[player].latchx+=mous.latcholdx*mous.sustain;
            //            PlaList[player].latchy+=mous.latcholdy*mous.sustain;
            //            mous.latcholdx = PlaList[player].latchx;
            //            mous.latcholdy = PlaList[player].latchy;
            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick A routines
        if ( ( device & INPUT_BITS_JOYA ) && joy[0].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[0].x * joy[0].x + joy[0].y * joy[0].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    if ( ChrList[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[0].x * ChrList[ChrList[character].attachedto].maxaccel * scale;
                        inputy = joy[0].y * ChrList[ChrList[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[0].x * ChrList[character].maxaccel * scale;
                        inputy = joy[0].y * ChrList[character].maxaccel * scale;
                    }
                }

                turnsin = ( gCamera.turnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;
                if ( autoturncamera == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                PlaList[player].latchx += newx;
                PlaList[player].latchy += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick B routines
        if ( ( device & INPUT_BITS_JOYB ) && joy[1].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[1].x * joy[1].x + joy[1].y * joy[1].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    if ( ChrList[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[1].x * ChrList[ChrList[character].attachedto].maxaccel * scale;
                        inputy = joy[1].y * ChrList[ChrList[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[1].x * ChrList[character].maxaccel * scale;
                        inputy = joy[1].y * ChrList[character].maxaccel * scale;
                    }
                }

                turnsin = ( gCamera.turnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;
                if ( autoturncamera == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                PlaList[player].latchx += newx;
                PlaList[player].latchy += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Keyboard routines
        if ( ( device & INPUT_BITS_KEYBOARD ) && keyb.on )
        {
            // Movement
            if ( ChrList[character].attachedto != MAXCHR )
            {
                // Mounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * ChrList[ChrList[character].attachedto].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * ChrList[ChrList[character].attachedto].maxaccel;
            }
            else
            {
                // Unmounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * ChrList[character].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * ChrList[character].maxaccel;
            }

            turnsin = ( gCamera.turnleftrightone * 16383 );
            turnsin = turnsin & 16383;
            turncos = ( turnsin + 4096 ) & 16383;
            if ( autoturncamera == 255 && local_numlpla == 1 )  inputx = 0;

            newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
            newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
            PlaList[player].latchx += newx;
            PlaList[player].latchy += newy;

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
    // ZZ> This function emulates AI thinkin' by setting latches from input devices
    int cnt;

    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void prime_names()
{
    // ZZ> This function prepares the name chopper for use
    int cnt, tnc;

    chop.count = 0;
    chop.carat = 0;
    cnt = 0;

    while ( cnt < MAXMODEL )
    {
        tnc = 0;

        while ( tnc < MAXSECTION )
        {
            CapList[cnt].chop_sectionstart[tnc] = MAXCHOP;
            CapList[cnt].chop_sectionsize[tnc] = 0;
            tnc++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    // ZZ> This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;

    int ticks;
    if ( console_mode ) return;

    ticks = SDL_GetTicks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // !!!BAD!!!  XP CHEAT
    if ( gDevMode && SDLKEYDOWN( SDLK_x ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && PlaList[0].index < MAXCHR )  { ChrList[PlaList[0].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && PlaList[1].index < MAXCHR )  { ChrList[PlaList[1].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && PlaList[2].index < MAXCHR )  { ChrList[PlaList[2].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && PlaList[3].index < MAXCHR )  { ChrList[PlaList[3].index].experience++; stat_check_delay = 500; }

        statdelay = 0;
    }

    // !!!BAD!!!  LIFE CHEAT
    if ( gDevMode && SDLKEYDOWN( SDLK_z ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && PlaList[0].index < MAXCHR )  { ChrList[PlaList[0].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[0].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && PlaList[1].index < MAXCHR )  { ChrList[PlaList[1].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[1].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && PlaList[2].index < MAXCHR )  { ChrList[PlaList[2].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[2].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && PlaList[3].index < MAXCHR )  { ChrList[PlaList[3].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[3].index].life, PERFECTBIG); stat_check_delay = 500; }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 8 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if (  SDLKEYDOWN( SDLK_LCTRL ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else if ( SDLKEYDOWN( SDLK_LALT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }

    // Show map cheat
    if ( gDevMode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        mapon = mapvalid;
        youarehereon = btrue;
        stat_check_delay = 1000;
    }
}

//--------------------------------------------------------------------------------------------
void show_stat( Uint16 statindex )
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char text[64];
    char gender[8];
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Name
            sprintf( text, "=%s=", ChrList[character].name );
            debug_message( text );

            // Level and gender and class
            gender[0] = 0;
            if ( ChrList[character].alive )
            {
                if ( ChrList[character].gender == GENMALE )
                {
                    sprintf( gender, "male " );
                }
                if ( ChrList[character].gender == GENFEMALE )
                {
                    sprintf( gender, "female " );
                }

                level = ChrList[character].experiencelevel;
                if ( level == 0 )
                    sprintf( text, " 1st level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level == 1 )
                    sprintf( text, " 2nd level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level == 2 )
                    sprintf( text, " 3rd level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level >  2 )
                    sprintf( text, " %dth level %s%s", level + 1, gender, CapList[ChrList[character].model].classname );
            }
            else
            {
                sprintf( text, " Dead %s", CapList[ChrList[character].model].classname );
            }

            // Stats
            debug_message( text );
            sprintf( text, " STR:~%2d~WIS:~%2d~DEF:~%d", FP8_TO_INT( ChrList[character].strength ), FP8_TO_INT( ChrList[character].wisdom ), 255 - ChrList[character].defense );
            debug_message( text );
            sprintf( text, " INT:~%2d~DEX:~%2d~EXP:~%d", FP8_TO_INT( ChrList[character].intelligence ), FP8_TO_INT( ChrList[character].dexterity ), ChrList[character].experience );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character
    char text[64], tmps[64];
    short character, skinlevel;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];
            skinlevel = ChrList[character].skin;

            // Armor Name
            sprintf( text, "=%s=", CapList[ChrList[character].model].skinname[skinlevel] );
            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - CapList[ChrList[character].model].defense[skinlevel],
                     CapList[ChrList[character].model].damagemodifier[0][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[1][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[2][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     CapList[ChrList[character].model].damagemodifier[3][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[4][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[5][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[6][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[7][skinlevel]&DAMAGESHIFT );
            debug_message( text );
            if ( CapList[ChrList[character].model].skindressy ) sprintf( tmps, "Light Armor" );
            else                   sprintf( tmps, "Heavy Armor" );

            sprintf( text, " Type: %s", tmps );

            // Speed and jumps
            if ( ChrList[character].jumpnumberreset == 0 )  sprintf( text, "None (0)" );
            if ( ChrList[character].jumpnumberreset == 1 )  sprintf( text, "Novice (1)" );
            if ( ChrList[character].jumpnumberreset == 2 )  sprintf( text, "Skilled (2)" );
            if ( ChrList[character].jumpnumberreset == 3 )  sprintf( text, "Master (3)" );
            if ( ChrList[character].jumpnumberreset > 3 )   sprintf( text, "Inhuman (%i)", ChrList[character].jumpnumberreset );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", CapList[ChrList[character].model].maxaccel[skinlevel]*80, tmps );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_full_status( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character including magic
    char text[64], tmps[64];
    short character;
    int i = 0;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAXENCHANT )
            {
                // Found a active enchantment that is not a skill of the character
                if ( EncList[i].on && EncList[i].spawner != character && EncList[i].target == character ) break;

                i++;
            }
            if ( i != MAXENCHANT ) sprintf( text, "=%s is enchanted!=", ChrList[character].name );
            else sprintf( text, "=%s is unenchanted=", ChrList[character].name );

            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                     255 - ChrList[character].defense,
                     ChrList[character].damagemodifier[0]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[1]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[2]&DAMAGESHIFT );
            debug_message( text );
            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     ChrList[character].damagemodifier[3]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[4]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[5]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[6]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[7]&DAMAGESHIFT );
            debug_message( text );

            // Speed and jumps
            if ( ChrList[character].jumpnumberreset == 0 )  sprintf( text, "None (0)" );
            if ( ChrList[character].jumpnumberreset == 1 )  sprintf( text, "Novice (1)" );
            if ( ChrList[character].jumpnumberreset == 2 )  sprintf( text, "Skilled (2)" );
            if ( ChrList[character].jumpnumberreset == 3 )  sprintf( text, "Master (3)" );
            if ( ChrList[character].jumpnumberreset > 3 )   sprintf( text, "Inhuman (4+)" );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", ChrList[character].maxaccel*80, tmps );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    // ZF> Displays special enchantment effects for the character
    char text[64], tmpa[64], tmpb[64];
    short character;
    int i = 0;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAXENCHANT )
            {
                // Found a active enchantment that is not a skill of the character
                if ( EncList[i].on && EncList[i].spawner != character && EncList[i].target == character ) break;

                i++;
            }
            if ( i != MAXENCHANT ) sprintf( text, "=%s is enchanted!=", ChrList[character].name );
            else sprintf( text, "=%s is unenchanted=", ChrList[character].name );

            debug_message( text );

            // Enchantment status
            if ( ChrList[character].canseeinvisible )  sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].canseekurse )      sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " See Invisible: %s~~See Kurses: %s", tmpa, tmpb );
            debug_message( text );
            if ( ChrList[character].canchannel )     sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].waterwalk )        sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " Channel Life: %s~~Waterwalking: %s", tmpa, tmpb );
            debug_message( text );
            if ( ChrList[character].flyheight > 0 )    sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].missiletreatment == MISREFLECT )       sprintf( tmpb, "Reflect" );
            else if ( ChrList[character].missiletreatment == MISREFLECT )  sprintf( tmpb, "Deflect" );
            else                           sprintf( tmpb, "None" );

            sprintf( text, " Flying: %s~~Missile Protection: %s", tmpa, tmpb );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_characters( void )
{
    // ZZ> This function sets handles characters hitting other characters or particles
    Uint16 character, particle, chara, charb, partb;
    Uint16 pip, direction;
    Uint32 fanblock, prtidparent, prtidtype, eveidremove;
    IDSZ   chridvulnerability;
    Sint8 hide;
    int tnc, dist, chrinblock, prtinblock, enchant, temp;
    float ax, ay, nx, ny, scale;  // For deflection
    Uint16 facing;

    // Clear the lists
    for ( fanblock = 0; fanblock < mesh.info.blocks_count; fanblock++ )
    {
        bumplist[fanblock].chr    = MAXCHR;
        bumplist[fanblock].chrnum = 0;

        bumplist[fanblock].prt    = TOTALMAXPRT;
        bumplist[fanblock].prtnum = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !ChrList[character].on ) continue;

        // reset the holding weight each update
        ChrList[character].holdingweight   = 0;
        ChrList[character].onwhichplatform = MAXCHR;
        ChrList[character].phys_level = ChrList[character].floor_level;

        // reset the fan and block position
        ChrList[character].onwhichfan   = mesh_get_tile ( ChrList[character].xpos, ChrList[character].ypos );
        ChrList[character].onwhichblock = mesh_get_block( ChrList[character].xpos, ChrList[character].ypos );

        // reject characters that are in packs, or are marked as non-colliding
        if ( ChrList[character].inpack || 0 == ChrList[character].bumpheight ) continue;

        // reject characters that are hidden
        hide = CapList[ ChrList[character].model ].hidestate;
        if ( hide != NOHIDE && hide == ChrList[character].ai.state ) continue;

        if ( INVALID_BLOCK != ChrList[character].onwhichblock )
        {
            // Insert before any other characters on the block
            ChrList[character].bumpnext = bumplist[ChrList[character].onwhichblock].chr;
            bumplist[ChrList[character].onwhichblock].chr = character;
            bumplist[ChrList[character].onwhichblock].chrnum++;
        }
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        // reject invalid particles
        if ( !PrtList[particle].on ) continue;

        // reset the fan and block position
        PrtList[particle].onwhichfan   = mesh_get_tile ( PrtList[particle].xpos, PrtList[particle].ypos );
        PrtList[particle].onwhichblock = mesh_get_block( PrtList[particle].xpos, PrtList[particle].ypos );

        if ( INVALID_BLOCK != PrtList[particle].onwhichblock )
        {
            // Insert before any other particles on the block
            PrtList[particle].bumpnext = bumplist[PrtList[particle].onwhichblock].prt;
            bumplist[PrtList[particle].onwhichblock].prt = particle;
            bumplist[PrtList[particle].onwhichblock].prtnum++;
        }
    }

    // blank the accumulators
    for ( character = 0; character < MAXCHR; character++ )
    {
        ChrList[character].phys_pos_x = 0.0f;
        ChrList[character].phys_pos_y = 0.0f;
        ChrList[character].phys_pos_z = 0.0f;
        ChrList[character].phys_vel_x = 0.0f;
        ChrList[character].phys_vel_y = 0.0f;
        ChrList[character].phys_vel_z = 0.0f;
    }


    // scan all possible object-object interactions, looking for platform attachments
    for ( chara = 0; chara < MAXCHR; chara++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        float xa, ya, za;

        // make sure that it is on
        if ( !ChrList[chara].on ) continue;

        // reject characters that are in packs, or are marked as non-colliding
        if ( ChrList[chara].inpack || 0 == ChrList[chara].bumpheight ) continue;

        // reject characters that are hidden
        hide = CapList[ ChrList[chara].model ].hidestate;
        if ( hide != NOHIDE && hide == ChrList[chara].ai.state ) continue;

        // only interested in object-platform interactions
        if( !ChrList[chara].platform && !CapList[ChrList[chara].model].canuseplatforms ) continue;

        xa = ChrList[chara].xpos;
        ya = ChrList[chara].ypos;
        za = ChrList[chara].zpos;

        // determine the size of this object in blocks
        ixmin = ChrList[chara].xpos - ChrList[chara].bumpsize; ixmin = CLIP(ixmin, 0, mesh.info.edge_x);
        ixmax = ChrList[chara].xpos + ChrList[chara].bumpsize; ixmax = CLIP(ixmax, 0, mesh.info.edge_x);

        iymin = ChrList[chara].ypos - ChrList[chara].bumpsize; iymin = CLIP(iymin, 0, mesh.info.edge_y);
        iymax = ChrList[chara].ypos + ChrList[chara].bumpsize; iymax = CLIP(iymax, 0, mesh.info.edge_y);

        ixmax_block = ixmax >> 9; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );
        ixmin_block = ixmin >> 9; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );

        iymax_block = iymax >> 9; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );
        iymin_block = iymin >> 9; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );

        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                // Allow raw access here because we were careful :)
                fanblock = mesh_get_block_int(&mesh, ix_block, iy_block);
                if ( INVALID_BLOCK != fanblock )
                {
                    chrinblock = bumplist[fanblock].chrnum;
                    prtinblock = bumplist[fanblock].prtnum;

                    for ( tnc = 0, charb = bumplist[fanblock].chr;
                        tnc < chrinblock && charb != MAXCHR;
                        tnc++, charb = ChrList[charb].bumpnext)
                    {
                        bool_t platform_a, platform_b;
                        bool_t mount_a, mount_b;
                        float  xb, yb, zb;
                        float  dx, dy, dist;
                        float  depth_z, lerp_z, radius, radius_xy;

                        bool_t collide_x  = bfalse;
                        bool_t collide_y  = bfalse;
                        bool_t collide_xy = bfalse;

                        // Don't collide with self, and only do each collision pair once
                        if ( charb <= chara ) continue;

                        // don't interact with your mount, or your held items
                        if ( chara == ChrList[charb].attachedto || charb == ChrList[chara].attachedto ) continue;

                        // only check possible object-platform interactions
                        platform_a = CapList[ChrList[charb].model].canuseplatforms && ChrList[chara].platform;
                        platform_b = CapList[ChrList[chara].model].canuseplatforms && ChrList[charb].platform;
                        if( !platform_a && !platform_b ) continue;

                        xb = ChrList[charb].xpos;
                        yb = ChrList[charb].ypos;
                        zb = ChrList[charb].zpos;

                        // If we can mount this platform, skip it
                        mount_a = !ChrList[charb].isitem && ChrList[chara].ismount && CapList[ChrList[chara].model].slotvalid[SLOT_LEFT] && INVALID_CHR(ChrList[chara].holdingwhich[SLOT_LEFT]);
                        if( mount_a && ChrList[chara].phys_level < zb + ChrList[charb].bumpheight + PLATTOLERANCE ) 
                            continue;

                        // If we can mount this platform, skip it
                        mount_b = !ChrList[chara].isitem && ChrList[charb].ismount && CapList[ChrList[charb].model].slotvalid[SLOT_LEFT] && INVALID_CHR(ChrList[charb].holdingwhich[SLOT_LEFT]);
                        if( mount_b && ChrList[charb].phys_level < za + ChrList[chara].bumpheight + PLATTOLERANCE ) 
                            continue;


                        dx = ABS( xa - xb );
                        dy = ABS( ya - yb );
                        dist = dx + dy;
                        depth_z = MIN( zb + ChrList[charb].bumpheight, za + ChrList[chara].bumpheight ) - MAX(za, zb);

                        if( depth_z > PLATTOLERANCE || depth_z < -PLATTOLERANCE ) continue;

                        // estimate the radius of interaction based on the z overlap
                        lerp_z  = depth_z / PLATTOLERANCE;
                        lerp_z  = CLIP( lerp_z, 0, 1 );

                        radius    = MIN(ChrList[chara].bumpsize,     ChrList[charb].bumpsize  ); /* * (1.0f - lerp_z) + (ChrList[chara].bumpsize    + ChrList[charb].bumpsize   ) * lerp_z; */
                        radius_xy = MIN(ChrList[chara].bumpsizebig, ChrList[charb].bumpsizebig); /* * (1.0f - lerp_z) + (ChrList[chara].bumpsizebig + ChrList[charb].bumpsizebig) * lerp_z; */

                        // estimate the collisions this frame
                        collide_x  = (dx <= ChrList[chara].bumpsize) || (dx <= ChrList[charb].bumpsize);
                        collide_y  = (dy <= ChrList[chara].bumpsize) || (dy <= ChrList[charb].bumpsize);
                        collide_xy = (dist <= ChrList[chara].bumpsizebig) || (dist <= ChrList[charb].bumpsizebig);

                        if( collide_x && collide_y && collide_xy && depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE )
                        {
                            // there is an interaction

                            bool_t chara_on_top;

                            // determine how the characters should be attached
                            chara_on_top = btrue;
                            depth_z = 2*PLATTOLERANCE;
                            if( platform_a && platform_b )
                            {
                                float depth_a, depth_b;

                                depth_a = zb + ChrList[charb].bumpheight - za;
                                depth_b = za + ChrList[chara].bumpheight - zb;

                                depth_z = MIN( zb + ChrList[charb].bumpheight, za + ChrList[chara].bumpheight ) - MAX(za, zb);

                                chara_on_top = ABS(depth_z - depth_a) < ABS(depth_z - depth_b);
                            }
                            else if( platform_a )
                            {
                                chara_on_top = bfalse;
                                depth_z = za + ChrList[chara].bumpheight - zb; 
                            }
                            else if( platform_b )
                            {
                                chara_on_top = btrue;
                                depth_z = zb + ChrList[charb].bumpheight - za;
                            }

                            // check for the best possible attachment
                            if( chara_on_top )
                            {
                                if( zb + ChrList[charb].bumpheight > ChrList[chara].phys_level )
                                {
                                    ChrList[chara].phys_level = zb + ChrList[charb].bumpheight;
                                    ChrList[chara].onwhichplatform = charb;
                                }
                            }
                            else
                            {
                                if( za + ChrList[chara].bumpheight > ChrList[charb].phys_level )
                                {
                                    ChrList[charb].phys_level = za + ChrList[chara].bumpheight;
                                    ChrList[charb].onwhichplatform = chara;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // handle all the character-character and character-particle interactions
    for ( chara = 0; chara < MAXCHR; chara++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        float xa, ya, za, was_xa, was_ya, was_za;

        // make sure that it is on
        if ( !ChrList[chara].on ) continue;

        // reject characters that are in packs, or are marked as non-colliding
        if ( ChrList[chara].inpack || 0 == ChrList[chara].bumpheight ) continue;

        // reject characters that are hidden
        hide = CapList[ ChrList[chara].model ].hidestate;
        if ( hide != NOHIDE && hide == ChrList[chara].ai.state ) continue;

        xa = ChrList[chara].xpos;
        ya = ChrList[chara].ypos;
        za = ChrList[chara].zpos;

        was_xa = xa - ChrList[chara].xvel;
        was_ya = ya - ChrList[chara].yvel;
        was_za = za - ChrList[chara].zvel;

        chridvulnerability = CapList[ChrList[chara].model].idsz[IDSZ_VULNERABILITY];

        // determine the size of this object in blocks
        ixmin = ChrList[chara].xpos - ChrList[chara].bumpsize; ixmin = CLIP(ixmin, 0, mesh.info.edge_x);
        ixmax = ChrList[chara].xpos + ChrList[chara].bumpsize; ixmax = CLIP(ixmax, 0, mesh.info.edge_x);

        iymin = ChrList[chara].ypos - ChrList[chara].bumpsize; iymin = CLIP(iymin, 0, mesh.info.edge_y);
        iymax = ChrList[chara].ypos + ChrList[chara].bumpsize; iymax = CLIP(iymax, 0, mesh.info.edge_y);

        ixmax_block = ixmax >> 9; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );
        ixmin_block = ixmin >> 9; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );

        iymax_block = iymax >> 9; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );
        iymin_block = iymin >> 9; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );

        // handle all the character-character interactions
        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                // Allow raw access here because we were careful :)
                fanblock = mesh_get_block_int(&mesh, ix_block, iy_block);
                if ( INVALID_BLOCK != fanblock )
                {
                    chrinblock = bumplist[fanblock].chrnum;
                    prtinblock = bumplist[fanblock].prtnum;

                    for ( tnc = 0, charb = bumplist[fanblock].chr;
                            tnc < chrinblock && charb != MAXCHR;
                            tnc++, charb = ChrList[charb].bumpnext)
                    {
                        float xb, yb, zb, was_xb, was_yb, was_zb;

                        float dx, dy, dist;
                        float was_dx, was_dy, was_dist;
                        float depth_z, was_depth_z;
                        float lerp_z, radius, radius_xy;

                        bool_t collide_x  = bfalse, was_collide_x;
                        bool_t collide_y  = bfalse, was_collide_y;
                        bool_t collide_xy = bfalse, was_collide_xy;
                        bool_t collide_z  = bfalse, was_collide_z;

                        // Don't collide with self, and only do each collision pair once
                        if ( charb <= chara ) continue;

                        // don't interact with your mount, or your held items
                        if ( chara == ChrList[charb].attachedto || charb == ChrList[chara].attachedto ) continue;

                        if( chara == ChrList[charb].onwhichplatform )
                        {
                            // we know that chara is a platform and charb is on it

                            lerp_z = (ChrList[charb].zpos - ChrList[charb].phys_level) / PLATTOLERANCE;
                            lerp_z = CLIP( lerp_z, -1, 1 );

                            if( lerp_z < 0 )
                            {
                                ChrList[charb].phys_pos_z += (ChrList[charb].phys_level - ChrList[charb].zpos) * 0.25f * (-lerp_z);
                            };

                            if( lerp_z > 0 )
                            {
                                ChrList[chara].holdingweight += ChrList[charb].weight * lerp_z;

                                ChrList[charb].phys_vel_x += ( ChrList[chara].xvel - ChrList[charb].xvel ) * platstick * lerp_z;
                                ChrList[charb].phys_vel_y += ( ChrList[chara].yvel - ChrList[charb].yvel ) * platstick * lerp_z;
                                ChrList[charb].phys_vel_z +=  ( ChrList[chara].zvel - ChrList[charb].zvel ) * lerp_z;
                                ChrList[charb].turnleftright += ( ChrList[chara].turnleftright - ChrList[chara].oldturn ) * platstick * lerp_z;

                                ChrList[charb].jumpready = btrue;
                                ChrList[charb].jumpnumber = ChrList[charb].jumpnumberreset;
                            }

                            // this is handled
                            continue;
                        }
                        
                        if( charb == ChrList[chara].onwhichplatform )
                        {
                            // we know that charb is a platform and chara is on it

                            lerp_z = (ChrList[chara].zpos - ChrList[chara].phys_level) / PLATTOLERANCE;
                            lerp_z = CLIP( lerp_z, -1, 1 );

                            if( lerp_z < 0 )
                            {
                                ChrList[chara].phys_pos_z += (ChrList[chara].phys_level - ChrList[chara].zpos) * 0.25f * lerp_z;
                            }

                            if( lerp_z > 0 )
                            {
                                ChrList[charb].holdingweight += ChrList[chara].weight * lerp_z;

                                ChrList[chara].phys_vel_x += ( ChrList[charb].xvel - ChrList[chara].xvel ) * platstick * lerp_z;
                                ChrList[chara].phys_vel_y += ( ChrList[charb].yvel - ChrList[chara].yvel ) * platstick * lerp_z;
                                ChrList[chara].phys_vel_z +=  ( ChrList[charb].zvel - ChrList[chara].zvel ) * lerp_z;
                                ChrList[chara].turnleftright += ( ChrList[charb].turnleftright - ChrList[charb].oldturn ) * platstick * lerp_z;

                                ChrList[chara].jumpready = btrue;
                                ChrList[chara].jumpnumber = ChrList[charb].jumpnumberreset;
                            }

                            // this is handled
                            continue;
                        }

                        xb = ChrList[charb].xpos;
                        yb = ChrList[charb].ypos;
                        zb = ChrList[charb].zpos;

                        was_xb = xb - ChrList[charb].xvel;
                        was_yb = yb - ChrList[charb].yvel;
                        was_zb = zb - ChrList[charb].zvel;

                        dx = ABS( xa - xb );
                        dy = ABS( ya - yb );
                        dist = dx + dy;

                        was_dx = ABS( was_xa - was_xb );
                        was_dy = ABS( was_ya - was_yb );
                        was_dist = was_dx + was_dy;

                        depth_z = MIN( zb + ChrList[charb].bumpheight, za + ChrList[chara].bumpheight ) - MAX(za, zb);
                        was_depth_z = MIN( was_zb + ChrList[charb].bumpheight, was_za + ChrList[chara].bumpheight ) - MAX(was_za, was_zb);

                        // estimate the radius of interaction based on the z overlap
                        lerp_z  = depth_z / PLATTOLERANCE;
                        lerp_z  = CLIP( lerp_z, 0, 1 );

                        radius    = MIN(ChrList[chara].bumpsize, ChrList[charb].bumpsize) * (1.0f - lerp_z) + (ChrList[chara].bumpsize + ChrList[charb].bumpsize) * lerp_z;
                        radius_xy = MIN(ChrList[chara].bumpsizebig, ChrList[charb].bumpsizebig) * (1.0f - lerp_z) + (ChrList[chara].bumpsizebig + ChrList[charb].bumpsizebig) * lerp_z;

                        // estimate the collisions this frame
                        collide_x  = (dx <= radius);
                        collide_y  = (dy <= radius);
                        collide_xy = (dist <= radius_xy);
                        collide_z  = (depth_z > 0);

                        // estimate the collisions last frame
                        was_collide_x  = (was_dx <= radius);
                        was_collide_y  = (was_dy <= radius);
                        was_collide_xy = (was_dist <= radius_xy);
                        was_collide_z  = (was_depth_z > 0);

                        //------------------
                        // do character-character interactions
                        if ( collide_x && collide_y && collide_xy && depth_z > -PLATTOLERANCE )
                        {
                            float wta, wtb;
                            bool_t collision = bfalse;

                            wta = 0xFFFFFFFF == ChrList[chara].weight ? -(float)0xFFFFFFFF : ChrList[chara].weight;
                            wtb = 0xFFFFFFFF == ChrList[charb].weight ? -(float)0xFFFFFFFF : ChrList[charb].weight;

                            if ( wta == 0 && wtb == 0 )
                            {
                                wta = wtb = 1;
                            }
                            else if ( wta == 0 )
                            {
                                wta = 1;
                                wtb = -0xFFFF;
                            }
                            else if ( wtb == 0 )
                            {
                                wtb = 1;
                                wta = -0xFFFF;
                            }

                            if ( 0.0f == ChrList[chara].bumpdampen && 0.0f == ChrList[charb].bumpdampen )
                            {
                                /* do nothing */
                            }
                            else if ( 0.0f == ChrList[chara].bumpdampen )
                            {
                                // make the weight infinite
                                wta = -0xFFFF;
                            }
                            else if ( 0.0f == ChrList[charb].bumpdampen )
                            {
                                // make the weight infinite
                                wtb = -0xFFFF;
                            }
                            else
                            {
                                // adjust the weights to respect bumpdampen
                                wta /= ChrList[chara].bumpdampen;
                                wtb /= ChrList[charb].bumpdampen;
                            }

                            if ( !collision && collide_z )
                            {
                                float depth_x, depth_y, depth_xy, depth_yx, depth_z;
                                GLvector3 nrm;
                                int exponent = 1;
                                
                                if( CapList[ChrList[chara].model].canuseplatforms && ChrList[charb].platform ) exponent += 2;
                                if( CapList[ChrList[charb].model].canuseplatforms && ChrList[chara].platform ) exponent += 2;


                                nrm.x = nrm.y = nrm.z = 0.0f;

                                depth_x  = MIN(xa + ChrList[chara].bumpsize, xb + ChrList[charb].bumpsize) - MAX(xa - ChrList[chara].bumpsize, xb - ChrList[charb].bumpsize);
                                if ( depth_x < 0.0f )
                                {
                                    depth_x = 0.0f;
                                }
                                else
                                {
                                    float sgn = xb - xa;
                                    sgn = sgn > 0 ? -1 : 1;

                                    nrm.x += sgn / POW(depth_x/PLATTOLERANCE, exponent);
                                }

                                depth_y  = MIN(ya + ChrList[chara].bumpsize, yb + ChrList[charb].bumpsize) - MAX(ya - ChrList[chara].bumpsize, yb - ChrList[charb].bumpsize);
                                if ( depth_y < 0.0f )
                                {
                                    depth_y = 0.0f;
                                }
                                else
                                {
                                    float sgn = yb - ya;
                                    sgn = sgn > 0 ? -1 : 1;

                                    nrm.y += sgn / POW(depth_y/PLATTOLERANCE, exponent);
                                }

                                depth_xy = MIN(xa + ya + ChrList[chara].bumpsizebig, xb + yb + ChrList[charb].bumpsizebig) - MAX(xa + ya - ChrList[chara].bumpsizebig, xb + yb - ChrList[charb].bumpsizebig);
                                if ( depth_xy < 0.0f )
                                {
                                    depth_xy = 0.0f;
                                }
                                else
                                {
                                    float sgn = (xb + yb) - (xa + ya);
                                    sgn = sgn > 0 ? -1 : 1;

                                    nrm.x += sgn / POW(depth_xy/PLATTOLERANCE, exponent);
                                    nrm.y += sgn / POW(depth_xy/PLATTOLERANCE, exponent);
                                }

                                depth_yx = MIN(-xa + ya + ChrList[chara].bumpsizebig, -xb + yb + ChrList[charb].bumpsizebig) - MAX(-xa + ya - ChrList[chara].bumpsizebig, -xb + yb - ChrList[charb].bumpsizebig);
                                if ( depth_yx < 0.0f )
                                {
                                    depth_yx = 0.0f;
                                }
                                else
                                {
                                    float sgn = (-xb + yb) - (-xa + ya);
                                    sgn = sgn > 0 ? -1 : 1;
                                    nrm.x -= sgn / POW(depth_yx/PLATTOLERANCE, exponent);
                                    nrm.y += sgn / POW(depth_yx/PLATTOLERANCE, exponent);
                                }

                                depth_z  = MIN(za + ChrList[chara].bumpheight, zb + ChrList[charb].bumpheight) - MAX( za, zb );
                                if ( depth_z < 0.0f )
                                {
                                    depth_z = 0.0f;
                                }
                                else
                                {
                                    float sgn = (zb + ChrList[charb].bumpheight/2) - (za + ChrList[chara].bumpheight / 2);
                                    sgn = sgn > 0 ? -1 : 1;

                                    nrm.z += sgn / POW(exponent * depth_z/PLATTOLERANCE, exponent);
                                }


                                if ( ABS(nrm.x) + ABS(nrm.y) + ABS(nrm.z) > 0.0f )
                                {

                                    nrm = VNormalize( nrm );

                                    if ( collide_xy != was_collide_xy || collide_x != was_collide_x || collide_y != was_collide_y )
                                    {
                                        // an actual collision
                                        GLvector3 vcom;
                                        float vdot, ratio;

                                        vcom.x = ChrList[chara].xvel * ABS(wta) + ChrList[charb].xvel * ABS(wtb);
                                        vcom.y = ChrList[chara].yvel * ABS(wta) + ChrList[charb].yvel * ABS(wtb);
                                        vcom.z = ChrList[chara].zvel * ABS(wta) + ChrList[charb].zvel * ABS(wtb);

                                        if ( ABS(wta) + ABS(wtb) > 0 )
                                        {
                                            vcom.x /= ABS(wta) + ABS(wtb);
                                            vcom.y /= ABS(wta) + ABS(wtb);
                                            vcom.z /= ABS(wta) + ABS(wtb);
                                        }

                                        // do the bounce
                                        if ( wta >= 0 )
                                        {
                                            vdot = ( ChrList[chara].xvel - vcom.x ) * nrm.x + ( ChrList[chara].yvel - vcom.y ) * nrm.y + ( ChrList[chara].zvel - vcom.z ) * nrm.z;

                                            ratio = (float)ABS(wtb) / ((float)ABS(wta) + (float)ABS(wtb));

                                            ChrList[chara].phys_vel_x  += -ChrList[chara].xvel + vcom.x - vdot * nrm.x * ratio;
                                            ChrList[chara].phys_vel_y  += -ChrList[chara].yvel + vcom.y - vdot * nrm.y * ratio;
                                            ChrList[chara].phys_vel_z  += -ChrList[chara].zvel + vcom.z - vdot * nrm.z * ratio;
                                        }

                                        if ( wtb >= 0 )
                                        {
                                            vdot = ( ChrList[charb].xvel - vcom.x ) * nrm.x + ( ChrList[charb].yvel - vcom.y ) * nrm.y + ( ChrList[charb].zvel - vcom.z ) * nrm.z;

                                            ratio = (float)ABS(wta) / ((float)ABS(wta) + (float)ABS(wtb));

                                            ChrList[charb].phys_vel_x  += -ChrList[charb].xvel + vcom.x - vdot * nrm.x * ratio;
                                            ChrList[charb].phys_vel_y  += -ChrList[charb].yvel + vcom.y - vdot * nrm.y * ratio;
                                            ChrList[charb].phys_vel_z  += -ChrList[charb].zvel + vcom.z - vdot * nrm.z * ratio;
                                        }

                                        collision = btrue;
                                    }
                                    else
                                    {
                                        float tmin;

                                        tmin = 1e6;
                                        if ( nrm.x != 0 )
                                        {
                                            tmin = MIN(tmin, depth_x / ABS(nrm.x) );
                                        }
                                        if ( nrm.y != 0 )
                                        {
                                            tmin = MIN(tmin, depth_y / ABS(nrm.y) );
                                        }
                                        if ( nrm.z != 0 )
                                        {
                                            tmin = MIN(tmin, depth_z / ABS(nrm.z) );
                                        }

                                        if ( nrm.x + nrm.y != 0 )
                                        {
                                            tmin = MIN(tmin, depth_xy / ABS(nrm.x + nrm.y) );
                                        }

                                        if ( -nrm.x + nrm.y != 0 )
                                        {
                                            tmin = MIN(tmin, depth_yx / ABS(-nrm.x + nrm.y) );
                                        }

                                        if ( tmin < 1e6 )
                                        {
                                            if ( wta >= 0.0f )
                                            {
                                                float ratio = (float)ABS(wtb) / ((float)ABS(wta) + (float)ABS(wtb));

                                                ChrList[chara].phys_pos_x += tmin * nrm.x * 0.25f * ratio;
                                                ChrList[chara].phys_pos_y += tmin * nrm.y * 0.25f * ratio;
                                                ChrList[chara].phys_pos_z += tmin * nrm.z * 0.25f * ratio;
                                            }

                                            if ( wtb >= 0.0f )
                                            {
                                                float ratio = (float)ABS(wta) / ((float)ABS(wta) + (float)ABS(wtb));

                                                ChrList[charb].phys_pos_x -= tmin * nrm.x * 0.25f * ratio;
                                                ChrList[charb].phys_pos_y -= tmin * nrm.y * 0.25f * ratio;
                                                ChrList[charb].phys_pos_z -= tmin * nrm.z * 0.25f * ratio;
                                            }
                                        }
                                    }
                                }

                                if ( collision )
                                {
                                    ChrList[chara].ai.bumplast = charb;
                                    ChrList[charb].ai.bumplast = chara;

                                    ChrList[chara].ai.alert |= ALERTIF_BUMPED;
                                    ChrList[charb].ai.alert |= ALERTIF_BUMPED;			
                                }
                            }
                        }
                    }

                    // do mounting
                    charb = ChrList[chara].ai.bumplast;
                    if ( charb != chara && ChrList[charb].on && !ChrList[charb].inpack && ChrList[charb].attachedto == MAXCHR && 0 != ChrList[chara].bumpheight && 0 != ChrList[charb].bumpheight )
                    {
                        float xb, yb, zb;
                        float dx, dy;

                        xb = ChrList[charb].xpos;
                        yb = ChrList[charb].ypos;
                        zb = ChrList[charb].zpos;

                        // First check absolute value diamond
                        dx = ABS( xa - xb );
                        dy = ABS( ya - yb );
                        dist = dx + dy;
                        if ( dist < ChrList[chara].bumpsizebig || dist < ChrList[charb].bumpsizebig )
                        {
                            // Then check bounding box square...  Square+Diamond=Octagon
                            if ( ( dx < ChrList[chara].bumpsize || dx < ChrList[charb].bumpsize ) &&
                                    ( dy < ChrList[chara].bumpsize || dy < ChrList[charb].bumpsize ) )
                            {
                                // Now see if either is on top the other like a platform
                                if ( za > zb + ChrList[charb].bumpheight - PLATTOLERANCE + ChrList[chara].zvel - ChrList[charb].zvel && ( CapList[ChrList[chara].model].canuseplatforms || za > zb + ChrList[charb].bumpheight ) )
                                {
                                    // Is A falling on B?
                                    if ( za < zb + ChrList[charb].bumpheight && ChrList[charb].platform && ChrList[chara].alive )//&&ChrList[chara].flyheight==0)
                                    {
                                        if ( MadList[ChrList[chara].inst.imad].actionvalid[ACTIONMI] && ChrList[chara].alive && ChrList[charb].alive && ChrList[charb].ismount && !ChrList[chara].isitem && ChrList[charb].holdingwhich[SLOT_LEFT] == MAXCHR && ChrList[chara].attachedto == MAXCHR && ChrList[chara].jumptime == 0 && ChrList[chara].flyheight == 0 )
                                        {
                                            attach_character_to_mount( chara, charb, GRIP_ONLY );
                                            ChrList[chara].ai.bumplast = chara;
                                            ChrList[charb].ai.bumplast = charb;
                                        }
                                    }
                                }
                                else
                                {
                                    if ( zb > za + ChrList[chara].bumpheight - PLATTOLERANCE + ChrList[charb].zvel - ChrList[chara].zvel && ( CapList[ChrList[charb].model].canuseplatforms || zb > za + ChrList[chara].bumpheight ) )
                                    {
                                        // Is B falling on A?
                                        if ( zb < za + ChrList[chara].bumpheight && ChrList[chara].platform && ChrList[charb].alive )//&&ChrList[charb].flyheight==0)
                                        {
                                            if ( MadList[ChrList[charb].inst.imad].actionvalid[ACTIONMI] && ChrList[chara].alive && ChrList[charb].alive && ChrList[chara].ismount && !ChrList[charb].isitem && ChrList[chara].holdingwhich[SLOT_LEFT] == MAXCHR && ChrList[charb].attachedto == MAXCHR && ChrList[charb].jumptime == 0 && ChrList[charb].flyheight == 0 )
                                            {
                                                attach_character_to_mount( charb, chara, GRIP_ONLY );

                                                ChrList[chara].ai.bumplast = chara;
                                                ChrList[charb].ai.bumplast = charb;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Now check collisions with every bump particle in same area
                    if ( ChrList[chara].alive )
                    {
                        for ( tnc = 0, partb = bumplist[fanblock].prt;
                                tnc < prtinblock;
                                tnc++, partb = PrtList[partb].bumpnext )
                        {
                            float xb, yb, zb;
                            float dx, dy;

                            // do not collide with the thing that you're attached to
                            if ( chara == PrtList[partb].attachedtocharacter ) continue;

                            // if there's no friendly fire, particles issued by chara can't hit it
                            if ( !PipList[PrtList[partb].pip].friendlyfire && chara == PrtList[partb].chr ) continue;

                            xb = PrtList[partb].xpos;
                            yb = PrtList[partb].ypos;
                            zb = PrtList[partb].zpos;

                            // First check absolute value diamond
                            dx = ABS( xa - xb );
                            dy = ABS( ya - yb );
                            dist = dx + dy;
                            if ( dist < ChrList[chara].bumpsizebig || dist < PrtList[partb].bumpsizebig )
                            {
                                // Then check bounding box square...  Square+Diamond=Octagon
                                if ( ( dx < ChrList[chara].bumpsize  || dx < PrtList[partb].bumpsize ) &&
                                        ( dy < ChrList[chara].bumpsize  || dy < PrtList[partb].bumpsize ) &&
                                        ( zb > za - PrtList[partb].bumpheight && zb < za + ChrList[chara].bumpheight + PrtList[partb].bumpheight ) )
                                {
                                    pip = PrtList[partb].pip;
                                    if ( zb > za + ChrList[chara].bumpheight + PrtList[partb].zvel && PrtList[partb].zvel < 0 && ChrList[chara].platform && PrtList[partb].attachedtocharacter == MAXCHR )
                                    {
                                        // Particle is falling on A
                                        PrtList[partb].zpos = za + ChrList[chara].bumpheight;
                                        PrtList[partb].zvel = -PrtList[partb].zvel * PipList[pip].dampen;
                                        PrtList[partb].xvel += ( ChrList[chara].xvel ) * platstick;
                                        PrtList[partb].yvel += ( ChrList[chara].yvel ) * platstick;
                                    }

                                    // Check reaffirmation of particles
                                    if ( PrtList[partb].attachedtocharacter != chara )
                                    {
                                        if ( ChrList[chara].reloadtime == 0 )
                                        {
                                            if ( ChrList[chara].reaffirmdamagetype == PrtList[partb].damagetype && ChrList[chara].damagetime == 0 )
                                            {
                                                reaffirm_attached_particles( chara );
                                            }
                                        }
                                    }

                                    // Check for missile treatment
                                    if ( ( ChrList[chara].damagemodifier[PrtList[partb].damagetype]&3 ) < 2 ||
                                            ChrList[chara].missiletreatment == MISNORMAL ||
                                            PrtList[partb].attachedtocharacter != MAXCHR ||
                                            ( PrtList[partb].chr == chara && !PipList[pip].friendlyfire ) ||
                                            ( ChrList[ChrList[chara].missilehandler].mana < ( ChrList[chara].missilecost << 4 ) && !ChrList[ChrList[chara].missilehandler].canchannel ) )
                                    {
                                        if ( ( TeamList[PrtList[partb].team].hatesteam[ChrList[chara].team] || ( PipList[pip].friendlyfire && ( ( chara != PrtList[partb].chr && chara != ChrList[PrtList[partb].chr].attachedto ) || PipList[pip].onlydamagefriendly ) ) ) && !ChrList[chara].invictus )
                                        {
                                            spawn_bump_particles( chara, partb ); // Catch on fire
                                            if ( ( PrtList[partb].damagebase | PrtList[partb].damagerand ) > 1 )
                                            {
                                                prtidparent = CapList[PrtList[partb].model].idsz[IDSZ_PARENT];
                                                prtidtype = CapList[PrtList[partb].model].idsz[IDSZ_TYPE];
                                                if ( ChrList[chara].damagetime == 0 && PrtList[partb].attachedtocharacter != chara && ( PipList[pip].damfx&DAMFX_ARRO ) == 0 )
                                                {
                                                    // Normal partb damage
                                                    if ( PipList[pip].allowpush )
                                                    {
                                                        ChrList[chara].phys_vel_x  += -ChrList[chara].xvel + PrtList[partb].xvel * ChrList[chara].bumpdampen;
                                                        ChrList[chara].phys_vel_y  += -ChrList[chara].yvel + PrtList[partb].yvel * ChrList[chara].bumpdampen;
                                                        ChrList[chara].phys_vel_z  += -ChrList[chara].zvel + PrtList[partb].zvel * ChrList[chara].bumpdampen;
                                                    }

                                                    direction = ( ATAN2( PrtList[partb].yvel, PrtList[partb].xvel ) + PI ) * 0xFFFF / ( TWO_PI );
                                                    direction = ChrList[chara].turnleftright - direction + 32768;
                                                    // Check all enchants to see if they are removed
                                                    enchant = ChrList[chara].firstenchant;

                                                    while ( enchant != MAXENCHANT )
                                                    {
                                                        eveidremove = EveList[EncList[enchant].eve].removedbyidsz;
                                                        temp = EncList[enchant].nextenchant;
                                                        if ( eveidremove != IDSZ_NONE && ( eveidremove == prtidtype || eveidremove == prtidparent ) )
                                                        {
                                                            remove_enchant( enchant );
                                                        }

                                                        enchant = temp;
                                                    }

                                                    // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                                                    //+2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                                                    if ( PipList[pip].intdamagebonus )
                                                    {
                                                        int percent;
                                                        percent = ( ChrList[PrtList[partb].chr].intelligence - 3584 ) >> 7;
                                                        percent /= 100;
                                                        PrtList[partb].damagebase *= 1 + percent;
                                                    }
                                                    if ( PipList[pip].wisdamagebonus )
                                                    {
                                                        int percent;
                                                        percent = ( ChrList[PrtList[partb].chr].wisdom - 3584 ) >> 7;
                                                        percent /= 100;
                                                        PrtList[partb].damagebase *= 1 + percent;
                                                    }

                                                    // Damage the character
                                                    if ( chridvulnerability != IDSZ_NONE && ( chridvulnerability == prtidtype || chridvulnerability == prtidparent ) )
                                                    {
                                                        damage_character( chara, direction, PrtList[partb].damagebase << 1, PrtList[partb].damagerand << 1, PrtList[partb].damagetype, PrtList[partb].team, PrtList[partb].chr, PipList[pip].damfx, bfalse );
                                                        ChrList[chara].ai.alert |= ALERTIF_HITVULNERABLE;
                                                    }
                                                    else
                                                    {
                                                        damage_character( chara, direction, PrtList[partb].damagebase, PrtList[partb].damagerand, PrtList[partb].damagetype, PrtList[partb].team, PrtList[partb].chr, PipList[pip].damfx, bfalse );
                                                    }

                                                    // Do confuse effects
                                                    if ( 0 == ( Md2FrameList[ChrList[chara].inst.frame].framefx&MADFXINVICTUS ) || PipList[pip].damfx&DAMFX_BLOC )
                                                    {
                                                        if ( PipList[pip].grogtime != 0 && CapList[ChrList[chara].model].canbegrogged )
                                                        {
                                                            ChrList[chara].grogtime += PipList[pip].grogtime;
                                                            if ( ChrList[chara].grogtime < 0 )  ChrList[chara].grogtime = 32767;

                                                            ChrList[chara].ai.alert |= ALERTIF_GROGGED;
                                                        }
                                                        if ( PipList[pip].dazetime != 0 && CapList[ChrList[chara].model].canbedazed )
                                                        {
                                                            ChrList[chara].dazetime += PipList[pip].dazetime;
                                                            if ( ChrList[chara].dazetime < 0 )  ChrList[chara].dazetime = 32767;

                                                            ChrList[chara].ai.alert |= ALERTIF_DAZED;
                                                        }
                                                    }

                                                    // Notify the attacker of a scored hit
                                                    if ( PrtList[partb].chr != MAXCHR )
                                                    {
                                                        ChrList[PrtList[partb].chr].ai.alert |= ALERTIF_SCOREDAHIT;
                                                        ChrList[PrtList[partb].chr].ai.hitlast = chara;
                                                    }
                                                }
                                                if ( ( frame_wld&31 ) == 0 && PrtList[partb].attachedtocharacter == chara )
                                                {
                                                    // Attached partb damage ( Burning )
                                                    if ( PipList[pip].xyvelbase == 0 )
                                                    {
                                                        // Make character limp
                                                        ChrList[chara].phys_vel_x += -ChrList[chara].xvel;
                                                        ChrList[chara].phys_vel_y += -ChrList[chara].yvel;
                                                    }

                                                    damage_character( chara, 32768, PrtList[partb].damagebase, PrtList[partb].damagerand, PrtList[partb].damagetype, PrtList[partb].team, PrtList[partb].chr, PipList[pip].damfx, bfalse );
                                                }
                                            }
                                            if ( PipList[pip].endbump )
                                            {
                                                if ( PipList[pip].bumpmoney )
                                                {
                                                    if ( ChrList[chara].cangrabmoney && ChrList[chara].alive && ChrList[chara].damagetime == 0 && ChrList[chara].money != MAXMONEY )
                                                    {
                                                        if ( ChrList[chara].ismount )
                                                        {
                                                            // Let mounts collect money for their riders
                                                            if ( ChrList[chara].holdingwhich[SLOT_LEFT] != MAXCHR )
                                                            {
                                                                ChrList[ChrList[chara].holdingwhich[SLOT_LEFT]].money += PipList[pip].bumpmoney;
                                                                if ( ChrList[ChrList[chara].holdingwhich[SLOT_LEFT]].money > MAXMONEY ) ChrList[ChrList[chara].holdingwhich[SLOT_LEFT]].money = MAXMONEY;
                                                                if ( ChrList[ChrList[chara].holdingwhich[SLOT_LEFT]].money < 0 ) ChrList[ChrList[chara].holdingwhich[SLOT_LEFT]].money = 0;

                                                                PrtList[partb].time = 1;
                                                            }
                                                        }
                                                        else
                                                        {
                                                            // Normal money collection
                                                            ChrList[chara].money += PipList[pip].bumpmoney;
                                                            if ( ChrList[chara].money > MAXMONEY ) ChrList[chara].money = MAXMONEY;
                                                            if ( ChrList[chara].money < 0 ) ChrList[chara].money = 0;

                                                            PrtList[partb].time = 1;
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    PrtList[partb].time = 1;
                                                    // Only hit one character, not several
                                                    PrtList[partb].damagebase = 0;
                                                    PrtList[partb].damagerand = 1;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if ( PrtList[partb].chr != chara )
                                        {
                                            cost_mana( ChrList[chara].missilehandler, ( ChrList[chara].missilecost << 4 ), PrtList[partb].chr );

                                            // Treat the missile
                                            if ( ChrList[chara].missiletreatment == MISDEFLECT )
                                            {
                                                // Use old position to find normal
                                                ax = PrtList[partb].xpos - PrtList[partb].xvel;
                                                ay = PrtList[partb].ypos - PrtList[partb].yvel;
                                                ax = ChrList[chara].xpos - ax;
                                                ay = ChrList[chara].ypos - ay;
                                                // Find size of normal
                                                scale = ax * ax + ay * ay;
                                                if ( scale > 0 )
                                                {
                                                    // Make the normal a unit normal
                                                    scale = SQRT( scale );
                                                    nx = ax / scale;
                                                    ny = ay / scale;
                                                    // Deflect the incoming ray off the normal
                                                    scale = ( PrtList[partb].xvel * nx + PrtList[partb].yvel * ny ) * 2;
                                                    ax = scale * nx;
                                                    ay = scale * ny;
                                                    PrtList[partb].xvel = PrtList[partb].xvel - ax;
                                                    PrtList[partb].yvel = PrtList[partb].yvel - ay;
                                                }
                                            }
                                            else
                                            {
                                                // Reflect it back in the direction it came
                                                PrtList[partb].xvel = -PrtList[partb].xvel;
                                                PrtList[partb].yvel = -PrtList[partb].yvel;
                                            }

                                            // Change the owner of the missile
                                            if ( !PipList[pip].homing )
                                            {
                                                PrtList[partb].team = ChrList[chara].team;
                                                PrtList[partb].chr = chara;
                                            }

                                            // Change the direction of the partb
                                            if ( PipList[pip].rotatetoface )
                                            {
                                                // Turn to face new direction
                                                facing = ATAN2( PrtList[partb].yvel, PrtList[partb].xvel ) * 0xFFFF / ( TWO_PI );
                                                facing += 32768;
                                                PrtList[partb].facing = facing;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // accumulate the accumulators
    for ( character = 0; character < MAXCHR; character++ )
    {
        float tmpx, tmpy, tmpz;
        float bump_str = 1.0f - (float)ChrList[character].phys_dismount_timer / PHYS_DISMOUNT_TIME;

        // decrement the dismount timer
        if ( ChrList[character].phys_dismount_timer > 0 ) ChrList[character].phys_dismount_timer--;

        // do the "integration" of the accumulated accelerations
        ChrList[character].xvel += ChrList[character].phys_vel_x;
        ChrList[character].yvel += ChrList[character].phys_vel_y;
        ChrList[character].zvel += ChrList[character].phys_vel_z;

        // do the "integration" on the position
        if ( ABS(ChrList[character].phys_pos_x) > 0 )
        {
            tmpx = ChrList[character].xpos;
            ChrList[character].xpos += ChrList[character].phys_pos_x;
            if ( __chrhitawall(character) )
            {
                // restore the old values
                ChrList[character].xpos = tmpx;
            }
            else
            {
                ChrList[character].xvel += ChrList[character].phys_pos_x * bump_str;
                ChrList[character].oldx = tmpx;
            }
        }

        if ( ABS(ChrList[character].phys_pos_y) > 0 )
        {
            tmpy = ChrList[character].ypos;
            ChrList[character].ypos += ChrList[character].phys_pos_y;
            if ( __chrhitawall(character) )
            {
                // restore the old values
                ChrList[character].ypos = tmpy;
            }
            else
            {
                ChrList[character].yvel += ChrList[character].phys_pos_y * bump_str;
                ChrList[character].oldy = tmpy;
            }
        }

        if ( ABS(ChrList[character].phys_pos_z) > 0 )
        {
            tmpz = ChrList[character].zpos;
            ChrList[character].zpos += ChrList[character].phys_pos_z;
            if ( ChrList[character].zpos < ChrList[character].phys_level )
            {
                // restore the old values
                ChrList[character].zpos = tmpz;
            }
            else
            {
                ChrList[character].zvel += ChrList[character].phys_pos_z * bump_str;
                ChrList[character].oldz = tmpz;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;

    // Do reload time
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( ChrList[cnt].reloadtime > 0 )
        {
            ChrList[cnt].reloadtime--;
        }

        cnt++;
    }

    // Do stats
    if ( clock_stat >= ONESECOND )
    {
        // Reset the clock
        clock_stat -= ONESECOND;

        // Do all the characters
        for ( cnt = 0; cnt < MAXCHR; cnt++ )
        {
            if ( !ChrList[cnt].on ) continue;

            // check for a level up
            do_level_up( cnt );

            // do the mana and life regen for "living" characters
            if ( ChrList[cnt].alive )
            {
                ChrList[cnt].mana += ( ChrList[cnt].manareturn / MANARETURNSHIFT );
                ChrList[cnt].mana = MAX(0, MIN(ChrList[cnt].mana, ChrList[cnt].manamax));

                ChrList[cnt].life += ChrList[cnt].lifereturn;
                ChrList[cnt].life = MAX(1, MIN(ChrList[cnt].life, ChrList[cnt].life));
            }
            if ( ChrList[cnt].grogtime > 0 )
            {
                ChrList[cnt].grogtime--;
            }
            if ( ChrList[cnt].dazetime > 0 )
            {
                ChrList[cnt].dazetime--;
            }
        }

        // Run through all the enchants as well
        for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
        {
            if ( !EncList[cnt].on ) continue;

            if ( EncList[cnt].time != 0 )
            {
                if ( EncList[cnt].time > 0 )
                {
                    EncList[cnt].time--;
                }

                owner = EncList[cnt].owner;
                target = EncList[cnt].target;
                eve = EncList[cnt].eve;

                // Do drains
                if ( ChrList[owner].alive )
                {
                    // Change life
                    ChrList[owner].life += EncList[cnt].ownerlife;
                    if ( ChrList[owner].life < 1 )
                    {
                        ChrList[owner].life = 1;
                        kill_character( owner, target );
                    }
                    if ( ChrList[owner].life > ChrList[owner].lifemax )
                    {
                        ChrList[owner].life = ChrList[owner].lifemax;
                    }

                    // Change mana
                    if ( !cost_mana( owner, -EncList[cnt].ownermana, target ) && EveList[eve].endifcantpay )
                    {
                        remove_enchant( cnt );
                    }
                }
                else if ( !EveList[eve].stayifnoowner )
                {
                    remove_enchant( cnt );
                }

                if ( EncList[cnt].on )
                {
                    if ( ChrList[target].alive )
                    {
                        // Change life
                        ChrList[target].life += EncList[cnt].targetlife;
                        if ( ChrList[target].life < 1 )
                        {
                            ChrList[target].life = 1;
                            kill_character( target, owner );
                        }
                        if ( ChrList[target].life > ChrList[target].lifemax )
                        {
                            ChrList[target].life = ChrList[target].lifemax;
                        }

                        // Change mana
                        if ( !cost_mana( target, -EncList[cnt].targetmana, owner ) && EveList[eve].endifcantpay )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else
                    {
                        remove_enchant( cnt );
                    }
                }
            }
            else
            {
                remove_enchant( cnt );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    // ZZ> This function sets all of the character's starting tilt values
    int cnt;
    Uint8 twist;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !ChrList[cnt].on ) continue;

        if ( ChrList[cnt].stickybutt && INVALID_TILE != ChrList[cnt].onwhichfan )
        {
            twist = mesh.mem.tile_list[ChrList[cnt].onwhichfan].twist;
            ChrList[cnt].turnmaplr = maplrtwist[twist];
            ChrList[cnt].turnmapud = mapudtwist[twist];
        }
        else
        {
            ChrList[cnt].turnmaplr = 32768;
            ChrList[cnt].turnmapud = 32768;
        }
    }

}

//--------------------------------------------------------------------------------------------
int load_all_objects( const char *modname )
{
    // ZZ> This function loads a module's local objects and overrides the global ones already loaded
    const char *filehandle;
    bool_t keeplooking;
    char newloadname[256];
    char filename[256];
    int cnt;
    int skin;
    int importplayer;

    // Log all of the script errors
    parseerror = bfalse;

    //This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        CapList[cnt].importslot = 10000;
    }

    // Load the import directory
    importplayer = -1;
    importobject = -100;
    skin = TX_LAST;  // Character skins start after the last special texture
    if ( importvalid )
    {
        for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
        {
            // Make sure the object exists...
            sprintf( filename, "import" SLASH_STR "temp%04d.obj", cnt );
            sprintf( newloadname, "%s" SLASH_STR "data.txt", filename );
            if ( fs_fileExists(newloadname) )
            {
                // new player found
                if ( 0 == ( cnt % MAXIMPORTPERPLAYER ) ) importplayer++;

                // store the slot info
                importobject = ( importplayer * MAXIMPORTPERPLAYER ) + ( cnt % MAXIMPORTPERPLAYER );
                CapList[importobject].importslot = cnt;

                // load it
                skin += load_one_object( filename, skin );
            }
        }
    }

    // Search for .obj directories and load them
    importobject = -100;
    make_newloadname( modname, "objects" SLASH_STR, newloadname );
    filehandle = fs_findFirstFile( newloadname, "obj" );

    keeplooking = btrue;
    if ( filehandle != NULL )
    {
        while ( keeplooking )
        {
            sprintf( filename, "%s%s", newloadname, filehandle );
            skin += load_one_object( filename, skin );

            filehandle = fs_findNextFile();

            keeplooking = ( filehandle != NULL );
        }
    }

    fs_findClose();
    return skin;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo )
{
    int cnt;
    char cTmp;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return bfalse;

    // check for another entry
    if ( !goto_colon_yesno( fileread ) ) return bfalse;

    fscanf( fileread, "%s", pinfo->spawn_name );
    for ( cnt = 0; cnt < sizeof(pinfo->spawn_name); cnt++ )
    {
        if ( pinfo->spawn_name[cnt] == '_' )  pinfo->spawn_name[cnt] = ' ';
    }

    pinfo->pname = pinfo->spawn_name;
    if ( 0 == strcmp( pinfo->spawn_name, "NONE") )
    {
        // Random pinfo->pname
        pinfo->pname = NULL;
    }

    fscanf( fileread, "%d", &pinfo->slot );

    fscanf( fileread, "%f%f%f", &pinfo->x, &pinfo->y, &pinfo->z );
    pinfo->x *= 128;  pinfo->y *= 128;  pinfo->z *= 128;

    pinfo->facing = NORTH;
    pinfo->attach = ATTACH_NONE;
    cTmp = fget_first_letter( fileread );
    if ( 'S' == toupper(cTmp) )  pinfo->facing = SOUTH;
    else if ( 'E' == toupper(cTmp) )  pinfo->facing = EAST;
    else if ( 'W' == toupper(cTmp) )  pinfo->facing = WEST;
    else if ( '?' == toupper(cTmp) )  pinfo->facing = RANDOM;
    else if ( 'L' == toupper(cTmp) )  pinfo->attach = ATTACH_LEFT;
    else if ( 'R' == toupper(cTmp) )  pinfo->attach = ATTACH_RIGHT;
    else if ( 'I' == toupper(cTmp) )  pinfo->attach = ATTACH_INVENTORY;

    fscanf( fileread, "%d%d%d%d%d", &pinfo->money, &pinfo->skin, &pinfo->passage, &pinfo->content, &pinfo->level );
    if (pinfo->skin >= MAXSKIN) pinfo->skin = rand() % MAXSKIN;     //Randomize skin?
	
    cTmp = fget_first_letter( fileread );
    pinfo->stat = ( 'T' == toupper(cTmp) );

    cTmp = fget_first_letter( fileread );   //BAD! Unused ghost value

    cTmp = fget_first_letter( fileread );
    pinfo->team = ( cTmp - 'A' ) % MAXTEAM;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo )
{
    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    if ( ichr >= MAXCHR || !ChrList[ichr].on ) return bfalse;

    ChrList[ichr].money += pinfo->money;
    if ( ChrList[ichr].money > MAXMONEY )  ChrList[ichr].money = MAXMONEY;
    if ( ChrList[ichr].money < 0 )  ChrList[ichr].money = 0;

    ChrList[ichr].ai.content = pinfo->content;
    ChrList[ichr].ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        pack_add_item( ichr, pinfo->parent );

        ChrList[ichr].ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        ChrList[ichr].attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        ChrList[ichr].attachedto = MAXCHR;  // Fix grab

    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        Uint16 grip = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, pinfo->parent, grip );

        // Handle the "grabbed" messages
        let_character_think( ichr );
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        while ( ChrList[ichr].experiencelevel < pinfo->level && ChrList[ichr].experience < MAXXP )
        {
            give_experience( ichr, 25, XPDIRECT, btrue );
            do_level_up( ichr );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void setup_characters( const char *modname )
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
    int new_object, tnc, local_index = 0;
    STRING newloadname;
    FILE *fileread;

    chr_setup_info_t info;

    // Turn all characters off
    free_all_characters();

    // Turn some back on
    make_newloadname( modname, "gamedat" SLASH_STR "spawn.txt", newloadname );
    fileread = fopen( newloadname, "r" );

    numpla = 0;
    info.parent = MAXCHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s", newloadname );
    }
    else
    {
        info.parent = 0;

        while ( chr_setup_read( fileread, &info ) )
        {
            // Spawn the character
            if ( info.team < numplayer || !rtscontrol || info.team >= MAXPLAYER )
            {
                new_object = spawn_one_character( info.x, info.y, info.z, info.slot, info.team, info.skin, info.facing, info.pname, MAXCHR );

                if ( MAXCHR != new_object )
                {
                    // determine the attachment
                    if ( info.attach == ATTACH_NONE )
                    {
                        // Free character
                        info.parent = new_object;
                        make_one_character_matrix( new_object );
                    }

                    chr_setup_apply( new_object, &info );

                    // Turn on numpla input devices
                    if ( info.stat )
                    {

                        if ( 0 == importamount && numpla < playeramount )
                        {
                            if ( 0 == local_numlpla )
                            {
                                // the first player gets everything
                                add_player( new_object, numpla, (Uint32)(~0) );
                            }
                            else
                            {
                                int i;
                                Uint32 bits;

                                // each new player steals an input device from the 1st player
                                bits = 1 << local_numlpla;
                                for ( i = 0; i < MAXPLAYER; i++ )
                                {
                                    PlaList[i].device &= ~bits;
                                }

                                add_player( new_object, numpla, bits );
                            }

                        }
                        else if ( numpla < numimport && numpla < importamount && numpla < playeramount )
                        {
                            // Multiplayer import module
                            local_index = -1;
                            for ( tnc = 0; tnc < numimport; tnc++ )
                            {
                                if ( CapList[ChrList[new_object].model].importslot == local_slot[tnc] )
                                {
                                    local_index = tnc;
                                    break;
                                }
                            }

                            if ( -1 != local_index )
                            {
                                // It's a local numpla
                                add_player( new_object, numpla, local_control[local_index] );
                            }
                            else
                            {
                                // It's a remote numpla
                                add_player( new_object, numpla, INPUT_BITS_NONE );
                            }
                        }

                        // Turn on the stat display
                        statlist_add( new_object );
                    }
                }
            }
        }

        fclose( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    statlist_sort();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void load_all_global_objects(int skin)
{
    //ZF> This function loads all global objects found in the basicdat folder
    const char *filehandle;
    bool_t keeplooking;
    STRING newloadname;
    STRING filename;

    //Warn the user for any duplicate slots
    overrideslots = bfalse;

    // Search for .obj directories and load them
    sprintf( newloadname, "basicdat" SLASH_STR "globalobjects" SLASH_STR );
    filehandle = fs_findFirstFile( newloadname, "obj" );

    keeplooking = btrue;
    if ( filehandle != NULL )
    {
        while ( keeplooking )
        {
            sprintf( filename, "%s%s", newloadname, filehandle );
            skin += load_one_object( filename, skin );

            filehandle = fs_findNextFile();

            keeplooking = ( filehandle != NULL );
        }
    }

    fs_findClose();
}

//--------------------------------------------------------------------------------------------
bool_t load_module( const char *smallname )
{
    // ZZ> This function loads a module
    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    // Load all the global icons
    if ( !load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
    }

    beatmodule = bfalse;
    timeron = bfalse;

    snprintf( modname, sizeof(modname), "modules" SLASH_STR "%s" SLASH_STR, smallname );
    make_randie();
    reset_teams();
    load_one_icon( "basicdat" SLASH_STR "nullicon" );  // This works (without transparency)

    load_global_waves( modname );

    reset_particles( modname );
    read_wawalite( modname );
    make_twist();
    reset_messages();
    prime_names();
    load_basic_textures( modname );
    release_all_ai_scripts();
    load_ai_script( "basicdat" SLASH_STR "script.txt" );
    release_all_models();
    free_all_enchants();

    //Load all objects
    {
        int skin;
        skin = load_all_objects(modname);
        load_all_global_objects(skin);
    }

    if ( NULL == mesh_load( modname, &mesh ) )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "Uh oh! Problems loading the mesh! (%s)\n", modname );
        return bfalse;
    }

    setup_particles();
    setup_passage( modname );
    reset_players();

    setup_characters( modname );

    reset_end_text();
    setup_alliances( modname );

    // Load fonts and bars after other images, as not to hog videomem
    font_load( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );
    load_bars( "basicdat" SLASH_STR "bars" );
    load_map( modname );

    log_madused( "slotused.txt" );

    // Start playing the damage tile sound silently...
    /*PORT
    play_sound_pvf_looped(damagetilesound, PANMID, VOLMIN, FRQDEFAULT);
    */

    return btrue;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has no attached particles
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( PrtList[particle].on && PrtList[particle].attachedtocharacter == character )
        {
            free_one_particle( particle );
        }
    }

    // Set the alert for disaffirmation ( wet torch )
    ChrList[character].ai.alert |= ALERTIF_DISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
    // ZZ> This function returns the number of particles attached to the given character
    Uint16 cnt, particle;

    cnt = 0;
    particle = 0;

    while ( particle < maxparticles )
    {
        if ( PrtList[particle].on && PrtList[particle].attachedtocharacter == character )
        {
            cnt++;
        }

        particle++;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has all of it's particles
    Uint16 numberattached;
    Uint16 particle;

    numberattached = number_of_attached_particles( character );

    while ( numberattached < CapList[ChrList[character].model].attachedprtamount )
    {
        particle = spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, ChrList[character].model, CapList[ChrList[character].model].attachedprttype, character, GRIP_LAST + numberattached, ChrList[character].team, character, numberattached, MAXCHR );
        if ( particle != TOTALMAXPRT )
        {
            attach_particle_to_character( particle, character, PrtList[particle].grip );
        }

        numberattached++;
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    ChrList[character].ai.alert |= ALERTIF_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    // BB > all of the de-initialization code after the module actually ends

    release_module();
    close_session();

    // reset the "ui" mouse state
    pressed           = bfalse;
    clicked           = bfalse;
    pending_click     = bfalse;
    mouse_wheel_event = bfalse;

    // reset some of the module state variables
    beatmodule  = bfalse;
    exportvalid = bfalse;
}

//-----------------------------------------------------------------
bool_t game_init_module( const char * modname, Uint32 seed )
{
    // BB> all of the initialization code before the module actually starts

    srand( seed );

    // start the new module
    if ( !load_module( modname ) )
    {
        moduleactive = bfalse;
        gameactive   = bfalse;
        return bfalse;
    };

    pressed = bfalse;

    make_onwhichfan();
    camera_reset(&gCamera);
    reset_timers();
    figure_out_what_to_draw();
    make_character_matrices();
    attach_particles();

    if ( networkon )
    {
        net_sayHello();
    }

    // Let the game go
    moduleactive = btrue;
    randsave = 0;
    srand( randsave );

    return moduleactive;
}

//-----------------------------------------------------------------
bool_t game_update_imports()
{
    // BB> This function saves all the players to the players dir
    //     and also copies them into the imports dir to prepare for the next module

    bool_t is_local;
    int cnt, tnc, j, character, player;
    STRING srcPlayer, srcDir, destDir;

    // do the normal export to save all the player settings
    export_all_players( btrue );

    // reload all of the available players
    check_player_import( "players", btrue  );
    check_player_import( "remote",  bfalse );

    // build the import directory using the player info
    empty_import_directory();
    fs_createDirectory( "import" );

    // export all of the players directly from memory straight to the "import" dir
    for ( player = 0, cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList[character].on ) continue;

        is_local = ( 0 != PlaList[cnt].device );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].dir, get_file_path(ChrList[character].name) ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            log_warning( "game_update_imports() - cannot find exported file for \"%s\" (\"%s\") \n", ChrList[character].name, get_file_path(ChrList[character].name) ) ;
            continue;
        }

        // grab the controls from the currently loaded players
        // calculate the slot from the current player count
        local_control[player] = PlaList[cnt].device;
        local_slot[player]    = player * MAXIMPORTPERPLAYER;
        player++;

        // Copy the character to the import directory
        if ( is_local )
        {
            sprintf( srcPlayer, "players" SLASH_STR "%s", loadplayer[tnc].dir );
        }
        else
        {
            sprintf( srcPlayer, "remote" SLASH_STR "%s", loadplayer[tnc].dir );
        }

        sprintf( destDir, "import" SLASH_STR "temp%04d.obj", local_slot[tnc] );
        fs_copyDirectory( srcPlayer, destDir );

        // Copy all of the character's items to the import directory
        for ( j = 0; j < 8; j++ )
        {
            sprintf( srcDir, "%s" SLASH_STR "%d.obj", srcPlayer, j );
            sprintf( destDir, "import" SLASH_STR "temp%04d.obj", local_slot[tnc] + j + 1 );

            fs_copyDirectory( srcDir, destDir );
        }
    }

    return btrue;
}

void release_module()
{
    // ZZ> This function frees up memory used by the module

    release_all_icons();
    release_all_titleimages();
    release_bars();
    release_blip();
    release_map();
    release_all_textures();
    release_all_models();

    // Close and then reopen SDL_mixer. it's easier than manually unloading each sound ;)
    sound_restart();
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
    // ZZ> This function attaches particles to their characters so everything gets
    //     drawn right
    int cnt;

    cnt = 0;

    while ( cnt < maxparticles )
    {
        if ( PrtList[cnt].on && PrtList[cnt].attachedtocharacter != MAXCHR )
        {
            attach_particle_to_character( cnt, PrtList[cnt].attachedtocharacter, PrtList[cnt].grip );

            // Correct facing so swords knock characters in the right direction...
            if ( PipList[PrtList[cnt].pip].damfx&DAMFX_TURN )
            {
                PrtList[cnt].facing = ChrList[PrtList[cnt].attachedtocharacter].turnleftright;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int add_player( Uint16 character, Uint16 player, Uint32 device )
{
    // ZZ> This function adds a player, returning bfalse if it fails, btrue otherwise
    int cnt;

    if ( !PlaList[player].valid )
    {
        ChrList[character].isplayer = btrue;
        PlaList[player].index = character;
        PlaList[player].valid = btrue;
        PlaList[player].device = device;

        PlaList[player].latchx = 0;
        PlaList[player].latchy = 0;
        PlaList[player].latchbutton = 0;

        for ( cnt = 0; cnt < MAXLAG; cnt++ )
        {
            PlaList[player].tlatch[cnt].x      = 0;
            PlaList[player].tlatch[cnt].y      = 0;
            PlaList[player].tlatch[cnt].button = 0;
            PlaList[player].tlatch[cnt].time   = (Uint32)(~0);
        }

        if ( device != INPUT_BITS_NONE )
        {
            local_noplayers = bfalse;
            ChrList[character].islocalplayer = btrue;
            local_numlpla++;
        }

        numpla++;
        return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void let_all_characters_think()
{
    // ZZ> This function lets every computer controlled character do AI stuff
    int character;
    static Uint32 last_frame = (Uint32)(~0);

    // make sure there is only one update per frame;
    if ( frame_wld == last_frame ) return;
    last_frame = frame_wld;

    numblip = 0;
    for ( character = 0; character < MAXCHR; character++ )
    {
        bool_t is_crushed, is_cleanedup, can_think;

        if ( !ChrList[character].on ) continue;

        // check for actions that must always be handled
        is_cleanedup = ( 0 != ( ChrList[character].ai.alert & ALERTIF_CLEANEDUP ) );
        is_crushed   = ( 0 != ( ChrList[character].ai.alert & ALERTIF_CRUSHED   ) );

        // let the script run sometimes even if the item is in your backpack
        can_think = !ChrList[character].inpack || CapList[ChrList[character].model].isequipment;

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if ( ( ChrList[character].alive && can_think ) || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts( character );

            // Crushed characters shouldn't be alert to anything else
            if ( is_crushed )  { ChrList[character].ai.alert = ALERTIF_CRUSHED; ChrList[character].ai.timer = frame_wld + 1; }

            // Cleaned up characters shouldn't be alert to anything else
            if ( is_cleanedup )  { ChrList[character].ai.alert = ALERTIF_CLEANEDUP; ChrList[character].ai.timer = frame_wld + 1; }

            let_character_think( character );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_allocate( mesh_mem_t * pmem, mesh_info_t * pinfo  )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mesh_mem_free(pmem) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate new memory
    pmem->vrt_x = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_x == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_y = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_y == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_z = ( float * ) calloc( pinfo->vertcount, sizeof(float) );
    if ( pmem->vrt_z == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_a = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_a == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    pmem->vrt_l = ( Uint8 * ) calloc( pinfo->vertcount, sizeof(Uint8) );
    if ( pmem->vrt_l == NULL )
    {
        log_error( "Reduce the maximum number of pinfo->vertcount! (Check MESH_MAXTOTALVERTRICES)\n" );
    }

    // set the vertex count
    pmem->vertcount = pinfo->vertcount;

    pmem->tile_list  = (tile_info_t *) calloc( pinfo->tiles_count, sizeof(tile_info_t) );

    pmem->blockstart = (Uint32*) calloc( pinfo->blocks_y, sizeof(Uint32) );
    pmem->tilestart  = (Uint32*) calloc( pinfo->tiles_y, sizeof(Uint32) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_free( mesh_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->vrt_x != NULL )
    {
        free( pmem->vrt_x );
        pmem->vrt_x = NULL;
    }

    if ( pmem->vrt_y != NULL )
    {
        free( pmem->vrt_y );
        pmem->vrt_y = NULL;
    }

    if ( pmem->vrt_z != NULL )
    {
        free( pmem->vrt_z );
        pmem->vrt_z = NULL;
    }

    if ( pmem->vrt_a != NULL )
    {
        free( pmem->vrt_a );
        pmem->vrt_a = NULL;
    }

    if ( pmem->vrt_l != NULL )
    {
        free( pmem->vrt_l );
        pmem->vrt_l = NULL;
    }

    if ( NULL != pmem->tile_list )
    {
        free(pmem->tile_list);
        pmem->tile_list = NULL;
    }

    if ( NULL != pmem->blockstart )
    {
        free(pmem->blockstart);
        pmem->blockstart = NULL;
    }

    if ( NULL != pmem->tilestart )
    {
        free(pmem->tilestart);
        pmem->tilestart = NULL;
    }

    // reset some values to safe values
    pmem->vertcount = 0;

    return btrue;
}

//struct s_line_of_sight_info
//{
//    float x0, y0;
//    float x1, y1;
//    Uint32 stopped_by;
//
//    Uint16 collide_chr;
//    Uint32 collide_fx;
//    int    collide_x;
//    int    collide_y;
//};
//
//typedef struct s_line_of_sight_info line_of_sight_info_t;
//
//bool_t do_line_of_sight( line_of_sight_info_t * plos )
//{
//    int Dx, Dy;
//
//    int xstep, ystep;
//    int TwoDy, TwoDyTwoDx, E;
//    int y;
//    int xDraw, yDraw;
//
//    int steep;
//
//    if(NULL == plos) return bfalse;
//
//    int steep = (ABS(Dy) >= ABS(Dx));
//
//    int Dx = plos->x1 - plos->x0;
//    int Dy = plos->y1 - plos->y0;
//
//    if (steep)
//    {
//        SWAP(int, plos->x0, plos->y0);
//        SWAP(int, plos->x1, plos->y1);
//
//        // recompute Dx, Dy after swap
//        Dx = plos->x1 - plos->x0;
//        Dy = plos->y1 - plos->y0;
//    }
//
//    xstep = 1;
//    if (Dx < 0)
//    {
//        xstep = -1;
//        Dx = -Dx;
//    }
//
//    ystep = 1;
//    if (Dy < 0)
//    {
//        ystep = -1;
//        Dy = -Dy;
//    }
//
//    TwoDy = 2*Dy;
//    TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
//    E = TwoDy - Dx; //2*Dy - Dx
//    y = plos->y0;
//    block_last = INVALID_BLOCK;
//    for (int x = plos->x0; x != plos->x1; x += xstep)
//    {
//        int fan;
//
//        if (steep)
//        {
//            xDraw = y;
//            yDraw = x;
//        }
//        else
//        {
//            xDraw = x;
//            yDraw = y;
//        }
//
//        // plot
//        fan = meshgetfan(xDraw, yDraw);
//        if( INVALID_FAN != fan && fan != last_fan )
//        {
//            // collide the ray with the mesh
//
//            if( 0!= (mesh.mem.tile_list[fan].fx & plos->stopped_by) )
//            {
//                plos->collide_x  = xDraw;
//                plos->collide_y  = yDraw;
//                plos->collide_fx = mesh.mem.tile_list[fan].fx & plos->stopped_by;
//
//                return btrue;
//            }
//
//            last_fan = fan;
//        }
//
//        block = meshgetblock(xDraw, yDraw);
//        if( INVALID_BLOCK != block && block != block_last )
//        {
//            // collide the ray with all items/characters in this block
//
//        };
//
//        // next
//        if (E > 0)
//        {
//            E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
//            y = y + ystep;
//        } else
//        {
//            E += TwoDy; //E += 2*Dy;
//        }
//    }
//}

mesh_info_t * mesh_info_new( mesh_info_t * pinfo )
{
    if (NULL == pinfo) return pinfo;

    memset( pinfo, 0, sizeof(mesh_info_t) );

    return pinfo;
}

mesh_mem_t * mesh_mem_new( mesh_mem_t * pmem )
{
    if (NULL == pmem) return pmem;

    memset( pmem, 0, sizeof(mesh_mem_t) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
void mesh_make_fanstart( mesh_t * pmesh )
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh) return;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    // do the tilestart
    for ( cnt = 0; cnt < pinfo->tiles_y; cnt++ )
    {
        pmem->tilestart[cnt] = pinfo->tiles_x * cnt;
    }

    // calculate some of the block info
    if ( pinfo->blocks_x >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", pinfo->blocks_x, MAXMESHBLOCKY );
    }

    if ( pinfo->blocks_y >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", pinfo->blocks_y, MAXMESHBLOCKY );
    }

    // do the blockstart
    for ( cnt = 0; cnt < pinfo->blocks_y; cnt++ )
    {
        pmem->blockstart[cnt] = pinfo->blocks_x * cnt;
    }

}

//--------------------------------------------------------------------------------------------
void mesh_make_vrtstart( mesh_t * pmesh )
{
    int x, y, vert;
    Uint32 fan;

    mesh_info_t * pinfo;
    mesh_mem_t  * pmem;

    if (NULL == pmesh) return;

    pinfo = &(pmesh->info);
    pmem  = &(pmesh->mem);

    vert = 0;
    for ( y = 0; y < pinfo->tiles_y; y++ )
    {
        for ( x = 0; x < pinfo->tiles_x; x++ )
        {
            // allow raw access because we are careful
            fan = mesh_get_tile_int( &mesh, x, y );
            if ( INVALID_TILE != fan )
            {
                Uint8 ttype = pmem->tile_list[fan].type;

                pmem->tile_list[fan].vrtstart = vert;

                if ( ttype > MAXMESHTYPE )
                {
                    vert += 4;
                }
                else
                {
                    // throw away any remaining upper bits
                    ttype &= 0x3F;
                    vert += tile_dict[ttype].numvertices;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_new( mesh_t * pmesh )
{
    if ( NULL != pmesh )
    {
        memset( pmesh, 0, sizeof(mesh_t) );

        mesh_mem_new( &(pmesh->mem) );
        mesh_info_new( &(pmesh->info) );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_delete( mesh_t * pmesh )
{
    if ( NULL != pmesh )
    {
        mesh_mem_delete( &(pmesh->mem) );
        mesh_info_delete( &(pmesh->info)  );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mesh_t * mesh_renew( mesh_t * pmesh )
{
    pmesh = mesh_delete( pmesh );

    return mesh_new( pmesh );
}

//--------------------------------------------------------------------------------------------
mesh_info_t * mesh_info_delete( mesh_info_t * pinfo )
{
    if ( NULL != pinfo )
    {
        memset( pinfo, 0, sizeof(mesh_info_t) );
    }

    return pinfo;
}

//--------------------------------------------------------------------------------------------
mesh_mem_t * mesh_mem_delete( mesh_mem_t * pmem )
{
    if ( NULL != pmem )
    {
        mesh_mem_free( pmem );
    }

    return pmem;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block_int( mesh_t * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->info.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->info.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->mem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile_int( mesh_t * pmesh, int tile_x,  int tile_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( tile_x < 0 || tile_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( tile_y < 0 || tile_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return tile_x + pmesh->mem.tilestart[tile_y];
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( int which )
{
    if ( !gamemenuactive )
    {
        gamemenu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        mnu_draw_background = bfalse;
        gamemenuactive = btrue;
    }

    return gamemenuactive;
}

//--------------------------------------------------------------------------------------------
void game_end_menu()
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= gamemenu_depth )
    {
        gamemenuactive = bfalse;
        gamemenu_depth = -1;
    }
}