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

/// @file   egolib/Math/Matrix44.cpp
/// @brief  4x4 matrices.
/// @author Michael Heilmann et al.

#include "egolib/Math/Matrix44.hpp"
#include "egolib/_math.h"

void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(Matrix4f4f& DST, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate)
{
    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    DST(0,0) = scale[kX] * (cz * cy);
    DST(1,0) = scale[kX] * (cz * sy * sx + sz * cx);
    DST(2,0) = scale[kX] * (sz * sx - cz * sy * cx);
    DST(3,0) = 0.0f;

    DST(0,1) = scale[kY] * (-sz * cy);
    DST(1,1) = scale[kY] * (-sz * sy * sx + cz * cx);
    DST(2,1) = scale[kY] * (sz * sy * cx + cz * sx);
    DST(3,1) = 0.0f;

    DST(0,2) = scale[kZ] * (sy);
    DST(1,2) = scale[kZ] * (-cy * sx);
    DST(2,2) = scale[kZ] * (cy * cx);
    DST(3,2) = 0.0f;

    DST(0,3) = translate[kX];
    DST(1,3) = translate[kY];
    DST(2,3) = translate[kZ];
    DST(3,3) = 1.0f;
}

void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(Matrix4f4f& DST, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate)
{
    /// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

    DST(0,0) = scale[kX] * (cz * cy - sz * sy * sx);
    DST(1,0) = scale[kX] * (sz * cy + cz * sy * sx);
    DST(2,0) = scale[kX] * (-cx * sy);
    DST(3,0) = 0.0f;

    DST(0,1) = scale[kY] * (-sz * cx);
    DST(1,1) = scale[kY] * (cz * cx);
    DST(2,1) = scale[kY] * (sx);
    DST(3,1) = 0.0f;

    DST(0,2) = scale[kZ] * (cz * sy + sz * sx * cy);
    DST(1,2) = scale[kZ] * (sz * sy - cz * sx * cy);
    DST(2,2) = scale[kZ] * (cy * cx);
    DST(3,2) = 0.0f;

    DST(0,3) = translate[kX];
    DST(1,3) = translate[kY];
    DST(2,3) = translate[kZ];
    DST(3,3) = 1.0f;
}

void mat_FourPoints(Matrix4f4f& dst, const Vector3f& ori, const Vector3f& wid, const Vector3f& frw, const Vector3f& up, const float scale)
{
	Vector3f vWid = wid - ori;
	Vector3f vUp = up - ori;
	Vector3f vFor = frw - ori;

    vWid.normalize();
    vUp.normalize();
    vFor.normalize();

	dst(0, 0) = -scale * vWid[kX];  // HUK
	dst(1, 0) = -scale * vWid[kY];  // HUK
	dst(2, 0) = -scale * vWid[kZ];  // HUK
	dst(3, 0) = 0.0f;

	dst(0, 1) = scale * vFor[kX];
	dst(1, 1) = scale * vFor[kY];
	dst(2, 1) = scale * vFor[kZ];
	dst(3, 1) = 0.0f;

	dst(0, 2) = scale * vUp[kX];
	dst(1, 2) = scale * vUp[kY];
	dst(2, 2) = scale * vUp[kZ];
	dst(3, 2) = 0.0f;

	dst(0, 3) = ori[kX];
	dst(1, 3) = ori[kY];
	dst(2, 3) = ori[kZ];
	dst(3, 3) = 1.0f;
}

Vector3f mat_getTranslate(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 3), mat(1, 3), mat(2, 3));
}

Vector3f mat_getChrUp(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 2), mat(1, 2), mat(2, 2));
}

Vector3f mat_getChrForward(const Matrix4f4f& mat)
{
    return Vector3f(-mat(0, 0), -mat(1, 0), -mat(2, 0));
}

Vector3f mat_getChrRight(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 1), mat(1, 1), mat(2, 1));
}

bool mat_getCamUp(const Matrix4f4f& mat, Vector3f& up)
{
    up[kX] = mat(1,0);
    up[kY] = mat(1,1);
    up[kZ] = mat(1,2);

    return true;
}

bool mat_getCamRight(const Matrix4f4f& mat, Vector3f& right)
{
    right[kX] = -mat(0,0);
    right[kY] = -mat(0,1);
    right[kZ] = -mat(0,2);

    return true;
}

bool mat_getCamForward(const Matrix4f4f& mat, Vector3f& forward)
{
    forward[kX] = -mat(2,0);
    forward[kY] = -mat(2,1);
    forward[kZ] = -mat(2,2);

    return true;
}

void dump_matrix(const Matrix4f4f& a)
{
    for (size_t i = 0; i < 4; ++i)
    {
        printf("  ");

		for (size_t j = 0; j < 4; ++j)
        {
            printf("%f ", a(i,j));
        }
        printf("\n");
    }
}
