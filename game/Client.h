/* Egoboo - Client.h
 * Basic skeleton for the client portion of a client-server architecture,
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

#ifndef egoboo_Client_h
#define egoboo_Client_h

#include "enet.h"
#include "Network.h"
#include "char.h"

#include "egoboo_types.h"
#include "egoboo.h"

typedef struct client_state_t
{
  bool_t timelatchvalid[MAXCHR][MAXLAG];
  Uint32 timelatchstamp[MAXCHR][MAXLAG];
  float  timelatchx[MAXCHR][MAXLAG];       // Timed latches
  float  timelatchy[MAXCHR][MAXLAG];       //
  Uint8  timelatchbutton[MAXCHR][MAXLAG];  //
  Uint32 numplatimes;
  Uint32 nexttimestamp;                    // Expected timestamp
} ClientState;


// Globally accesible client state
extern ClientState AClientState;

void cl_reset( ClientState * cs );
void cl_resetTimeLatches( ClientState * cs, Sint32 ichr );
void cl_bufferLatches( ClientState * cs );

int  cl_init();
void cl_shutDown();
void cl_frameStep();

void cl_talkToHost( ClientState * cs );
int  cl_joinGame( const char *hostname );

void cl_unbufferLatches( ClientState * cs );
bool_t cl_handlePacket( ClientState * cs, ENetEvent *event );

// Much more to come...

//int  cl_connectToServer(...);
//int  cl_loadModule(...);
#endif // include guard
