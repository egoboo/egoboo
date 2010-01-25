/* Egoboo - MainLoop.c
 * This code is not currently in use.
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

#include "Log.h"
#include "Clock.h"
#include "System.h"
#include "Server.h"
#include "Client.h"
#include "Menu.h"

int quitRequested = 0;

int MainLoop()
{
    int rVal = 0;

    log_info( "Entering main loop." );

    while ( !quitRequested )
    {
        // Absolute first thing to do is update the clock
        clock_frameStep();

        // Next, let the system module gather any input from this frame, and
        // process any window system events.  This could result in having to
        // quit rather quickly
        rVal = sys_frameStep();
        if ( rVal != 0 ) break;

        // Update the server component.  This component is responsible for updating
        // the game world as well. (Gameplay is hosted by the server)
        sv_frameStep();

        // Update the client component.  This is where my stuff breaks down... shoot.
        // How do I fit menus & ui in with the client-server model?
        // Also, world rendering happens inside the client
        cl_frameStep();

        // Update menus that exist outside of the client/server structure.  This is
        // stuff like the menus that you run through before the game starts, so most
        // of the time this will be inactive.
        // The menu might request that the game quit, but I don't need to worry about
        // checking it right now because it's the last thing that happens in the loop.
        menu_frameStep();
    }

    log_info( "Leaving main loop." );
    return ( rVal != 0 ) ? rVal : quitRequested; // Return whichever one isn't zero
}
