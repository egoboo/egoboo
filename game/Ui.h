/* Egoboo - Ui.h
 * A basic library for implementing user interfaces, based off of Casey Muratori's
 * IMGUI.  (https://mollyrocket.com/forums/viewtopic.php?t=134)
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

#ifndef egoboo_ui_h
#define egoboo_ui_h

#include "Font.h"
#include "ogl_texture.h"
#include "egoboo_types.h"
#include <SDL.h>

#define UI_Nothing (UI_ID)(-1)
#define UI_Invalid (UI_ID)(-2)

typedef struct ui_context_t UiContext;
typedef Uint32 UI_ID;


typedef enum ui_button_values_e
{
  BUTTON_NOCHANGE = 0,
  BUTTON_DOWN,
  BUTTON_UP
} ui_buttonValues;

enum ui_button_bits_e
{
  UI_BITS_NONE      = 0,
  UI_BITS_MOUSEOVER = 1 << 0,
  UI_BITS_CLICKED   = 1 << 1,
};


typedef struct ui_widget_t
{
  UI_ID      id;
  TTFont    *pfont;
  char      *text;
  GLtexture *img;
  int        x;
  int        y;
  int        width;
  int        height;
  Uint32 mask, state, timeout;
} ui_Widget;

// Initialize or shut down the ui system
int  ui_initialize();
void ui_shutdown();

// Pass input data from SDL to the ui
void ui_handleSDLEvent( SDL_Event *evt );

// Allow the ui to do work that needs to be done before and after each frame
void ui_beginFrame( float deltaTime );
void ui_endFrame();

// UI widget

// UI controls
ui_buttonValues  ui_doButton( ui_Widget * pWidget );
ui_buttonValues  ui_doImageButton( ui_Widget * pWidget );
ui_buttonValues  ui_doImageButtonWithText( ui_Widget * pWidget );
//int  ui_doTextBox(ui_Widget * pWidget);

// Utility functions
void    ui_doCursor();
int     ui_mouseInside( int x, int y, int width, int height );
void    ui_setActive( UI_ID id );
void    ui_setHot( UI_ID id );
TTFont* ui_getFont();

bool_t ui_copyWidget( ui_Widget * pw2, ui_Widget * pw1 );
bool_t ui_shrinkWidget( ui_Widget * pw2, ui_Widget * pw1, int pixels );
bool_t ui_initWidget( ui_Widget * pw, UI_ID id, TTFont * pfont, const char *text, GLtexture *img, int x, int y, int width, int height );
bool_t ui_widgetAddMask( ui_Widget * pw, Uint32 mbits );
bool_t ui_widgetRemoveMask( ui_Widget * pw, Uint32 mbits );
bool_t ui_widgetSetMask( ui_Widget * pw, Uint32 mbits );

/*****************************************************************************/
// Most users won't need to worry about stuff below here; it's mostly for
// implementing new controls.
/*****************************************************************************/

// Behaviors
ui_buttonValues ui_buttonBehavior( ui_Widget * pWidget );

// Drawing
void ui_drawButton( ui_Widget * pWidget );
void ui_drawImage( ui_Widget * pWidget );
void ui_drawTextBox( ui_Widget * pWidget, int spacing );

int ui_getMouseX();
int ui_getMouseY();


#endif // include guard
