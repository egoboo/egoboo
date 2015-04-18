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

/// @file egolib/Extensions/ogl_debug.c
/// @ingroup _ogl_extensions_
/// @brief Implementation of the debugging extensions for OpenGL
/// @details

#include "egolib/Extensions/ogl_debug.h"
#include "egolib/file_common.h"
#include "egolib/log.h"

const char *next_cmd = NULL;
int next_line = -1;
const char *next_file = "BAD FILE";

void handle_gl_error()
{
    GLint err = glGetError();
    if (GL_NO_ERROR != err)
    {
        const GLubyte *err_str = gluErrorString(err);
        log_warning("%s (\"%s\" - %d)- %s\n", next_cmd, next_file, next_line, err_str);
    }
}

void print_gl_command()
{
    log_warning("%s (\"%s\" - %d)\n", next_cmd, next_file, next_line);
}
