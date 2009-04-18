#include "lua_console.h"

#include <SDL.h>
#include <SDL_console.h>

#include <stdio.h>
#include <string.h>

#define lua_c

#include <lua.h>

#include <lauxlib.h>
#include <lualib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_lua_console
{
    // place this data first so that the address of the struct is the same as the address of
    // c, and we can fake a statc_cast<> typecast!
    ConsoleInformation * c;

    lua_State * L;

    size_t len;
    char   buffer[1024];
};
//typedef struct s_lua_console lua_console_t;

static lua_console_t a_console;

static lua_console_t * last_lua_console = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int         atexit_lua_registered = 0;
lua_State * global_L = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// this is the hook for connecting a Lua state to the egoboo data
extern void luaopen_ego(lua_State* L );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void shutdown_lua( void)
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
    if (NULL != global_L) return;

    global_L = lua_open();

    // open some libraries
    // do not open the io library for security!
    luaopen_base(global_L);
    luaopen_string(global_L);
    luaopen_math(global_L);

    // load the wrappered module
    luaopen_ego(global_L);

    if ( !atexit_lua_registered )
    {
        atexit( shutdown_lua );
    }
};

//--------------------------------------------------------------------------------------------
static int lua_console_report (lua_console_t * pcon, int status)
{
    if (status && !lua_isnil(pcon->L, -1))
    {
        const char *msg;

        msg = lua_tostring(pcon->L, -1);
        if ( NULL == msg )
        {
            msg = "(error object is not a string)";
        };

        CON_Out( pcon->c, "%s\n", msg );

        lua_pop(pcon->L, 1);
    }

    return status;
}


//--------------------------------------------------------------------------------------------
void Command_Handler( ConsoleInformation *console, char* command )
{
    // fake an upcast using static_cast<>  ;)
    lua_console_t * pcon = last_lua_console;

    if (NULL == pcon) return;

    if( 0 == strcmp(command, "bye") )
    {
        CON_Hide( console );
    }

    if( 0 == strcmp(command, "exit") )
    {
        CON_Hide( console );
    }

    if( 0 == strcmp(command, "quit") )
    {
        CON_Hide( console );
    }

    // copy the command string into the buffer
    strncpy( pcon->buffer, command, sizeof(pcon->buffer) - 2 );
    pcon->buffer[sizeof(pcon->buffer)-1] = '\0';
    pcon->len = strlen( pcon->buffer );

    // actually do the lua call
    if ( !lua_console_run( pcon ) );
};

//--------------------------------------------------------------------------------------------
lua_console_t * lua_console_new( lua_console_t * pcon, SDL_Rect Con_rect )
{
    SDL_bool local_allocation = SDL_FALSE;
    SDL_Surface *DisplayScreen = NULL;

    if ( NULL == pcon )
    {
        local_allocation = SDL_TRUE;
        pcon = (lua_console_t *) calloc(1, sizeof(lua_console_t));
    }

    if (NULL == pcon) return NULL;

    // make sure we have an instance of our lua environment
    initialize_lua();

    /* STEP 1: Init the consoles */
    DisplayScreen = SDL_GetVideoSurface();
    Con_rect.x = Con_rect.y = 0;
    Con_rect.w = Con_rect.h = 300;
    pcon->c = CON_Init("basicdat/console_font.bmp", DisplayScreen, 100, Con_rect);
    if ( NULL == pcon->c )
    {
        if (local_allocation) free(pcon);
        return NULL;
    }

    /* STEP 2: Attach the Command handling function to the consoles. Remark that every
       console can have its own command handler */
    CON_SetExecuteFunction( pcon->c, Command_Handler );

    // connect to Lua
    pcon->L = lua_newthread( global_L );  /* create state */
    if ( NULL == pcon->L)
    {
        CON_Out( pcon->c, "lua_console_init() - cannot create Lua state: not enough memory\n" );
        return pcon;
    }

    last_lua_console = pcon;

    return pcon;
}

//--------------------------------------------------------------------------------------------
/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static SDL_bool lua_console_print ( lua_console_t * pcon )
{
    int n = lua_gettop(pcon->L);  /* number of arguments */
    int i;
    lua_getglobal(pcon->L, "tostring");
    for (i = 1; i <= n; i++)
    {
        const char *s;
        lua_pushvalue(pcon->L, -1);  /* function to be called */
        lua_pushvalue(pcon->L, i);   /* value to print */
        lua_call(pcon->L, 1, 1);
        s = lua_tostring(pcon->L, -1);  /* get result */
        if (s == NULL)
        {
            return SDL_FALSE;
        }
        if (i > 1) CON_Out( pcon->c, "\t", stdout);
        CON_Out( pcon->c, s, stdout);
        lua_pop(pcon->L, 1);  /* pop result */
    }
    CON_Out( pcon->c, "\n", stdout);

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
SDL_bool lua_console_run( lua_console_t * pcon )
{
    int status;

    if (NULL == pcon->L) return SDL_FALSE;

    status = luaL_loadbuffer(pcon->L, pcon->buffer, pcon->len, "line");

    if ( status )
    {
        CON_Out(pcon->c, "Could not load the line \"%s\"\n", pcon->buffer);
        CON_Out(pcon->c, "Lua status: \"%s\"\n", lua_tostring(pcon->L, -1));
    }
    else
    {
        status = lua_pcall(pcon->L, 0, 0, 0);
        lua_console_report(pcon, status);

        //if (0 == status && lua_gettop(pcon->L) > 0)
        //{
        //    /* any result to print? */
        //    if( !lua_console_print( pcon ) )
        //    {
        //        CON_Out(pcon->c, "Error calling lua_console_print()\n" );
        //    };
        //}
    };

    return 0 == status;
};

//--------------------------------------------------------------------------------------------
SDL_bool lua_console_delete( lua_console_t * pcon )
{
    lua_close( pcon->L );
    CON_Destroy( pcon->c );

    free( pcon );

    if ( pcon == last_lua_console )
    {
        last_lua_console = NULL;
    }

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
SDL_bool lua_console_draw( lua_console_t * pcon )
{
    if ( NULL == pcon || NULL == pcon->c ) return SDL_FALSE;

    CON_DrawConsole(pcon->c);

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void lua_console_show( lua_console_t * pcon )
{
    if( NULL != pcon )
    {
        // we only have one console, so keyrepeat is enabled globally
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

        CON_Show( pcon->c );
        CON_Topmost( pcon->c );
    }
}


//--------------------------------------------------------------------------------------------
void lua_console_hide( lua_console_t * pcon )
{
    if( NULL != pcon )
    {
        // we only have one console, so keyrepeat is disabled globally
        SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

        CON_Hide( pcon->c );
    }
}

//--------------------------------------------------------------------------------------------
SDL_Event * lua_console_handle_events( lua_console_t * pcon, SDL_Event * pevt )
{
    SDL_Event * retval = pevt;

    if( NULL == pcon || NULL == pevt ) return pevt;

    // let the console handle the event. It should trap all special keystrokes like "return" and such
    retval = CON_Events(pevt);

    // the default SDL_console lets most keypresses fall through
    // this is disruptive to teh game, so we need to intercept them ;)
    if( NULL != retval && CON_isVisible( pcon->c ) )
    {
        if( SDL_KEYDOWN == pevt->type || SDL_KEYUP == pevt->type )
        {
            if( isprint( pevt->key.keysym.sym ) )
            {
                // the keycode is grabbed by the console
                retval = NULL;
            }
        }
    }

    return retval;
}