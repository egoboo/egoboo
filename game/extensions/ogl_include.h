#pragma once
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

/// @file extensions/ogl_include.h
/// @ingroup _ogl_extensions_
/// @brief Base definitions of the extensions to OpenGL
/// @details

#include <SDL_opengl.h>

#include <assert.h>
#include "file_common.h"

#if defined(DEBUG_ATTRIB) && defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&xx); GL_DEBUG(glPushAttrib)(BITS); vfs_printf( stdout, "INFO: PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; GL_DEBUG(glPopAttrib)(); GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&xx); vfs_printf( stdout, "INFO: POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&XX); vfs_printf( stdout, "INFO: OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) { vfs_printf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); } else vfs_printf( stdout, "INFO: CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    GL_DEBUG(glPushAttrib)(BITS);
#    define ATTRIB_POP(TXT)           GL_DEBUG(glPopAttrib)();
#    define ATTRIB_GUARD_OPEN(XX)     { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&YY); EGOBOO_ASSERTXX==YY); if(XX!=YY) { vfs_printf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); }  }
#else
#    define ATTRIB_PUSH(TXT, BITS)     /* { ogl_state_t attrib_begin, attrib_end; ogl_state_comp_t attrib_diff; oglx_grab_state(&attrib_begin); */ GL_DEBUG(glPushAttrib)(BITS);
#    define ATTRIB_POP(TXT)           GL_DEBUG(glPopAttrib)(); /* oglx_grab_state(&attrib_end); gl_comp_state(&attrib_diff, &attrib_begin, &attrib_end); } */
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

//--------------------------------------------------------------------------------------------

/// OpenGL compliant definition of an invalid texture binding
#define INVALID_GL_ID  ( (GLuint) (~0) )

//--------------------------------------------------------------------------------------------

enum { XX = 0, YY, ZZ, WW };         ///< indices for x, y, z, and w coordinates in a 4-vector
enum { RR = 0, GG, BB, AA };         ///< indices for r, g, b, and alpha coordinates in a 4-color vector
enum { SS = 0, TT };                 ///< indices for s and t, 4-vector texture coordinate

typedef GLfloat GLXmatrix[16];       ///< generic 4x4 matrix type
typedef GLfloat GLXvector4f[4];      ///< generic 4-vector
typedef GLfloat GLXvector3f[3];      ///< generic 3-vector
typedef GLfloat GLXvector2f[2];      ///< generic 2-vector

//--------------------------------------------------------------------------------------------
/// generic OpenGL vertex
struct oglx_vertex
{
    GLXvector4f pos;     ///< the position of the vertex
    GLXvector3f rt, up;  ///< 3-vectors for billboarding
    GLfloat     dist;    ///< a generic float parameter
    GLXvector4f col;     ///< r,g,b,a usinf glColor4fv
    GLuint      color;   ///< r,g,b,a using glColor4ubv
    GLXvector2f tx;      ///< s,t coorsinates for texture mapping glTexCoord2fv
};

//--------------------------------------------------------------------------------------------
/// generic OpenGL lighting struct
struct s_oglx_light
{
    GLXvector4f emission, diffuse, specular;
    float     shininess[1];
};
typedef struct s_oglx_light oglx_light_t;

//--------------------------------------------------------------------------------------------
GLboolean handle_opengl_error( void );

void oglx_ViewMatrix( GLXmatrix view,
                      const GLXvector3f from,      ///< @var camera location
                      const GLXvector3f at,        ///< @var camera look-at target
                      const GLXvector3f world_up,  ///< @var world’s up, usually 0, 0, 1
                      const GLfloat roll );        ///< @var clockwise roll around viewing direction, in radians

void oglx_ProjectionMatrix( GLXmatrix proj,
                            const GLfloat near_plane,    ///< @var distance to near clipping plane
                            const GLfloat far_plane,     ///< @var distance to far clipping plane
                            const GLfloat fov );         ///< @var field of view angle, in radians

/// Set the FILE that ogl_include will use to dump debugging information.
/// If not set, it will default to stderr.
FILE * set_ogl_include_stderr( FILE * pfile );
