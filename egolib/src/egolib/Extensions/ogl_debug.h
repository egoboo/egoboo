//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
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

/// @defgroup _ogl_extensions_ Extensions to OpenGL

/// @file egolib/Extensions/ogl_debug.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for the debugging extensions for OpenGL
/// @details

#pragma once

#include "egolib/Extensions/ogl_include.h"

/// this macro is set to do nothing if USE_GL_DEBUG is not defined
#define GL_DEBUG(XX)   XX
/// this macro is set to the normal glEnd() USE_GL_DEBUG is not defined
#define GL_DEBUG_END() glEnd();
