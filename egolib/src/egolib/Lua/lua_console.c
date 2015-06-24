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

/// @file egolib/Lua/lua_console.c
/// @brief Quake-like console running Lua behind Egoboo
/// @details

#include "egolib/Lua/lua_console.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#    include <lua.h>
#    include <lauxlib.h>
#    include <lualib.h>

#if defined(__cplusplus)
}

#endif

#include "egolib/typedef.h"
#include "egolib/file_common.h"

#include "egolib/console.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A console for interacting with lua
struct s_lua_console
{
    lua_State * L;

    egolib_console_t base;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int             atexit_lua_registered = 0;
static lua_State     * global_L = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// this is the hook for connecting a Lua state to the Egoboo data
int luaopen_ego( lua_State* L );

static bool lua_console_run( egolib_console_t * pcon, void * data );

static int lua_console_print( lua_State * L );
static int lua_console_report( lua_console_t * pcon, int status );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void shutdown_lua( void )
{
    if ( NULL != global_L )
    {
        lua_close( global_L );
        global_L = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void initialize_lua( void )
{
    if ( NULL != global_L ) return;

	global_L = luaL_newstate();

    // open some libraries
    // do not open the io library for security!
    luaopen_base( global_L );
    luaopen_string( global_L );
    luaopen_math( global_L );

    // load the wrappered module
    luaopen_ego( global_L );

    // override the global lua "print" function with our own custom version
    lua_register( global_L, "print", lua_console_print );

    if ( !atexit_lua_registered )
    {
        atexit( shutdown_lua );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
lua_console_t * lua_console_ctor( lua_console_t * pcon, SDL_Rect Con_rect )
{
    if ( NULL == pcon ) return NULL;

    // reset all the console data
    memset( pcon, 0, sizeof( *pcon ) );

    // call the new function for the "base class"
    egolib_console_create( &( pcon->base ), Con_rect, lua_console_run, pcon );

    // connect to Lua
    pcon->L = lua_newthread( global_L );  /* create state */
    if ( NULL == pcon->L )
    {
        egolib_console_t::print(&(pcon->base), "lua_console_init() - cannot create Lua state\n");
        return pcon;
    }

    return pcon;
}

//--------------------------------------------------------------------------------------------
lua_console_t * lua_console_create( lua_console_t * pcon, SDL_Rect Con_rect )
{
    bool local_allocation = false;

    // make sure we have an instance of our lua environment
    initialize_lua();

    // make sure that we have a valid pointer to a console
    if ( NULL == pcon )
    {
        local_allocation = true;
        pcon = EGOBOO_NEW( lua_console_t );
    }

    return lua_console_ctor( pcon, Con_rect );
}

//--------------------------------------------------------------------------------------------
lua_console_t *lua_console_dtor(lua_console_t *self)
{
    if (nullptr == self) return nullptr;

    // uninitialize our own data
    if (nullptr != global_L && nullptr != self->L)
    {
        bool removed = false;
        int top = lua_gettop(global_L);
        
        for (int i = top; i > 0; i++)
        {
            lua_State *state = lua_tothread(global_L, i);
            if (state == self->L)
            {
                lua_remove(global_L, i);
                removed = true;
                break;
            }
        }
        
        EGOBOO_ASSERT(removed);
    }

    // delete the "base class", but tell it not to actuall free the data
	/// @todo Shouldn't we just call the dtor of the base class?
    egolib_console_t *base = &(self->base);
    egolib_console_destroy(&base, SDL_FALSE);

    return self;
}

//--------------------------------------------------------------------------------------------
bool lua_console_destroy(lua_console_t **pcon)
{
    if (nullptr == pcon) return false;
    if (nullptr == lua_console_dtor(*pcon)) return false;
    // do the free-ing here
    EGOBOO_DELETE(*pcon);
    return true;
}

//--------------------------------------------------------------------------------------------
egolib_console_t *lua_console_get_base(lua_console_t *self)
{
    if (nullptr == self) return nullptr;

    return &(self->base);
}

//--------------------------------------------------------------------------------------------
int lua_console_report( lua_console_t * pcon, int status )
{
    if ( status && !lua_isnil( pcon->L, -1 ) )
    {
        const char *msg;

        msg = lua_tostring( pcon->L, -1 );
        if ( NULL == msg )
        {
            msg = "(error object is not a string)";
        };

        egolib_console_t::print(&(pcon->base), "%s\n", msg);

        lua_pop( pcon->L, 1 );
    }

    return status;
}

//--------------------------------------------------------------------------------------------
bool lua_console_run( egolib_console_t * ego_con, void * data )
{
    int status;

    if (nullptr == ego_con || nullptr == data) return false;

	lua_console_t *lua_con = ( lua_console_t * )data;

	if (nullptr == lua_con->L) return false;
	EGOBOO_ASSERT( nullptr != global_L );

    status = luaL_loadbuffer( lua_con->L, ego_con->buffer, strlen( ego_con->buffer ), "lua_console" );

    if ( status )
    {
        egolib_console_t::print(ego_con, "Could not load the line \"%s\"\n", ego_con->buffer);
        egolib_console_t::print(ego_con, "Lua status: \"%s\"\n", lua_tostring( lua_con->L, -1 ));
    }
    else
    {
        status = lua_pcall( lua_con->L, 0, 0, 0 );
        lua_console_report( lua_con, status );
    };

    return (0 == status);
}

//--------------------------------------------------------------------------------------------
/*
** An override for the global Lua "print" function. Modified from the version of luaL_print int the Lua source
** prints to the top console, not to the top Lua console. Probably need to fix this!
*/
int lua_console_print( lua_State * L )
{
    egolib_console_t * ego_con = egolib_console_top;

    int n = lua_gettop( L );  /* number of arguments */
    int i;
    lua_getglobal( L, "tostring" );
    for ( i = 1; i <= n; i++ )
    {
        const char *s;
        lua_pushvalue( L, -1 );  /* function to be called */
        lua_pushvalue( L, i ); /* value to print */
        lua_call( L, 1, 1 );
        s = lua_tostring( L, -1 );  /* get result */
        if ( NULL == s )
        {
            return SDL_FALSE;
        }
        if ( i > 1 ) egolib_console_t::print(ego_con, "    ");
        egolib_console_t::print(ego_con, "%s", s);
        lua_pop( L, 1 );  /* pop result */
    }
    egolib_console_t::print(ego_con, "\n");

    return true;
}
