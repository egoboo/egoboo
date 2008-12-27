/* Egoboo - Ui.h
 * A basic library for implementing user interfaces, based off of Casey Muratori's
 * IMGUI.  (https:// mollyrocket.com/forums/viewtopic.php?t=134)
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

#ifndef egoboo_ui_h
#define egoboo_ui_h

#include "font.h"
#include "gltexture.h"
#include <SDL.h>

typedef Uint32 UI_ID;
#define UI_Nothing (UI_ID)(-1)

// Initialize or shut down the ui system
int  ui_initialize();
void ui_shutdown();

// Pass input data from SDL to the ui
void ui_handleSDLEvent( SDL_Event *evt );

// Allow the ui to do work that needs to be done before and after each frame
void ui_beginFrame( float deltaTime );
void ui_endFrame();

// UI controls
int  ui_doButton( UI_ID id, const char *text, int x, int y, int width, int height );
int  ui_doImageButton( UI_ID id, GLTexture *img, int x, int y, int width, int height );
int  ui_doImageButtonWithText( UI_ID id, GLTexture *img, const char *text, int x, int y, int width, int height );
// int  ui_doTextBox(UI_ID id, const char *text, int x, int y, int width, int height);

// Utility functions
int  ui_mouseInside( int x, int y, int width, int height );
void ui_setActive( UI_ID id );
void ui_setHot( UI_ID id );
Font* ui_getFont();

/*****************************************************************************/
// Most users won't need to worry about stuff below here; it's mostly for
// implementing new controls.
/*****************************************************************************/

// Behaviors
int  ui_buttonBehavior( UI_ID id, int x, int y, int width, int height );

// Drawing
void ui_drawButton( UI_ID id, int x, int y, int width, int height );
void ui_drawImage( UI_ID id, GLTexture *img, int x, int y, int width, int height );
void ui_drawTextBox( const char *text, int x, int y, int width, int height, int spacing );

#endif // include guard
