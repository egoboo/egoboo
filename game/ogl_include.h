#pragma once

#include <SDL_opengl.h>
#include "egoboo_math.h"

typedef matrix_4x4 GLmatrix;
typedef vect4      GLvector;

typedef struct ogl_vertex_t
{
  GLvector pos;
  GLvector col;
  Uint32 color; // should replace r,g,b,a and be called by glColor4ubv
  GLfloat s, t; // u and v in D3D I guess
  vect3 nrm;
  vect3 up;
  vect3 rt;
} GLVertex;