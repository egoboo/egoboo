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

/// @file    game/egoboo_typedef.h
/// @details Base type definitions and config options.

#pragma once

#include "egolib/egolib.h"

/**
 * @todo
 *	Remove this.
 */
typedef egolib_rv gfx_rv;
#define gfx_error rv_error
#define gfx_fail rv_fail
#define gfx_success rv_success

#include "game/egoboo_config.h"

