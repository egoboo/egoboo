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

/// @file  egolib/scancode.h
/// @brief Translation of SDL scancodes to keys.

#pragma once

#include "egolib/platform.h"

/// @brief Maps SDL 1 scan codes to ASCII characters as if shift up is not held down.
extern int scancode_to_ascii[SDLK_LAST];
/// @brief Maps SDL 1 scan codes to ASCII characters as if shift up is     held down.
extern int scancode_to_ascii_shift[SDLK_LAST];

/**
 * @brief
 *  Initialize the scancode translation.
 */
void scancode_begin();
