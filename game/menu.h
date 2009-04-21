#pragma once

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

/* Egoboo - menu.h
 * Implements the main menu tree, using the code in Ui.*.  This could probably
 * just go in proto.h.
 */

#include "network.h"
#include "egoboo.h"

//Input player control
#define MAXLOADPLAYER     100
struct s_load_player_info
{
    STRING name;
    STRING dir;
};
typedef struct s_load_player_info LOAD_PLAYER_INFO;

int              loadplayer_count;
LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

extern int    mnu_selectedPlayerCount;
extern int    mnu_selectedInput[MAXPLAYER];
extern Uint16 mnu_selectedPlayer[MAXPLAYER];

void menu_frameStep();

void  check_player_import( const char *dirname, bool_t initialize );

int doMenu( float deltaTime );
int initMenus();

#define egoboo_Menu_h
