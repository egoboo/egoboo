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
/// @file egolib/matrix.h
/// @details matrices
#pragma once

#include "egolib/vec.h"

typedef float  fmat_4x4_base_t[16];      ///< the basic 4x4 single precision floating point ("float")  matrix type
#if 0
typedef double dmat_4x4_base_t[16];      ///< the basic 4x4 double precision floating point ("double") matrix type
#endif

struct s_fmat_4x4;
typedef struct s_fmat_4x4  fmat_4x4_t;

/// A wrapper for fmat_4x4_base_t.
/// Necessary in C so that the function return can be assigned to another matrix more simply.
struct s_fmat_4x4 { fmat_4x4_base_t  v; };

#if 0
// A wrapper for dmat_4x4_base_t.
/// Necessary in C so that the function return can be assigned to another matrix more simply.
struct s_dmat_4x4 { dmat_4x4_base_t v;  };
#endif

float *mat_Copy(fmat_4x4_base_t DST, const fmat_4x4_base_t src);
float *mat_Identity(fmat_4x4_base_t DST);
float *mat_Zero(fmat_4x4_base_t DST);
float *mat_Multiply(fmat_4x4_base_t DST, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2);
float *mat_Translate(fmat_4x4_base_t DST, const float dx, const float dy, const float dz);
float *mat_RotateX(fmat_4x4_base_t DST, const float rads);
float *mat_RotateY(fmat_4x4_base_t DST, const float rads);
float *mat_RotateZ(fmat_4x4_base_t DST, const float rads);
float *mat_ScaleXYZ(fmat_4x4_base_t DST, const float sizex, const float sizey, const float sizez);
float *mat_FourPoints(fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale);
float *mat_View(fmat_4x4_base_t DST, const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll);
float *mat_Projection(fmat_4x4_base_t DST, const float near_plane, const float far_plane, const float fov, const float ar);
float *mat_Projection_orig(fmat_4x4_base_t DST, const float near_plane, const float far_plane, const float fov);
void   mat_TransformVertices(const fmat_4x4_base_t Matrix, const fvec4_t pSourceV[], fvec4_t pDestV[], const Uint32 NumVertor);

bool   mat_getChrUp(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getChrForward(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getChrRight(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getCamUp(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getCamRight(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getCamForward(const fmat_4x4_base_t mat, fvec3_base_t vec);
bool   mat_getTranslate(const fmat_4x4_base_t mat, fvec3_base_t vec);
float *mat_getTranslate_v(const fmat_4x4_base_t mat);

float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);
float *mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(fmat_4x4_base_t mat, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z);
