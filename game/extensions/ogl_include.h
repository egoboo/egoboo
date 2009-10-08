#pragma once

///
/// @file
/// @brief Basic OpenGL Wrapper
/// @details Basic definitions for using OpenGL in Egoboo

#include <SDL_opengl.h>

#ifdef __cplusplus
#    include <cassert>
#    include <cstdio>
extern "C"
{
#else
#    include <assert.h>
#    include <stdio.h>
#endif

#if defined(DEBUG_ATTRIB) && USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&xx); GL_DEBUG(glPushAttrib)(BITS); vfs_printf( stdout, "INFO: PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; GL_DEBUG(glPopAttrib)(); GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&xx); vfs_printf( stdout, "INFO: POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&XX); vfs_printf( stdout, "INFO: OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) { vfs_printf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); } else vfs_printf( stdout, "INFO: CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    GL_DEBUG(glPushAttrib)(BITS);
#    define ATTRIB_POP(TXT)           GL_DEBUG(glPopAttrib)();
#    define ATTRIB_GUARD_OPEN(XX)     { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { GL_DEBUG(glGetIntegerv)(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) { vfs_printf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); }  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    GL_DEBUG(glPushAttrib)(BITS);
#    define ATTRIB_POP(TXT)           GL_DEBUG(glPopAttrib)();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

//--------------------------------------------------------------------------------------------

#define INVALID_TX_ID  ( (GLuint) (~0) )

//--------------------------------------------------------------------------------------------

    enum { XX = 0, YY, ZZ, WW };
    enum { RR = 0, GG, BB, AA };
    enum { SS = 0, TT };

    typedef GLfloat GLXmatrix[16];
    typedef GLfloat GLXvector4f[4];
    typedef GLfloat GLXvector3f[3];
    typedef GLfloat GLXvector2f[2];

//--------------------------------------------------------------------------------------------
    struct oglx_vertex
    {
        GLXvector4f pos;
        GLXvector3f rt, up;
        GLfloat     dist;
        GLXvector4f col;
        GLuint      color; // should replace r,g,b,a and be called by glColor4ubv
        GLXvector2f tx;    // u and v in D3D I guess
    };

//--------------------------------------------------------------------------------------------
// generic OpenGL lighting struct
    struct s_oglx_light
    {
        GLXvector4f emission, diffuse, specular;
        float     shininess[1];
    };
    typedef struct s_oglx_light oglx_light_t;

//--------------------------------------------------------------------------------------------
    GLboolean handle_opengl_error( void );

    void oglx_ViewMatrix( GLXmatrix view,
                          const GLXvector3f from,      // camera location
                          const GLXvector3f at,        // camera look-at target
                          const GLXvector3f world_up,  // world’s up, usually 0, 0, 1
                          const GLfloat roll );      // clockwise roll around viewing direction, in radians

    void oglx_ProjectionMatrix( GLXmatrix proj,
                                const GLfloat near_plane,    // distance to near clipping plane
                                const GLfloat far_plane,     // distance to far clipping plane
                                const GLfloat fov );         // field of view angle, in radians

    FILE * set_ogl_include_stderr(FILE * pfile);

#ifdef __cplusplus
};
#endif

