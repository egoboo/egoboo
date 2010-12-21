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

/// @file link.c
/// @brief Manages in-game links connecting modules
/// @details

#include "link.h"

#include "char.inl"
#include "camera.h"

#include "menu.h"
#include "log.h"
#include "graphic.h"
#include "module_file.h"
#include "game.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_typedef.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LINK_HEROES_MAX MAX_PLAYER
#define LINK_STACK_MAX  10

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t link_push_module();
static bool_t link_test_module( const char * modname );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_hero_spawn_data
{
    Uint32 object_index;
    fvec3_t   pos;
    fvec3_t   pos_stt;

    // are there any other hero things to add here?
};
typedef struct s_hero_spawn_data hero_spawn_data_t;

/// A list of all the active links
struct s_link_stack_entry
{
    STRING            modname;
    int               hero_count;
    hero_spawn_data_t hero[LINK_HEROES_MAX];

    // more module parameters, like whether it is beaten or some other things?
};
typedef struct s_link_stack_entry link_stack_entry_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Link_t LinkList[LINK_COUNT];

static int                link_stack_count = 0;
static link_stack_entry_t link_stack[LINK_STACK_MAX];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t link_follow_modname( const char * modname, bool_t push_current_module )
{
    /// @details BB@> This causes the game to follow a link, given the module name

    bool_t retval;
    int old_link_stack_count = link_stack_count;

    if ( !VALID_CSTR( modname ) ) return bfalse;

    // can this module be loaded?
    if ( !link_test_module( modname ) ) return bfalse;

    // push the link BEFORE you change the module data
    // otherwise you won't save the correct data!
    if ( push_current_module )
    {
        link_push_module();
    }

    // export all the local and remote characters and
    // quit the old module
    game_finish_module();

    // try to load the new module
    retval = game_begin_module( modname, PMod->seed );

    if ( !retval )
    {
        // if the module linking fails, make sure to remove any bad info from the stack
        link_stack_count = old_link_stack_count;
    }
    else
    {
        pickedmodule_index         = -1;
        pickedmodule_path[0]       = CSTR_END;
        pickedmodule_name[0]       = CSTR_END;
        pickedmodule_write_path[0] = CSTR_END;

        pickedmodule_index = mnu_get_mod_number( modname );
        if ( -1 != pickedmodule_index )
        {
            strncpy( pickedmodule_path,       mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pickedmodule_path ) );
            strncpy( pickedmodule_name,       mnu_ModList_get_name( pickedmodule_index ), SDL_arraysize( pickedmodule_name ) );
            strncpy( pickedmodule_write_path, mnu_ModList_get_dest_path( pickedmodule_index ), SDL_arraysize( pickedmodule_write_path ) );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_build_vfs( const char * fname, Link_t list[] )
{
    vfs_FILE * pfile;
    int i;

    if ( !VALID_CSTR( fname ) ) return bfalse;

    pfile = vfs_openRead( fname );
    if ( NULL == pfile ) return bfalse;

    i = 0;
    while ( goto_colon( NULL, pfile, btrue ) && i < LINK_COUNT )
    {
        fget_string( pfile, list[i].modname, SDL_arraysize( list[i].modname ) );
        list[i].valid = btrue;
        i++;
    }

    vfs_close( pfile );

    return i > 0;
}

//--------------------------------------------------------------------------------------------
bool_t link_pop_module()
{
    bool_t retval;
    link_stack_entry_t * pentry;

    if ( link_stack_count <= 0 ) return bfalse;
    link_stack_count--;

    pentry = link_stack + link_stack_count;

    retval = link_follow_modname( pentry->modname, bfalse );

    if ( retval )
    {
        int i;
        CHR_REF j;

        // restore the heroes' positions before jumping out of the module
        for ( i = 0; i < pentry->hero_count; i++ )
        {
            chr_t * pchr;
            hero_spawn_data_t * phero = pentry->hero + i;

            pchr = NULL;
            for ( j = 0; j < MAX_CHR; j++ )
            {
                if ( !INGAME_CHR( j ) ) continue;

                if ( phero->object_index == ChrList.lst[j].profile_ref )
                {
                    pchr = ChrList.lst + j;
                    break;
                };
            }

            // is the character is found, restore the old position
            if ( NULL != pchr )
            {
                chr_set_pos( pchr, phero->pos.v );
                pchr->pos_old  = phero->pos;
                pchr->pos_stt  = phero->pos_stt;

                chr_update_safe( pchr, btrue );
            }
        };
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_push_module()
{
    bool_t retval;
    link_stack_entry_t * pentry;
    PLA_REF ipla;

    if ( link_stack_count >= MAX_PLAYER || pickedmodule_index < 0 ) return bfalse;

    // grab an entry
    pentry = link_stack + link_stack_count;
    memset( pentry, 0, sizeof( *pentry ) );

    // store the load name of the module
    strncpy( pentry->modname, mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pentry->modname ) );

    // find all of the exportable characters
    pentry->hero_count = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        chr_t * pchr;

        hero_spawn_data_t * phero;

        if ( !PlaStack.lst[ipla].valid ) continue;

        // Is it alive?
        ichr = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( ichr ) ) continue;
        pchr = ChrList.lst + ichr;

        if ( pentry->hero_count < LINK_HEROES_MAX )
        {
            phero = pentry->hero + pentry->hero_count;
            pentry->hero_count++;

            // copy some important info
            phero->object_index = REF_TO_INT( pchr->profile_ref );

            phero->pos_stt.x    = pchr->pos_stt.x;
            phero->pos_stt.y    = pchr->pos_stt.y;
            phero->pos_stt.z    = pchr->pos_stt.z;

            phero->pos = chr_get_pos( pchr );
        }
    }

    // the function only succeeds if at least one hero's info was cached
    retval = bfalse;
    if ( pentry->hero_count > 0 )
    {
        link_stack_count++;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_load_parent( const char * modname, fvec3_t   pos )
{
    int i;
    link_stack_entry_t * pentry;
    fvec3_t   pos_diff;

    if ( !VALID_CSTR( modname ) ) return bfalse;

    // push this module onto the stack so we can count the heroes
    if ( !link_push_module() ) return bfalse;

    // grab the stored data
    pentry = link_stack + ( link_stack_count - 1 );

    // determine how you would have to shift the heroes so that they fall on top of the spawn point
    pos_diff.x = pos.x * GRID_FSIZE - pentry->hero[0].pos_stt.x;
    pos_diff.y = pos.y * GRID_FSIZE - pentry->hero[0].pos_stt.y;
    pos_diff.z = pos.z * GRID_FSIZE - pentry->hero[0].pos_stt.z;

    // adjust all the hero spawn points
    for ( i = 0; i < pentry->hero_count; i++ )
    {
        hero_spawn_data_t * phero = pentry->hero + i;

        phero->pos_stt.x += pos_diff.x;
        phero->pos_stt.y += pos_diff.y;
        phero->pos_stt.z += pos_diff.z;

        phero->pos = phero->pos_stt;
    }

    // copy the module name
    strncpy( pentry->modname, modname, SDL_arraysize( pentry->modname ) );

    // now pop this "fake" module reference off the stack
    return link_pop_module();
}

//--------------------------------------------------------------------------------------------
bool_t link_test_module( const char * modname )
{
    bool_t retval = bfalse;

    LoadPlayer_list_t tmp_loadplayer = LOADPLAYER_LIST_INIT;

    if ( !VALID_CSTR( modname ) ) return bfalse;

    // generate a temporary list of loadplayers
    LoadPlayer_list_from_players( &tmp_loadplayer );

    // test the given module
    retval = mnu_test_module_by_name( &tmp_loadplayer, modname );

    // blank out the list (not necessary since the list is local, but just in case)
    LoadPlayer_list_init( &tmp_loadplayer );

    return retval;
}
