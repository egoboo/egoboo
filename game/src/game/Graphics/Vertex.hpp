#pragma once

#include "egolib/typedef.h"

/**
 * @brief
 *	Vertex as currently used by the graphics system.
 */
struct GLvertex
{
	GLfloat pos[4];
	GLfloat nrm[3];
	GLfloat env[2];
	GLfloat tex[2];

	GLfloat col[4];      ///< generic per-vertex lighting
	GLint   color_dir;   ///< "optimized" per-vertex directional lighting
};