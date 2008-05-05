/* Egoboo - mathstuff.h
 * The name's pretty self explanatory, doncha think?
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _MATHSTUFF_H_
#define _MATHSTUFF_H_


/**> HEADER FILES <**/
//#include "egoboo.h"
#include <math.h>
#include "egobootypedef.h"

/**> MACROS <**/
#define _CNV(i,j) .v[4*i+j]
#define CopyMatrix( pMatrixSource, pMatrixDest ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( GLMATRIX ) )


/**> DATA STRUCTURES <**/
typedef struct glmatrix { float v[16]; } GLMATRIX;
typedef struct glvector { float x,y,z,w; } GLVECTOR;


/**> GLOBAL VARIABLES <**/
extern float                   turntosin[16384];           // Convert chrturn>>2...  to sine


/**> FUNCTION PROTOTYPES <**/
GLVECTOR vsub(GLVECTOR A, GLVECTOR B);
GLVECTOR Normalize(GLVECTOR vec);
GLVECTOR CrossProduct(GLVECTOR A, GLVECTOR B);
float DotProduct(GLVECTOR A, GLVECTOR B);
GLMATRIX IdentityMatrix(void);
GLMATRIX ZeroMatrix(void);
GLMATRIX MatrixMult(const GLMATRIX a, const GLMATRIX b);
GLMATRIX Translate(const float dx, const float dy, const float dz);
GLMATRIX RotateX(const float rads);
GLMATRIX RotateY(const float rads);
GLMATRIX RotateZ(const float rads);
GLMATRIX ScaleXYZ(const float sizex, const float sizey, const float sizez);
GLMATRIX ScaleXYZRotateXYZTranslate(const float sizex, const float sizey, const float sizez, unsigned short turnz, unsigned short turnx, unsigned short turny, float tx, float ty, float tz);
GLMATRIX FourPoints(float orix, float oriy, float oriz, float widx, float widy, float widz, float forx, float fory, float forz, float upx,  float upy,  float upz, float scale);
GLMATRIX ViewMatrix(const GLVECTOR from, const GLVECTOR at, const GLVECTOR world_up, const float roll);
GLMATRIX ProjectionMatrix(const float near_plane, const float far_plane, const float fov);
void	TransformVertices( GLMATRIX *pMatrix, GLVECTOR *pSourceV, GLVECTOR *pDestV, Uint32 pNumVertor );

#endif

