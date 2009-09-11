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

#include "game.h"

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
#include "network.h"
#include "mad.h"
#include "mesh.h"
#include "texture.h"
#include "wawalite.h"

#include "char.h"
#include "particle.h"
#include "camera.h"
#include "id_md2.h"

#include "script_compile.h"
#include "script.h"

#include "egoboo_vfs.h"
#include "egoboo_endian.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"

#include "SDL_extensions.h"

#include <SDL_image.h>

#include <time.h>
#include <assert.h>
#include <float.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CHR_MAX_COLLISIONS    512*16
#define COLLISION_HASH_NODES (CHR_MAX_COLLISIONS*2)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_line_of_sight_info
{
    float x0, y0, z0;
    float x1, y1, z1;
    Uint32 stopped_by;

    Uint16 collide_chr;
    Uint32 collide_fx;
    int    collide_x;
    int    collide_y;
};

typedef struct s_line_of_sight_info line_of_sight_info_t;

static bool_t collide_ray_with_mesh( line_of_sight_info_t * plos );
static bool_t collide_ray_with_characters( line_of_sight_info_t * plos );
static bool_t do_line_of_sight( line_of_sight_info_t * plos );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_chr_setup_info
{
    bool_t     do_spawn;
    STRING     spawn_coment;

    STRING     spawn_name;
    char      *pname;
    Sint32     slot;
    GLvector3  pos;
    int        passage, content, money, level, skin;
    bool_t     stat;
    Uint8      team;
    Uint16     facing;
    Uint16     attach;
    Uint16     parent;
};
typedef struct s_chr_setup_info chr_setup_info_t;

//--------------------------------------------------------------------------------------------
// Bump List
struct s_bumplist
{
    Uint16  chr;                     // For character collisions
    Uint16  chrnum;                  // Number on the block
    Uint16  prt;                     // For particle collisions
    Uint16  prtnum;                  // Number on the block
};
typedef struct s_bumplist bumplist_t;

//--------------------------------------------------------------------------------------------
// pair-wise collision data

struct s_collision_data
{
    Uint16 chra, chrb;
    Uint16 prtb;
};

typedef struct s_collision_data co_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ego_mpd_t         _mesh[2];
static camera_t          _camera[2];
static ego_process_t     _eproc;
static menu_process_t    _mproc;
static game_process_t    _gproc;
static module_instance_t gmod;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t  overrideslots      = bfalse;

bool_t    screenshotkeyready = btrue;

// End text
char   endtext[MAXENDTEXT] = { '\0' };
int    endtextwrite = 0;

// Status displays
bool_t staton     = btrue;
int    numstat    = 0;
Uint16 statlist[MAXSTAT];

ego_mpd_t         * PMesh   = _mesh + 0;
camera_t          * PCamera = _camera + 0;
module_instance_t * PMod    = &gmod;
ego_process_t     * EProc   = &_eproc;
menu_process_t    * MProc   = &_mproc;
game_process_t    * GProc   = &_gproc;

bool_t  pitskill  = bfalse;
bool_t  pitsfall  = bfalse;
Uint32  pitx;
Uint32  pity;
Uint32  pitz;

Uint16  glouseangle = 0;                                        // actually still used

Uint32                animtile_update_and = 0;
animtile_instance_t   animtile[2];
damagetile_instance_t damagetile;
weather_instance_t    weather;
water_instance_t      water;
fog_instance_t        fog;

Uint8  local_senseenemiesTeam = TEAM_GOOD; // TEAM_MAX;
IDSZ   local_senseenemiesID   = IDSZ_NONE;

Uint32  randindex = 0;
Uint16  randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void make_randie();
static void reset_teams();
static void reset_messages();
static void reset_timers();
static void _quit_game( ego_process_t * pgame );

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
static bool_t game_load_module_data( const char *smallname );
static void   game_release_module_data();
static void   game_load_all_objects( const char *modname );

static void   setup_characters( const char *modname );
static void   setup_alliances( const char *modname );
static int    load_one_object( const char* tmploadname, int slot_override );
static void   load_all_global_objects();

static bool_t chr_setup_read( vfs_FILE * fileread, chr_setup_info_t *pinfo );
static bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo );
static void   setup_characters( const char *modname );

// Model stuff
static void log_madused( const char *savename );

// Collision stuff
static bool_t add_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );
static bool_t add_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );

static bool_t detect_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b );
static bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b );

static bumplist_t bumplist[MAXMESHFAN/16];

static int           chr_co_count = 0;
static hash_list_t * chr_co_list;

// "process" management
static int do_ego_proc_begin( ego_process_t * eproc );
static int do_ego_proc_running( ego_process_t * eproc );
static int do_ego_proc_leaving( ego_process_t * eproc );
static int do_ego_proc_run( ego_process_t * eproc, double frameDuration );

static int do_menu_proc_begin( menu_process_t * mproc );
static int do_menu_proc_running( menu_process_t * mproc );
static int do_menu_proc_leaving( menu_process_t * mproc );
static int do_menu_proc_run( menu_process_t * mproc, double frameDuration );

static int do_game_proc_begin( game_process_t * gproc );
static int do_game_proc_running( game_process_t * gproc );
static int do_game_proc_leaving( game_process_t * gproc );
static int do_game_proc_run( game_process_t * gproc, double frameDuration );

// profile handling
static void init_all_pip();
static void init_all_eve();
static void init_all_cap();
static void init_all_mad();

static void release_all_pip();
static void release_all_eve();
static void release_all_cap();
static void release_all_mad();

// misc
static bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which );
static void   game_end_menu( menu_process_t * mproc );

static void   memory_cleanUp(void);

static void   do_game_hud();

//--------------------------------------------------------------------------------------------
// Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local )
{
    // ZZ> This function exports a character
    int tnc = 0, profile;
    STRING fromdir;
    STRING todir;
    STRING fromfile;
    STRING tofile;
    STRING todirname;
    STRING todirfullname;

    // Don't export enchants
    disenchant_character( character );

    profile = ChrList.lst[character].model;
    if ( ( CapList[profile].cancarrytonextmodule || !CapList[profile].isitem ) && PMod->exportvalid )
    {
        // TWINK_BO.OBJ
        snprintf( todirname, SDL_arraysize( todirname), "%s", str_encode_path(ChrList.lst[owner].name) );

        // Is it a character or an item?
        if ( owner != character )
        {
            // Item is a subdirectory of the owner directory...
            snprintf( todirfullname, SDL_arraysize( todirfullname), "%s" SLASH_STR "%d.obj", todirname, number );
        }
        else
        {
            // Character directory
            snprintf( todirfullname, SDL_arraysize( todirfullname), "%s", todirname );
        }

        // players/twink.obj or players/twink.obj/sword.obj
        if ( is_local )
        {
            snprintf( todir, SDL_arraysize( todir), "players" SLASH_STR "%s", todirfullname );
        }
        else
        {
            snprintf( todir, SDL_arraysize( todir), "remote" SLASH_STR "%s", todirfullname );
        }

        // modules/advent.mod/objects/advent.obj
        snprintf( fromdir, SDL_arraysize( fromdir), "%s", MadList[profile].name );

        // Delete all the old items
        if ( owner == character )
        {
            for ( tnc = 0; tnc < MAXIMPORTOBJECTS; tnc++ )
            {
                snprintf( tofile, SDL_arraysize( tofile), "%s" SLASH_STR "%d.obj", todir, tnc );  /*.OBJ*/
                vfs_removeDirectoryAndContents( tofile, btrue );
            }
        }

        // Make the directory
        vfs_mkdir( todir );

        // Build the DATA.TXT file
        snprintf( tofile, SDL_arraysize( tofile), "%s" SLASH_STR "data.txt", todir );  /*DATA.TXT*/
        export_one_character_profile( tofile, character );

        // Build the SKIN.TXT file
        snprintf( tofile, SDL_arraysize( tofile), "%s" SLASH_STR "skin.txt", todir );  /*SKIN.TXT*/
        export_one_character_skin( tofile, character );

        // Build the NAMING.TXT file
        snprintf( tofile, SDL_arraysize( tofile), "%s" SLASH_STR "naming.txt", todir );  /*NAMING.TXT*/
        export_one_character_name( tofile, character );

        // Copy all of the misc. data files
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "message.txt", fromdir );  /*MESSAGE.TXT*/
        snprintf( tofile, SDL_arraysize( tofile), "%s" SLASH_STR "message.txt", todir );  /*MESSAGE.TXT*/
        vfs_copyFile( fromfile, tofile );
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "tris.md2", fromdir );  /*TRIS.MD2*/
        snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "tris.md2", todir );  /*TRIS.MD2*/
        vfs_copyFile( fromfile, tofile );
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "copy.txt", fromdir );  /*COPY.TXT*/
        snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "copy.txt", todir );  /*COPY.TXT*/
        vfs_copyFile( fromfile, tofile );
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "script.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "script.txt", todir );
        vfs_copyFile( fromfile, tofile );
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "enchant.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "enchant.txt", todir );
        vfs_copyFile( fromfile, tofile );
        snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "credits.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "credits.txt", todir );
        vfs_copyFile( fromfile, tofile );
        //    snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "quest.txt", fromdir );     Zefz> We can't do this yet, quests are written directly into players/x.obj
        //    snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "quest.txt", todir );       instead of import/x.obj which should be changed or all changes are lost.
        vfs_copyFile( fromfile, tofile );

        // Copy all of the particle files
        for ( tnc = 0; tnc < MAX_PIP_PER_PROFILE; tnc++ )
        {
            snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "part%d.txt", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "part%d.txt", todir,   tnc );
            vfs_copyFile( fromfile, tofile );
        }

        // Copy all of the sound files
        for ( tnc = 0; tnc < MAX_WAVE; tnc++ )
        {
            snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "sound%d.wav", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "sound%d.wav", todir,   tnc );
            vfs_copyFile( fromfile, tofile );

            snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "sound%d.ogg", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "sound%d.ogg", todir,   tnc );
            vfs_copyFile( fromfile, tofile );
        }

        // Copy all of the image files (try to copy all supported formats too)
        for ( tnc = 0; tnc < MAXSKIN; tnc++ )
        {
            Uint8 type;

            for (type = 0; type < maxformattypes; type++)
            {
                snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "tris%d%s", fromdir, tnc, TxFormatSupported[type] );
                snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "tris%d%s", todir,   tnc, TxFormatSupported[type] );
                vfs_copyFile( fromfile, tofile );
                snprintf( fromfile, SDL_arraysize( fromfile), "%s" SLASH_STR "icon%d%s", fromdir, tnc, TxFormatSupported[type] );
                snprintf( tofile, SDL_arraysize( tofile),   "%s" SLASH_STR "icon%d%s", todir,   tnc, TxFormatSupported[type] );
                vfs_copyFile( fromfile, tofile );
            }
        }
    }
}

//---------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    // ZZ> This function saves all the local players in the
    //    PLAYERS directory
    bool_t is_local;
    int cnt, character, item, number;

    // Don't export if the module isn't running
    if ( !process_instance_running( PROC_PBASE(GProc) ) ) return;

    // Stop if export isnt valid
    if (!PMod->exportvalid) return;

    // Check each player
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        is_local = ( 0 != PlaList[cnt].device );
        if ( require_local && !is_local ) continue;
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList.lst[character].on || !ChrList.lst[character].alive ) continue;

        // Export the character
        number = 0;
        export_one_character( character, character, number, is_local );

        // Export the left hand item
        number = SLOT_LEFT;
        item = ChrList.lst[character].holdingwhich[number];
        if ( item != MAX_CHR && ChrList.lst[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the right hand item
        number = SLOT_RIGHT;
        item = ChrList.lst[character].holdingwhich[number];
        if ( item != MAX_CHR && ChrList.lst[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the inventory
        for ( number = 2, item = ChrList.lst[character].pack_next;
                number < MAXIMPORTOBJECTS && item != MAX_CHR;
                item = ChrList.lst[item].pack_next )
        {
            if ( ChrList.lst[item].isitem )
            {
                export_one_character( item, character, number++, is_local );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void _quit_game( ego_process_t * pgame )
{
    // ZZ> This function exits the game entirely

    if ( process_instance_running( PROC_PBASE(pgame) ) )
    {
        game_quit_module();
    }

    // tell the game to kill itself
    process_instance_kill( PROC_PBASE(pgame) );

    vfs_empty_import_directory();
}

//--------------------------------------------------------------------------------------------
void getadd( int min, int value, int max, int* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //    to not overflow the min and max bounds
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
    //    to not overflow the min and max bounds
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
void log_madused( const char *savename )
{
    // ZZ> This is a debug function for checking model loads
    vfs_FILE* hFileWrite;
    int cnt;

    hFileWrite = vfs_openWrite( savename );
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        vfs_printf( hFileWrite, "%d of %d frames used...\n", md2_loadframe, MAXFRAME );
        cnt = 0;

        while ( cnt < MAX_PROFILE )
        {
            vfs_printf( hFileWrite, "%3d %32s %s\n", cnt, CapList[cnt].classname, MadList[cnt].name );
            cnt++;
        }

        vfs_close( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_add( Uint16 character )
{
    // ZZ> This function adds a status display to the do list
    if ( numstat < MAXSTAT )
    {
        statlist[numstat] = character;
        ChrList.lst[character].staton = btrue;
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

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].loaded ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        pchr->nextaction = ACTION_DA;
        pchr->action = action;

        pchr->inst.lip = 0;
        pchr->inst.frame_lst = pchr->inst.frame_nxt;
        pchr->inst.frame_nxt = pmad->actionstart[pchr->action];
        pchr->actionready    = actionready;
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip )
{
    // ZZ> This function sets the frame for a character explicitly...  This is used to
    //    rotate Tank turrets

    chr_t * pchr;
    mad_t * pmad;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].loaded ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        int framesinaction, frame_stt, frame_end;

        pchr->nextaction = ACTION_DA;
        pchr->action = ACTION_DA;
        pchr->actionready = btrue;

        framesinaction = (pmad->actionend[action] - pmad->actionstart[action]) + 1;
        if ( framesinaction <= 1 )
        {
            frame_stt = pmad->actionstart[action];
            frame_end = frame_stt;
        }
        else
        {
            frame = MIN(frame, framesinaction);
            frame_stt = pmad->actionstart[action] + frame;

            frame = MIN(frame + 1, framesinaction);
            frame_end = frame_stt + 1;
        }

        pchr->inst.lip = ( lip << 6 );
        pchr->inst.frame_lst = frame_stt;
        pchr->inst.frame_nxt = frame_end;
    }
}

//--------------------------------------------------------------------------------------------
int generate_number( IPair num )
{
    // ZZ> This function generates a random number

    int tmp = 0;
    int irand = RANDIE;

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
int generate_randmask( int base, int mask )
{
    // ZZ> This function generates a random number
    int tmp;
    int irand = RANDIE;

    tmp = base;
    if ( mask > 0 )
    {
        tmp += irand & mask;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
void setup_alliances( const char *modname )
{
    // ZZ> This function reads the alliance file
    STRING newloadname;
    STRING szTemp;
    Uint8 teama, teamb;
    vfs_FILE *fileread;

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "alliance.txt", newloadname );
    fileread = vfs_openRead( newloadname );
    if ( fileread )
    {
        while ( goto_colon( NULL, fileread, btrue ) )
        {
            fget_string( fileread, szTemp, SDL_arraysize(szTemp) );
            teama = ( szTemp[0] - 'A' ) % TEAM_MAX;

            fget_string( fileread, szTemp, SDL_arraysize(szTemp) );
            teamb = ( szTemp[0] - 'A' ) % TEAM_MAX;
            TeamList[teama].hatesteam[teamb] = bfalse;
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void update_game()
{
    // ZZ> This function does several iterations of character movements and such
    //    to keep the game in sync.
    //    This is the main game loop
    int cnt, numdead, numalive;

    // Check for all local players being dead
    local_allpladead   = bfalse;
    local_seeinvisible = bfalse;
    local_seekurse     = bfalse;

    numdead = numalive = 0;
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid) continue;

        // only interested in local players
        if ( INPUT_BITS_NONE == PlaList[cnt].device ) continue;

        // fix bad players
        if ( INVALID_CHR(PlaList[cnt].index) )
        {
            PlaList[cnt].valid = bfalse;
            continue;
        }

        if ( ChrList.lst[PlaList[cnt].index].alive )
        {
            numalive++;

            if ( ChrList.lst[PlaList[cnt].index].canseeinvisible )
            {
                local_seeinvisible = btrue;
            }

            if ( ChrList.lst[PlaList[cnt].index].canseekurse )
            {
                local_seekurse = btrue;
            }
        }
        else
        {
            numdead++;
        }
    }

    // Did everyone die?
    if ( numdead >= local_numlpla )
    {
        local_allpladead = btrue;
    }

    // check for autorespawn
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        if ( INVALID_CHR(PlaList[cnt].index) ) continue;

        if ( !ChrList.lst[PlaList[cnt].index].alive )
        {
            if ( cfg.difficulty < GAME_HARD && local_allpladead && SDLKEYDOWN( SDLK_SPACE ) && PMod->respawnvalid && 0 == revivetimer )
            {
                respawn_character( PlaList[cnt].index );
                ChrList.lst[cnt].experience *= EXPKEEP;  // Apply xp Penality
                if (cfg.difficulty > GAME_EASY) ChrList.lst[cnt].money *= EXPKEEP;
            }
        }
    }

    // get all player latches from the "remotes"
    sv_talkToRemotes();

    // [claforte Jan 6th 2001]
    // TODO: Put that back in place once networking is functional.
    while ( clock_wld < clock_all && numplatimes > 0 )
    {
        // Important stuff to keep in sync
        srand( PMod->randsave );
        PMod->randsave = rand();

        resize_characters();
        keep_weapons_with_holders();
        let_all_characters_think();
        do_weather_spawn();
        do_enchant_spawn();
        unbuffer_player_latches();

        make_onwhichfan();
        move_characters();
        move_particles();
        make_character_matrices( update_wld != 0 );
        attach_particles();
        bump_characters();

        stat_return();
        update_pits();

        // Stuff for which sync doesn't matter
        animate_tiles();
        move_water();
        looped_update_all_sound();

        // update all the billboards
        BillboardList_update_all();

        // Timers
        clock_wld  += UPDATE_SKIP;
        clock_stat += UPDATE_SKIP;

        // Reset the respawn timer
        if ( revivetimer > 0 )
        {
            revivetimer -= UPDATE_SKIP;
        }

        update_wld++;
    }

    if ( PNet->on )
    {
        if ( numplatimes == 0 )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            clock_wld += 25;
        }
        if ( numplatimes > 3 && !PNet->hostactive )
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
void reset_teams()
{
    // ZZ> This function makes everyone hate everyone else
    int teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamList[teama].hatesteam[teamb] = btrue;
        }

        // Make the team like itself
        TeamList[teama].hatesteam[teama] = bfalse;

        // Set defaults
        TeamList[teama].leader = NOLEADER;
        TeamList[teama].sissy = 0;
        TeamList[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamList[teama].hatesteam[TEAM_NULL] = bfalse;
        TeamList[TEAM_NULL].hatesteam[teama] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    // ZZ> This makes messages safe to use
    int cnt;

    MessageOffset.count = 0;
    message_buffer_carat = 0;
    msgtimechange = 0;
    DisplayMsg.count = 0;

    for ( cnt = 0; cnt < MAX_MESSAGE; cnt++ )
    {
        DisplayMsg.lst[cnt].time = 0;
    }

    for ( cnt = 0; cnt < MAXTOTALMESSAGE; cnt++ )
    {
        MessageOffset.lst[cnt] = 0;
    }

    message_buffer[0] = '\0';
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    // ZZ> This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    cnt = 0;
    while ( cnt < RANDIE_COUNT )
    {
        randie[cnt] = rand() << 1;
        cnt++;
    }

    // Keep adjusting those values
    tnc = 0;

    while ( tnc < 20 )
    {
        cnt = 0;

        while ( cnt < RANDIE_COUNT )
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
    clock_pit = 0;  pitskill = pitsfall = bfalse;

    update_wld = 0;
    frame_all = 0;
    frame_fps = 0;
    outofsync = bfalse;
}

//--------------------------------------------------------------------------------------------
int game_do_menu( menu_process_t * mproc )
{
    // BB> do menus

    int menuResult;
    bool_t need_menu = bfalse;

    need_menu = bfalse;
    if ( flip_pages_requested() )
    {
        // someone else (and that means the game) has drawn a frame
        // so we just need to draw the menu over that frame
        need_menu = btrue;

        // force the menu to be displayed immediately when the game stops
        mproc->base.dtime = 1.0f / (float)cfg.framelimit;
    }
    else if (  !process_instance_running( PROC_PBASE(GProc) ) )
    {
        // the menu's frame rate is controlled by a timer
        mproc->frame_now = SDL_GetTicks();
        if (mproc->frame_now > mproc->frame_next)
        {
            float  frameskip = (float)TICKS_PER_SEC / (float)cfg.framelimit;
            mproc->frame_next = mproc->frame_now + frameskip; // FPS limit

            need_menu = btrue;
            mproc->base.dtime = 1.0f / (float)cfg.framelimit;
        }
    }

    menuResult = 0;
    if ( need_menu )
    {
        ui_beginFrame( mproc->base.dtime );
        {
            menuResult = doMenu( mproc->base.dtime );
            request_flip_pages();
        }
        ui_endFrame();
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_ego_proc_begin( ego_process_t * eproc )
{
    // Initialize logging first, so that we can use it everywhere.
    log_init();
    log_setLoggingLevel( 2 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n" );

    sys_initialize();
    clk_init();
    vfs_init( eproc->argv0 );

    // read the "setup.txt" file
    if ( !setup_read( "setup.txt" ) )
    {
        log_error( "Could not find Setup.txt\n" );
    }

    // download the "setup.txt" values into the cfg struct
    setup_download( &cfg );

    // do basic system initialization
    sdl_init();
    ogl_init();
    net_initialize();

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL ); \
    GLSetup_SupportedFormats();

    // read all the scantags
    scantag_read_all( "basicdat" SLASH_STR "scancode.txt" );
    input_settings_load( "controls.txt" );

    // synchronoze the config values with the various game subsystems
    // do this acter the sdl_init() and ogl_init() in case the config values are clamped
    // to valid values
    setup_synch( &cfg );

    // initialize the sound system
    sound_initialize();
    load_all_music_sounds();

    // make sure that a bunch of stuff gets initialized properly
    module_instance_init( &gmod );
    init_all_graphics();
    init_all_profiles();
    free_all_objects();
    mesh_new( PMesh );

    // setup the menu system's gui
    ui_initialize( "basicdat" SLASH_STR "Negatori.ttf", 24 );
    font_load( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );  // must be done after init_all_graphics()

    // clear out the import directory
    vfs_empty_import_directory();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // initialize the game process (not active)
    game_process_init( GProc );

    // initialize the menu process (active)
    menu_process_init( MProc );
    process_instance_start( PROC_PBASE(MProc) );

    // Initialize the process
    process_instance_start( PROC_PBASE(eproc) );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_running( ego_process_t * eproc )
{
    bool_t menu_valid, game_valid;

    if ( !process_instance_validate( PROC_PBASE(eproc) ) ) return -1;

    eproc->was_active  = eproc->base.valid;

    menu_valid = process_instance_validate( PROC_PBASE(MProc) );
    game_valid = process_instance_validate( PROC_PBASE(GProc) );
    if ( !menu_valid && !game_valid )
    {
        process_instance_kill( PROC_PBASE(eproc) );
        return 1;
    }

    if ( eproc->base.paused ) return 0;

    if ( process_instance_running( PROC_PBASE(MProc) ) )
    {
        // menu settings
        SDL_WM_GrabInput ( SDL_GRAB_OFF );
        SDL_ShowCursor( SDL_ENABLE  );
    }
    else
    {
        // in-game settings
        SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
        SDL_WM_GrabInput ( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
    }

    // Clock updates each frame
    clk_frameStep();
    eproc->frameDuration = clk_getFrameDuration();

    // read the input values
    input_read();

    if ( pickedmodule_ready && !process_instance_running( PROC_PBASE(MProc) ) )
    {
        // a new module has been picked

        // reset the flag
        pickedmodule_ready = bfalse;

        // start the game process
        process_instance_start( PROC_PBASE(GProc) );
    }

    // Test the panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
        // terminate the program
        process_instance_kill( PROC_PBASE(eproc) );
    }

    // Check for screenshots
    if ( !SDLKEYDOWN( SDLK_F11 ) )
    {
        screenshotkeyready = btrue;
    }
    else if ( screenshotkeyready && SDLKEYDOWN( SDLK_F11 ) && keyb.on )
    {
        screenshotkeyready = bfalse;

        if ( !dump_screenshot() )                // Take the shot, returns bfalse if failed
        {
            debug_printf( "Error writing screenshot!" );
            log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
        }
    }

    // handle an escape by passing it on to all active sub-processes
    if ( eproc->escape_requested )
    {
        eproc->escape_requested = bfalse;

        if ( process_instance_running( PROC_PBASE(GProc) ) )
        {
            GProc->escape_requested = btrue;
        }

        if ( process_instance_running( PROC_PBASE(MProc) ) )
        {
            MProc->escape_requested = btrue;
        }
    }

    // run the sub-processes
    do_game_proc_run( GProc, EProc->frameDuration );
    do_menu_proc_run( MProc, EProc->frameDuration );

    // a heads up display that can be used to debug values that are used by both the menu and the game
    // do_game_hud();

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_leaving( ego_process_t * eproc )
{
    if ( !process_instance_validate( PROC_PBASE(eproc) )  ) return -1;

    // make sure that the
    if ( !GProc->base.terminated )
    {
        do_game_proc_run( GProc, eproc->frameDuration );
    }

    if ( !MProc->base.terminated )
    {
        do_menu_proc_run( MProc, eproc->frameDuration );
    }

    if ( GProc->base.terminated && MProc->base.terminated )
    {
        process_instance_terminate( PROC_PBASE(eproc) );
    }

    return eproc->base.terminated ? 0 : 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_run( ego_process_t * eproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(eproc) )  ) return -1;
    eproc->base.dtime = frameDuration;

    if ( !eproc->base.paused ) return 0;

    if ( eproc->base.killme )
    {
        eproc->base.state = proc_leaving;
    }

    switch ( eproc->base.state )
    {
        case proc_begin:
            proc_result = do_ego_proc_begin( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_ego_proc_entering( eproc );

            eproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_ego_proc_running( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_ego_proc_leaving( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state  = proc_finish;
                eproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(eproc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_menu_proc_begin( menu_process_t * mproc )
{
    // play some music
    sound_play_song( MENU_SONG, 0, -1 );

    // initialize all these structures
    initMenus();        // start the menu menu

    // load all module info at menu initialization
    // this will not change unless a new module is downloaded for a network menu?
    modlist_load_all_info();

    // initialize the process state
    mproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_running( menu_process_t * mproc )
{
    int menuResult;

    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;

    mproc->was_active = mproc->base.valid;

    if ( mproc->base.paused ) return 0;

    // play the menu music
    mnu_draw_background = !process_instance_running( PROC_PBASE(GProc) );
    menuResult          = game_do_menu( mproc );

    switch ( menuResult )
    {
        case MENU_SELECT:
            // go ahead and start the game
            process_instance_pause( PROC_PBASE(mproc) );
            break;

        case MENU_QUIT:
            // the user selected "quit"
            process_instance_kill( PROC_PBASE(mproc) );
            break;
    }

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        GProc->menu_depth   = -1;
        GProc->escape_latch = bfalse;

        // We have exited the menu and restarted the game
        GProc->mod_paused = bfalse;
        process_instance_pause( PROC_PBASE(MProc) );
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_leaving( menu_process_t * mproc )
{
    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;

    // finish the menu song
    sound_finish_song( 500 );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_run( menu_process_t * mproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;
    mproc->base.dtime = frameDuration;

    if ( mproc->base.paused ) return 0;

    if ( mproc->base.killme )
    {
        mproc->base.state = proc_leaving;
    }

    switch ( mproc->base.state )
    {
        case proc_begin:
            proc_result = do_menu_proc_begin( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_menu_proc_entering( mproc );

            mproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_menu_proc_running( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_menu_proc_leaving( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state  = proc_finish;
                mproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(mproc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_game_proc_begin( game_process_t * gproc )
{
    BillboardList_init_all();

    gproc->escape_latch = bfalse;

    // initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info( "Initializing module linking... " );
    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    load_ai_codes( "basicdat" SLASH_STR "aicodes.txt" );
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    // initialize the IDSZ values for various slots
    init_slot_idsz();

    // do some graphics initialization
    make_lightdirectionlookup();
    make_enviro();
    camera_new( PCamera );

    // try to start a new module
    if ( !game_begin_module( pickedmodule_name, (Uint32)~0 ) )
    {
        // failure - kill the game process
        process_instance_kill( PROC_PBASE(gproc) );
        process_instance_resume( PROC_PBASE(MProc) );
    }

    // Initialize the process
    gproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_running( game_process_t * gproc )
{
    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;

    gproc->was_active  = gproc->base.valid;

    if ( gproc->base.paused ) return 0;

    // This is the control loop
    if ( PNet->on && console_done )
    {
        net_send_message();
    }

    // Do important things
    if ( !gproc->mod_paused || PNet->on )
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
        if ( !PNet->waitingforplayers )
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
    gproc->frame_now = SDL_GetTicks();
    if (gproc->frame_now > gproc->frame_next)
    {
        float  frameskip = (float)TICKS_PER_SEC / (float)cfg.framelimit;
        gproc->frame_next = gproc->frame_now + frameskip; // FPS limit

        camera_move(PCamera, PMesh);
        draw_main();

        msgtimechange++;
    }

    // Check for quitters
    // :TODO: local_noplayers is not set correctly
    // if( local_noplayers  )
    // {
    //  gproc->escape_requested  = btrue;
    // }

    if ( gproc->escape_requested )
    {
        gproc->escape_requested = bfalse;

        if ( !gproc->escape_latch )
        {
            if ( PMod->beat )
            {
                game_begin_menu( MProc, emnu_ShowEndgame );
            }
            else
            {
                game_begin_menu( MProc, emnu_GamePaused );
            }

            gproc->escape_latch = btrue;
            gproc->mod_paused   = btrue;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_leaving( game_process_t * gproc )
{
    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;

    // get rid of all module data
    game_quit_module();

    // resume the menu
    process_instance_resume( PROC_PBASE(MProc) );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_run( game_process_t * gproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;
    gproc->base.dtime = frameDuration;

    if ( gproc->base.paused ) return 0;

    if ( gproc->base.killme )
    {
        gproc->base.state = proc_leaving;
    }

    switch ( gproc->base.state )
    {
        case proc_begin:
            proc_result = do_game_proc_begin( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            // proc_result = do_game_proc_entering( gproc );

            gproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_game_proc_running( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_game_proc_leaving( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state  = proc_finish;
                gproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(gproc) );
            process_instance_resume   ( PROC_PBASE(MProc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    // ZZ> This is where the program starts and all the high level stuff happens

    int   max_rate, result = 0;
    float frameskip;

    // initialize the process
    ego_process_init( EProc, argc, argv );

    // turn on all basic services
    do_ego_proc_begin( EProc );

    // run the processes
    request_clear_screen();
    max_rate  = MAX(cfg.framelimit, UPDATE_SKIP) * 10;
    frameskip = (float)TICKS_PER_SEC / (float)max_rate;
    while ( !EProc->base.killme && !EProc->base.terminated )
    {
        // put a throttle on the ego process
        EProc->frame_now = SDL_GetTicks();
        if (EProc->frame_now < EProc->frame_next) continue;

        // update the timer
        EProc->frame_next = EProc->frame_now + frameskip;

        // clear the screen if needed
        do_clear_screen();

        do_ego_proc_running( EProc );

        // flip the graphics page if need be
        do_flip_pages();

        // let the OS breathe. It may delay as long as 10ms
        // SDL_Delay(1);
    }

    // terminate the game and menu processes
    process_instance_kill( PROC_PBASE(GProc) );
    process_instance_kill( PROC_PBASE(MProc) );
    while ( !EProc->base.terminated )
    {
        result = do_ego_proc_leaving( EProc );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp(void)
{
    // ZF> This function releases all loaded things in memory and cleans up everything properly

    log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

    // quit any existing game
    _quit_game( EProc );

    // synchronoze the config values with the various game subsystems
    setup_synch( &cfg );

    // quit the setup system, making sure that the setup file is written
    setup_upload( &cfg );
    setup_write();
    setup_quit();

    // delete all the graphics allocated by SDL and OpenGL
    delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save( "controls.txt" );

    // shut down the ui
    ui_shutdown();

    // shut down the network
    if (PNet->on)
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
int load_one_object( const char* tmploadname, int slot_override )
{
    // ZZ> This function loads one object and returns the object slot

    int object;
    STRING newloadname;

    // Load the object data file and get the object number
    object = load_one_character_profile( tmploadname, slot_override, !VALID_CAP_RANGE(slot_override) );

    if ( !VALID_CAP(object) ) return MAX_PROFILE; // no skins for an invalid object

    // Load the model for this object
    load_one_model_profile( tmploadname, object );

    // Load the enchantment for this object
    make_newloadname( tmploadname, SLASH_STR "enchant.txt", newloadname );
    load_one_enchant_profile( newloadname, object );

    // Fix lighting if need be
    if ( CapList[object].uniformlit )
    {
        mad_make_equally_lit( object );
    }

    return object;
}

//--------------------------------------------------------------------------------------------
Uint16 get_particle_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget )
{
    // ZF> This is the new improved targeting system for particles. Also includes distance in the Z direction.
    Uint16 besttarget = MAX_CHR, cnt;
    Uint16 longdist = WIDE;

    for (cnt = 0; cnt < MAX_CHR; cnt++)
    {
        if (ChrList.lst[cnt].on && ChrList.lst[cnt].alive && !ChrList.lst[cnt].isitem && ChrList.lst[cnt].attachedto == MAX_CHR
                && !ChrList.lst[cnt].invictus)
        {
            if ((PipStack.lst[particletype].onlydamagefriendly && team == ChrList.lst[cnt].team) || (!PipStack.lst[particletype].onlydamagefriendly && TeamList[team].hatesteam[ChrList.lst[cnt].team]) )
            {
                // Don't retarget someone we already had or not supposed to target
                if (cnt != oldtarget && cnt != donttarget)
                {
                    Uint16 angle = (ATAN2( ChrList.lst[cnt].pos.y - pos_y, ChrList.lst[cnt].pos.x - pos_x ) * 0xFFFF / ( TWO_PI ))
                                   + ATK_BEHIND - facing;

                    // Only proceed if we are facing the target
                    if (angle < PipStack.lst[particletype].targetangle || angle > ( 0xFFFF - PipStack.lst[particletype].targetangle ) )
                    {
                        Uint32 dist = ( Uint32 ) SQRT(ABS( pow(ChrList.lst[cnt].pos.x - pos_x, 2))
                                                      + ABS( pow(ChrList.lst[cnt].pos.y - pos_y, 2))
                                                      + ABS( pow(ChrList.lst[cnt].pos.z - pos_z, 2)) );
                        if (dist < longdist && dist <= WIDE )
                        {
                            glouseangle = angle;
                            besttarget = cnt;
                            longdist = dist;
                        }
                    }
                }
            }
        }
    }

    // All done
    return besttarget;
}

//--------------------------------------------------------------------------------------------
Uint16 check_target( Uint16 ichr_src, Uint16 ichr_test, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz )
{
    bool_t retval;

    bool_t is_hated, hates_me;
    bool_t is_friend, is_prey, is_predator, is_mutual;
    chr_t * psrc, * ptst;       

    // Skip non-existing objects
    if( INVALID_CHR(ichr_src) || INVALID_CHR(ichr_test) ) return bfalse;
    psrc = ChrList.lst + ichr_src;
    ptst = ChrList.lst + ichr_test;

    // Skip held objects and self
    if ( ichr_test == ichr_src || psrc->attachedto == ichr_test || VALID_CHR(ptst->attachedto) || ptst->pack_ispacked ) return bfalse;

    // Target items
    if ( !target_items && ( ptst->isitem || ptst->invictus ) ) return bfalse;

    // Either only target dead stuff or alive stuff
    if ( target_dead == ptst->alive ) return bfalse;

    // Dont target invisible stuff, unless we can actually see them
    if ( !psrc->canseeinvisible && FF_MUL( ptst->inst.alpha, ptst->inst.max_light ) < INVISIBLE ) return bfalse;

    is_hated = TeamList[psrc->team].hatesteam[ptst->team];
    hates_me = TeamList[ptst->team].hatesteam[psrc->team];

    is_friend    = !is_hated && !hates_me;
    is_prey      =  is_hated && !hates_me;
    is_predator  = !is_hated &&  hates_me;
    is_mutual    =  is_hated &&  hates_me;

    // Which target_type to target
    retval = bfalse;  
    if ( target_type == TARGET_ALL || (target_type == TARGET_ENEMY && is_hated) || (target_type == TARGET_FRIEND && !is_hated) )
    {
        bool_t match_idsz = (target_idsz == CapList[ptst->model].idsz[IDSZ_PARENT] ) || ( target_idsz == CapList[ptst->model].idsz[IDSZ_TYPE] );

        // Check for specific IDSZ too?
        if ( target_idsz == IDSZ_NONE || ( exclude_idsz != match_idsz ) )
        {
            retval = btrue;
        }
    }

    return retval;
}


//--------------------------------------------------------------------------------------------
Uint16 get_target( Uint16 ichr_src, Uint32 max_dist, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz  )
{
    // ZF> This is the new improved AI targeting system. Also includes distance in the Z direction.
    //    If max_dist is 0 then it searches without a max limit.

    int    ichr_test;
    int    irand;
    float  max_dist2 = max_dist * max_dist;
    line_of_sight_info_t los_info;

    Uint16    best_target;
    float     best_dist2;
    Uint32    current_ticks;
    chr_t   * psrc;

    if( TARGET_NONE == target_type ) return MAX_CHR;

    if ( INVALID_CHR(ichr_src) ) return MAX_CHR;
    psrc = ChrList.lst + ichr_src;  

    current_ticks = SDL_GetTicks();

    // do not run another search if it is too soon
    if ( psrc->ai.los_timer > current_ticks )
    {
        // Zefz> we can't return the old AI target here, it makes the scripts think it has found a target
        // BB>   I took the distance test out of here. The target should not lose it's target only because it is too far away

        Uint16 retval = MAX_CHR;

        if( VALID_CHR(psrc->ai.target) )
        {
            if( check_target( ichr_src, psrc->ai.target, target_type, target_items, target_dead, target_idsz, exclude_idsz) )
            {
                retval = psrc->ai.target;
            }
        }

        return retval;
    }

    // set the timer for next time
    irand = RANDIE;
    psrc->ai.los_timer = current_ticks + TICKS_PER_SEC * 0.5f * ( 1.0f + irand / (float)RAND_MAX );

    // set the line-of-sight source
    los_info.x0         = psrc->pos.x;
    los_info.y0         = psrc->pos.y;
    los_info.z0         = psrc->pos.z + psrc->bumpheight;
    los_info.stopped_by = psrc->stoppedby;

    best_target = MAX_CHR;
    best_dist2  = max_dist2;
    for ( ichr_test = 0; ichr_test < MAX_CHR; ichr_test++ )
    {
        float  dist2;
        GLvector3 diff;
        chr_t * ptst;

        if( INVALID_CHR(ichr_test) ) continue;
        ptst = ChrList.lst + ichr_test;    

        if( !check_target( ichr_src, ichr_test, target_type, target_items, target_dead, target_idsz, exclude_idsz) ) 
        {
            continue;
        }

        diff  = VSub( psrc->pos, ptst->pos );
        dist2 = VDotProduct( diff, diff );

        if ( (0 == max_dist2 || dist2 <= max_dist2) && (MAX_CHR == best_target || dist2 < best_dist2) )
        {
            // set the line-of-sight source
            los_info.x1 = ptst->pos.x;
            los_info.y1 = ptst->pos.y;
            los_info.z1 = ptst->pos.z + MAX(1, ptst->bumpheight);

            if ( !do_line_of_sight( &los_info ) )
            {
                best_target = ichr_test;
                best_dist2  = dist2;
            }
        }
    }

    // make sure the target is valid
    if ( INVALID_CHR(best_target) ) best_target = MAX_CHR;

    // set the ai target
    if ( MAX_CHR != best_target )
    {
        psrc->ai.target = best_target;
    }

    return best_target;
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
    // ZZ> This function figures out which fan characters are on and sets their level
    Uint16 character, distance, particle;
    int ripand;
    float level;

    // First figure out which fan each character is in
    for ( character = 0; character < MAX_CHR; character++ )
    {
        Uint8 hide;
        Uint16 icap;
        if ( !ChrList.lst[character].on ) continue;

        ChrList.lst[character].onwhichfan   = mesh_get_tile ( PMesh, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y );
        ChrList.lst[character].onwhichblock = mesh_get_block( PMesh, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y );

        // reject characters that are hidden
        icap = ChrList.lst[character].model;
        hide = CapList[ icap ].hidestate;
        ChrList.lst[character].is_hidden = bfalse;
        if ( hide != NOHIDE && hide == ChrList.lst[character].ai.state )
        {
            ChrList.lst[character].is_hidden = btrue;
        }
    }

    // Get levels every update
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList.lst[character].on || ChrList.lst[character].pack_ispacked ) continue;

        level = get_mesh_level( PMesh, ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].waterwalk ) + RAISE;

        if ( ChrList.lst[character].alive )
        {
            if ( VALID_TILE( PMesh, ChrList.lst[character].onwhichfan ) &&
                    ( 0 != mesh_test_fx( PMesh, ChrList.lst[character].onwhichfan, MPDFX_DAMAGE ) ) &&
                    ( ChrList.lst[character].pos.z <= ChrList.lst[character].floor_level + DAMAGERAISE ) &&
                    ( MAX_CHR == ChrList.lst[character].attachedto ) )
            {
                if ( ( ChrList.lst[character].damagemodifier[damagetile.type]&DAMAGESHIFT ) != 3 && !ChrList.lst[character].invictus ) // 3 means they're pretty well immune
                {
                    distance = ABS( PCamera->track_pos.x - ChrList.lst[character].pos.x ) + ABS( PCamera->track_pos.y - ChrList.lst[character].pos.y );
                    if ( distance < damagetile.min_distance )
                    {
                        damagetile.min_distance = distance;
                    }
                    if ( distance < damagetile.min_distance + 256 )
                    {
                        damagetile.sound_time = 0;
                    }
                    if ( ChrList.lst[character].damagetime == 0 )
                    {
                        damage_character( character, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_DAMAGE, ChrList.lst[character].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, bfalse );
                        ChrList.lst[character].damagetime = DAMAGETILETIME;
                    }
                    if ( (damagetile.parttype != ((Sint16)~0)) && ( update_wld & damagetile.partand ) == 0 )
                    {
                        spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].pos.z,
                                            0, MAX_PROFILE, damagetile.parttype, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( ChrList.lst[character].reaffirmdamagetype == damagetile.type )
                {
                    if ( ( update_wld&TILEREAFFIRMAND ) == 0 )
                        reaffirm_attached_particles( character );
                }
            }
        }

        if ( ChrList.lst[character].pos.z < water.surface_level && (0 != mesh_test_fx( PMesh, ChrList.lst[character].onwhichfan, MPDFX_WATER )) )
        {
            if ( !ChrList.lst[character].inwater )
            {
                // Splash
                if ( INVALID_CHR( ChrList.lst[character].attachedto ) )
                {
                    spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, water.surface_level + RAISE,
                                        0, MAX_PROFILE, SPLASH, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );
                }

                if ( water.is_water )
                {
                    ChrList.lst[character].ai.alert |= ALERTIF_INWATER;
                }
            }
            else
            {
                // Ripple
                if ( ChrList.lst[character].pos.z > water.surface_level - RIPPLETOLERANCE && CapList[ChrList.lst[character].model].ripple )
                {
                    // Ripples
                    ripand = ( ( int )ChrList.lst[character].vel.x != 0 ) | ( ( int )ChrList.lst[character].vel.y != 0 );
                    ripand = RIPPLEAND >> ripand;
                    if ( ( update_wld&ripand ) == 0 && ChrList.lst[character].pos.z < water.surface_level && ChrList.lst[character].alive )
                    {
                        spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, water.surface_level,
                                            0, MAX_PROFILE, RIPPLE, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( water.is_water && HAS_NO_BITS( frame_all, 7 ) )
                {
                    ChrList.lst[character].jumpready = btrue;
                    ChrList.lst[character].jumpnumber = 1; // ChrList.lst[character].jumpnumberreset;
                }
            }

            ChrList.lst[character].inwater  = btrue;
        }
        else
        {
            ChrList.lst[character].inwater = bfalse;
        }

        ChrList.lst[character].floor_level = level;
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        Uint16 ichr;
        if ( !PrtList.lst[particle].on ) continue;

        PrtList.lst[particle].onwhichfan   = mesh_get_tile ( PMesh, PrtList.lst[particle].pos.x, PrtList.lst[particle].pos.y );
        PrtList.lst[particle].onwhichblock = mesh_get_block( PMesh, PrtList.lst[particle].pos.x, PrtList.lst[particle].pos.y );
        PrtList.lst[particle].floor_level  = mesh_get_level( PMesh, PrtList.lst[character].pos.x, PrtList.lst[character].pos.y );

        // reject particles that are hidden
        PrtList.lst[particle].is_hidden = bfalse;
        ichr = PrtList.lst[particle].attachedtocharacter;
        if ( VALID_CHR( ichr ) )
        {
            PrtList.lst[particle].is_hidden = ChrList.lst[ichr].is_hidden;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //    to the major.
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
    //    to the major, but not by much.  Makes turning faster.
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

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        if ( !EncList.lst[cnt].on ) continue;

        eve = EncList.lst[cnt].eve;
        if ( EveStack.lst[eve].contspawnamount > 0 )
        {
            EncList.lst[cnt].spawntime--;
            if ( EncList.lst[cnt].spawntime == 0 )
            {
                character = EncList.lst[cnt].target;
                EncList.lst[cnt].spawntime = EveStack.lst[eve].contspawntime;
                facing = ChrList.lst[character].turn_z;
                for ( tnc = 0; tnc < EveStack.lst[eve].contspawnamount; tnc++ )
                {
                    spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].pos.z,
                                        facing, eve, EveStack.lst[eve].contspawnpip,
                                        MAX_CHR, GRIP_LAST, ChrList.lst[EncList.lst[cnt].owner].team, EncList.lst[cnt].owner, tnc, MAX_CHR );
                    facing += EveStack.lst[eve].contspawnfacingadd;
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
        if ( clock_pit > 19 )
        {
            clock_pit = 0;

            // Kill any particles that fell in a pit, if they die in water...
            for ( cnt = 0; cnt < maxparticles; cnt++ )
            {
                if ( INVALID_PRT( cnt ) || INVALID_PIP( PrtList.lst[cnt].pip ) ) continue;

                if ( PrtList.lst[cnt].pos.z < PITDEPTH && PipStack.lst[PrtList.lst[cnt].pip].endwater )
                {
                    PrtList.lst[cnt].time  = frame_all + 1;
                    PrtList.lst[cnt].poofme = btrue;
                }
            }

            // Kill or teleport any characters that fell in a pit...
            for ( cnt = 0; cnt < MAX_CHR; cnt++ )
            {
                // Is it a valid character?
                if ( INVALID_CHR( cnt ) || ChrList.lst[cnt].invictus || !ChrList.lst[cnt].alive  ) continue;
                if ( ChrList.lst[cnt].attachedto != MAX_CHR || ChrList.lst[cnt].pack_ispacked ) continue;

                // Do we kill it?
                if ( pitskill && ChrList.lst[cnt].pos.z < PITDEPTH )
                {
                    // Got one!
                    kill_character( cnt, MAX_CHR );
                    ChrList.lst[cnt].vel.x = 0;
                    ChrList.lst[cnt].vel.y = 0;

                    // Play sound effect
                    sound_play_chunk( ChrList.lst[cnt].pos, g_wavelist[GSND_PITFALL] );
                }

                // Do we teleport it?
                if ( pitsfall && ChrList.lst[cnt].pos.z < PITDEPTH << 3 )
                {
                    float nrm[2];

                    // Teleport them back to a "safe" spot
                    detach_character_from_mount( cnt, btrue, bfalse );
                    ChrList.lst[cnt].pos_old.x = ChrList.lst[cnt].pos.x;
                    ChrList.lst[cnt].pos_old.y = ChrList.lst[cnt].pos.y;
                    ChrList.lst[cnt].pos.x = pitx;
                    ChrList.lst[cnt].pos.y = pity;
                    ChrList.lst[cnt].pos.z = pitz;
                    if ( __chrhitawall( cnt, nrm ) )
                    {
                        // It did not work...
                        ChrList.lst[cnt].pos = ChrList.lst[cnt].pos_safe;
                        ChrList.lst[cnt].vel = ChrList.lst[cnt].vel_old;

                        // Kill it instead
                        kill_character( cnt, MAX_CHR );
                        ChrList.lst[cnt].vel.x = 0;
                        ChrList.lst[cnt].vel.y = 0;
                    }
                    else
                    {
                        // It worked!
                        ChrList.lst[cnt].pos_safe = ChrList.lst[cnt].pos;
                        ChrList.lst[cnt].pos_old  = ChrList.lst[cnt].pos;
                        ChrList.lst[cnt].vel_old  = ChrList.lst[cnt].vel;

                        // Stop movement
                        ChrList.lst[cnt].vel.z = 0;
                        ChrList.lst[cnt].vel.x = 0;
                        ChrList.lst[cnt].vel.y = 0;

                        // Play sound effect
                        if (ChrList.lst[cnt].isplayer)
                        {
                            sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_PITFALL] );
                        }
                        else
                        {
                            sound_play_chunk( ChrList.lst[cnt].pos, g_wavelist[GSND_PITFALL] );
                        }

                        // Do some damage (same as damage tile)
                        damage_character( cnt, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_DAMAGE, ChrList.lst[cnt].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, btrue );
                    }
                }
            }
        }
        else
        {
            clock_pit++;
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

    if ( weather.time > 0 )
    {
        weather.time--;
        if ( weather.time == 0 )
        {
            weather.time = weather.timer_reset;

            // Find a valid player
            foundone = bfalse;
            cnt = 0;

            while ( cnt < MAXPLAYER )
            {
                weather.iplayer = ( weather.iplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( PlaList[weather.iplayer].valid )
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
                cnt = PlaList[weather.iplayer].index;
                if ( ChrList.lst[cnt].on && !ChrList.lst[cnt].pack_ispacked )
                {
                    // Yes, so spawn over that character
                    x = ChrList.lst[cnt].pos.x;
                    y = ChrList.lst[cnt].pos.y;
                    z = ChrList.lst[cnt].pos.z;
                    particle = spawn_one_particle( x, y, z, 0, MAX_PROFILE, WEATHER4, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );

                    if (particle != TOTAL_MAX_PRT)
                    {
                        if (__prthitawall( particle ) ) PrtList_free_one( particle );
                        else if ( weather.over_water )
                        {
                            if ( !prt_is_over_water( particle ) )
                            {
                                PrtList_free_one( particle );
                            }
                        }
                    }

                }
            }
        }
    }

    PCamera->swing = ( PCamera->swing + PCamera->swingrate ) & 16383;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( Uint16 player )
{
    // ZZ> This function converts input readings to latch settings, so players can
    //    move around
    float newx, newy;
    Uint16 turnsin, character;
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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
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
                    inputx = mous.x * scale;
                    inputy = mous.y * scale;

                    turnsin = PCamera->turn_z >> 2;
                    turnsin = turnsin & 16383;
                    if ( PCamera->turn_mode == CAMTURN_GOOD &&
                            local_numlpla == 1 &&
                            control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                    newx = ( inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                    newy = (-inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
                    //                   PlaList[player].latchx+=newx;
                    //                   PlaList[player].latchy+=newy;
                }
            }

            PlaList[player].latchx += newx * mous.cover + mous.latcholdx * mous.sustain;
            PlaList[player].latchy += newy * mous.cover + mous.latcholdy * mous.sustain;
            mous.latcholdx = PlaList[player].latchx;
            mous.latcholdy = PlaList[player].latchy;

            // Sustain old movements to ease mouse play
            PlaList[player].latchx += mous.latcholdx * mous.sustain;
            PlaList[player].latchy += mous.latcholdy * mous.sustain;
            mous.latcholdx = PlaList[player].latchx;
            mous.latcholdy = PlaList[player].latchy;

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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = joy[0].x;
                inputy = joy[0].y;

                dist = inputx * inputx + inputy * inputy;
                if ( dist > 1.0f )
                {
                    scale = 1.0f / SQRT( dist );
                    inputx *= scale;
                    inputy *= scale;
                }

                turnsin = PCamera->turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( PCamera->turn_mode == CAMTURN_GOOD &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = joy[1].x;
                inputy = joy[1].y;

                dist = inputx * inputx + inputy * inputy;
                if ( dist > 1.0f )
                {
                    scale = 1.0f / SQRT( dist );
                    inputx *= scale;
                    inputy *= scale;
                }

                turnsin = PCamera->turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( PCamera->turn_mode == CAMTURN_GOOD &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
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
            if ( ChrList.lst[character].attachedto != MAX_CHR )
            {
                // Mounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) );
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) );
            }
            else
            {
                // Unmounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) );
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) );
            }

            turnsin = PCamera->turn_z >> 2;
            turnsin = turnsin & 16383;
            if ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 )  inputx = 0;

            newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
            newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
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

    chop.count = 0;
    chop.carat = 0;
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

    // Show map cheat
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) && mapvalid )
    {
        mapon = !mapon;
        youarehereon = btrue;
        stat_check_delay = 150;
    }

    // !!!BAD!!!  XP CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_x ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList.lst[PlaList[0].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList.lst[PlaList[1].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList.lst[PlaList[2].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList.lst[PlaList[3].index].experience += 25; stat_check_delay = 1; }

    }

    // !!!BAD!!!  LIFE CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_z ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList.lst[PlaList[0].index].life += 32; ChrList.lst[PlaList[0].index].life = MIN(ChrList.lst[PlaList[0].index].life, ChrList.lst[PlaList[0].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList.lst[PlaList[1].index].life += 32; ChrList.lst[PlaList[0].index].life = MIN(ChrList.lst[PlaList[1].index].life, ChrList.lst[PlaList[1].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList.lst[PlaList[2].index].life += 32; ChrList.lst[PlaList[0].index].life = MIN(ChrList.lst[PlaList[2].index].life, ChrList.lst[PlaList[2].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList.lst[PlaList[3].index].life += 32; ChrList.lst[PlaList[0].index].life = MIN(ChrList.lst[PlaList[3].index].life, ChrList.lst[PlaList[3].index].lifemax); stat_check_delay = 12; }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
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

    // Display character special powers?
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

}

//--------------------------------------------------------------------------------------------
void show_stat( Uint16 statindex )
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char gender[8];

    if ( statindex < numstat )
    {
        character = statlist[statindex];

        // Name
        debug_printf( "=%s=", ChrList.lst[character].name );

        // Level and gender and class
        gender[0] = 0;
        if ( ChrList.lst[character].alive )
        {
            int itmp;
            char * gender_str;

            gender_str = "";
            switch( ChrList.lst[character].gender )
            {
                case GENDER_MALE: gender_str = "male "; break;
                case GENDER_FEMALE: gender_str = "female "; break;
            }

            level = 1 + ChrList.lst[character].experiencelevel;
            itmp = level % 10;
            if ( 1 == itmp )
            {
                debug_printf( " %dst level %s%s", level, gender_str, CapList[ChrList.lst[character].model].classname );
            }
            else if ( 2 == itmp )
            {
                debug_printf( " %dnd level %s%s", level, gender_str, CapList[ChrList.lst[character].model].classname );
            }
            else if ( 3 == itmp )
            {
                debug_printf( " %drd level %s%s", level, gender_str, CapList[ChrList.lst[character].model].classname );
            }
            else
            {
                debug_printf( " %dth level %s%s", level, gender_str, CapList[ChrList.lst[character].model].classname );
            }
        }
        else
        {
            debug_printf( " Dead %s", CapList[ChrList.lst[character].model].classname );
        }

        // Stats
        debug_printf( " STR:~%2d~WIS:~%2d~DEF:~%d", FP8_TO_INT( ChrList.lst[character].strength ), FP8_TO_INT( ChrList.lst[character].wisdom ), 255 - ChrList.lst[character].defense );
        debug_printf( " INT:~%2d~DEX:~%2d~EXP:~%d", FP8_TO_INT( ChrList.lst[character].intelligence ), FP8_TO_INT( ChrList.lst[character].dexterity ), ChrList.lst[character].experience );
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character
    STRING tmps;

    if ( statindex < numstat )
    {
        Uint16 ichr = statlist[statindex];

        if ( VALID_CHR(ichr) )
        {
            Uint16 icap;
            Uint8  skinlevel;

            icap      = ChrList.lst[ichr].model;
            skinlevel = ChrList.lst[ichr].skin;

            // Armor Name
            debug_printf( "=%s=", CapList[icap].skinname[skinlevel] );

            // Armor Stats
            debug_printf( "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - CapList[icap].defense[skinlevel],
                     CapList[icap].damagemodifier[0][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[1][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[2][skinlevel]&DAMAGESHIFT );

            debug_printf( "~HOLY:~%i~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP:~%i",
                     CapList[icap].damagemodifier[3][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[4][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[5][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[6][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[7][skinlevel]&DAMAGESHIFT );

            debug_printf( "~Type: %s", ( CapList[icap].skindressy & (1 << skinlevel) ) ? "Light Armor" : "Heavy Armor" );

            // jumps
            tmps[0] = '\0';
            switch ( CapList[icap].jumpnumber )
            {
                case 0:  snprintf( tmps, SDL_arraysize( tmps), "None    (%i)", ChrList.lst[ichr].jumpnumberreset ); break;
                case 1:  snprintf( tmps, SDL_arraysize( tmps), "Novice  (%i)", ChrList.lst[ichr].jumpnumberreset ); break;
                case 2:  snprintf( tmps, SDL_arraysize( tmps), "Skilled (%i)", ChrList.lst[ichr].jumpnumberreset ); break;
                case 3:  snprintf( tmps, SDL_arraysize( tmps), "Adept   (%i)", ChrList.lst[ichr].jumpnumberreset ); break;
                default: snprintf( tmps, SDL_arraysize( tmps), "Master  (%i)", ChrList.lst[ichr].jumpnumberreset ); break;
            };

            debug_printf( "~Speed:~%3.0f~Jump Skill:~%s", ChrList.lst[ichr].maxaccel*80, tmps );
        }
    }

}

//--------------------------------------------------------------------------------------------
void show_full_status( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character including magic
    STRING text, tmps;
    Uint16 character, enchant;
    float manaregen, liferegen;
    if ( statindex < numstat )
    {
        character = statlist[statindex];

        // Enchanted?
        if ( ChrList.lst[character].firstenchant != MAX_ENC )
        {
            debug_printf( text, SDL_arraysize( text), "=%s is enchanted!=", ChrList.lst[character].name );
        }
        else
        {
            debug_printf( text, SDL_arraysize( text), "=%s is unenchanted=", ChrList.lst[character].name );
        }

        // Armor Stats
        debug_printf( " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                 255 - ChrList.lst[character].defense,
                 ChrList.lst[character].damagemodifier[0]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[1]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[2]&DAMAGESHIFT );

        debug_printf( " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                 ChrList.lst[character].damagemodifier[3]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[4]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[5]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[6]&DAMAGESHIFT,
                 ChrList.lst[character].damagemodifier[7]&DAMAGESHIFT );

        // Life and mana regeneration
        manaregen = ChrList.lst[character].manareturn / MANARETURNSHIFT;
        liferegen = ChrList.lst[character].lifereturn;
        for ( enchant = 0; enchant < MAX_ENC; enchant++ )                                   //Don't forget to add gains and costs from enchants
        {
            if ( !EncList.lst[enchant].on ) continue;

            if ( EncList.lst[enchant].target == character )
            {
                liferegen += EncList.lst[enchant].targetlife;
                manaregen += EncList.lst[enchant].targetmana;
            }
            if ( EncList.lst[enchant].owner == character )
            {
                liferegen += EncList.lst[enchant].ownerlife;
                manaregen += EncList.lst[enchant].ownermana;
            }

        }

        debug_printf( "Mana Regen:~%4.2f Life Regen:~%4.2f~~%s", manaregen / 256.0f, liferegen / 256.0f, tmps );
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    // ZF> Displays special enchantment effects for the character
    STRING text;
    Uint16 character;

    if ( statindex < numstat )
    {
        character = statlist[statindex];
        if( VALID_CHR(character) )
        {
            char * missile_str;
            chr_t * pchr = ChrList.lst + character;

            // Enchanted?
            if ( pchr->firstenchant != MAX_ENC )
            {
                debug_printf( "=%s is enchanted!=", pchr->name );
            }
            else
            {
                debug_printf( "=%s is unenchanted=", pchr->name );
            }

            // Enchantment status
            debug_printf( " See Invisible: %s~~See Kurses: %s",
                pchr->canseeinvisible ? "Yes" : "No",
                pchr->canseekurse ? "Yes" : "No" );

            debug_printf( " Channel Life: %s~~Waterwalking: %s",
                pchr->canchannel ? "Yes" : "No",
                pchr->waterwalk ? "Yes" : "No" );

            missile_str = "None";
            switch( pchr->missiletreatment )
            {
                case MISREFLECT: missile_str = "Reflect"; break;
                case MISDEFLECT: missile_str = "Deflect"; break;
            }

            debug_printf( text, SDL_arraysize( text), " Flying: %s~~Missile Protection: %s",
                (pchr->flyheight > 0) ? "Yes" : "No", missile_str );

        }
    }
}

//--------------------------------------------------------------------------------------------
void fill_bumplists()
{
    Uint16 character, particle;
    Uint32 fanblock;

    // Clear the lists
    for ( fanblock = 0; fanblock < PMesh->info.blocks_count; fanblock++ )
    {
        bumplist[fanblock].chr    = MAX_CHR;
        bumplist[fanblock].chrnum = 0;

        bumplist[fanblock].prt    = TOTAL_MAX_PRT;
        bumplist[fanblock].prtnum = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;

        if ( !ChrList.lst[character].on ) continue;
        pchr = ChrList.lst + character;

        // reset the holding weight each update
        pchr->holdingweight   = 0;
        pchr->onwhichplatform = MAX_CHR;
        pchr->phys.level      = pchr->floor_level;

        // reset the fan and block position
        pchr->onwhichfan   = mesh_get_tile ( PMesh, pchr->pos.x, pchr->pos.y );
        pchr->onwhichblock = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr->pack_ispacked ) continue;

        // reject characters that are hidden
        if ( pchr->is_hidden ) continue;

        if ( INVALID_BLOCK != pchr->onwhichblock )
        {
            // Insert before any other characters on the block
            pchr->fanblock_next = bumplist[pchr->onwhichblock].chr;
            bumplist[pchr->onwhichblock].chr = character;
            bumplist[pchr->onwhichblock].chrnum++;
        }
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;

        // reject invalid particles
        if ( !PrtList.lst[particle].on ) continue;
        pprt = PrtList.lst + particle;

        // reject characters that are hidden
        if ( pprt->is_hidden ) continue;

        // reset the fan and block position
        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

        if ( INVALID_BLOCK != pprt->onwhichblock )
        {
            // Insert before any other particles on the block
            pprt->fanblock_next = bumplist[pprt->onwhichblock].prt;
            bumplist[pprt->onwhichblock].prt = particle;
            bumplist[pprt->onwhichblock].prtnum++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void fill_collision_list( co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint16 ichr_a, ichr_b, iprt_b;

    Uint32 fanblock;
    int tnc, chrinblock, prtinblock;
    int cnt;

    if (NULL == chr_co_list)
    {
        chr_co_list = hash_list_create(-1);
        assert(NULL != chr_co_list);
    }

    // renew the collision list. Since we are filling this list with pre-allocated hash_node_t's,
    // there is no need to delete any of the existing chr_co_list->sublist elements
    for (cnt = 0; cnt < 256; cnt++)
    {
        chr_co_list->subcount[cnt] = 0;
        chr_co_list->sublist[cnt]  = NULL;
    }
    (*cdata_count) = 0;
    (*hn_count)    = 0;

    for ( ichr_a = 0; ichr_a < MAX_CHR; ichr_a++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        chr_t * pchr_a;

        // make sure that it is on
        if ( !ChrList.lst[ichr_a].on ) continue;
        pchr_a = ChrList.lst + ichr_a;

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr_a->pack_ispacked ) continue;

        // reject characters that are hidden
        if ( pchr_a->is_hidden ) continue;

        // determine the size of this object in blocks
        ixmin = pchr_a->pos.x - pchr_a->bumpsize; ixmin = CLIP(ixmin, 0, PMesh->info.edge_x);
        ixmax = pchr_a->pos.x + pchr_a->bumpsize; ixmax = CLIP(ixmax, 0, PMesh->info.edge_x);

        iymin = pchr_a->pos.y - pchr_a->bumpsize; iymin = CLIP(iymin, 0, PMesh->info.edge_y);
        iymax = pchr_a->pos.y + pchr_a->bumpsize; iymax = CLIP(iymax, 0, PMesh->info.edge_y);

        ixmin_block = ixmin >> BLOCK_BITS; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );
        ixmax_block = ixmax >> BLOCK_BITS; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );

        iymin_block = iymin >> BLOCK_BITS; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );
        iymax_block = iymax >> BLOCK_BITS; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );

        // handle all the interactions on this block
        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                // Allow raw access here because we were careful :)
                fanblock = mesh_get_block_int(PMesh, ix_block, iy_block);
                if ( INVALID_BLOCK != fanblock )
                {
                    chrinblock = bumplist[fanblock].chrnum;
                    prtinblock = bumplist[fanblock].prtnum;

                    // detect all the character-character interactions
                    for ( tnc = 0, ichr_b = bumplist[fanblock].chr;
                            tnc < chrinblock && ichr_b != MAX_CHR;
                            tnc++, ichr_b = ChrList.lst[ichr_b].fanblock_next)
                    {
                        if ( detect_chr_chr_collision(ichr_a, ichr_b) )
                        {
                            add_chr_chr_collision( ichr_a, ichr_b, cdata, cdata_count, hnlst, hn_count );
                        }
                    }

                    if ( pchr_a->alive )
                    {
                        // detect all the character-particle interactions
                        for ( tnc = 0, iprt_b = bumplist[fanblock].prt;
                                tnc < prtinblock;
                                tnc++, iprt_b = PrtList.lst[iprt_b].fanblock_next )
                        {
                            if ( detect_chr_prt_collision(ichr_a, iprt_b) )
                            {
                                add_chr_prt_collision( ichr_a, iprt_b, cdata, cdata_count, hnlst, hn_count );
                            }
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t can_mount( Uint16 ichr_a, Uint16 ichr_b )
{
    bool_t is_valid_rider_a, is_valid_mount_b;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    // make sure that A is valid
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    is_valid_rider_a = !pchr_a->isitem && pchr_a->alive && 0 == pchr_a->flyheight &&
                       INVALID_CHR(pchr_a->attachedto) && MadList[pchr_a->inst.imad].actionvalid[ACTION_MI];

    is_valid_mount_b = pchr_b->ismount && pchr_b->alive &&
                       pcap_b->slotvalid[SLOT_LEFT] && INVALID_CHR(pchr_b->holdingwhich[SLOT_LEFT]);

    return is_valid_rider_a && is_valid_mount_b;
}

//--------------------------------------------------------------------------------------------
bool_t do_platforms( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa, ya, za;
    float xb, yb, zb;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    bool_t platform_a, platform_b;
    bool_t mount_a, mount_b;
    float  dx, dy, dist;
    float  depth_z, lerp_z, radius, radius_xy;

    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t chara_on_top;

    // make sure that A is valid
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // if you are mounted, only your mount is affected by platforms
    if ( VALID_CHR(pchr_a->attachedto) || VALID_CHR(pchr_b->attachedto) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pcap_b->canuseplatforms && pchr_a->platform;
    platform_b = pcap_a->canuseplatforms && pchr_b->platform;
    if ( !platform_a && !platform_b ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // If we can mount this platform, skip it
    mount_a = can_mount(ichr_b, ichr_a);
    if ( mount_a && pchr_a->phys.level < zb + pchr_b->bumpheight + PLATTOLERANCE )
        return bfalse;

    //// If we can mount this platform, skip it
    mount_b = can_mount(ichr_a, ichr_b);
    if ( mount_b && pchr_b->phys.level < za + pchr_a->bumpheight + PLATTOLERANCE )
        return bfalse;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;
    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

    if ( depth_z > PLATTOLERANCE || depth_z < -PLATTOLERANCE ) return bfalse;

    // estimate the radius of interaction based on the z overlap
    lerp_z  = depth_z / PLATTOLERANCE;
    lerp_z  = CLIP( lerp_z, 0, 1 );

    radius    = MIN(pchr_a->bumpsize,     pchr_b->bumpsize  ); /* * (1.0f - lerp_z) + (pchr_a->bumpsize    + pchr_b->bumpsize   ) * lerp_z; */
    radius_xy = MIN(pchr_a->bumpsizebig, pchr_b->bumpsizebig); /* * (1.0f - lerp_z) + (pchr_a->bumpsizebig + pchr_b->bumpsizebig) * lerp_z; */

    // determine how the characters can be attached
    chara_on_top = btrue;
    depth_z = 2 * PLATTOLERANCE;
    if ( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = zb + pchr_b->bumpheight - za;
        depth_b = za + pchr_a->bumpheight - zb;

        depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

        chara_on_top = ABS(depth_z - depth_a) < ABS(depth_z - depth_b);

        // the collision is determined by the platform size
        if ( chara_on_top )
        {
            collide_x  = (dx <= pchr_b->bumpsize);
            collide_y  = (dy <= pchr_b->bumpsize);
            collide_xy = (dist <= pchr_b->bumpsizebig);
        }
        else
        {
            collide_x  = (dx <= pchr_a->bumpsize);
            collide_y  = (dy <= pchr_a->bumpsize);
            collide_xy = (dist <= pchr_a->bumpsizebig);
        }
    }
    else if ( platform_a )
    {
        chara_on_top = bfalse;
        depth_z = za + pchr_a->bumpheight - zb;

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_a->bumpsize);
        collide_y  = (dy <= pchr_a->bumpsize);
        collide_xy = (dist <= pchr_a->bumpsizebig);
    }
    else if ( platform_b )
    {
        chara_on_top = btrue;
        depth_z = zb + pchr_b->bumpheight - za;

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_b->bumpsize);
        collide_y  = (dy <= pchr_b->bumpsize);
        collide_xy = (dist <= pchr_b->bumpsizebig);
    }

    if ( collide_x && collide_y && collide_xy && depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE )
    {
        // check for the best possible attachment
        if ( chara_on_top )
        {
            if ( zb + pchr_b->bumpheight > pchr_a->phys.level )
            {
                pchr_a->phys.level = zb + pchr_b->bumpheight;
                pchr_a->onwhichplatform = ichr_b;
            }
        }
        else
        {
            if ( za + pchr_a->bumpheight > pchr_b->phys.level )
            {
                pchr_b->phys.level = za + pchr_a->bumpheight;
                pchr_b->onwhichplatform = ichr_a;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_mounts( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa, ya, za;
    float xb, yb, zb;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    bool_t mount_a, mount_b;
    float  dx, dy, dist;
    float  depth_z;

    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;

    bool_t mounted;

    // make sure that A is valid
    if ( INVALID_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // can either of these objects mount the other?
    mount_a = can_mount(ichr_b, ichr_a);
    mount_b = can_mount(ichr_a, ichr_b);

    if ( !mount_a && !mount_b ) return bfalse;

    mounted = bfalse;
    if ( !mounted && mount_b && (pchr_a->vel.z - pchr_b->vel.z) < 0 )
    {
        // A falling on B?
        GLvector4 point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int frame_nxt, frame_lst, lip, vertex;
            float flip;
            chr_instance_t * pinst = &(pchr_b->inst);

            frame_nxt = pinst->frame_nxt;
            frame_lst = pinst->frame_lst;
            lip = pinst->lip >> 6;
            flip = lip / 4.0f;

            vertex = MadList[pinst->imad].md2_data.vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( &(ChrList.lst[ichr_b].inst), vertex, vertex );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &(pinst->matrix), point, nupoint, 1 );
        }

        dx = ABS( xa - nupoint[0].x );
        dy = ABS( ya - nupoint[0].y );
        dist = dx + dy;
        depth_z = za - nupoint[0].z;

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_a->bumpsize * 2);
            collide_y  = (dy <= pchr_a->bumpsize * 2);
            collide_xy = (dist <= pchr_a->bumpsizebig * 2);

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
                mounted = VALID_CHR( pchr_a->attachedto );
            }
        }
    }

    if ( !mounted && mount_a && (pchr_b->vel.z - pchr_a->vel.z) < 0 )
    {
        // B falling on A?

        GLvector4 point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int frame_nxt, frame_lst, lip, vertex;
            float flip;
            chr_instance_t * pinst = &(pchr_a->inst);

            frame_nxt = pinst->frame_nxt;
            frame_lst = pinst->frame_lst;
            lip = pinst->lip >> 6;
            flip = lip / 256.0f;

            vertex = MadList[pinst->imad].md2_data.vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( &(ChrList.lst[ichr_a].inst), vertex, vertex );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &(pinst->matrix), point, nupoint, 1 );
        }

        dx = ABS( xb - nupoint[0].x );
        dy = ABS( yb - nupoint[0].y );
        dist = dx + dy;
        depth_z = zb - nupoint[0].z;

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_b->bumpsize * 2);
            collide_y  = (dy <= pchr_b->bumpsize * 2);
            collide_xy = (dist <= pchr_b->bumpsizebig * 2);

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY );
                mounted = VALID_CHR( pchr_a->attachedto );
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa, ya, za, xb, yb, zb;
    float was_xa, was_ya, was_za, was_xb, was_yb, was_zb;
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    float dx, dy, dist;
    float was_dx, was_dy, was_dist;
    float depth_z, was_depth_z;
    float lerp_z, radius, radius_xy;
    float wta, wtb;

    bool_t collide_x  = bfalse, was_collide_x;
    bool_t collide_y  = bfalse, was_collide_y;
    bool_t collide_xy = bfalse, was_collide_xy;
    bool_t collide_z  = bfalse, was_collide_z;
    bool_t collision  = bfalse;

    float interaction_strength = 1.0f;

    // make sure that it is on
    if ( !VALID_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that it is on
    if ( !VALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // don't do anything if there is no interaction strength
    interaction_strength = 1.0f;
    if ( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;
    interaction_strength *= pchr_b->inst.alpha * INV_FF;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    was_xa = xa - pchr_a->vel.x;
    was_ya = ya - pchr_a->vel.y;
    was_za = za - pchr_a->vel.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    was_xb = xb - pchr_b->vel.x;
    was_yb = yb - pchr_b->vel.y;
    was_zb = zb - pchr_b->vel.z;

    // platform interaction
    if ( ichr_a == pchr_b->onwhichplatform )
    {
        // we know that ichr_a is a platform and ichr_b is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_b->pos.z - pchr_b->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if ( lerp_z < 0 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.25f * (-lerp_z);
            pchr_b->jumpnumber = pchr_b->jumpnumberreset;
        }
        else if ( lerp_z < 1 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if ( lerp_z > 0 )
        {
            pchr_a->holdingweight += pchr_b->weight * lerp_z;

            pchr_b->phys.avel.z += ( pchr_a->vel.z  - pchr_b->vel.z      ) * lerp_z * 0.25f;
            pchr_b->turn_z     += ( rot_a - rot_b ) * platstick * lerp_z;
        };

        if ( lerp_z < 0.25f )
        {
            pchr_b->jumpready = btrue;
        };

        // this is handled
        return btrue;
    }

    // platform interaction
    if ( ichr_b == pchr_a->onwhichplatform )
    {
        // we know that ichr_b is a platform and ichr_a is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_a->pos.z - pchr_a->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if ( lerp_z < 0 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.25f * (-lerp_z);
            pchr_a->jumpnumber = pchr_a->jumpnumberreset;
        }
        else if ( lerp_z < 1 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if ( lerp_z > 0 )
        {
            pchr_b->holdingweight += pchr_a->weight * lerp_z;

            pchr_a->phys.avel.z += ( pchr_b->vel.z  - pchr_a->vel.z      ) * lerp_z * 0.25f;
            pchr_a->turn_z     += ( rot_b - rot_a ) * platstick * lerp_z;
        }

        if ( lerp_z < 0.25f )
        {
            pchr_a->jumpready = btrue;
        }

        // this is handled
        return btrue;
    }

    // items can interact with platforms but not with other characters/objects
    if ( pchr_a->isitem || pchr_b->isitem ) return bfalse;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;

    was_dx = ABS( was_xa - was_xb );
    was_dy = ABS( was_ya - was_yb );
    was_dist = was_dx + was_dy;

    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);
    was_depth_z = MIN( was_zb + pchr_b->bumpheight, was_za + pchr_a->bumpheight ) - MAX(was_za, was_zb);

    // estimate the radius of interaction based on the z overlap
    lerp_z  = depth_z / PLATTOLERANCE;
    lerp_z  = CLIP( lerp_z, 0, 1 );

    radius    = MIN(pchr_a->bumpsize, pchr_b->bumpsize) * (1.0f - lerp_z) + (pchr_a->bumpsize + pchr_b->bumpsize) * lerp_z;
    radius_xy = MIN(pchr_a->bumpsizebig, pchr_b->bumpsizebig) * (1.0f - lerp_z) + (pchr_a->bumpsizebig + pchr_b->bumpsizebig) * lerp_z;

    // estimate the collisions this frame
    collide_x  = (dx < radius);
    collide_y  = (dy < radius);
    collide_xy = (dist < radius_xy);
    collide_z  = (depth_z > 0);

    // estimate the collisions last frame
    was_collide_x  = (was_dx < radius);
    was_collide_y  = (was_dy < radius);
    was_collide_xy = (was_dist < radius_xy);
    was_collide_z  = (was_depth_z > 0);

    //------------------
    // do character-character interactions
    if ( !collide_x || !collide_y || !collide_xy || depth_z < -PLATTOLERANCE ) return bfalse;

    wta = (0xFFFFFFFF == pchr_a->weight) ? -(float)0xFFFFFFFF : pchr_a->weight;
    wtb = (0xFFFFFFFF == pchr_b->weight) ? -(float)0xFFFFFFFF : pchr_b->weight;

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

    if ( 0.0f == pchr_a->bumpdampen && 0.0f == pchr_b->bumpdampen )
    {
        /* do nothing */
    }
    else if ( 0.0f == pchr_a->bumpdampen )
    {
        // make the weight infinite
        wta = -0xFFFF;
    }
    else if ( 0.0f == pchr_b->bumpdampen )
    {
        // make the weight infinite
        wtb = -0xFFFF;
    }
    else
    {
        // adjust the weights to respect bumpdampen
        wta /= pchr_a->bumpdampen;
        wtb /= pchr_b->bumpdampen;
    }

    if ( !collision && collide_z )
    {
        float depth_x, depth_y, depth_xy, depth_yx, depth_z;
        GLvector3 nrm;
        int exponent = 1;

        if ( pcap_a->canuseplatforms && pchr_b->platform ) exponent += 2;
        if ( pcap_b->canuseplatforms && pchr_a->platform ) exponent += 2;

        nrm.x = nrm.y = nrm.z = 0.0f;

        depth_x  = MIN(xa + pchr_a->bumpsize, xb + pchr_b->bumpsize) - MAX(xa - pchr_a->bumpsize, xb - pchr_b->bumpsize);
        if ( depth_x <= 0.0f )
        {
            depth_x = 0.0f;
        }
        else
        {
            float sgn = xb - xa;
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_x / PLATTOLERANCE, exponent);
        }

        depth_y  = MIN(ya + pchr_a->bumpsize, yb + pchr_b->bumpsize) - MAX(ya - pchr_a->bumpsize, yb - pchr_b->bumpsize);
        if ( depth_y <= 0.0f )
        {
            depth_y = 0.0f;
        }
        else
        {
            float sgn = yb - ya;
            sgn = sgn > 0 ? -1 : 1;

            nrm.y += sgn / POW(depth_y / PLATTOLERANCE, exponent);
        }

        depth_xy = MIN(xa + ya + pchr_a->bumpsizebig, xb + yb + pchr_b->bumpsizebig) - MAX(xa + ya - pchr_a->bumpsizebig, xb + yb - pchr_b->bumpsizebig);
        if ( depth_xy <= 0.0f )
        {
            depth_xy = 0.0f;
        }
        else
        {
            float sgn = (xb + yb) - (xa + ya);
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_xy / PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_xy / PLATTOLERANCE, exponent);
        }

        depth_yx = MIN(-xa + ya + pchr_a->bumpsizebig, -xb + yb + pchr_b->bumpsizebig) - MAX(-xa + ya - pchr_a->bumpsizebig, -xb + yb - pchr_b->bumpsizebig);
        if ( depth_yx <= 0.0f )
        {
            depth_yx = 0.0f;
        }
        else
        {
            float sgn = (-xb + yb) - (-xa + ya);
            sgn = sgn > 0 ? -1 : 1;
            nrm.x -= sgn / POW(depth_yx / PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_yx / PLATTOLERANCE, exponent);
        }

        depth_z  = MIN(za + pchr_a->bumpheight, zb + pchr_b->bumpheight) - MAX( za, zb );
        if ( depth_z <= 0.0f )
        {
            depth_z = 0.0f;
        }
        else
        {
            float sgn = (zb + pchr_b->bumpheight / 2) - (za + pchr_a->bumpheight / 2);
            sgn = sgn > 0 ? -1 : 1;

            nrm.z += sgn / POW(exponent * depth_z / PLATTOLERANCE, exponent);
        }

        if ( ABS(nrm.x) + ABS(nrm.y) + ABS(nrm.z) > 0.0f )
        {
            GLvector3 vel_a, vel_b;
            GLvector3 vpara_a, vperp_a;
            GLvector3 vpara_b, vperp_b;
            GLvector3 imp_a, imp_b;
            float     vdot;

            nrm = VNormalize( nrm );

            vel_a.x = pchr_a->vel.x;
            vel_a.y = pchr_a->vel.y;
            vel_a.z = pchr_a->vel.z;

            vel_b.x = pchr_b->vel.x;
            vel_b.y = pchr_b->vel.y;
            vel_b.z = pchr_b->vel.z;

            vdot = VDotProduct( nrm, vel_a );
            vperp_a.x = nrm.x * vdot;
            vperp_a.y = nrm.y * vdot;
            vperp_a.z = nrm.z * vdot;
            vpara_a = VSub( vel_a, vperp_a );

            vdot = VDotProduct( nrm, vel_b );
            vperp_b.x = nrm.x * vdot;
            vperp_b.y = nrm.y * vdot;
            vperp_b.z = nrm.z * vdot;
            vpara_b = VSub( vel_b, vperp_b );

            // clear the "impulses"
            imp_a.x = imp_a.y = imp_a.z = 0.0f;
            imp_b.x = imp_b.y = imp_b.z = 0.0f;

            if ( collide_xy != was_collide_xy || collide_x != was_collide_x || collide_y != was_collide_y )
            {
                // an actual collision

                // generic coefficient of restitution
                float cr = 0.5f;

                if ( (wta < 0 && wtb < 0) || (wta == wtb) )
                {
                    float factor = 0.5f * (1.0f - cr);

                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if ( (wta < 0) || (wtb == 0) )
                {
                    float factor = (1.0f - cr);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if ( (wtb < 0) || (wta == 0) )
                {
                    float factor = (1.0f - cr);

                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);
                }
                else
                {
                    float factor;

                    factor = (1.0f - cr) * wtb / ( wta + wtb );
                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);

                    factor = (1.0f - cr) * wta / ( wta + wtb );
                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }

                // add in the collision impulses
                pchr_a->phys.avel.x += imp_a.x;
                pchr_a->phys.avel.y += imp_a.y;
                pchr_a->phys.avel.z += imp_a.z;
                LOG_NAN(pchr_a->phys.avel.z);

                pchr_b->phys.avel.x += imp_b.x;
                pchr_b->phys.avel.y += imp_b.y;
                pchr_b->phys.avel.z += imp_b.z;
                LOG_NAN(pchr_b->phys.avel.z);

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

                        imp_a.x = tmin * nrm.x * 0.25f * ratio;
                        imp_a.y = tmin * nrm.y * 0.25f * ratio;
                        imp_a.z = tmin * nrm.z * 0.25f * ratio;
                    }

                    if ( wtb >= 0.0f )
                    {
                        float ratio = (float)ABS(wta) / ((float)ABS(wta) + (float)ABS(wtb));

                        imp_b.x = -tmin * nrm.x * 0.25f * ratio;
                        imp_b.y = -tmin * nrm.y * 0.25f * ratio;
                        imp_b.z = -tmin * nrm.z * 0.25f * ratio;
                    }
                }

                // add in the collision impulses
                pchr_a->phys.apos_1.x += imp_a.x;
                pchr_a->phys.apos_1.y += imp_a.y;
                pchr_a->phys.apos_1.z += imp_a.z;

                pchr_b->phys.apos_1.x += imp_b.x;
                pchr_b->phys.apos_1.y += imp_b.y;
                pchr_b->phys.apos_1.z += imp_b.z;

                // you could "bump" something if you changed your velocity, even if you were still touching
                collision = (VDotProduct( pchr_a->vel, nrm ) * VDotProduct( pchr_a->vel_old, nrm ) < 0 ) ||
                            (VDotProduct( pchr_b->vel, nrm ) * VDotProduct( pchr_b->vel_old, nrm ) < 0 );

            }

            // add in the friction due to the "collision"
            // assume coeff of friction of 0.5
            if ( ABS(imp_a.x) + ABS(imp_a.y) + ABS(imp_a.z) > 0.0f &&
                    ABS(vpara_a.x) + ABS(vpara_a.y) + ABS(vpara_a.z) > 0.0f &&
                    pchr_a->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;

                imp = 0.5f * SQRT( imp_a.x * imp_a.x + imp_a.y * imp_a.y + imp_a.z * imp_a.z );
                vel = SQRT( vpara_a.x * vpara_a.x + vpara_a.y * vpara_a.y + vpara_a.z * vpara_a.z );

                factor = imp / vel;
                factor = CLIP(factor, 0.0f, 1.0f);

                pchr_a->phys.avel.x -= factor * vpara_a.x;
                pchr_a->phys.avel.y -= factor * vpara_a.y;
                pchr_a->phys.avel.z -= factor * vpara_a.z;
                LOG_NAN(pchr_a->phys.avel.z);
            }

            if ( ABS(imp_b.x) + ABS(imp_b.y) + ABS(imp_b.z) > 0.0f &&
                    ABS(vpara_b.x) + ABS(vpara_b.y) + ABS(vpara_b.z) > 0.0f &&
                    pchr_b->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;

                imp = 0.5f * SQRT( imp_b.x * imp_b.x + imp_b.y * imp_b.y + imp_b.z * imp_b.z );
                vel = SQRT( vpara_b.x * vpara_b.x + vpara_b.y * vpara_b.y + vpara_b.z * vpara_b.z );

                factor = imp / vel;
                factor = CLIP(factor, 0.0f, 1.0f);

                pchr_b->phys.avel.x -= factor * vpara_b.x;
                pchr_b->phys.avel.y -= factor * vpara_b.y;
                pchr_b->phys.avel.z -= factor * vpara_b.z;
                LOG_NAN(pchr_b->phys.avel.z);
            }
        }
    }

    if ( collision )
    {
        pchr_a->ai.bumplast = ichr_b;
        pchr_b->ai.bumplast = ichr_a;

        pchr_a->ai.alert |= ALERTIF_BUMPED;
        pchr_b->ai.alert |= ALERTIF_BUMPED;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    Uint16 ipip_b, direction;
    Uint32 prtidparent, prtidtype, eveidremove;
    int enchant, temp;
    float ax, ay, nx, ny, scale;  // For deflection
    Uint16 facing;

    float xa, ya, za, xb, yb, zb;
    float was_xa, was_ya, was_za, was_xb, was_yb, was_zb;
    chr_t * pchr_a;
    cap_t * pcap_a;

    prt_t * pprt_b;
    pip_t * ppip_b;

    float dx, dy, dist;
    bool_t collide_x, collide_y, collide_xy, collide_z;
    float depth_z;

    float interaction_strength = 1.0f;

    // make sure that it is on
    if ( !ChrList.lst[ichr_a].on ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( !pchr_a->alive ) return bfalse;

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    if ( !VALID_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    ipip_b = pprt_b->pip;
    if ( INVALID_PIP( ipip_b ) ) return bfalse;
    ppip_b = PipStack.lst + ipip_b;

    // do not collide with the thing that you're attached to
    if ( ichr_a == pprt_b->attachedtocharacter ) return bfalse;

    // if there's no friendly fire, particles issued by ichr_a can't hit it
    if ( !ppip_b->friendlyfire && ichr_a == pprt_b->chr ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    was_xa = xa - pchr_a->vel.x;
    was_ya = ya - pchr_a->vel.y;
    was_za = za - pchr_a->vel.z;

    xb = pprt_b->pos.x;
    yb = pprt_b->pos.y;
    zb = pprt_b->pos.z;

    was_xb = xb - pprt_b->vel.x;
    was_yb = yb - pprt_b->vel.y;
    was_zb = zb - pprt_b->vel.z;

    if ( 0 == pchr_a->bumpsize ) interaction_strength = 0;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;

    collide_x  = dx < pchr_a->bumpsize    || dx < pprt_b->bumpsize;
    collide_y  = dy < pchr_a->bumpsize    || dy < pprt_b->bumpsize;
    collide_xy = dist < pchr_a->bumpsizebig || dist < pprt_b->bumpsize * 1.41f;

    depth_z = MIN( za + pchr_a->bumpheight, zb + pprt_b->bumpheight ) - MAX( za, zb - pprt_b->bumpheight );
    collide_z = depth_z > 0;

    if ( !collide_x || !collide_y || !collide_xy || !collide_z ) return bfalse;

    if ( pchr_a->platform && INVALID_CHR(pprt_b->attachedtocharacter) &&
            (zb > za + pchr_a->bumpheight + pprt_b->vel.z) && (pprt_b->vel.z - pchr_a->vel.z < 0) )
    {
        // Particle is falling on A
        pprt_b->pos.z = za + pchr_a->bumpheight;
        pprt_b->vel.z = -pprt_b->vel.z * ppip_b->dampen;
        pprt_b->vel.x += ( pchr_a->vel.x - pchr_a->vel_old.x ) * platstick;
        pprt_b->vel.y += ( pchr_a->vel.y - pchr_a->vel_old.y  ) * platstick;
    }

    // Check reaffirmation of particles
    if ( pchr_a->reloadtime == 0 && pchr_a->damagetime == 0 )
    {
        if ( ichr_a != pprt_b->attachedtocharacter && pchr_a->reaffirmdamagetype == pprt_b->damagetype )
        {
            reaffirm_attached_particles( ichr_a );
        }
    }

    // Check for missile treatment
    if (  pchr_a->missiletreatment == MISNORMAL            ||
          pprt_b->damage.base+pprt_b->damage.rand == 0     ||
          pprt_b->attachedtocharacter != MAX_CHR           ||
        ( pprt_b->chr == ichr_a && !ppip_b->friendlyfire ) ||
        ( ChrList.lst[pchr_a->missilehandler].mana < ( pchr_a->missilecost << 8 ) && !ChrList.lst[pchr_a->missilehandler].canchannel ) )
    {
        if ( ( TeamList[pprt_b->team].hatesteam[pchr_a->team] || ( ppip_b->friendlyfire && ( ( ichr_a != pprt_b->chr && ichr_a != ChrList.lst[pprt_b->chr].attachedto ) || ppip_b->onlydamagefriendly ) ) ) && !pchr_a->invictus )
        {
            spawn_bump_particles( ichr_a, iprt_b ); // Catch on fire

            if ( ( pprt_b->damage.base | pprt_b->damage.rand ) > 1 )
            {
                prtidparent = CapList[pprt_b->model].idsz[IDSZ_PARENT];
                prtidtype = CapList[pprt_b->model].idsz[IDSZ_TYPE];
                if ( pchr_a->damagetime == 0 && pprt_b->attachedtocharacter != ichr_a && ( ppip_b->damfx&DAMFX_ARRO ) == 0 )
                {
                    // Normal iprt_b damage
                    if ( ppip_b->allowpush && pchr_a->weight != 0xFFFFFFFF )
                    {
                        if ( 0 == pchr_a->weight )
                        {
                            pchr_a->phys.avel.x  += pprt_b->vel.x - pchr_a->vel.x;
                            pchr_a->phys.avel.y  += pprt_b->vel.y - pchr_a->vel.y;
                            pchr_a->phys.avel.z  += pprt_b->vel.z - pchr_a->vel.z;
                            LOG_NAN(pchr_a->phys.avel.z);
                        }
                        else
                        {
                            float factor = MIN( 1.0f, 110 / pchr_a->weight );   // 110 is the "iconic" weight of the adventurer
                            factor = MIN( 1.0f, factor * pchr_a->bumpdampen );

                            pchr_a->phys.avel.x  += pprt_b->vel.x * factor;
                            pchr_a->phys.avel.y  += pprt_b->vel.y * factor;
                            pchr_a->phys.avel.z  += pprt_b->vel.z * factor;
                            LOG_NAN(pchr_a->phys.avel.z);
                        }
                    }

                    direction = ( ATAN2( pprt_b->vel.y, pprt_b->vel.x ) + PI ) * 0xFFFF / ( TWO_PI );
                    direction = pchr_a->turn_z - direction + 32768;

                    // Check all enchants to see if they are removed
                    enchant = pchr_a->firstenchant;
                    while ( enchant != MAX_ENC )
                    {
                        eveidremove = EveStack.lst[EncList.lst[enchant].eve].removedbyidsz;
                        temp = EncList.lst[enchant].nextenchant;
                        if ( eveidremove != IDSZ_NONE && ( eveidremove == prtidtype || eveidremove == prtidparent ) )
                        {
                            remove_enchant( enchant );
                        }

                        enchant = temp;
                    }

                    // Do confuse effects
                    if ( 0 == ( Md2FrameList[pchr_a->inst.frame_nxt].framefx&MADFX_INVICTUS ) || ppip_b->damfx&DAMFX_NBLOC )
                    {
                        if ( ppip_b->grogtime >= pchr_a->grogtime && pcap_a->canbegrogged )
                        {
                            pchr_a->grogtime = MAX(0, pchr_a->grogtime + ppip_b->grogtime);
                            pchr_a->ai.alert |= ALERTIF_GROGGED;
                        }
                        if ( ppip_b->dazetime >= pchr_a->dazetime && pcap_a->canbedazed )
                        {
                            pchr_a->dazetime = MAX(0, pchr_a->dazetime + ppip_b->dazetime);
                            pchr_a->ai.alert |= ALERTIF_DAZED;
                        }
                    }

                    // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                    // +2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                    if ( ppip_b->intdamagebonus )
                    {
                        float percent;
                        percent = ( (FP8_TO_INT(ChrList.lst[pprt_b->chr].intelligence)) - 14 ) * 2;
                        percent /= 100;
                        pprt_b->damage.base *= 1.00f + percent;
                    }

                    if ( ppip_b->wisdamagebonus )
                    {
                        float percent;
                        percent = ( FP8_TO_INT(ChrList.lst[pprt_b->chr].wisdom) - 14 ) * 2;
                        percent /= 100;
                        pprt_b->damage.base *= 1.00f + percent;
                    }

                    // Damage the character
                    if ( pcap_a->idsz[IDSZ_VULNERABILITY] != IDSZ_NONE && ( pcap_a->idsz[IDSZ_VULNERABILITY] == prtidtype || pcap_a->idsz[IDSZ_VULNERABILITY] == prtidparent ) )
                    {
                        IPair tmp_damage;

                        tmp_damage.base = (pprt_b->damage.base << 1);
                        tmp_damage.rand = (pprt_b->damage.rand << 1) | 1;

                        damage_character( ichr_a, direction, tmp_damage, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                        pchr_a->ai.alert |= ALERTIF_HITVULNERABLE;
                    }
                    else
                    {
                        damage_character( ichr_a, direction, pprt_b->damage, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                    }

                    // Notify the attacker of a scored hit
                    if ( pprt_b->chr != MAX_CHR )
                    {
                        ChrList.lst[pprt_b->chr].ai.alert |= ALERTIF_SCOREDAHIT;
                        ChrList.lst[pprt_b->chr].ai.hitlast = ichr_a;

                        //Tell the weapons who the attacker hit last
                        if (ChrList.lst[pprt_b->chr].holdingwhich[SLOT_LEFT] != MAX_CHR)
                        {
                            ChrList.lst[ChrList.lst[pprt_b->chr].holdingwhich[SLOT_LEFT]].ai.hitlast = ichr_a;
                        }
                        if (ChrList.lst[pprt_b->chr].holdingwhich[SLOT_LEFT] != MAX_CHR)
                        {
                            ChrList.lst[ChrList.lst[pprt_b->chr].holdingwhich[SLOT_LEFT]].ai.hitlast = ichr_a;
                        }
                    }
                }

                if (  HAS_NO_BITS( frame_all, 31 ) && pprt_b->attachedtocharacter == ichr_a )
                {
                    // Attached iprt_b damage ( Burning )
                    if ( ppip_b->xyvel_pair.base == 0 )
                    {
                        // Make character limp
                        pchr_a->phys.avel.x += -pchr_a->vel.x;
                        pchr_a->phys.avel.y += -pchr_a->vel.y;
                    }

                    damage_character( ichr_a, ATK_BEHIND, pprt_b->damage, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                }
            }

            if ( ppip_b->endbump )
            {
                if ( ppip_b->bumpmoney )
                {
                    if ( pchr_a->cangrabmoney && pchr_a->alive && 0 == pchr_a->damagetime && MAXMONEY != pchr_a->money )
                    {
                        if ( pchr_a->ismount )
                        {
                            // Let mounts collect money for their riders
                            if ( pchr_a->holdingwhich[SLOT_LEFT] != MAX_CHR )
                            {
                                ChrList.lst[pchr_a->holdingwhich[SLOT_LEFT]].money += ppip_b->bumpmoney;
                                if ( ChrList.lst[pchr_a->holdingwhich[SLOT_LEFT]].money > MAXMONEY ) ChrList.lst[pchr_a->holdingwhich[SLOT_LEFT]].money = MAXMONEY;
                                if ( ChrList.lst[pchr_a->holdingwhich[SLOT_LEFT]].money < 0 ) ChrList.lst[pchr_a->holdingwhich[SLOT_LEFT]].money = 0;

                                pprt_b->time = frame_all + 1;
                                pprt_b->poofme = btrue;
                            }
                        }
                        else
                        {
                            // Normal money collection
                            pchr_a->money += ppip_b->bumpmoney;
                            if ( pchr_a->money > MAXMONEY ) pchr_a->money = MAXMONEY;
                            if ( pchr_a->money < 0 ) pchr_a->money = 0;

                            pprt_b->time = frame_all + 1;
                            pprt_b->poofme = btrue;
                        }
                    }
                }
                else
                {
                    pprt_b->time  = frame_all + 1;
                    pprt_b->poofme = btrue;

                    // Only hit one character, not several
                    pprt_b->damage.base = 0;
                    pprt_b->damage.rand = 1;
                }
            }
        }
    }
    else if ( pprt_b->chr != ichr_a )
    {
        bool_t mana_paid = cost_mana( pchr_a->missilehandler, pchr_a->missilecost << 8, pprt_b->chr );

        if ( mana_paid )
        {
            // Treat the missile
            if ( pchr_a->missiletreatment == MISDEFLECT )
            {
                // Use old position to find normal
                ax = pprt_b->pos.x - pprt_b->vel.x;
                ay = pprt_b->pos.y - pprt_b->vel.y;
                ax = pchr_a->pos.x - ax;
                ay = pchr_a->pos.y - ay;

                // Find size of normal
                scale = ax * ax + ay * ay;
                if ( scale > 0 )
                {
                    // Make the normal a unit normal
                    scale = SQRT( scale );
                    nx = ax / scale;
                    ny = ay / scale;

                    // Deflect the incoming ray off the normal
                    scale = ( pprt_b->vel.x * nx + pprt_b->vel.y * ny ) * 2;
                    ax = scale * nx;
                    ay = scale * ny;
                    pprt_b->vel.x = pprt_b->vel.x - ax;
                    pprt_b->vel.y = pprt_b->vel.y - ay;
                }
            }
            else
            {
                // Reflect it back in the direction it came
                pprt_b->vel.x = -pprt_b->vel.x;
                pprt_b->vel.y = -pprt_b->vel.y;
            }

            // Change the owner of the missile
            pprt_b->team = pchr_a->team;
            pprt_b->chr = ichr_a;
            ppip_b->homing = bfalse;

            // Change the direction of the particle
            if ( ppip_b->rotatetoface )
            {
                // Turn to face new direction
                facing = ATAN2( pprt_b->vel.y, pprt_b->vel.x ) * 0xFFFF / ( TWO_PI );
                facing += 32768;
                pprt_b->facing = facing;
            }
        }
        else
        {
            // what happes if the misslehandler can't pay the mana?
            PrtList_free_one( iprt_b );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// collision data
static int       cdata_count = 0;
static co_data_t cdata[CHR_MAX_COLLISIONS];

// collision data hash nodes
static int         hn_count = 0;
static hash_node_t hnlst[COLLISION_HASH_NODES];

void bump_characters( void )
{
    // ZZ> This function sets handles characters hitting other characters or particles

    int tnc, cnt;

    co_data_t   * d;

    // fill up the bumplists
    fill_bumplists();

    // blank the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.lst[cnt].phys.apos_0.x = 0.0f;
        ChrList.lst[cnt].phys.apos_0.y = 0.0f;
        ChrList.lst[cnt].phys.apos_0.z = 0.0f;

        ChrList.lst[cnt].phys.apos_1.x = 0.0f;
        ChrList.lst[cnt].phys.apos_1.y = 0.0f;
        ChrList.lst[cnt].phys.apos_1.z = 0.0f;

        ChrList.lst[cnt].phys.avel.x = 0.0f;
        ChrList.lst[cnt].phys.avel.y = 0.0f;
        ChrList.lst[cnt].phys.avel.z = 0.0f;
    }

    // fill the collision list with all possible binary interactions
    fill_collision_list( cdata, &cdata_count, hnlst, &hn_count );

    // Do platforms
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if (TOTAL_MAX_PRT != d->prtb) continue;

            do_platforms( d->chra, d->chrb );
        }
    }

    // Do mounts
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if (TOTAL_MAX_PRT != d->prtb) continue;

            do_mounts( d->chra, d->chrb );
        }
    }

    // do all interactions
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);

            if ( TOTAL_MAX_PRT == d->prtb )
            {
                do_chr_chr_collision( d->chra, d->chrb );
            }
            else if ( MAX_CHR == d->chrb )
            {
                do_chr_prt_collision( d->chra, d->prtb );
            }
        }
    }

    // accumulate the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        float tmpx, tmpy, tmpz;
        chr_t * pchr;
        float bump_str;
        float nrm[2];

        if ( !ChrList.lst[cnt].on ) continue;
        pchr = ChrList.lst + cnt;

        bump_str = 1.0f;
        if ( VALID_CHR( pchr->attachedto ) )
        {
            bump_str = 0;
        }
        else if ( pchr->phys.dismount_timer > 0 )
        {
            bump_str = 1.0f - (float)pchr->phys.dismount_timer / PHYS_DISMOUNT_TIME;
            bump_str = bump_str * bump_str * 0.5f;
        };

        // decrement the dismount timer
        if ( pchr->phys.dismount_timer > 0 ) pchr->phys.dismount_timer--;

        // do the "integration" of the accumulated accelerations
        pchr->vel.x += pchr->phys.avel.x;
        pchr->vel.y += pchr->phys.avel.y;
        pchr->vel.z += pchr->phys.avel.z;

        // do the "integration" on the position
        if ( ABS(pchr->phys.apos_0.x + pchr->phys.apos_1.x) > 0 )
        {
            tmpx = pchr->pos.x;
            pchr->pos.x += pchr->phys.apos_0.x + pchr->phys.apos_1.x;
            if ( __chrhitawall(cnt, nrm) )
            {
                // restore the old values
                pchr->pos.x = tmpx;
            }
            else
            {
                pchr->vel.x += pchr->phys.apos_1.x * bump_str;
                pchr->pos_safe.x = tmpx;
            }
        }

        if ( ABS(pchr->phys.apos_0.y + pchr->phys.apos_1.y) > 0 )
        {
            tmpy = pchr->pos.y;
            pchr->pos.y += pchr->phys.apos_0.y + pchr->phys.apos_1.y;
            if ( __chrhitawall(cnt, nrm) )
            {
                // restore the old values
                pchr->pos.y = tmpy;
            }
            else
            {
                pchr->vel.y += pchr->phys.apos_1.y * bump_str;
                pchr->pos_safe.y = tmpy;
            }
        }

        if ( ABS(pchr->phys.apos_0.z + pchr->phys.apos_1.z) > 0 )
        {
            tmpz = pchr->pos.z;
            pchr->pos.z += pchr->phys.apos_0.z + pchr->phys.apos_1.z;
            if ( pchr->pos.z < pchr->floor_level )
            {
                // restore the old values
                pchr->pos.z = tmpz;
            }
            else
            {
                pchr->vel.z += pchr->phys.apos_1.z * bump_str;
                pchr->pos_safe.z = tmpz;
            }
        }

        pchr->safe_valid = ( 0 == __chrhitawall(cnt, nrm) );
    }
}

//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;

    // Do reload time
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList.lst[cnt].on ) continue;

        if ( ChrList.lst[cnt].reloadtime > 0 )
        {
            ChrList.lst[cnt].reloadtime--;
        }
    }

    // Do stats
    if ( clock_stat >= ONESECOND )
    {
        // Reset the clock
        clock_stat -= ONESECOND;

        // Do all the characters
        for ( cnt = 0; cnt < MAX_CHR; cnt++ )
        {
            if ( !ChrList.lst[cnt].on ) continue;

            // check for a level up
            do_level_up( cnt );

            // do the mana and life regen for "living" characters
            if ( ChrList.lst[cnt].alive )
            {
                ChrList.lst[cnt].mana += ( ChrList.lst[cnt].manareturn / MANARETURNSHIFT );
                ChrList.lst[cnt].mana = MAX(0, MIN(ChrList.lst[cnt].mana, ChrList.lst[cnt].manamax));

                ChrList.lst[cnt].life += ChrList.lst[cnt].lifereturn;
                ChrList.lst[cnt].life = MAX(1, MIN(ChrList.lst[cnt].life, ChrList.lst[cnt].lifemax));
            }

            // countdown cofuse effects
            if ( ChrList.lst[cnt].grogtime > 0 )
            {
                ChrList.lst[cnt].grogtime--;
            }

            if ( ChrList.lst[cnt].dazetime > 0 )
            {
                ChrList.lst[cnt].dazetime--;
            }
        }

        // Run through all the enchants as well
        for ( cnt = 0; cnt < MAX_ENC; cnt++ )
        {
            if ( !EncList.lst[cnt].on ) continue;

            if ( 0 == EncList.lst[cnt].time )
            {
                remove_enchant( cnt );
            }
            else
            {
                // Do enchant timer
                if ( EncList.lst[cnt].time > 0 )
                {
                    EncList.lst[cnt].time--;
                }

                // To make life easier
                owner  = EncList.lst[cnt].owner;
                target = EncList.lst[cnt].target;
                eve    = EncList.lst[cnt].eve;

                // Do drains
                if ( ChrList.lst[owner].alive )
                {
                    bool_t mana_paid;

                    // Change life
                    ChrList.lst[owner].life += EncList.lst[cnt].ownerlife;
                    if ( ChrList.lst[owner].life < 1 )
                    {
                        ChrList.lst[owner].life = 1;
                        kill_character( owner, target );
                    }

                    if ( ChrList.lst[owner].life > ChrList.lst[owner].lifemax )
                    {
                        ChrList.lst[owner].life = ChrList.lst[owner].lifemax;
                    }

                    // Change mana
                    mana_paid = cost_mana(owner, -EncList.lst[cnt].ownermana, target);
                    if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                    {
                        remove_enchant( cnt );
                    }
                }
                else if ( !EveStack.lst[eve].stayifnoowner )
                {
                    remove_enchant( cnt );
                }

                if ( EncList.lst[cnt].on )
                {
                    if ( ChrList.lst[target].alive )
                    {
                        bool_t mana_paid;

                        // Change life
                        ChrList.lst[target].life += EncList.lst[cnt].targetlife;
                        if ( ChrList.lst[target].life < 1 )
                        {
                            ChrList.lst[target].life = 1;
                            kill_character( target, owner );
                        }
                        if ( ChrList.lst[target].life > ChrList.lst[target].lifemax )
                        {
                            ChrList.lst[target].life = ChrList.lst[target].lifemax;
                        }

                        // Change mana
                        mana_paid = cost_mana( target, -EncList.lst[cnt].targetmana, owner );
                        if ( EveStack.lst[eve].endifcantpay && !mana_paid )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else if ( !EveStack.lst[eve].stayifdead )
                    {
                        remove_enchant( cnt );
                    }
                }
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

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList.lst[cnt].on ) continue;

        if ( ChrList.lst[cnt].stickybutt && VALID_TILE(PMesh, ChrList.lst[cnt].onwhichfan) )
        {
            twist = PMesh->mmem.tile_list[ChrList.lst[cnt].onwhichfan].twist;
            ChrList.lst[cnt].map_turn_y = map_twist_y[twist];
            ChrList.lst[cnt].map_turn_x = map_twist_x[twist];
        }
        else
        {
            ChrList.lst[cnt].map_turn_y = 32768;
            ChrList.lst[cnt].map_turn_x = 32768;
        }
    }

}

//--------------------------------------------------------------------------------------------
void load_all_objects_import_dir( const char * dirname )
{
    STRING newloadname;
    STRING filename;
    int cnt;

    if( NULL == PMod || INVALID_CSTR(dirname) ) return;

    if( !PMod->importvalid || 0 == PMod->importamount ) return;

    for ( cnt = 0; cnt < PMod->importamount*MAXIMPORTPERPLAYER; cnt++ )
    {
        // Make sure the object exists...
        snprintf( filename, SDL_arraysize( filename), "%s" SLASH_STR "temp%04d.obj", dirname, cnt );
        snprintf( newloadname, SDL_arraysize( newloadname), "%s" SLASH_STR "data.txt", filename );

        if ( vfs_exists(newloadname) )
        {
            // new player found
            if ( 0 == ( cnt % MAXIMPORTPERPLAYER ) ) import_data.player++;

            // store the slot info
            import_data.object = cnt;

            // load it
            import_data.slot_lst[cnt] = load_one_object( filename, MAX_PROFILE );
            import_data.max_slot      = cnt;
        }
    }

}

//--------------------------------------------------------------------------------------------
void load_all_objects_import()
{
    int cnt;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        import_data.slot_lst[cnt] = 10000;
    }
    import_data.max_slot = 0;

    // This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;
    import_data.player = -1;
    import_data.object = -100;

    load_all_objects_import_dir( "import" );
    load_all_objects_import_dir( "remote" );
}

//--------------------------------------------------------------------------------------------
void game_load_module_objects( const char *modname )
{
    // BB> Search for .obj directories int the module directory and load them

    const char *filehandle;
    STRING newloadname;

    import_data.object = -100;
    make_newloadname( modname, "objects", newloadname );

    filehandle = vfs_findFirst( newloadname, "obj", VFS_SEARCH_DIR );
    while ( filehandle != NULL )
    {
        load_one_object( filehandle, MAX_PROFILE );
        filehandle = vfs_findNext();
    }
    vfs_findClose();
}

//--------------------------------------------------------------------------------------------
void game_load_global_objects()
{
    // load all special objects
    load_one_object( "basicdat" SLASH_STR "book.obj", SPELLBOOK );

    // load the objects from various import directories
    load_all_objects_import();
}

//--------------------------------------------------------------------------------------------
void game_load_all_objects( const char *modname )
{
    // ZZ> This function loads a module's local objects and overrides the global ones already loaded

    // Log all of the script errors
    parseerror = bfalse;

    // clear out all old objrct definitions
    free_all_objects();

    // load the global objects
    game_load_global_objects();

    // load the objects from the module's directory
    game_load_module_objects( modname );

    log_madused( "slotused.txt" );
}

//--------------------------------------------------------------------------------------------
chr_setup_info_t * chr_setup_info_init( chr_setup_info_t *pinfo )
{
    // BB> safe values for all parameters

    if( NULL == pinfo ) return pinfo;

    memset( pinfo, 0, sizeof(chr_setup_info_t) );

    pinfo->attach = ATTACH_NONE;
    pinfo->team   = TEAM_NULL;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
chr_setup_info_t * chr_setup_info_reinit( chr_setup_info_t *pinfo )
{
    Uint16 old_parent;

    if( NULL == pinfo ) return pinfo;

    // save the parent data just in case
    old_parent = pinfo->parent;

    // init the data
    chr_setup_info_init( pinfo );

    // restore the parent data
    pinfo->parent = old_parent;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_read( vfs_FILE * fileread, chr_setup_info_t *pinfo )
{
    char cTmp, delim;
    bool_t retval;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return bfalse;

    chr_setup_info_reinit( pinfo );

    // check for another entry, either the "#" or ":" delimiters
    delim = goto_delimiter_list( pinfo->spawn_coment, fileread, "#:", btrue );
    if ( '\0' == delim ) return bfalse;

    retval = bfalse;
    if( ':' == delim )
    {
        retval = btrue;

        pinfo->do_spawn = btrue;

        fget_string( fileread, pinfo->spawn_name, SDL_arraysize(pinfo->spawn_name) );
        str_decode( pinfo->spawn_name, SDL_arraysize(pinfo->spawn_name), pinfo->spawn_name );

        pinfo->pname = pinfo->spawn_name;
        if ( 0 == strcmp( pinfo->spawn_name, "NONE") )
        {
            // Random pinfo->pname
            pinfo->pname = NULL;
        }

        pinfo->slot = fget_int( fileread );

        pinfo->pos.x = fget_float( fileread ) * TILE_SIZE;
        pinfo->pos.y = fget_float( fileread ) * TILE_SIZE;
        pinfo->pos.z = fget_float( fileread ) * TILE_SIZE;

        pinfo->facing = FACE_NORTH;
        pinfo->attach = ATTACH_NONE;
        cTmp = fget_first_letter( fileread );
        if ( 'S' == toupper(cTmp) )       pinfo->facing = FACE_SOUTH;
        else if ( 'E' == toupper(cTmp) )  pinfo->facing = FACE_EAST;
        else if ( 'W' == toupper(cTmp) )  pinfo->facing = FACE_WEST;
        else if ( '?' == toupper(cTmp) )  pinfo->facing = FACE_RANDOM;
        else if ( 'L' == toupper(cTmp) )  pinfo->attach = ATTACH_LEFT;
        else if ( 'R' == toupper(cTmp) )  pinfo->attach = ATTACH_RIGHT;
        else if ( 'I' == toupper(cTmp) )  pinfo->attach = ATTACH_INVENTORY;

        pinfo->money   = fget_int( fileread );
        pinfo->skin    = fget_int( fileread );
        pinfo->passage = fget_int( fileread );
        pinfo->content = fget_int( fileread );
        pinfo->level   = fget_int( fileread );

        if (pinfo->skin >= MAXSKIN)
        {
            int irand = RANDIE;
            pinfo->skin = irand % MAXSKIN;     // Randomize skin?
        }

        pinfo->stat = fget_bool( fileread );

        fget_first_letter( fileread );   // BAD! Unused ghost value

        cTmp = fget_first_letter( fileread );
        pinfo->team = ( cTmp - 'A' ) % TEAM_MAX;
    }
    else if( '#' == delim )
    {
        STRING szTmp1, szTmp2;
        int    iTmp, fields;

        pinfo->do_spawn = bfalse;

        fields = vfs_scanf( fileread, "%255s%255s%d", szTmp1, szTmp2, &iTmp );
        if( 3 == fields && 0 == strcmp(szTmp1, "dependency") )
        {
            retval = btrue;

            // seed the info with the data
            strncpy( pinfo->spawn_coment, szTmp2, SDL_arraysize(pinfo->spawn_coment) );
            pinfo->slot = iTmp;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo )
{
    chr_t * pchr;

    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    if ( INVALID_CHR(ichr) ) return bfalse;
    pchr = ChrList.lst + ichr;

    pchr->money += pinfo->money;
    if ( pchr->money > MAXMONEY )  pchr->money = MAXMONEY;
    if ( pchr->money < 0 )  pchr->money = 0;

    pchr->ai.content = pinfo->content;
    pchr->ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        inventory_add_item( ichr, pinfo->parent );

        pchr->ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        pchr->attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        pchr->attachedto = MAX_CHR;  // Fix grab

    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        grip_offset_t grip_off = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, pinfo->parent, grip_off );

        // Handle the "grabbed" messages
        let_character_think( ichr );
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        while ( pchr->experiencelevel < pinfo->level && pchr->experience < MAXXP )
        {
            give_experience( ichr, 25, XP_DIRECT, btrue );
            do_level_up( ichr );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
#if defined(__GNUC__)
//int strlwr( char * str )
//{
//    if( NULL == str ) return -1;
//
//    while( '\0' != *str )
//    {
//        *str = tolower(*str);
//        str++;
//    }
//
//    return 0;
//}
#endif

//--------------------------------------------------------------------------------------------
bool_t setup_characters_load_object( chr_setup_info_t * pinfo )
{
    // BB> Try to load a global object named pinfo->spawn_coment into slot pinfo->slot

    STRING filename;

    if( NULL == pinfo || VALID_CAP(pinfo->slot) ) return bfalse;

    // trim any excess spaces off the pinfo->spawn_coment
    str_trim(pinfo->spawn_coment);

    if( NULL == strstr(pinfo->spawn_coment, ".obj" ) )
    {
        strcat( pinfo->spawn_coment, ".obj" );
    }

    strlwr( pinfo->spawn_coment );

    // do the loading
    if( '\0' != pinfo->spawn_coment[0] )
    {
        snprintf( filename, SDL_arraysize(filename), "basicdat" SLASH_STR "globalobjects" SLASH_STR "%s", pinfo->spawn_coment );

        pinfo->slot = load_one_object( filename, pinfo->slot );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_characters_spawn( chr_setup_info_t * pinfo )
{
    int tnc;
    int new_object, local_index = 0;

    if( NULL == pinfo || !pinfo->do_spawn ) return bfalse;

    // Spawn the character
    new_object = spawn_one_character( pinfo->pos, pinfo->slot, pinfo->team, pinfo->skin, pinfo->facing, pinfo->pname, MAX_CHR );
    if ( INVALID_CHR(new_object) )
        return bfalse;

    // determine the attachment
    if ( pinfo->attach == ATTACH_NONE )
    {
        // Free character
        pinfo->parent = new_object;
        make_one_character_matrix( new_object );
    }

    chr_setup_apply( new_object, pinfo );

    // Turn on numpla input devices
    if ( pinfo->stat )
    {
        if ( 0 == PMod->importamount && numpla < PMod->playeramount )
        {
            if ( 0 == local_numlpla )
            {
                // the first player gets everything
                add_player( new_object, numpla, (Uint32)(~0) );
            }
            else
            {
                Uint32 bits;

                // each new player steals an input device from the 1st player
                bits = 1 << local_numlpla;
                for ( tnc = 0; tnc < MAXPLAYER; tnc++ )
                {
                    PlaList[tnc].device &= ~bits;
                }

                add_player( new_object, numpla, bits );
            }
        }
        else if ( numpla < numimport && numpla < PMod->importamount && numpla < PMod->playeramount )
        {
            // Multiplayer import module
            local_index = -1;
            for ( tnc = 0; tnc < numimport; tnc++ )
            {
                if ( ChrList.lst[new_object].model < import_data.max_slot && ChrList.lst[new_object].model < MAX_PROFILE )
                {
                    if( import_data.slot_lst[ChrList.lst[new_object].model] == local_slot[tnc] )
                    {
                        local_index = tnc;
                        break;
                    }
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

    return btrue;
}

//--------------------------------------------------------------------------------------------
void setup_characters( const char *modname )
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"

    chr_setup_info_t info;
    STRING newloadname;
    vfs_FILE  *fileread;

    // Turn some back on
    make_newloadname( modname, "gamedat" SLASH_STR "spawn.txt", newloadname );
    fileread = vfs_openRead( newloadname );

    numpla = 0;
    info.parent = MAX_CHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s\n", newloadname );
    }
    else
    {
        info.parent = 0;

        while ( chr_setup_read( fileread, &info ) )
        {
            int save_slot = info.slot;

            // check to see if the slot is valid
            if( -1 == info.slot || info.slot >= MAX_PROFILE )
            {
                log_warning( "Invalid slot %d for \"%s\" in file \"%s\"\n", info.slot, info.spawn_coment, newloadname );
                continue;
            }

            // check to see if something is in that slot
            if( INVALID_CAP(info.slot) )
            {
                setup_characters_load_object( &info );
            }

            if( INVALID_CAP(info.slot) )
            {
                if ( save_slot > PMod->importamount * MAXIMPORTPERPLAYER )
                {
                    log_warning( "The object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", info.spawn_coment, save_slot, newloadname );
                }
                continue;
            }

            // do the spawning if need be
            setup_characters_spawn( &info );
        }

        vfs_close( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    statlist_sort();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void load_all_global_objects()
{
    // ZF> This function loads all global objects found in the basicdat folder
    const char *filehandle;

    // Warn the user for any duplicate slots
    overrideslots = bfalse;

    // Search for .obj directories and load them
    filehandle = vfs_findFirst( "basicdat" SLASH_STR "globalobjects", "obj", VFS_SEARCH_DIR );
    while ( VALID_CSTR(filehandle) )
    {
        load_one_object( filehandle, MAX_PROFILE );
        filehandle = vfs_findNext();
    }

    vfs_findClose();
}

//--------------------------------------------------------------------------------------------
void game_reset_module_data()
{
    // reset all
    log_info( "Resetting module data\n" );

    // unload a lot of data
    reset_teams();
    release_all_profiles();
    free_all_objects();
    reset_messages();
    prime_names();
    reset_players();
    reset_end_text();
    reset_renderlist();
}

//--------------------------------------------------------------------------------------------
void game_load_global_assets()
{
    // load a bunch of assets that are used in the module

    // Load all the global icons
    if ( !load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
    }
    load_blip_bitmap();
    load_bars();
    font_load( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );
}

//--------------------------------------------------------------------------------------------
void game_load_module_assets( const char *modname )
{
    // load a bunch of assets that are used in the module
    load_global_waves( modname );
    reset_particles( modname );
    read_wawalite( modname, NULL );
    load_basic_textures( modname );
    load_map( modname );

    upload_wawalite();
}

//--------------------------------------------------------------------------------------------
void game_load_all_assets( const char *modname )
{
    game_load_global_assets();

    game_load_module_assets( modname );
}

//--------------------------------------------------------------------------------------------
void game_setup_module( const char *smallname )
{
    // ZZ> This runst the setup functions for a module

    STRING modname;

    // generate the module directory
    strncpy(modname, smallname, SDL_arraysize(modname));
    str_append_slash(modname, SDL_arraysize(modname));

    setup_particles();
    setup_passage( modname );
    setup_characters( modname );
    setup_alliances( modname );
}

//--------------------------------------------------------------------------------------------
bool_t game_load_module_data( const char *smallname )
{
    // ZZ> This function loads a module
    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    load_ai_script( "basicdat" SLASH_STR "script.txt" );

    // generate the module directory
    strncpy(modname, smallname, SDL_arraysize(modname));
    str_append_slash(modname, SDL_arraysize(modname));

    // load all module assets
    game_load_all_assets( modname );

    // load all module objects
    game_load_all_objects(modname);

    if ( NULL == mesh_load( modname, PMesh ) )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "Uh oh! Problems loading the mesh! (%s)\n", modname );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has no attached particles
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( PrtList.lst[particle].on && PrtList.lst[particle].attachedtocharacter == character )
        {
            free_one_particle_in_game( particle );
        }
    }

    // Set the alert for disaffirmation ( wet torch )
    ChrList.lst[character].ai.alert |= ALERTIF_DISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
    // ZZ> This function returns the number of particles attached to the given character
    Uint16 cnt = 0;
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( PrtList.lst[particle].on && PrtList.lst[particle].attachedtocharacter == character )
        {
            cnt++;
        }
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

    while ( numberattached < CapList[ChrList.lst[character].model].attachedprtamount )
    {
        particle = spawn_one_particle( ChrList.lst[character].pos.x, ChrList.lst[character].pos.y, ChrList.lst[character].pos.z, 0, ChrList.lst[character].model, CapList[ChrList.lst[character].model].attachedprttype, character, GRIP_LAST + numberattached, ChrList.lst[character].team, character, numberattached, MAX_CHR );
        if ( particle != TOTAL_MAX_PRT )
        {
            attach_particle_to_character( particle, character, PrtList.lst[particle].vrt_off );
        }

        numberattached++;
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    ChrList.lst[character].ai.alert |= ALERTIF_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    // BB > all of the de-initialization code after the module actually ends

    // stop the module
    module_stop( PMod );

    // get rid of the game/module data
    game_release_module_data();

    // turn off networking
    close_session();

    // reset the "ui" mouse state
    cursor_reset();

    // re-initialize all game/module data
    game_reset_module_data();

    // finish whatever in-game song is playing
    sound_finish_sound();
}

//-----------------------------------------------------------------
bool_t game_begin_module( const char * modname, Uint32 seed )
{
    // BB> all of the initialization code before the module actually starts

    if ( ~0 == seed ) seed = time(NULL);

    // make sure the old game has been quit
    game_quit_module();

    // load all the in-game module data
    srand( seed );
    if ( !game_load_module_data( modname ) )
    {
        module_stop( PMod );
        return bfalse;
    };

    game_setup_module( modname );

    timeron = bfalse;
    reset_timers();

    // make sure the per-module configuration settings are correct
    setup_synch( &cfg );

    // initialize the game objects
    make_onwhichfan();
    cursor_reset();
    module_reset( PMod, seed );
    camera_reset( PCamera, PMesh );
    make_character_matrices( update_wld != 0 );
    attach_particles();

    // initialize the network
    net_initialize();
    net_sayHello();

    // start the module
    module_start( PMod );

    return btrue;
}

//-----------------------------------------------------------------
bool_t game_update_imports()
{
    // BB> This function saves all the players to the players dir
    //    and also copies them into the imports dir to prepare for the next module

    bool_t is_local;
    int cnt, tnc, j, character, player;
    STRING srcPlayer, srcDir, destDir;

    // do the normal export to save all the player settings
    export_all_players( btrue );

    // reload all of the available players
    check_player_import( "players", btrue  );
    check_player_import( "remote",  bfalse );

    // build the import directory using the player info
    vfs_empty_import_directory();
    vfs_mkdir( "import" );

    // export all of the players directly from memory straight to the "import" dir
    for ( player = 0, cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList.lst[character].on ) continue;

        is_local = ( 0 != PlaList[cnt].device );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].dir, str_encode_path(ChrList.lst[character].name) ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            log_warning( "game_update_imports() - cannot find exported file for \"%s\" (\"%s\") \n", ChrList.lst[character].name, str_encode_path(ChrList.lst[character].name) ) ;
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
            snprintf( srcPlayer, SDL_arraysize( srcPlayer), "players" SLASH_STR "%s", loadplayer[tnc].dir );
        }
        else
        {
            snprintf( srcPlayer, SDL_arraysize( srcPlayer), "remote" SLASH_STR "%s", loadplayer[tnc].dir );
        }

        snprintf( destDir, SDL_arraysize( destDir), "import" SLASH_STR "temp%04d.obj", local_slot[tnc] );
        vfs_copyDirectory( srcPlayer, destDir );

        // Copy all of the character's items to the import directory
        for ( j = 0; j < MAXIMPORTOBJECTS; j++ )
        {
            snprintf( srcDir, SDL_arraysize( srcDir), "%s" SLASH_STR "%d.obj", srcPlayer, j );
            snprintf( destDir, SDL_arraysize( destDir), "import" SLASH_STR "temp%04d.obj", local_slot[tnc] + j + 1 );

            vfs_copyDirectory( srcDir, destDir );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_release_module_data()
{
    // ZZ> This function frees up memory used by the module

    // Disable EMP
    local_senseenemiesID = IDSZ_NONE;
    local_senseenemiesTeam = TEAM_MAX;

    release_all_graphics();
    release_all_profiles();
    release_all_ai_scripts();

    mesh_delete( PMesh );
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
    // ZZ> This function attaches particles to their characters so everything gets
    //    drawn right
    int cnt;

    cnt = 0;

    while ( cnt < maxparticles )
    {
        if ( PrtList.lst[cnt].on && PrtList.lst[cnt].attachedtocharacter != MAX_CHR )
        {
            attach_particle_to_character( cnt, PrtList.lst[cnt].attachedtocharacter, PrtList.lst[cnt].vrt_off );

            // Correct facing so swords knock characters in the right direction...
            if ( PipStack.lst[PrtList.lst[cnt].pip].damfx&DAMFX_TURN )
            {
                PrtList.lst[cnt].facing = ChrList.lst[PrtList.lst[cnt].attachedtocharacter].turn_z;
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
        ChrList.lst[character].isplayer = btrue;
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
            ChrList.lst[character].islocalplayer = btrue;
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
    static Uint32 last_update = (Uint32)(~0);

    // make sure there is only one update per frame;
    if ( update_wld == last_update ) return;
    last_update = update_wld;

    numblip = 0;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        bool_t is_crushed, is_cleanedup, can_think;

        if ( !ChrList.lst[character].on ) continue;

        // check for actions that must always be handled
        is_cleanedup = HAS_SOME_BITS( ChrList.lst[character].ai.alert, ALERTIF_CLEANEDUP );
        is_crushed   = HAS_SOME_BITS( ChrList.lst[character].ai.alert, ALERTIF_CRUSHED   );

        // let the script run sometimes even if the item is in your backpack
        can_think = !ChrList.lst[character].pack_ispacked || CapList[ChrList.lst[character].model].isequipment;

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if ( ( ChrList.lst[character].alive && can_think ) || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts( character );

            // Crushed characters shouldn't be alert to anything else
            if ( is_crushed )  { ChrList.lst[character].ai.alert = ALERTIF_CRUSHED; ChrList.lst[character].ai.timer = update_wld + 1; }

            // Cleaned up characters shouldn't be alert to anything else
            if ( is_cleanedup )  { ChrList.lst[character].ai.alert = ALERTIF_CLEANEDUP; ChrList.lst[character].ai.timer = update_wld + 1; }

            let_character_think( character );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which )
{
    if ( NULL == mproc ) return bfalse;

    if ( !process_instance_running( PROC_PBASE(mproc) ) )
    {
        GProc->menu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        process_instance_start( PROC_PBASE(mproc) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_end_menu( menu_process_t * mproc )
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        process_instance_resume( PROC_PBASE(MProc) );
        GProc->menu_depth = -1;
    }
}

//--------------------------------------------------------------------------------------------
void game_finish_module()
{
    // export all the local and remote characters
    game_update_imports();

    // quit the old module
    game_quit_module();
}

//--------------------------------------------------------------------------------------------
void free_all_objects( void )
{
    // BB > free every instance of the three object types used in the game.

    PrtList_free_all();
    EncList_free_all();
    free_all_chraracters();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t add_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint32 hashval = 0;
    int count;
    bool_t found;

    hash_node_t * n;
    co_data_t   * d;

    // create a hash that is order-independent
    hashval = (ichr_a * 0x0111 + 0x006E) + (ichr_b * 0x0111 + 0x006E);
    hashval &= 0xFF;

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if ( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for (i = 0; i < count; i++)
        {
            d = (co_data_t *)(n->data);
            if (d->chra == ichr_a && d->chrb == ichr_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if (!found)
    {
        // pick a free collision data
        assert((*cdata_count) < CHR_MAX_COLLISIONS);
        d = cdata + (*cdata_count);
        (*cdata_count)++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = ichr_b;
        d->prtb = TOTAL_MAX_PRT;

        // generate a new hash node
        assert((*hn_count) < COLLISION_HASH_NODES);
        n = hnlst + (*hn_count);
        (*hn_count)++;
        hash_node_ctor(n, (void*)d);

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before(chr_co_list->sublist[hashval], n);
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t add_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    bool_t found;
    int    count;
    Uint32 hashval = 0;

    hash_node_t * n;
    co_data_t   * d;

    // create a hash that is order-independent
    hashval = (ichr_a * 0x0111 + 0x006E) + (iprt_b * 0x0111 + 0x006E);
    hashval &= 0xFF;

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if ( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for (i = 0; i < count; i++)
        {
            d = (co_data_t *)(n->data);
            if (d->chra == ichr_a && d->prtb == iprt_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if (!found)
    {
        // pick a free collision data
        assert((*cdata_count) < CHR_MAX_COLLISIONS);
        d = cdata + (*cdata_count);
        (*cdata_count)++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = MAX_CHR;
        d->prtb = iprt_b;

        // generate a new hash node
        assert((*hn_count) < COLLISION_HASH_NODES);
        n = hnlst + (*hn_count);
        (*hn_count)++;
        hash_node_ctor(n, (void*)d);

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before(chr_co_list->sublist[hashval], n);
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b )
{
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_z  = bfalse;

    float xa, ya, za;
    float xb, yb, zb;
    float dxy, dx, dy, depth_z;

    chr_t *pchr_a, *pchr_b;
    cap_t *pcap_a, *pcap_b;

    // Don't collide with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( INVALID_CAP(pchr_a->model) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    if ( INVALID_CAP(pchr_b->model) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // don't collide if there is no interaction
    if ( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // First check absolute value diamond
    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dxy = dx + dy;

    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

    // estimate the collisions this frame
    collide_x  = (dx  <= pchr_a->bumpsize    + pchr_b->bumpsize   );
    collide_y  = (dy  <= pchr_a->bumpsize    + pchr_b->bumpsize   );
    collide_xy = (dxy <= pchr_a->bumpsizebig + pchr_b->bumpsizebig);

    if ( (pchr_a->platform && pcap_b->canuseplatforms) ||
            (pchr_b->platform && pcap_a->canuseplatforms) )
    {
        collide_z  = (depth_z > -PLATTOLERANCE);
    }
    else
    {
        collide_z  = (depth_z > 0);
    }

    if ( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_z  = bfalse;

    float xa, ya, za;
    float xb, yb, zb;
    float dxy, dx, dy, depth_z;

    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( INVALID_PRT(iprt_b) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->is_hidden ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pprt_b->pos.x;
    yb = pprt_b->pos.y;
    zb = pprt_b->pos.z;

    // don't collide if there is no interaction
    if ( 0 == pchr_a->bumpsize || 0 == pprt_b->bumpsize ) return bfalse;

    // First check absolute value diamond
    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dxy = dx + dy;

    depth_z = MIN( za + pchr_a->bumpheight, zb + pprt_b->bumpheight ) - MAX(za, zb - pprt_b->bumpheight);

    // estimate the collisions this frame
    collide_x  = (dx  <= pchr_a->bumpsize    + pprt_b->bumpsize   );
    collide_y  = (dy  <= pchr_a->bumpsize    + pprt_b->bumpsize   );
    collide_xy = (dxy <= pchr_a->bumpsizebig + pprt_b->bumpsizebig);
    collide_z  = (depth_z > 0);

    if ( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t * set_PMesh( ego_mpd_t * pmpd )
{
    ego_mpd_t * pmpd_old = PMesh;

    PMesh = pmpd;

    return pmpd_old;
}

//--------------------------------------------------------------------------------------------
camera_t * set_PCamera( camera_t * pcam )
{
    camera_t * pcam_old = PCamera;

    PCamera = pcam;

    // Matrix init stuff (from remove.c)
    rotmeshtopside    = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHDOWN / ( 1.33333f );

    return pcam_old;
}

//---------------------------------------------------------------------------------------------
float get_mesh_level( ego_mpd_t * pmesh, float x, float y, bool_t waterwalk )
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //    If waterwalk is nonzero and the fan is watery, then the level returned is the
    //    level of the water.

    float zdone;

    zdone = mesh_get_level( pmesh, x, y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = mesh_get_tile( pmesh, x, y );

        if (  0 != mesh_test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t make_water( water_instance_t * pinst, wawalite_water_t * pdata )
{
    // ZZ> This function sets up water movements
    int layer, frame, point, cnt;
    float temp;
    Uint8 spek;

    if ( NULL == pinst || NULL == pdata ) return bfalse;

    for ( layer = 0; layer < pdata->layer_count; layer++ )
    {
        pinst->layer[layer].tx.x = 0;
        pinst->layer[layer].tx.y = 0;

        for ( frame = 0; frame < MAXWATERFRAME; frame++ )
        {
            // Do first mode
            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI / 2 * layer / MAXWATERLAYER ) );
                pinst->layer_z_add[layer][frame][point] = temp * pdata->layer[layer].amp;
            }
        }
    }

    // Calculate specular highlights
    spek = 0;
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;
        if ( cnt > pdata->spek_start )
        {
            temp = cnt - pdata->spek_start;
            temp = temp / ( 256 - pdata->spek_start );
            temp = temp * temp;
            spek = temp * pdata->spek_level;
        }

        // [claforte] Probably need to replace this with a
        //           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( gfx.shading == GL_FLAT )
            pinst->spek[cnt] = 0;
        else
            pinst->spek[cnt] = spek;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    // ZZ> This function resets the end-module text
    endtextwrite = snprintf( endtext, SDL_arraysize( endtext), "The game has ended..." );

    /*
    if ( numpla > 1 )
    {
        endtextwrite = snprintf( endtext, SDL_arraysize( endtext), "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( numpla == 0 )
        {
            // No players???
            endtextwrite = snprintf( endtext, SDL_arraysize( endtext), "The game has ended..." );
        }
        else
        {
            // One player
            endtextwrite = snprintf( endtext, SDL_arraysize( endtext), "Sadly, no trace was ever found..." );
        }
    }
    */

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
void expand_escape_codes( Uint16 ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end )
{
    int    cnt;
    STRING szTmp;

    chr_t      * pchr, *ptarget, *powner;
    ai_state_t * pai;

    pchr    = INVALID_CHR(ichr) ? NULL : ChrList.lst + ichr;
    pai     = (NULL == pchr)    ? NULL : &(pchr->ai);

    ptarget = ((NULL == pai) || INVALID_CHR(pai->target)) ? pchr : ChrList.lst + pai->target;
    powner  = ((NULL == pai) || INVALID_CHR(pai->owner )) ? pchr : ChrList.lst + pai->owner;

    cnt = 0;
    while ( '\0' != *src && src < src_end && dst < dst_end )
    {
        if ( '%' == *src )
        {
            char * ebuffer, * ebuffer_end;

            // go to the escape character
            src++;

            // set up the buffer to hold the escape data
            ebuffer     = szTmp;
            ebuffer_end = szTmp + SDL_arraysize(szTmp) - 1;

            // make the excape buffer an empty string
            *ebuffer = '\0';

            switch ( *src )
            {
                case '%' : // the % symbol
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp), "%%" );
                    }
                    break;

                case 'n' : // Name
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp), "%s", chr_get_name( ichr ) );
                    }
                    break;

                case 'c':  // Class name
                    {
                        if ( NULL != pchr )
                        {
                            ebuffer     = CapList[pchr->model].classname;
                            ebuffer_end = ebuffer + SDL_arraysize(CapList[pchr->model].classname);
                        }
                    }
                    break;

                case 't':  // Target name
                    {
                        if ( NULL != pai )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%s", chr_get_name( pai->target ) );
                        }
                    }
                    break;

                case 'o':  // Owner name
                    {
                        if ( NULL != pai )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%s", chr_get_name( pai->owner ) );
                        }
                    }
                    break;

                case 's':  // Target class name
                    {
                        if ( NULL != ptarget)
                        {
                            ebuffer     = CapList[ptarget->model].classname;
                            ebuffer_end = ebuffer + SDL_arraysize(CapList[ptarget->model].classname);
                        }
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3': // Target's skin name
                    {
                        if ( NULL != ptarget)
                        {
                            ebuffer = CapList[ptarget->model].skinname[(*src)-'0'];
                            ebuffer_end = ebuffer + SDL_arraysize(CapList[ptarget->model].skinname[(*src)-'0']);
                        }
                    }
                    break;

                case 'a':  // Character's ammo
                    {
                        if ( NULL != pchr)
                        {
                            if ( pchr->ammoknown )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "%d", pchr->ammo );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "?" );
                            }
                        }
                    }
                    break;

                case 'k':  // Kurse state
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->iskursed )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "kursed" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "unkursed" );
                            }
                        }
                    }
                    break;

                case 'p':  // Character's possessive
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "her" );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "its" );
                            }
                        }
                    }
                    break;

                case 'm':  // Character's gender
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "female " );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "male " );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), " " );
                            }
                        }
                    }
                    break;

                case 'g':  // Target's possessive
                    {
                        if ( NULL != ptarget )
                        {
                            if ( ptarget->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "her" );
                            }
                            else if ( ptarget->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp), "its" );
                            }
                        }
                    }
                    break;

                case '#':  // New line (enter)
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp), "\n" );
                    }
                    break;

                case 'd':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%d", pstate->distance );
                        }
                    }
                    break;

                case 'x':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%d", pstate->x );
                        }
                    }
                    break;

                case 'y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%d", pstate->y );
                        }
                    }
                    break;

                case 'D':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%2d", pstate->distance );
                        }
                    }
                    break;

                case 'X':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%2d", pstate->x );
                        }
                    }
                    break;

                case 'Y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp), "%2d", pstate->y );
                        }
                    }
                    break;

                default:
                    snprintf( szTmp, SDL_arraysize( szTmp), "%%%c???", (*src) );
                    break;
            }

            if ( '\0' == *ebuffer )
            {
                ebuffer     = szTmp;
                ebuffer_end = szTmp + SDL_arraysize(szTmp);
                snprintf( szTmp, SDL_arraysize( szTmp), "%%%c???", (*src) );
            }

            // make the line capitalized if necessary
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = toupper( *ebuffer );

            // Copy the generated text
            while ( '\0' != *ebuffer && ebuffer < ebuffer_end && dst < dst_end )
            {
                *dst++ = *ebuffer++;
            }
            *dst = '\0';
        }
        else
        {
            // Copy the letter
            *dst = *src;
            dst++;
        }

        src++;
        cnt++;
    }

    // make sure the destination string is terminated
    if ( dst < dst_end )
    {
        *dst = '\0';
    }
    *dst_end = '\0';
}

//--------------------------------------------------------------------------------------------
void append_end_text( script_state_t * pstate, int message, Uint16 character )
{
    // ZZ> This function appends a message to the end-module text
    int read;

    endtext[0] = '\0';

    if ( message < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        // Copy the message
        read = MessageOffset.lst[message];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = endtext;
        dst_end = endtext + MAXENDTEXT - 1;

        expand_escape_codes( character, pstate, src, src_end, dst, dst_end );

        *dst_end = '\0';
    }

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
bool_t game_choose_module( int imod, int seed )
{
    if ( seed < 0 ) seed = time(NULL);

    if ( NULL == PMod ) PMod = &gmod;

    return module_upload( PMod, imod, seed );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
process_instance_t * process_instance_init( process_instance_t * proc )
{
    if ( NULL == proc ) return proc;

    memset( proc, 0, sizeof(process_instance_t) );

    proc->terminated = btrue;

    return proc;
}

bool_t process_instance_start( process_instance_t * proc )
{
    if ( NULL == proc ) return bfalse;

    // choose the correct proc->state
    if ( proc->terminated || proc->state > proc_leaving )
    {
        // must re-initialize the process
        proc->state = proc_begin;
    }
    if ( proc->state > proc_entering )
    {
        // the process is already initialized, just put it back in
        // proc_entering mode
        proc->state = proc_entering;
    }

    // tell it to run
    proc->terminated = bfalse;
    proc->valid      = btrue;
    proc->paused     = bfalse;

    return btrue;
}

bool_t process_instance_kill( process_instance_t * proc )
{
    if ( NULL == proc ) return bfalse;
    if ( !process_instance_validate(proc) ) return btrue;

    // turn the process back on with an order to commit suicide
    proc->paused = bfalse;
    proc->killme = btrue;

    return btrue;
}

bool_t process_instance_validate( process_instance_t * proc )
{
    if (NULL == proc) return bfalse;

    if ( !proc->valid || proc->terminated )
    {
        process_instance_terminate( proc );
    }

    return proc->valid;
}

bool_t process_instance_terminate( process_instance_t * proc )
{
    if (NULL == proc) return bfalse;

    proc->valid      = bfalse;
    proc->terminated = btrue;
    proc->state      = proc_begin;

    return btrue;
}

bool_t process_instance_pause( process_instance_t * proc )
{
    bool_t old_value;

    if ( !process_instance_validate(proc) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = btrue;

    return old_value != proc->paused;
}

bool_t process_instance_resume( process_instance_t * proc )
{
    bool_t old_value;

    if ( !process_instance_validate(proc) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = bfalse;

    return old_value != proc->paused;
}

bool_t process_instance_running( process_instance_t * proc )
{
    if ( !process_instance_validate(proc) ) return bfalse;

    return !proc->paused;
}

//--------------------------------------------------------------------------------------------
ego_process_t * ego_process_init( ego_process_t * eproc, int argc, char **argv )
{
    if ( NULL == eproc ) return NULL;

    memset( eproc, 0, sizeof(ego_process_t) );

    process_instance_init( PROC_PBASE(eproc) );

    eproc->argv0 = (argc > 0) ? argv[0] : NULL;

    return eproc;
}

//--------------------------------------------------------------------------------------------
menu_process_t * menu_process_init( menu_process_t * mproc )
{
    if ( NULL == mproc ) return NULL;

    memset( mproc, 0, sizeof(menu_process_t) );

    process_instance_init( PROC_PBASE(mproc) );

    return mproc;
}

//--------------------------------------------------------------------------------------------
game_process_t * game_process_init( game_process_t * gproc )
{
    if ( NULL == gproc ) return NULL;

    memset( gproc, 0, sizeof(game_process_t) );

    process_instance_init( PROC_PBASE(gproc) );

    gproc->menu_depth = -1;
    gproc->pause_key_ready = btrue;

    return gproc;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_all_pip()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipStack.lst + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_eve()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        memset( EveStack.lst + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_cap()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( CapList + cnt, 0, sizeof(cap_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_mad()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( MadList + cnt, 0, sizeof(mad_t) );

        strncpy( MadList[cnt].name, "*NONE*", SDL_arraysize(MadList[cnt].name) );
        MadList[cnt].ai = 0;
    }

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void init_all_profiles()
{
    // ZZ> This function initializes all of the model profiles

    init_all_pip();
    init_all_eve();
    init_all_cap();
    init_all_mad();
    init_all_ai_scripts();
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void release_all_pip()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        release_one_pip( cnt );
    }
}

//---------------------------------------------------------------------------------------------
bool_t release_one_eve( Uint16 ieve )
{
    eve_t * peve;

    if( !VALID_EVE_RANGE( ieve) ) return bfalse;
    peve = EveStack.lst + ieve;

    if(!peve->loaded) return btrue;

    memset( peve, 0, sizeof(pip_t) );

    return btrue;
}

//---------------------------------------------------------------------------------------------
void release_all_eve()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_EVE; cnt++ )
    {
        release_one_eve( cnt );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_cap()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_cap( cnt );
    };
}

//---------------------------------------------------------------------------------------------
void release_all_mad()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_mad( cnt );
    }

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_profiles()
{
    // ZZ> This function clears out all of the model data

    release_all_pip();
    release_all_eve();
    release_all_cap();
    release_all_mad();
    release_all_ai_scripts();
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
fanblock = mesh_get_block_int(PMesh, x,y);

enemies = bfalse;
if ( !onlyfriends ) enemies = btrue;

charb = bumplist[fanblock].chr;
cnt = 0;
while ( cnt < bumplist[fanblock].chrnum )
{
if ( ChrList.lst[charb].alive && !ChrList.lst[charb].invictus && charb != donttarget && charb != oldtarget )
{
if ( anyone || ( ChrList.lst[charb].team == team && onlyfriends ) || ( TeamList[team].hatesteam[ChrList.lst[charb].team] && enemies ) )
{
distance = ABS( ChrList.lst[charb].pos.x - chrx ) + ABS( ChrList.lst[charb].pos.y - chry );
if ( distance < globestdistance )
{
angle = ( ATAN2( ChrList.lst[charb].pos.y - chry, ChrList.lst[charb].pos.x - chrx ) + PI ) * 0xFFFF / ( TWO_PI );
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
charb = ChrList.lst[charb].fanblock_next;
cnt++;
}
}
return returncode;
}*/

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
x = x >> BLOCK_BITS;
y = y >> BLOCK_BITS;
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

return MAX_CHR;
}*/

void do_game_hud()
{
    int y = 0;

    if ( flip_pages_requested() && cfg.dev_mode )
    {
        GL_DEBUG(glColor4f)( 1, 1, 1, 1 );
        if ( fpson )
        {
            y = draw_string( 0, y, szfpstext );
        }

        y = draw_string( 0, y, "Menu time %f", MProc->base.dtime );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_mesh( line_of_sight_info_t * plos )
{
    Uint32 fan_last;

    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool_t steep;

    if (NULL == plos) return bfalse;

    if ( 0 == plos->stopped_by ) return bfalse;

    ix_stt = plos->x0 / TILE_SIZE;
    ix_end = plos->x1 / TILE_SIZE;

    iy_stt = plos->y0 / TILE_SIZE;
    iy_end = plos->y1 / TILE_SIZE;

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = (ABS(Dy) >= ABS(Dx));

    // determine which are the big and small values
    if (steep)
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if (Dbig < 0)
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if (Dsmall < 0)
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    fan_last = INVALID_TILE;
    for (ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        Uint32 fan;

        if (steep)
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        fan = mesh_get_tile_int(PMesh, ix, iy);
        if ( INVALID_TILE != fan && fan != fan_last )
        {
            // collide the ray with the mesh

            if ( HAS_SOME_BITS(PMesh->mmem.tile_list[fan].fx, plos->stopped_by) )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = PMesh->mmem.tile_list[fan].fx & plos->stopped_by;

                return btrue;
            }

            fan_last = fan;
        }

        // go to the next step
        if (TwoDsmallMinusDbig > 0)
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall             += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_characters( line_of_sight_info_t * plos )
{
    Uint16 ichr;

    if (NULL == plos) return bfalse;

    for ( ichr = 0; ichr < MAX_CHR; ichr++)
    {
        if ( !ChrList.lst[ichr].on ) continue;

        // do line/character intersection
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t do_line_of_sight( line_of_sight_info_t * plos )
{
    bool_t mesh_hit = bfalse, chr_hit = bfalse;

    mesh_hit = collide_ray_with_mesh( plos );

    /*if ( mesh_hit )
    {
        plos->x1 = (plos->collide_x + 0.5f) * TILE_SIZE;
        plos->y1 = (plos->collide_y + 0.5f) * TILE_SIZE;
    }

    chr_hit = collide_ray_with_characters( plos );
    */

    return mesh_hit || chr_hit;
}

//--------------------------------------------------------------------------------------------
void reset_players()
{
    // ZZ> This function clears the player list data
    int cnt;

    // Reset the local data stuff
    local_seekurse         = bfalse;
    local_senseenemiesTeam = TEAM_MAX;
    local_seeinvisible     = bfalse;
    local_allpladead       = bfalse;

    // Reset the initial player data and latches
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        memset( PlaList + cnt, 0, sizeof(player_t) );
    }
    numpla        = 0;

    nexttimestamp = ((Uint32)~0);
    numplatimes   = 0;
}

//--------------------------------------------------------------------------------------------
bool_t release_one_model_profile( Uint16 object )
{
    if( object > MAX_PROFILE ) return bfalse;

    // free the model definition
    release_one_cap( object );

    // free the model data
    release_one_mad( object );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t upload_water_layer_data( water_instance_layer_t inst[], wawalite_water_layer_t data[], int layer_count )
{
    int layer;

    if( NULL == inst || 0 == layer_count ) return bfalse;

    // clear all data
    memset(inst, 0, layer_count * sizeof(water_instance_layer_t));

    // set the frame
    for ( layer = 0; layer < layer_count; layer++)
    {
        inst[layer].frame = generate_randmask( 0 , WATERFRAMEAND );
    }

    if ( NULL != data )
    {
        for ( layer = 0; layer < layer_count; layer++)
        {
            inst[layer].z         = data[layer].z;
            inst[layer].amp       = data[layer].amp;

            inst[layer].dist.x    = data[layer].dist[XX];
            inst[layer].dist.y    = data[layer].dist[YY];

            inst[layer].light_dir = data[layer].light_dir / 63.0f;
            inst[layer].light_add = data[layer].light_add / 63.0f;

            inst[layer].tx_add.x    = data[layer].tx_add[XX];
            inst[layer].tx_add.y    = data[layer].tx_add[YY];

            inst[layer].alpha       = data[layer].alpha;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_water_data( water_instance_t * pinst, wawalite_water_t * pdata )
{
    int layer;

    if (NULL == pinst) return bfalse;

    memset(pinst, 0, sizeof(water_instance_t));

    if ( NULL != pdata )
    {
        // upload the data

        pinst->surface_level = pdata->surface_level;
        pinst->douse_level   = pdata->douse_level;

        pinst->is_water         = pdata->is_water;
        pinst->overlay_req      = pdata->overlay_req;
        pinst->background_req   = pdata->background_req;

        pinst->light            = pdata->light;

        pinst->foregroundrepeat = pdata->foregroundrepeat;
        pinst->backgroundrepeat = pdata->backgroundrepeat;

        // upload the layer data
        pinst->layer_count   = pdata->layer_count;
        upload_water_layer_data( pinst->layer, pdata->layer, pdata->layer_count );
    }

    // fix the alpha in case of self-lit textures
    if ( pdata->light )
    {
        for ( layer = 0; layer < pinst->layer_count; layer++ )
        {
            pinst->layer[layer].alpha = 255;  // Some cards don't support alpha lights...
        }
    }

    make_water( pinst, pdata );

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && pinst->layer_count > 1 )
    {
        int iTmp = pdata->layer[0].alpha;
        iTmp = FF_MUL( pdata->layer[1].alpha, iTmp ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        pinst->layer_count = 1;
        pinst->layer[0].alpha = iTmp;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_weather_data( weather_instance_t * pinst, wawalite_weather_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(weather_instance_t) );

    // set a default value
    pinst->timer_reset = 10;

    if ( NULL != pdata )
    {
        // copy the data
        pinst->timer_reset = pdata->timer_reset;
        pinst->over_water  = pdata->over_water;
    }

    // set the new data
    pinst->time = pinst->timer_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_fog_data( fog_instance_t * pinst, wawalite_fog_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(fog_instance_t) );

    pdata->top      = 0;
    pdata->bottom   = -100;
    pinst->on       = cfg.fog_allowed;

    if ( NULL != pdata )
    {
        pinst->on     = pdata->found && pinst->on;
        pinst->top    = pdata->top;
        pinst->bottom = pdata->bottom;

        pinst->red    = pdata->red * 0xFF;
        pinst->grn    = pdata->grn * 0xFF;
        pinst->blu    = pdata->blu * 0xFF;
    }

    pinst->distance = ( pdata->top - pdata->bottom );
    pinst->on       = ( pinst->distance < 1.0f ) && pinst->on;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_damagetile_data( damagetile_instance_t * pinst, wawalite_damagetile_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(damagetile_instance_t) );

    pinst->sound_time   = TILESOUNDTIME;
    pinst->min_distance = 9999;

    if ( NULL != pdata )
    {
        pinst->amount.base  = pdata->amount;
        pinst->amount.rand  = 1;
        pinst->type         = pdata->type;

        pinst->parttype     = pdata->parttype;
        pinst->partand      = pdata->partand;
        pinst->sound        = CLIP(pdata->sound, -1, MAX_WAVE);
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_animtile_data( animtile_instance_t inst[], wawalite_animtile_t * pdata, size_t animtile_count )
{
    int size;

    if ( NULL == inst || 0 == animtile_count ) return bfalse;

    memset( inst, 0, sizeof(damagetile_instance_t) );

    for( size = 0; size < animtile_count; size++ )
    {
        inst[size].frame_and  = (1 << (size+2)) - 1;
        inst[size].base_and   = ~inst[size].frame_and;
        inst[size].frame_add  = 0;
    }

    if ( NULL != pdata )
    {
        inst[0].update_and = pdata->update_and;
        inst[0].frame_and  = pdata->frame_and;
        inst[0].base_and   = ~inst[0].frame_and;

        for( size = 1; size < animtile_count; size++ )
        {
            inst[size].update_and = pdata->update_and;
            inst[size].frame_and  = ( inst[size-1].frame_and << 1 ) | 1;
            inst[size].base_and   = ~inst[size].frame_and;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_light_data( wawalite_data_t * pdata )
{
    if( NULL == pdata ) return bfalse;

    // upload the lighting data
    light_x = pdata->light_x;
    light_y = pdata->light_y;
    light_z = pdata->light_z;
    light_a = pdata->light_a * 10.0f;

    light_d = 0.0f;
    if ( ABS(light_x) + ABS(light_y) + ABS(light_z) > 0 )
    {
        float fTmp = SQRT( light_x * light_x + light_y * light_y + light_z * light_z );

        // get the extra magnitude of the direct light
        light_d = (1.0f - light_a) * fTmp;
        light_d = CLIP(light_d, 0.0f, 1.0f);

        light_x /= fTmp;
        light_y /= fTmp;
        light_z /= fTmp;
    }

    make_lighttable( pdata->light_x, pdata->light_y, pdata->light_z, pdata->light_a * 10.0f );
    make_lighttospek();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_phys_data( wawalite_physics_t * pdata )
{
    if( NULL == pdata ) return bfalse;

    // upload the physics data
    hillslide      = pdata->hillslide;
    slippyfriction = pdata->slippyfriction;
    airfriction    = pdata->airfriction;
    waterfriction  = pdata->waterfriction;
    noslipfriction = pdata->noslipfriction;
    gravity        = pdata->gravity;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_graphics_data( wawalite_graphics_t * pdata )
{
    if( NULL == pdata ) return bfalse;

    // Read extra data
    gfx.exploremode = pdata->exploremode;
    gfx.usefaredge  = pdata->usefaredge;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_camera_data( wawalite_camera_t * pdata )
{
    if( NULL == pdata ) return bfalse;

    PCamera->swing     = pdata->swing;
    PCamera->swingrate = pdata->swingrate;
    PCamera->swingamp  = pdata->swingamp;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    // ZZ> This function sets up water and lighting for the module

    wawalite_data_t * pdata = &wawalite_data;

    upload_light_data( pdata );
    upload_phys_data( &(pdata->phys) );
    upload_graphics_data( &(pdata->graphics) );
    upload_camera_data( &(pdata->camera) );
    upload_fog_data( &fog, &(pdata->fog) );
    upload_water_data( &water, &(pdata->water) );
    upload_weather_data( &weather, &(pdata->weather) );
    upload_damagetile_data( &damagetile, &(pdata->damagetile) );
    upload_animtile_data( animtile, &(pdata->animtile), SDL_arraysize(animtile) );
}
