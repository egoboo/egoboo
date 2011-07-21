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

/// @file lua_console.c
/// @brief Quale-like console running running Lua behing Egoboo
/// @details

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

#include "lua_console.h"
#include "file_common.h"

#include "egoboo_console.inl"
#include "egoboo_typedef.h"

#include <string.h>

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A console for interacting with lua
struct s_lua_console
{
    lua_State * L;

    egoboo_console_t base;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int             atexit_lua_registered = 0;
static lua_State     * global_L = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// this is the hook for connecting a Lua state to the Egoboo data
int luaopen_ego( lua_State* L );
//extern void luaopen_ego( lua_State* L );

static SDL_bool lua_console_run( egoboo_console_t * pcon, void * data );

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
void initialize_lua()
{
    if ( NULL != global_L ) return;

    global_L = lua_open();

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
    egoboo_console_create( &( pcon->base ), Con_rect, lua_console_run, pcon );

    // connect to Lua
    pcon->L = lua_newthread( global_L );  /* create state */
    if ( NULL == pcon->L )
    {
        egoboo_console_fprint( &( pcon->base ), "lua_console_init() - cannot create Lua state\n" );
        return pcon;
    }

    return pcon;
}

//--------------------------------------------------------------------------------------------
lua_console_t * lua_console_create( lua_console_t * pcon, SDL_Rect Con_rect )
{
    SDL_bool local_allocation = SDL_FALSE;

    // make sure we have an instance of our lua environment
    initialize_lua();

    // make sure that we have a valid pointer to a console
    if ( NULL == pcon )
    {
        local_allocation = SDL_TRUE;
        pcon = EGOBOO_NEW( lua_console_t );
    }

    return lua_console_ctor( pcon, Con_rect );
}

//--------------------------------------------------------------------------------------------
lua_console_t * lua_console_dtor( lua_console_t * pcon )
{
    egoboo_console_t * ptr;

    if ( NULL == pcon ) return NULL;

    // uninitialize our own data
    lua_close( pcon->L );

    // delete the "base class", but tell it not to actuall free the data
    ptr = &( pcon->base );
    egoboo_console_destroy( &ptr, SDL_FALSE );

    return pcon;
}

//--------------------------------------------------------------------------------------------
SDL_bool lua_console_destroy( lua_console_t ** pcon )
{
    if ( NULL == pcon ) return SDL_FALSE;

    if ( NULL == lua_console_dtor( *pcon ) ) return SDL_FALSE;

    // do the free-ing here
    EGOBOO_DELETE( *pcon );

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
egoboo_console_t * lua_console_get_base( lua_console_t * pcon )
{
    if ( NULL == pcon ) return NULL;

    return &( pcon->base );
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

        egoboo_console_fprint( &( pcon->base ), "%s\n", msg );

        lua_pop( pcon->L, 1 );
    }

    return status;
}

//--------------------------------------------------------------------------------------------
SDL_bool lua_console_run( egoboo_console_t * ego_con, void * data )
{
    int status;
    lua_console_t * lua_con;

    if ( NULL == ego_con || NULL == data ) return SDL_FALSE;

    lua_con = ( lua_console_t * )data;

    status = luaL_loadbuffer( lua_con->L, ego_con->buffer, strlen( ego_con->buffer ), "lua_console" );

    if ( status )
    {
        egoboo_console_fprint( ego_con, "Could not load the line \"%s\"\n", ego_con->buffer );
        egoboo_console_fprint( ego_con, "Lua status: \"%s\"\n", lua_tostring( lua_con->L, -1 ) );
    }
    else
    {
        status = lua_pcall( lua_con->L, 0, 0, 0 );
        lua_console_report( lua_con, status );
    };

    return ( SDL_bool )( 0 == status );
}

//--------------------------------------------------------------------------------------------
/*
** An override for the global Lua "print" function. Modified from the version of luaL_print int the Lua source
** prints to the top console, not to the top Lua console. Probably need to fix this!
*/
int lua_console_print( lua_State * L )
{
    egoboo_console_t * ego_con = egoboo_console_top;

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
        if ( i > 1 ) egoboo_console_fprint( ego_con, "    ", stdout );
        egoboo_console_fprint( ego_con, s );
        lua_pop( L, 1 );  /* pop result */
    }
    egoboo_console_fprint( ego_con, "\n" );

    return SDL_TRUE;
}
