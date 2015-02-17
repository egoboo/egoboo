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

/// @file game/link.c
/// @brief Manages in-game links connecting modules
/// @details

#include "game/link.h"
#include "game/graphic.h"
#include "game/game.h"
#include "game/player.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/module/Module.hpp"
#include "game/entities/ObjectHandler.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_hero_spawn_data;
typedef struct s_hero_spawn_data hero_spawn_data_t;

struct s_link_stack_entry;
typedef struct s_link_stack_entry link_stack_entry_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LINK_HEROES_MAX MAX_PLAYER
#define LINK_STACK_MAX  10

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool link_push_module();
static bool link_test_module( const char * modname );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_hero_spawn_data
{
    Uint32 object_index;
    fvec3_t   pos;
    fvec3_t   pos_stt;

    // are there any other hero things to add here?
};

/// A list of all the active links
struct s_link_stack_entry
{
    STRING            modname;
    int               hero_count;
    hero_spawn_data_t hero[LINK_HEROES_MAX];

    // more module parameters, like whether it is beaten or some other things?
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Link_t LinkList[LINK_COUNT];

static int                link_stack_count = 0;
static link_stack_entry_t link_stack[LINK_STACK_MAX];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool link_follow_modname( const char * modname, bool push_current_module )
{
//ZF> TODO: not implemented
    return false;
#if 0

    /// @author BB
    /// @details This causes the game to follow a link, given the module name

    bool retval;
    int old_link_stack_count = link_stack_count;

    if ( !VALID_CSTR( modname ) ) return false;

    // can this module be loaded?
    if ( !link_test_module( modname ) ) return false;

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
    //retval = game_begin_module(modname);
    retval = false; //ZF> TODO: not implemented

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
#endif
}

//--------------------------------------------------------------------------------------------
bool link_build_vfs( const char * fname, Link_t list[] )
{
    if ( !VALID_CSTR( fname ) ) return false;

    ReadContext ctxt(fname);
    if (!ctxt.ensureOpen()) return false;

    size_t i = 0;
    while (goto_colon_vfs(ctxt, true) && i < LINK_COUNT)
    {
        vfs_get_string( ctxt, list[i].modname, SDL_arraysize( list[i].modname ) );
        list[i].valid = true;
        i++;
    }

    return i > 0;
}

//--------------------------------------------------------------------------------------------
bool link_pop_module()
{
    bool retval;
    link_stack_entry_t * pentry;

    if ( link_stack_count <= 0 ) return false;
    link_stack_count--;

    pentry = link_stack + link_stack_count;

    retval = link_follow_modname( pentry->modname, false );

    if ( retval )
    {
        int i;

        // restore the heroes' positions before jumping out of the module
        for ( i = 0; i < pentry->hero_count; i++ )
        {
            std::shared_ptr<Object> pchr;
            hero_spawn_data_t * phero = pentry->hero + i;

            pchr = NULL;
            for(const std::shared_ptr<Object> &object : _gameObjects.iterator())
            {
                if(object->isTerminated()) {
                    continue;
                }

                if ( phero->object_index == object->profile_ref )
                {
                    pchr = object;
                    break;
                };
            }

            // is the character is found, restore the old position
            if ( nullptr != pchr )
            {
                pchr->setPosition(phero->pos);
                pchr->pos_old  = phero->pos;
                pchr->pos_stt  = phero->pos_stt;

                chr_update_safe( pchr.get(), true );
            }
        };
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool link_push_module()
{
    return false;
    //TODO: not ported
#if 0
    bool retval;
    link_stack_entry_t * pentry;
    PLA_REF ipla;

    if ( link_stack_count >= MAX_PLAYER || pickedmodule_index < 0 ) return false;

    // grab an entry
    pentry = link_stack + link_stack_count;
    BLANK_STRUCT_PTR( pentry )

    // store the load name of the module
    strncpy( pentry->modname, mnu_ModList_get_vfs_path( pickedmodule_index ), SDL_arraysize( pentry->modname ) );

    // find all of the exportable characters
    pentry->hero_count = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        Object * pchr;

        hero_spawn_data_t * phero;

        if ( !PlaStack.lst[ipla].valid ) continue;

        // Is it alive?
        ichr = PlaStack.lst[ipla].index;
        if ( !_gameObjects.exists( ichr ) ) continue;
        pchr = _gameObjects.get( ichr );

        if ( pentry->hero_count < LINK_HEROES_MAX )
        {
            phero = pentry->hero + pentry->hero_count;
            pentry->hero_count++;

            // copy some important info
            phero->object_index = REF_TO_INT( pchr->profile_ref );

            phero->pos_stt.x    = pchr->pos_stt.x;
            phero->pos_stt.y    = pchr->pos_stt.y;
            phero->pos_stt.z    = pchr->pos_stt.z;

            chr_get_pos( pchr, phero->pos.v );
        }
    }

    // the function only succeeds if at least one hero's info was cached
    retval = false;
    if ( pentry->hero_count > 0 )
    {
        link_stack_count++;
        retval = true;
    }

    return retval;
#endif
}

//--------------------------------------------------------------------------------------------
bool link_load_parent( const char * modname, fvec3_t pos )
{
    int i;
    link_stack_entry_t * pentry;
    fvec3_t   pos_diff;

    if ( !VALID_CSTR( modname ) ) return false;

    // push this module onto the stack so we can count the heroes
    if ( !link_push_module() ) return false;

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
bool link_test_module( const char * modname )
{
    if ( !VALID_CSTR( modname ) ) return false;

    //ZF> Not supported yet, needs porting
/*
    bool retval = false;

    LoadPlayer_list_t tmp_loadplayer = LOADPLAYER_LIST_INIT;

    // generate a temporary list of loadplayers
    LoadPlayer_list_from_players( &tmp_loadplayer );

    // test the given module
    retval = mnu_test_module_by_name( &tmp_loadplayer, modname );

    // blank out the list (not necessary since the list is local, but just in case)
    LoadPlayer_list_init( &tmp_loadplayer );

    return retval;
*/
    return false;
}
