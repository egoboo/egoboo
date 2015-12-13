#pragma once

#include "egolib/typedef.h"

/**
 * @brief
 *	Vertex as currently used by the graphics system.
 */
struct GLvertex
{
    GLvertex() :
        pos{0.0f, 0.0f, 0.0f, 0.0f},
        nrm{0.0f, 0.0f, 0.0f},
        env{0.0f, 0.0f},
        tex{0.0f, 0.0f},
        col{0.0f, 0.0f, 0.0f, 0.0f},
        color_dir(0) 
    {
        //ctor
    }
    
	GLfloat pos[4];
	GLfloat nrm[3];
	GLfloat env[2];
	GLfloat tex[2];

	GLfloat col[4];      ///< generic per-vertex lighting
	GLint   color_dir;   ///< "optimized" per-vertex directional lighting
};