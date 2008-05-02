#pragma once

#include <SDL_opengl.h>
#include "egoboo_math.h"

#ifndef EGOBOO_CONFIG
#    error Somehow, egoboo_config.h is not included in this file...
#endif

#if defined(DEBUG_ATTRIB) && defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); glPushAttrib(BITS); log_info("PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; glPopAttrib(); glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); log_info("POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX); log_info("OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); else log_info("CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY);  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

typedef matrix_4x4 GLmatrix;
typedef vect4      GLvector;

typedef struct ogl_vertex_t
{
  GLvector pos;
  GLvector col;
  Uint32 color;  // should replace r,g,b,a and be called by glColor4ubv

  vect2 tx;      // u and v in D3D I guess
  vect3 nrm;
  vect3 up;
  vect3 rt;
} GLVertex;