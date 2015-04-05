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

/// @file  egolib/Math/Matrix44.cpp
/// @brief 4x4 matrices.

#include "egolib/Math/Matrix44.hpp"

//--------------------------------------------------------------------------------------------
float *mat_Zero(fmat_4x4_base_t DST)
{
	// initializes matrix to zero

	if (NULL == DST) return NULL;

	DST[MAT_IDX(0, 0)] = 0; DST[MAT_IDX(1, 0)] = 0; DST[MAT_IDX(2, 0)] = 0; DST[MAT_IDX(3, 0)] = 0;
	DST[MAT_IDX(0, 1)] = 0; DST[MAT_IDX(1, 1)] = 0; DST[MAT_IDX(2, 1)] = 0; DST[MAT_IDX(3, 1)] = 0;
	DST[MAT_IDX(0, 2)] = 0; DST[MAT_IDX(1, 2)] = 0; DST[MAT_IDX(2, 2)] = 0; DST[MAT_IDX(3, 2)] = 0;
	DST[MAT_IDX(0, 3)] = 0; DST[MAT_IDX(1, 3)] = 0; DST[MAT_IDX(2, 3)] = 0; DST[MAT_IDX(3, 3)] = 0;

	return DST;
}

//--------------------------------------------------------------------------------------------
float *mat_Multiply(fmat_4x4_base_t dst, const fmat_4x4_base_t src1, const fmat_4x4_base_t src2)
{
	if (NULL == mat_Zero(dst)) return NULL;

#if fmat_4x4_layout == fmat_4x4_layout_RowMajor
	/* \f$ C_{i,j} = \sum_{i=0}^4 A_{i,k} * B_{k,j}\f$ */
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			for (size_t k = 0; k < 4; ++k)
			{
				dst[MAT_IDX(i, j)] += src1[MAT_IDX(i, k)] * src2[MAT_IDX(k, j)];
			}
		}
	}
#else
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			for (size_t k = 0; k < 4; ++k)
			{
				dst[MAT_IDX(i, j)] += src1[MAT_IDX(k, j)] * src2[MAT_IDX(i, k)];
			}
		}
	}
#endif
	return dst;
}
//--------------------------------------------------------------------------------------------
float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(fmat_4x4_base_t DST, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z)
{
	float cx = turntocos[turn_x & TRIG_TABLE_MASK];
	float sx = turntosin[turn_x & TRIG_TABLE_MASK];
	float cy = turntocos[turn_y & TRIG_TABLE_MASK];
	float sy = turntosin[turn_y & TRIG_TABLE_MASK];
	float cz = turntocos[turn_z & TRIG_TABLE_MASK];
	float sz = turntosin[turn_z & TRIG_TABLE_MASK];

	if (NULL == DST) return NULL;

	DST[MAT_IDX(0, 0)] = scale_x * (cz * cy);
	DST[MAT_IDX(0, 1)] = scale_x * (cz * sy * sx + sz * cx);
	DST[MAT_IDX(0, 2)] = scale_x * (sz * sx - cz * sy * cx);
	DST[MAT_IDX(0, 3)] = 0.0f;

	DST[MAT_IDX(1, 0)] = scale_y * (-sz * cy);
	DST[MAT_IDX(1, 1)] = scale_y * (-sz * sy * sx + cz * cx);
	DST[MAT_IDX(1, 2)] = scale_y * (sz * sy * cx + cz * sx);
	DST[MAT_IDX(1, 3)] = 0.0f;

	DST[MAT_IDX(2, 0)] = scale_z * (sy);
	DST[MAT_IDX(2, 1)] = scale_z * (-cy * sx);
	DST[MAT_IDX(2, 2)] = scale_z * (cy * cx);
	DST[MAT_IDX(2, 3)] = 0.0f;

	DST[MAT_IDX(3, 0)] = translate_x;
	DST[MAT_IDX(3, 1)] = translate_y;
	DST[MAT_IDX(3, 2)] = translate_z;
	DST[MAT_IDX(3, 3)] = 1.0f;

	return DST;
}

//--------------------------------------------------------------------------------------------
float * mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(fmat_4x4_base_t DST, const float scale_x, const float scale_y, const float scale_z, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const float translate_x, const float translate_y, const float translate_z)
{
	/// @author BB
	/// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

	float cx = turntocos[turn_x & TRIG_TABLE_MASK];
	float sx = turntosin[turn_x & TRIG_TABLE_MASK];
	float cy = turntocos[turn_y & TRIG_TABLE_MASK];
	float sy = turntosin[turn_y & TRIG_TABLE_MASK];
	float cz = turntocos[turn_z & TRIG_TABLE_MASK];
	float sz = turntosin[turn_z & TRIG_TABLE_MASK];

	if (NULL == DST) return NULL;

	DST[MAT_IDX(0, 0)] = scale_x * (cz * cy - sz * sy * sx);
	DST[MAT_IDX(0, 1)] = scale_x * (sz * cy + cz * sy * sx);
	DST[MAT_IDX(0, 2)] = scale_x * (-cx * sy);
	DST[MAT_IDX(0, 3)] = 0.0f;

	DST[MAT_IDX(1, 0)] = scale_y * (-sz * cx);
	DST[MAT_IDX(1, 1)] = scale_y * (cz * cx);
	DST[MAT_IDX(1, 2)] = scale_y * (sx);
	DST[MAT_IDX(1, 3)] = 0.0f;

	DST[MAT_IDX(2, 0)] = scale_z * (cz * sy + sz * sx * cy);
	DST[MAT_IDX(2, 1)] = scale_z * (sz * sy - cz * sx * cy);
	DST[MAT_IDX(2, 2)] = scale_z * (cy * cx);
	DST[MAT_IDX(2, 3)] = 0.0f;

	DST[MAT_IDX(3, 0)] = translate_x;
	DST[MAT_IDX(3, 1)] = translate_y;
	DST[MAT_IDX(3, 2)] = translate_z;
	DST[MAT_IDX(3, 3)] = 1.0f;

	return DST;
}

//--------------------------------------------------------------------------------------------
float * mat_FourPoints(fmat_4x4_base_t DST, const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t up, const float scale)
{
	fvec3_t vWid, vFor, vUp;

	if (NULL == DST) return NULL;

	vWid.x = wid[kX] - ori[kX];
	vWid.y = wid[kY] - ori[kY];
	vWid.z = wid[kZ] - ori[kZ];

	vUp.x = up[kX] - ori[kX];
	vUp.y = up[kY] - ori[kY];
	vUp.z = up[kZ] - ori[kZ];

	vFor.x = frw[kX] - ori[kX];
	vFor.y = frw[kY] - ori[kY];
	vFor.z = frw[kZ] - ori[kZ];

	vWid.normalize();
	vUp.normalize();
	vFor.normalize();

	DST[MAT_IDX(0, 0)] = -scale * vWid.x;  // HUK
	DST[MAT_IDX(0, 1)] = -scale * vWid.y;  // HUK
	DST[MAT_IDX(0, 2)] = -scale * vWid.z;  // HUK
	DST[MAT_IDX(0, 3)] = 0.0f;

	DST[MAT_IDX(1, 0)] = scale * vFor.x;
	DST[MAT_IDX(1, 1)] = scale * vFor.y;
	DST[MAT_IDX(1, 2)] = scale * vFor.z;
	DST[MAT_IDX(1, 3)] = 0.0f;

	DST[MAT_IDX(2, 0)] = scale * vUp.x;
	DST[MAT_IDX(2, 1)] = scale * vUp.y;
	DST[MAT_IDX(2, 2)] = scale * vUp.z;
	DST[MAT_IDX(2, 3)] = 0.0f;

	DST[MAT_IDX(3, 0)] = ori[kX];
	DST[MAT_IDX(3, 1)] = ori[kY];
	DST[MAT_IDX(3, 2)] = ori[kZ];
	DST[MAT_IDX(3, 3)] = 1.0f;

	return DST;
}

//--------------------------------------------------------------------------------------------
void mat_View(fmat_4x4_t& DST,const fvec3_t& from,const fvec3_t& at,const fvec3_t& world_up,const float roll)
{
	fvec3_t up, right, view_dir, temp;

	DST = fmat_4x4_t::identity();
	view_dir = at - from;
	view_dir.normalize();
	right = world_up.cross(view_dir);
	up = view_dir.cross(right);
	right.normalize();
	up.normalize();

	// 0th row.
	DST(0, 0) = right.x;
	DST(0, 1) = right.y;
	DST(0, 2) = right.z;

	// 1st row.
	DST(1, 0) = up.x;
	DST(1, 1) = up.y;
	DST(1, 2) = up.z;

	// 2nd row.
	DST(2,0) = view_dir.x;
	DST(2,1) = view_dir.y;
	DST(2,2) = view_dir.z;

	// 3rd row.
	DST(3,0) = -right.dot(from);
	DST(3,1) = -up.dot(from);
	DST(3,2) = -view_dir.dot(from);

	if (roll != 0.0f)
	{
		// mat_Multiply function shown above
		fmat_4x4_t tmp1 = fmat_4x4_t::rotationZ(-roll), tmp2;
		tmp2 = DST;
		mat_Multiply(DST.v, tmp1.v, tmp2.v);
	}
}
//--------------------------------------------------------------------------------------------
bool mat_getTranslate(const fmat_4x4_t& mat, fvec3_t& translate)
{
	translate[kX] = mat.v[MAT_IDX(3, 0)];
	translate[kY] = mat.v[MAT_IDX(3, 1)];
	translate[kZ] = mat.v[MAT_IDX(3, 2)];

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getChrUp(const fmat_4x4_t& mat, fvec3_t& up)
{
	// for a character
	up[kX] = mat.v[MAT_IDX(2, 0)]; // m(0,2)
	up[kY] = mat.v[MAT_IDX(2, 1)]; // m(1,2)
	up[kZ] = mat.v[MAT_IDX(2, 2)]; // m(2,2)

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getChrForward(const fmat_4x4_t& mat, fvec3_t& forward)
{
	// for a character
	forward[kX] = -mat.v[MAT_IDX(0, 0)]; // -mat(0,0)
	forward[kY] = -mat.v[MAT_IDX(0, 1)]; // -mat(1,0)
	forward[kZ] = -mat.v[MAT_IDX(0, 2)]; // -mat(2,0)

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getChrRight(const fmat_4x4_t& mat, fvec3_t& right)
{
	// for a character's matrix
	right[kX] = mat.v[MAT_IDX(1, 0)]; // mat(0,1)
	right[kY] = mat.v[MAT_IDX(1, 1)]; // mat(1,1)
	right[kZ] = mat.v[MAT_IDX(1, 2)]; // mat(2,1)

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getCamUp(const fmat_4x4_t& mat, fvec3_t& up)
{
	// for the camera
	up[kX] = mat.v[MAT_IDX(0, 1)]; // m(1,0)
	up[kY] = mat.v[MAT_IDX(1, 1)]; // m(1,1)
	up[kZ] = mat.v[MAT_IDX(2, 1)]; // m(1,2)

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getCamRight(const fmat_4x4_t& mat, fvec3_t& right)
{
	// for the camera
	right[kX] = -mat.v[MAT_IDX(0, 0)]; // -m(0,0)
	right[kY] = -mat.v[MAT_IDX(1, 0)]; // -m(0,1)
	right[kZ] = -mat.v[MAT_IDX(2, 0)]; // -m(0,2)

	return true;
}

//--------------------------------------------------------------------------------------------
bool mat_getCamForward(const fmat_4x4_t& mat, fvec3_t& forward)
{
	// for the camera
	forward[kX] = -mat.v[MAT_IDX(0, 2)];
	forward[kY] = -mat.v[MAT_IDX(1, 2)];
	forward[kZ] = -mat.v[MAT_IDX(2, 2)];

	return true;
}

//--------------------------------------------------------------------------------------------
fvec3_t mat_getTranslate_v(const fmat_4x4_base_t mat)
{
	fvec3_t pos;

	pos.x = mat[MAT_IDX(3, 0)];
	pos.y = mat[MAT_IDX(3, 1)];
	pos.z = mat[MAT_IDX(3, 2)];

	return pos;
}

//--------------------------------------------------------------------------------------------

void mat_gluLookAt(fmat_4x4_t &dst, const fmat_4x4_t &src, const fvec3_t& eye, const fvec3_t& center, const fvec3_t& up)
{
    fvec3_t f = center - eye;
    f.normalize();
    fvec3_t up_ = up;
    up_.normalize();

    fvec3_t s = f.cross(up);
    s.normalize();
    
    fvec3_t u = s.cross(f);
    
    auto M = fmat_4x4_t
        (
         s.x,  s.y,  s.z, 0.0f,
         u.x,  u.y,  u.z, 0.0f,
        -f.x, -f.y, -f.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
        );
    
    dst = src * M * fmat_4x4_t::translation(-eye);
}

void mat_glRotate(fmat_4x4_t &dst, const fmat_4x4_t &src, const float angle, const fvec3_t& axis)
{
    fmat_4x4_t R;
    float s = std::sin(Ego::Math::degToRad(angle));
    float c = std::cos(Ego::Math::degToRad(angle));
    
    fvec3_t axis_ = axis;
    axis_.normalize();

    // First row.
    R(0, 0) = axis_.x * axis_.x * (1 - c) + c;
    R(0, 1) = axis_.x * axis_.y * (1 - c) - axis_.z * s;
    R(0, 2) = axis_.x * axis_.z * (1 - c) + axis_.y * s;
    R(0, 3) = 0.0f;
    
    // 2nd row.
    R(1, 0) = axis_.y * axis_.x * (1 - c) + axis_.z * s;
    R(1, 1) = axis_.y * axis_.y * (1 - c) + c;
    R(1, 2) = axis_.y * axis_.z * (1 - c) - axis_.x * s;
    R(1, 3) = 0.0f;
    
    // 3rd row.
    R(2, 0) = axis_.z * axis_.x * (1 - c) - axis_.y * s;
    R(2, 1) = axis_.z * axis_.y * (1 - c) + axis_.x * s;
    R(2, 2) = axis_.z * axis_.z * (1 - c) + c;
    R(2, 3) = 0.0f;
    
    R(3, 0) = 0;
    R(3, 1) = 0;
    R(3, 2) = 0;
    R(3, 3) = 1;
    
    dst = src * R;
}

void dump_matrix(const fmat_4x4_base_t a)
{
	if (NULL == a) return;

	for (size_t j = 0; j < 4; j++)
	{
		printf("  ");

		for (size_t i = 0; i < 4; i++)
		{
			printf("%f ", a[MAT_IDX(i, j)]);
		}
		printf("\n");
	}
}
