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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file ui.h
/// @details A basic library for implementing user interfaces, based off of Casey Muratori's
/// IMGUI.  (https://mollyrocket.com/forums/viewtopic.php?t=134)

#include "font_ttf.h"
#include "ogl_texture.h"
#include "egoboo_typedef.h"

#include <SDL.h>

#define UI_Nothing (ui_id_t)(-1)
#define UI_Invalid (ui_id_t)(-2)

typedef struct s_ui_Context ui_Context_t;
typedef Uint32 ui_id_t;

/// Possible UI button states
enum e_ui_button_values
{
    BUTTON_NOCHANGE = 0,
    BUTTON_DOWN,
    BUTTON_UP
};
typedef enum e_ui_button_values ui_buttonValues;

/// Possible UI button properties
enum e_ui_button_bits
{
    UI_BITS_NONE      = 0,
    UI_BITS_MOUSEOVER = 1 << 0,
    UI_BITS_CLICKED   = 1 << 1
};

/// The data descibing the state of a UI widget
struct s_ui_Widget
{
    ui_id_t       id;
    Font         *pfont;
    SDL_Surface  *text_surf;
    const char   *text;
    oglx_texture_t *img;
    Uint32 mask,  state, timeout;

    // virtual screen coordinates
    float         vx, vy;
    float         vwidth, vheight;
};
typedef struct s_ui_Widget ui_Widget_t;

bool_t ui_copyWidget( ui_Widget_t * pw2, ui_Widget_t * pw1 );
bool_t ui_shrinkWidget( ui_Widget_t * pw2, ui_Widget_t * pw1, float pixels );
bool_t ui_initWidget( ui_Widget_t * pw, ui_id_t id, Font * pfont, const char *text, oglx_texture_t *img, float x, float y, float width, float height );
bool_t ui_widgetAddMask( ui_Widget_t * pw, Uint32 mbits );
bool_t ui_widgetRemoveMask( ui_Widget_t * pw, Uint32 mbits );
bool_t ui_widgetSetMask( ui_Widget_t * pw, Uint32 mbits );

/// Initialize or shut down the ui system
int  ui_begin( const char *default_font, int default_font_size );
void ui_end();
void ui_Reset();

/// Pass input data from SDL to the ui
bool_t ui_handleSDLEvent( SDL_Event *evt );

/// Allow the ui to do work that needs to be done before and after each frame
void ui_beginFrame( float deltaTime );
void ui_endFrame();

/// UI controls
ui_buttonValues ui_doWidget( ui_Widget_t * pWidget );
ui_buttonValues ui_doButton( ui_id_t id, const char *text, Font * font, float x, float y, float width, float height );
ui_buttonValues ui_doImageButton( ui_id_t id, oglx_texture_t *img, float x, float y, float width, float height, GLXvector3f image_tint );
ui_buttonValues ui_doImageButtonWithText( ui_id_t id, oglx_texture_t *img, const char *text, Font * font, float x, float y, float width, float height );
/// int  ui_doTextBox(ui_id_t id, const char *text, float x, float y, float width, float height);

/// Utility functions
int  ui_mouseInside( float x, float y, float width, float height );
// void ui_setActive( ui_id_t id );
// void ui_setHot( ui_id_t id );
Font* ui_getFont();
Font* ui_setFont( Font * font );

/*****************************************************************************/
/// Most users won't need to worry about stuff below here; it's mostly for
/// implementing new controls.
/*****************************************************************************/

/// Behaviors
ui_buttonValues  ui_buttonBehavior( ui_id_t id, float x, float y, float width, float height );
ui_buttonValues  ui_WidgetBehavior( ui_Widget_t * pw );

/// Drawing
void ui_drawButton( ui_id_t id, float x, float y, float width, float height, GLXvector4f pcolor );
void ui_drawImage( ui_id_t id, oglx_texture_t *img, float x, float y, float width, float height, GLXvector4f image_tint );
void ui_drawTextBox( Font * font, const char *text, float x, float y, float width, float height, float spacing );
int  ui_drawBar( ui_id_t id, float vx, float vy, int current, int max, Uint8 bar_type );

/// virtual screen
void ui_set_virtual_screen( float vw, float vh, float ww, float wh );
Font * ui_loadFont( const char * font_name, float vpointSize );

#define egoboo_ui_h

