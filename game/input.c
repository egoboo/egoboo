/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Ui.h"

//--------------------------------------------------------------------------------------------
void read_mouse()
{
  int x, y, b;
  if ( menuactive )
    b = SDL_GetMouseState( &x, &y );
  else
    b = SDL_GetRelativeMouseState( &x, &y );
  mousex = x; // mousex and mousey are the wrong type to use in above call
  mousey = y;
  mousebutton[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
  mousebutton[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
  mousebutton[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
  mousebutton[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;
}

//--------------------------------------------------------------------------------------------
void read_key()
{
  sdlkeybuffer = SDL_GetKeyState( NULL );
//  if(sdlkeybuffer[SDLK_RETURN]) exit(1); MN: this should no longer be necessary
}

//--------------------------------------------------------------------------------------------
void read_joystick()
{
  int button;

  if ( joyaon )
  {
    SDL_JoystickUpdate();
    joyax = SDL_JoystickGetAxis( sdljoya, 0 ) / 32;
    if ( joyax < 100 && joyax > -100 ) joyax = 0;
    joyay = SDL_JoystickGetAxis( sdljoya, 1 ) / 32;
    if ( joyay < 100 && joyay > -100 ) joyay = 0;
    button = SDL_JoystickNumButtons( sdljoya );
    while ( button >= 0 )
    {
      joyabutton[button] = SDL_JoystickGetButton( sdljoya, button );
      button--;
    }
  }
  if ( joybon )
  {
    SDL_JoystickUpdate();
    joybx = SDL_JoystickGetAxis( sdljoyb, 0 ) / 32;
    if ( joybx < 100 && joybx > -100 ) joybx = 0;
    joyby = SDL_JoystickGetAxis( sdljoyb, 1 ) / 32;
    if ( joyby < 100 && joyby > -100 ) joyby = 0;
    button = SDL_JoystickNumButtons( sdljoyb );
    while ( button >= 0 )
    {
      joybbutton[button] = SDL_JoystickGetButton( sdljoyb, button );
      button--;
    }
  }
}

//--------------------------------------------------------------------------------------------
void read_input()
{
  // ZZ> This function gets all the current player input states
  int cnt;
  SDL_Event evt;

  // Run through SDL's event loop to get info in the way that we want
  // it for the Gui code
  while ( SDL_PollEvent( &evt ) )
  {
    ui_handleSDLEvent( &evt );

    switch ( evt.type )
    {
      case SDL_MOUSEBUTTONDOWN:
        pending_click = btrue;
        break;

      case SDL_MOUSEBUTTONUP:
        pending_click = bfalse;
        break;
    }
  }

  // Get immediate mode state for the rest of the game
  read_key();
  read_mouse();
  read_joystick();


  // Set up for button masks
  jab = 0;
  jbb = 0;
  msb = 0;


  // Joystick mask
  cnt = 0;
  while ( cnt < JOYBUTTON )
  {
    jab |= ( joyabutton[cnt] << cnt );
    jbb |= ( joybbutton[cnt] << cnt );
    cnt++;
  }


  // Mouse mask
  msb = ( mousebutton[3] << 3 ) | ( mousebutton[2] << 2 ) | ( mousebutton[1] << 1 ) | ( mousebutton[0] << 0 );
}

