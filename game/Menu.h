/* Egoboo - menu.h
 * Implements the main menu tree, using the code in Ui.*.  This could probably
 * just go in proto.h.
 */

/*
   This file is part of Egoboo.

   Egoboo is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Egoboo is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef egoboo_Menu_h
#define egoboo_Menu_h

#include "char.h"

//Input player control
#define MAXLOADPLAYER     100
typedef struct load_player_info_t
{
  char name[MAXCAPNAMESIZE];
  char dir[16];
} LOAD_PLAYER_INFO;

extern int       loadplayer_count;
LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

void mnu_frameStep();
void mnu_saveSettings();
void mnu_service_select();
void mnu_start_or_join();
void mnu_pick_player( int module );
void mnu_module_loading( int module );
void mnu_choose_host();
void mnu_choose_module();
void mnu_boot_players();
void mnu_end_text();
void mnu_initial_text();

void mnu_enterMenuMode();
void mnu_exitMenuMode();

int mnu_RunIngame( float deltaTime );

#endif // include guard
