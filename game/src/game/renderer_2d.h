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

/// @file game/renderer_2d.h
/// @brief Routines for rendering 2d primitves

#pragma once

#include "egolib/egolib.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_msg;
typedef struct s_msg msg_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the minimum number of on-screen messages
#define EGO_MESSAGE_MIN      1

/// the normal number of on-screen messages
#define EGO_MESSAGE_STD      4

/// the maximum number of on-screen messages
#define EGO_MESSAGE_MAX      8

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A display messages
struct s_msg
{
    int             time;                            ///< The time for this message
    ego_message_t   textdisplay;                     ///< The displayed text
};

//--------------------------------------------------------------------------------------------

/// array of display messages
DECLARE_STATIC_ARY_TYPE( DisplayMsgAry, msg_t, EGO_MESSAGE_MAX );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

DECLARE_EXTERN_STATIC_ARY( DisplayMsgAry, DisplayMsg );

extern void  DisplayMsg_clear();
extern void  DisplayMsg_reset();
extern int   DisplayMsg_get_free();
extern int   DisplayMsg_printf(const char *format, ...) GCC_PRINTF_FUNC( 1 );
extern void  DisplayMsg_print(const char *text);
extern int   DisplayMsg_vprintf(const char *format, va_list args);
extern float DisplayMsg_draw_all(float y);

extern int  DisplayMsg_timechange;     ///< how much time has elapsed for messages
extern int  DisplayMsg_count;          ///< maximum number of displayed messages
extern bool DisplayMsg_on;             ///< Messages?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// mode control
void gfx_begin_2d();
void gfx_end_2d();
void gfx_begin_text();
void gfx_end_text();
void gfx_enable_texturing();
void gfx_disable_texturing();
void gfx_reshape_viewport( int w, int h );

// bitmap font functions
float draw_string( float x, float y, const char *format, ... ) GCC_PRINTF_FUNC( 3 );
float draw_wrap_string( const char *szText, float x, float y, int maxx );
void  draw_one_font( oglx_texture_t * ptex, int fonttype, float x, float y );
int   draw_string_raw( float x, float y, const char *format, ... ) GCC_PRINTF_FUNC( 3 );

// debugging functions
int   DisplayMsg_printf( const char *format, ... ) GCC_PRINTF_FUNC( 1 );

// graphics primitive functions
void draw_quad_2d( oglx_texture_t * ptex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, const bool use_alpha, const GLXvector4f quad_tint );
bool dump_screenshot();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_renderer_2d_h