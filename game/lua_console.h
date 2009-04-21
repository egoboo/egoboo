#pragma once

#include "egoboo_console.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_lua_console;

typedef struct s_lua_console lua_console_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
lua_console_t    * lua_console_new( lua_console_t * pcon, SDL_Rect Con_rect );
SDL_bool           lua_console_delete( lua_console_t * pcon );

egoboo_console_t * lua_console_get_base( lua_console_t * pcon );

SDL_Event * lua_console_handle_events( SDL_Event * evt );

