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

/// @file egolib/_math.h
/// @details The name's pretty self explanatory, doncha think?

#pragma once

#include "egolib/typedef.h"
#include "egolib/Math/_Include.hpp"
#include "egolib/Log/_Include.hpp"

#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_debug.h"

//--------------------------------------------------------------------------------------------
// IEEE 32-BIT FLOATING POINT NUMBER FUNCTIONS
//--------------------------------------------------------------------------------------------


#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define FACE_RANDOM  Random::next<FACING_T>(std::numeric_limits<FACING_T>::max())

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// basic constants

/**
 * @brief
 *  Convert an angle from radians to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in "facing"
 */
inline FACING_T RAD_TO_FACING(float x) {
	// UINT16_MAX / (2 * PI).
	return x * 10430.378350470452724949566316381f;
}

/**
 * @brief
 *  Convert an angle "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$) to radians.
 * @param x
 *  the angle in facing
 * @return
 *  the angle in radians
 */
inline float FACING_TO_RAD(FACING_T facing) {
	// (2 * PI) / UINT16_MAX
	return facing * 0.000095873799242852576857380474343257f;
}

/**
 * @brief
 *  Convert an angle from turns to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in turns
 * @return
 *  the angle in "facing"
 */
inline FACING_T TurnsToFacing(float x) {
	return CLIP_TO_16BITS((int)(x * (float)0x00010000));
}

/**
 * @brief
 *  Convert an angle from "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$)) to turns.
 * @param x
 *	the angle in "facing"
 * @return
 *  the angle in turns
 */
inline float FacingToTurns(FACING_T x) {
	return ((float)CLIP_TO_16BITS(x) / (float)0x00010000);
}

/**
 * @brief
 *  Convert an angle from radians (\f$[0,2\pi]\f$) to turns (\f$[0,1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in turns
 */
inline float RadiansToTurns(float x) {
	return x * Ego::Math::invTwoPi<float>();
}

/**
 * @brief
 *  Convert an angle from turns (\f$[0,1]\f$) to radians (\f$[0,2\pi]\f$).
 * @param x
 *  the angle in turns
 * @return
 *  the angle in radians
 */
inline float TurnsToRadians(float x) {
	return x * Ego::Math::twoPi<float>();
}

//--------------------------------------------------------------------------------------------
// the lookup tables for sine and cosine

#define TRIG_TABLE_BITS   14
#define TRIG_TABLE_SIZE   (1<<TRIG_TABLE_BITS)
#define TRIG_TABLE_MASK   (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_OFFSET (TRIG_TABLE_SIZE>>2)

/// @note - Aaron uses two terms without much attention to their meaning
///         I think that we should use "face" or "facing" to mean the fill 16-bit value
///         and use "turn" to be the TRIG_TABLE_BITS-bit value

    extern float turntosin[TRIG_TABLE_SIZE];           ///< Convert TURN_T == FACING_T>>2...  to sine
    extern float turntocos[TRIG_TABLE_SIZE];           ///< Convert TURN_T == FACING_T>>2...  to cosine

/// pre defined directions
#define FACE_WEST    0x0000
#define FACE_NORTH   0x4000                                 ///< Character facings
#define FACE_EAST    0x8000
#define FACE_SOUTH   0xC000

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if !defined(SGN)
#    define SGN(X)  LAMBDA( 0 == (X), 0, LAMBDA( (X) > 0, 1, -1) )
#endif

#if !defined(SQR)
#    define SQR(A) ((A)*(A))
#endif

//--------------------------------------------------------------------------------------------
// FAST CONVERSIONS

#if !defined(INV_FF)
#   define INV_FF              0.003921568627450980392156862745098f
#endif

#if !defined(INV_0100)
#   define INV_0100            0.00390625f
#endif

#if !defined(INV_FFFF)
#   define INV_FFFF            0.000015259021896696421759365224689097f
#endif

#define FF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FF )

#define FFFF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FFFF )
#define FLOAT_TO_FFFF( V1 )  ( (int)((V1) * 0xFFFF) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// prototypes of other math functions

    void make_turntosin( void );

// conversion functions
    FACING_T vec_to_facing( const float dx, const float dy );
    void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
    int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
    void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const FRange num );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
