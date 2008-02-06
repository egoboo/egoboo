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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Ui.h"

//--------------------------------------------------------------------------------------------
void read_mouse()
{
  int x, y, b;

  if ( mouseon )
  {
    b = SDL_GetRelativeMouseState( &x, &y );

    mousedx = x;
    mousedy = y;

    mousex += x; if ( mousex < 0 ) mousex = 0; if ( mousex > CData.scrx ) mousex = CData.scrx;
    mousey += y; if ( mousey < 0 ) mousey = 0; if ( mousey > CData.scry ) mousey = CData.scry;

    mousebutton[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    mousebutton[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    mousebutton[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0;  // Middle is 2 on SDL
    mousebutton[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;
  }
  else
  {
    SDL_GetMouseState( &mousex, &mousey );

    mousedx = mousedy = 0;

    mousebutton[0] = 0;
    mousebutton[1] = 0;
    mousebutton[2] = 0;
    mousebutton[3] = 0;
  };
}

//--------------------------------------------------------------------------------------------
void read_key()
{
  sdlkeybuffer = SDL_GetKeyState( NULL );
}

//--------------------------------------------------------------------------------------------
#define JOYMASK (0xFF00)
void read_joystick()
{
  int button;
  float jx, jy;
  const float jthresh = .5;

  if (( !joyaon || NULL == sdljoya ) && ( !joybon || NULL == sdljoyb ) )
  {
    joyax = joyay = 0.0f;
    jab = 0;

    joybx = joyby = 0.0f;
    jbb = 0;
    return;
  };


  SDL_JoystickUpdate();

  if ( joyaon )
  {
    jx = ( float )(( Sint16 )( SDL_JoystickGetAxis( sdljoya, 0 ) & JOYMASK ) ) / ( float )( 1 << 15 );
    jy = ( float )(( Sint16 )( SDL_JoystickGetAxis( sdljoya, 1 ) & JOYMASK ) ) / ( float )( 1 << 15 );

    joyax = 0.0f;
    joyay = 0.0f;
    if ( ABS( jx ) + ABS( jy ) > 0.0f )
    {
      joyax = jx;
      joyay = jy;

      jx = sqrt(( jx * jx + jy * jy ) * 0.5f );
      jy = ( jx - jthresh * jx / ( jthresh + fabs( jx ) ) ) * ( jthresh + 1.0f );

      joyax *= jy / jx / sqrt( 2.0f );
      joyay *= jy / jx / sqrt( 2.0f );
    }

    button = SDL_JoystickNumButtons( sdljoya );
    while ( button >= 0 )
    {
      joyabutton[button] = SDL_JoystickGetButton( sdljoya, button );
      button--;
    }
  }

  if ( joybon )
  {
    jx = ( float )(( Sint16 )( SDL_JoystickGetAxis( sdljoyb, 0 ) & JOYMASK ) ) / ( float )( 1 << 15 );
    jy = ( float )(( Sint16 )( SDL_JoystickGetAxis( sdljoyb, 1 ) & JOYMASK ) ) / ( float )( 1 << 15 );

    joybx = ( jx - jthresh * jx / ( jthresh + fabs( jx ) ) ) * ( jthresh + 1.0f );
    joyby = ( jy - jthresh * jy / ( jthresh + fabs( jy ) ) ) * ( jthresh + 1.0f );

    if ( ABS( joybx ) + ABS( joyby ) > 0 )
    {
      jx = sqrt( joybx * joybx + joyby * joyby );
      joybx /= jx;
      joyby /= jx;
    }

    button = SDL_JoystickNumButtons( sdljoyb );
    while ( button >= 0 )
    {
      joybbutton[button] = SDL_JoystickGetButton( sdljoyb, button );
      button--;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_press()
{
  // ZZ> This function resets key press information
  /*PORT
      int cnt;
      cnt = 0;
      while(cnt < 256)
      {
          keypress[cnt] = bfalse;
          cnt++;
      }
  */
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

  // Joystick mask
  jab = 0;
  jbb = 0;
  for ( cnt = 0; cnt < JOYBUTTON; cnt++ )
  {
    jab |= ( joyabutton[cnt] << cnt );
    jbb |= ( joybbutton[cnt] << cnt );
  }

  // Mouse mask
  msb = 0;
  for ( cnt = 0; cnt < 4; cnt++ )
  {
    msb |= ( mousebutton[cnt] << cnt );
  }
}

