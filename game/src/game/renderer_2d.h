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

/// the maximum length egoboo messages
#define EGO_MESSAGE_SIZE      90

/// the minimum number of on-screen messages
#define EGO_MESSAGE_MIN      1

/// the normal number of on-screen messages
#define EGO_MESSAGE_STD      4

/// the maximum number of on-screen messages
#define EGO_MESSAGE_MAX      8

//--------------------------------------------------------------------------------------------

/// A display messages
struct msg_t
{
    int time;                            ///< The remaining time for this message.
    char textdisplay[EGO_MESSAGE_SIZE];  ///< The displayed text.
};

//--------------------------------------------------------------------------------------------

struct DisplayMsgs {
    DisplayMsgs() : timechange(0), count(EGO_MESSAGE_MAX), elements(), on(true) {
    }
    static DisplayMsgs& get() {
        static DisplayMsgs singleton;
        return singleton;
    }
    void clear();
    void reset();
    int get_free();
    int printf(const char *format, ...) GCC_PRINTF_FUNC(1);
    void print(const char *text);
    int vprintf(const char *format, va_list args);
    float draw_all(float y);
    void initialize();
    void uninitialize();
    void update();
    void download(egoboo_config_t& cfg);
    void upload(egoboo_config_t& cfg);

    /// An array of display messages.
    StaticArray<msg_t, EGO_MESSAGE_MAX> elements;
    int timechange;     ///< how much time has elapsed for messages
    int count;          ///< maximum number of displayed messages
    bool on;             ///< Messages?
};

//--------------------------------------------------------------------------------------------

// mode control
void gfx_begin_2d();
void gfx_end_2d();
void gfx_begin_text();
void gfx_end_text();

// bitmap font functions
/**
 * @details This function spits a line of null terminated text onto the backbuffer
 * @details Uses gfx_begin_2d() ... gfx_end_2d() so that the function can basically be called from anywhere.
 *          The way they are currently implemented, this breaks the icon drawing in draw_status() if
 *          you use draw_string() and then draw_icon(). Use draw_string_raw(), instead.
 * @todo    Is there actually anything that does not break something else or is itself broken or conceptually flawed?
 */
float draw_string(float x, float y, const char *format, ...) GCC_PRINTF_FUNC( 3 );
float draw_wrap_string(const char *szText, float x, float y, int maxx);
void draw_one_font(const std::shared_ptr<const Ego::Texture>& ptex, int fonttype, float x, float y);
int draw_string_raw(float x, float y, const char *format, ...) GCC_PRINTF_FUNC( 3 );

// graphics primitive functions
/// Draw a coloured quad.
void draw_quad_2d(const ego_frect_t scr_rect, bool use_alpha, const Ego::Math::Colour4f& tint);
/// Draw a coloured and textured quad.
void draw_quad_2d(const std::shared_ptr<const Ego::Texture>& tex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, const bool useAlpha, const Ego::Colour4f& tint = Ego::Colour4f::white());
bool dump_screenshot();
