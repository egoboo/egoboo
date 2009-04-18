#pragma once

#include <SDL.h>

struct s_lua_console;

typedef struct s_lua_console lua_console_t;

lua_console_t * lua_console_new( lua_console_t * pcon, SDL_Rect Con_rect );
SDL_bool        lua_console_delete( lua_console_t * pcon );

SDL_bool lua_console_run( lua_console_t * pcon );
SDL_bool lua_console_draw( lua_console_t * pcon );

void lua_console_show( lua_console_t * pcon );
void lua_console_hide( lua_console_t * pcon );
