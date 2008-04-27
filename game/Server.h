/* Egoboo - Server.h
 * Basic skeleton for the server portion of a client-server architecture,
 * this is totally not in use yet.
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

#ifndef egoboo_Server_h
#define egoboo_Server_h

#include "enet.h"
#include "Network.h"
#include "char.h"
#include "egoboo.h"

typedef struct GameState GameState;

typedef struct server_state_t
{
  // a copy of all the character latches
  float  latchx[MAXCHR];
  float  latchy[MAXCHR];
  Uint8  latchbutton[MAXCHR];

  // the buffered latches that have been stored on the server
  bool_t timelatchvalid[MAXCHR][MAXLAG];
  Uint32 timelatchstamp[MAXCHR][MAXLAG];
  float  timelatchx[MAXCHR][MAXLAG];
  float  timelatchy[MAXCHR][MAXLAG];
  Uint8  timelatchbutton[MAXCHR][MAXLAG];
  Uint32 numplatimes;
  Uint32 nexttimestamp;                    // Expected timestamp

// GameState gameState;
} ServerState;

// Globally accessible server state
extern ServerState AServerState;

int  sv_init();
void sv_shutDown();
void sv_frameStep();

void sv_reset( ServerState * cs );
void sv_talkToRemotes( ServerState * ss );
void sv_bufferLatches( ServerState * ss );
void sv_unbufferLatches( ServerState * ss );
void sv_resetTimeLatches( ServerState * ss, Sint32 ichr );

int  sv_hostGame();
void sv_letPlayersJoin();

bool_t sv_handlePacket( ServerState * ss, ENetEvent *event );

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)

#endif // include guard
