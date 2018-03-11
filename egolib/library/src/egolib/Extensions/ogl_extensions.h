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

/// @file egolib/Extensions/ogl_extensions.h
/// @brief Definitions for extended functions and variables for OpenGL

#pragma once

#include "egolib/platform.h"
#include <string>
#define GLEW_STATIC
#include <GL/glew.h>

//--------------------------------------------------------------------------------------------

namespace Ego {
namespace OpenGL {

struct Utilities
{
public:
    /// @brief Clear the OpenGL error flag.
    static void clearError();

    /// @brief If the OpenGL error flag is set, log a description of the error as a warning, and clear the error flag.
    /// @param raise if @a true, an std::runtime_error is raised if the OpenGL error flag wa set
    /// @return @a true if the OpenGL error flag was set, @a false otherwise
    static bool isError();

    static const std::string& toString(GLenum error);

};

struct PushAttrib
{
public:
    PushAttrib(GLbitfield bitfield);
    ~PushAttrib();
}; // struct PushAttrib

struct PushClientAttrib
{
public:
    PushClientAttrib(GLbitfield bitfield);
    ~PushClientAttrib();
}; // struct PushClientAttrib

} // namespace OpenGL
} // namespace Ego

//--------------------------------------------------------------------------------------------

struct oglx_caps_t
{
    static void report();
};

enum { XX = 0, YY, ZZ, WW };         ///< indices for x, y, z, and w coordinates in a 4-vector
enum { RR = 0, GG, BB, AA };         ///< indices for r, g, b, and alpha coordinates in a 4-color vector
enum { SS = 0, TT };                 ///< indices for s and t, 4-vector texture coordinate

typedef GLfloat GLXvector4f[4];      ///< generic 4-vector
typedef GLfloat GLXvector3f[3];      ///< generic 3-vector
typedef GLfloat GLXvector2f[2];      ///< generic 2-vector

                                     /// this macro is set to do nothing if USE_GL_DEBUG is not defined
#define GL_DEBUG(XX)   XX

                                     /// this macro is set to the normal glEnd() USE_GL_DEBUG is not defined
#define GL_DEBUG_END() glEnd();
