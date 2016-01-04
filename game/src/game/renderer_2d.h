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
    static DisplayMsgs& get() {
        static DisplayMsgs singleton;
    int vprintf(const char *format, va_list args);
    float draw_all(float y);
    void initialize();
    void uninitialize();
    void update();
    void download(egoboo_config_t& cfg);
    void upload(egoboo_config_t& cfg);
};

//--------------------------------------------------------------------------------------------
// mode control
void gfx_begin_2d();
void gfx_end_2d();

// bitmap font functions
/**
 * @details This function spits a line of null terminated text onto the backbuffer
 * @details Uses gfx_begin_2d() ... gfx_end_2d() so that the function can basically be called from anywhere.
 *          The way they are currently implemented, this breaks the icon drawing in draw_status() if
 *          you use draw_string() and then draw_icon(). Use draw_string_raw(), instead.
 */

 //ZF> TODO: move these to UI manager
int draw_string_raw(float x, float y, const char *format, ...) GCC_PRINTF_FUNC( 3 );
void draw_one_font(const std::shared_ptr<const Ego::Texture>& ptex, int fonttype, float x, float y, float alpha = 1.0f);

// graphics primitive functions
/// Draw a coloured quad.
void draw_quad_2d(const ego_frect_t scr_rect, bool use_alpha, const Ego::Math::Colour4f& tint);
/// Draw a coloured and textured quad.
void draw_quad_2d(const std::shared_ptr<const Ego::Texture>& tex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, const bool useAlpha, const Ego::Colour4f& tint = Ego::Colour4f::white());
