/* Egoboo - graphic.h
 * 
 * 
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

#pragma once

#include "ogl_texture.h"

int draw_string( GLtexture * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... );
bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect );

void BeginText( GLtexture * pfnt );
void EndText( void );

void Begin2DMode( void );
void End2DMode( void );

