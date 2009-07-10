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

#include "link.h"

#include "char.h"
#include "camera.h"

#include "menu.h"
#include "log.h"
#include "graphic.h"
#include "module.h"
#include "game.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_typedef.h"
#include "egoboo.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LINK_HEROES_MAX MAXPLAYER
#define LINK_STACK_MAX  10

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t link_push_module();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_hero_spawn_data
{
    Uint32 object_index;
    GLvector3 pos;
    GLvector3 pos_stt;

    // are there any other hero things to add here?
};
typedef struct s_hero_spawn_data hero_spawn_data_t;

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
    // BB> This causes the game to follow a link, given the module name

    bool_t retval;
    int old_link_stack_count = link_stack_count;

    if ( !VALID_CSTR(modname) ) return bfalse;

    if ( !modlist_test_by_name(modname) ) return bfalse;

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
        pickedmodule_name[0] = '\0';
        pickedmodule_index = modlist_get_mod_number(modname);
        if ( -1 != pickedmodule_index )
        {
            strncpy( pickedmodule_name, ModList[pickedmodule_index].loadname, SDL_arraysize(pickedmodule_name) );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_build( const char * fname, Link_t list[] )
{
    FILE * pfile;
    int i;

    if ( !VALID_CSTR(fname) ) return bfalse;

    pfile = fopen( fname, "r" );
    if ( NULL == pfile ) return bfalse;

    i = 0;
    while ( goto_colon( NULL, pfile, btrue ) && i < LINK_COUNT )
    {
        fscanf( pfile, "%s", list[i].modname );
        list[i].valid = btrue;
        i++;
    }

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
        int i, j;

        // restore the heroes' positions before jumping out of the module
        for (i = 0; i < pentry->hero_count; i++)
        {
            chr_t * pchr;
            hero_spawn_data_t * phero = pentry->hero + i;

            pchr = NULL;
            for (j = 0; j < MAX_CHR; j++)
            {
                if ( !ChrList[j].on ) continue;

                if ( phero->object_index == ChrList[j].model )
                {
                    pchr = ChrList + j;
                    break;
                };
            }

            // is the character is found, restore the old position
            if ( NULL != pchr )
            {
                pchr->pos      = phero->pos;
                pchr->pos_old  = phero->pos;
                pchr->pos_safe = phero->pos;
                pchr->pos_stt  = phero->pos_stt;
            }
        };
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t link_push_module()
{
    int cnt;
    bool_t retval;
    link_stack_entry_t * pentry;

    if ( link_stack_count >= MAXPLAYER || pickedmodule_index < 0 ) return bfalse;

    // grab an entry
    pentry = link_stack + link_stack_count;
    memset( pentry, 0, sizeof(link_stack_entry_t) );

    // store the load name of the module
    strncpy(
        pentry->modname,
        ModList[pickedmodule_index].loadname,
        SDL_arraysize(pentry->modname) );

    // find all of the exportable characters
    pentry->hero_count = 0;
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        Uint16 ichr;
        chr_t * pchr;

        hero_spawn_data_t * phero;

        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        ichr = PlaList[cnt].index;
        if ( INVALID_CHR( ichr ) ) continue;
        pchr = ChrList + ichr;

        if ( pentry->hero_count < LINK_HEROES_MAX )
        {
            phero = pentry->hero + pentry->hero_count;
            pentry->hero_count++;

            // copy some important info
            phero->object_index = pchr->model;

            phero->pos_stt.x    = pchr->pos_stt.x;
            phero->pos_stt.y    = pchr->pos_stt.y;
            phero->pos_stt.z    = pchr->pos_stt.z;

            phero->pos.x        = pchr->pos.y;
            phero->pos.y        = pchr->pos.y;
            phero->pos.z        = pchr->pos.z;
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
bool_t link_load_parent( const char * modname, GLvector3 pos )
{
    int i;
    link_stack_entry_t * pentry;
    GLvector3 pos_diff;

    if ( !VALID_CSTR(modname) ) return bfalse;

    // push this module onto the stack so we can count the heroes
    if ( !link_push_module() ) return bfalse;

    // grab the stored data
    pentry = link_stack + (link_stack_count - 1);

    // determine how you would have to shift the heroes so that they fall on top of the spawn point
    pos_diff.x = pos.x * 128 - pentry->hero[0].pos_stt.x;
    pos_diff.y = pos.y * 128 - pentry->hero[0].pos_stt.y;
    pos_diff.z = pos.z * 128 - pentry->hero[0].pos_stt.z;

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
    strncpy( pentry->modname, modname, SDL_arraysize(pentry->modname) );

    // now pop this "fake" module reference off the stack
    return link_pop_module();
}
