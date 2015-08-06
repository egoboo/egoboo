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

/// @file    egolib/egolib_typedef.h
/// @brief   Basic types used throughout the code.
/// @details Some basic types that are used throughout the game code.

#pragma once

// this include must be the absolute last include
#include "egolib/egolib_config.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
// place the definition of the lambda operator in a macro
#define LAMBDA(AA,BB,CC) ((AA) ? (BB) : (CC))

//--------------------------------------------------------------------------------------------
// portable definition of assert. the c++ version can be activated below.
// make assert into a warning if _DEBUG is not defined
//void non_fatal_assert( bool val, const char * format, ... ) GCC_PRINTF_FUNC( 2 );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// definitions for the compiler environment

#if defined(_DEBUG)
#define EGOBOO_ASSERT(expression) assert(expression)
#else
#define EGOBOO_ASSERT(expression) //non_fatal_assert(expression, "%s - failed an assert \"%s\"\n", __FUNCTION__, #expression)
#endif



//--------------------------------------------------------------------------------------------
// a replacement for memset()

#    if !defined(BLANK_STRUCT_PTR)
#       define BLANK_STRUCT_PTR(XX)  memset( XX, 0, sizeof( *(XX) ) );
#    endif

#    if !defined(BLANK_ARY)
#       define BLANK_ARY(XX)  memset( XX, 0, sizeof( XX ) );
#    endif

//--------------------------------------------------------------------------------------------
// BOOLEAN

#if !defined(TO_EGO_BOOL)
	#if defined(__cplusplus)
		#define TO_C_BOOL(VAL)   LAMBDA(VAL, true, false)
	#else
		#define TO_C_BOOL(VAL) (VAL)
	#endif
#endif

//--------------------------------------------------------------------------------------------

	/**
	 * @brief
	 *	Special return values.
	 * @todo
	 *	Rename to Ego::Result.
	 */
    enum egolib_rv
    {
        rv_error   = -1,
        rv_fail    = false,
        rv_success = true
    };

#define EGO_ANIMATION_FRAMERATE_SCALING 1
#define EGO_ANIMATION_MULTIPLIER 255

//--------------------------------------------------------------------------------------------
// 24.8 fixed point types

    typedef Uint32 UFP8_T;
    typedef Sint32 SFP8_T;

    /// fast version of V1 / 256
#   define UFP8_TO_UINT(V1)   ( ((unsigned)(V1)) >> 8 )
    /// signed version of V1 / 256

template<typename T>
signed SFP8_TO_SINT(const T& val)
{
	if(std::is_signed<T>::value && val < 0) {
		return - static_cast<signed>(UFP8_TO_UINT(-val));
	}
	return static_cast<signed>(UFP8_TO_UINT(val));
}

    /// fast version of V1 / 256
#   define UINT_TO_UFP8(V1)   ( ((unsigned)(V1)) << 8 )
    /// signed version of V1 / 256
#   define SINT_TO_SFP8(V1)   LAMBDA( (V1) < 0, -((signed)UINT_TO_UFP8(-V1)), (signed)UINT_TO_UFP8(V1) )

    /// version of V1 / 256.0f
#   define FP8_TO_FLOAT(V1)   ( (float)(V1) * INV_0100 )
    /// version of V1 * 256.0f
#   define FLOAT_TO_FP8(V1)   ( (Uint32)((V1) * (float)(0x0100) ) )

#   define FP8_MUL(V1, V2)    ( ((V1)*(V2)) >> 8 )               ///< this may overflow if V1 or V2 have non-zero bits in their upper 8 bits
#   define FP8_DIV(V1, V2)    ( ((V1)<<8) / (V2) )               ///< this  will fail if V1 has bits in the upper 8 bits

//--------------------------------------------------------------------------------------------
/// the type for the 16-bit value used to store angles
typedef Uint16 FACING_T;

    /// the type for the 14-bit value used to store angles
    typedef FACING_T TURN_T;

#   define TO_FACING(X) ((FACING_T)(X))
#   define TO_TURN(X)   ((TURN_T)((TO_FACING(X)>>2) & TRIG_TABLE_MASK))

//--------------------------------------------------------------------------------------------
// 16.16 fixed point types

    typedef Uint32 UFP16_T;
    typedef Sint32 SFP16_T;

#   define FLOAT_TO_FP16( V1 )  ( (Uint32)((V1) * 0x00010000) )
#   define FP16_TO_FLOAT( V1 )  ( (float )((V1) * 0.0000152587890625f ) )

//--------------------------------------------------------------------------------------------
// BIT FIELDS
    typedef Uint32 BIT_FIELD;                              ///< A big string supporting 32 bits

#   define FULL_BIT_FIELD      0x7FFFFFFF                  ///< A bit string where all bits are flagged as 1
#   define EMPTY_BIT_FIELD     0                           ///< A bit string where all bits are flagged as 0
#   define FILL_BIT_FIELD(XX)  (XX) = FULL_BIT_FIELD       ///< Fills up all bits in a bit pattern
#   define RESET_BIT_FIELD(XX) (XX) = EMPTY_BIT_FIELD      ///< Resets all bits in a BIT_FIELD to 0

#    if !defined(SET_BIT)
#       define SET_BIT(XX, YY) (XX) |= (YY)
#    endif

#    if !defined(UNSET_BIT)
#       define UNSET_BIT(XX, YY) (XX) &= ~(YY)
#    endif

#    if !defined(BOOL_TO_BIT)
#       define BOOL_TO_BIT(XX)       LAMBDA(XX, 1, 0 )
#    endif

#    if !defined(BIT_TO_BOOL)
#       define BIT_TO_BOOL(XX)       (1 == (XX))
#    endif

#    if !defined(HAS_SOME_BITS)
#       define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#    endif

#    if !defined(HAS_ALL_BITS)
#       define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#    endif

#    if !defined(HAS_NO_BITS)
#       define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#    endif

#    if !defined(MISSING_BITS)
#       define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))
#    endif

#   define CLIP_TO_08BITS( V1 )  ( (V1) & 0xFF       )
#   define CLIP_TO_16BITS( V1 )  ( (V1) & 0xFFFF     )
#   define CLIP_TO_24BITS( V1 )  ( (V1) & 0xFFFFFF   )
#   define CLIP_TO_32BITS( V1 )  ( (V1) & 0xFFFFFFFF )

//--------------------------------------------------------------------------------------------
// RECTANGLE

	namespace Ego
	{
		/**
		 * @brief
		 *	A rectangle in a 2 dimensional Cartesian coordinate system
		 *  (positive x-axis from left to right, positive y-axis from top to bottom).
		 */
		template <typename Type>
		struct Rectangle
		{
			Type _left;   ///< @brief The coordinate of the left side   of the rectangle.
			              ///< @invariant <tt>left <= right</tt>.
			Type _bottom; ///< @brief The coordinate of the bottom side of the rectangle.
			              ///< @invariant <tt>top <= bottom</tt>.
			Type _right;  ///< @brief The coordinate of the right side  of the rectangle.
			Type _top;    ///< @brief The coordinate of the top side    of the rectangle.

			/**
			 * @brief
			 *	Construct an empty rectangle.
			 */
			Rectangle() : 
				_left{}, 
				_bottom{}, 
				_right{}, 
				_top{}
			{
			}

			/**
			 * @brief
			 *	Get left coordinate of this rectangle.
			 * @return
			 *	the left coordinate of this rectangle
			 */
			inline Type getLeft() const { return _left; }

			/**
			 * @brief
			 *	Get top coordinate of this rectangle.
			 * @return
			 *	the top coordinate of this rectangle
			 */
			inline Type getTop() const { return _top; }

			/**
			 * @brief
			 *	Get right coordinate of this rectangle.
			 * @return
			 *	the right coordinate of this rectangle
			 */
			inline Type getRight() const { return _right; }

			/**
			 * @brief
			 *	Get bottom coordinate of this rectangle.
			 * @return
			 *	the bottom coordinate of this rectangle
			 */
			inline Type getBottom() const { return _bottom; }

			/**
			 * @brief
			 *	Construct this rectangle with the specified sides.
			 * @param left
			 *	the coordinate of the left side
			 * @param bottom
			 *	the coordinate of the bottom side
			 * @param right
			 *	the coordinate of the right side
			 * @param top
			 *	the coordinate of the top side
			 * @throws std::domain_error
			 *	if <tt>left > right</tt> or <tt>bottom > top</tt>
			 */
			Rectangle(const Type& left, const Type& bottom, const Type& right, const Type& top) : 
				_left(left), 
				_bottom(bottom), 
				_right(right), 
				_top(top)
			{
				if (!(_left <= _right))
				{
					throw std::domain_error("the coordinate of the left side must be smaller than or equal to the coordinate of the right side");
				}
				if (!(_top <= _bottom))
				{
					throw std::domain_error("the coordinate of the top side must be smaller than or equal to the coordinate of the bottom side");
				}
			}

			bool point_inside(const Type& x, const Type& y) const
			{
				EGOBOO_ASSERT(_left <= _right && _top <= _bottom);
				if (x < _left || x > _right) return false;
				if (y < _top  || y > _bottom) return false;
				return true;
			}
		};
	};
	/** @todo Remove this. */
	typedef Ego::Rectangle<int> irect_t;
	/** @todo Remove this. */
	typedef Ego::Rectangle<float> frect_t;

	struct ego_irect_t
    {
        int xmin, ymin;
        int xmax, ymax;
    };

    struct ego_frect_t
    {
        float xmin, ymin;
        float xmax, ymax;
    };

//--------------------------------------------------------------------------------------------
// PAIR AND RANGE

    /// Specifies a value between "base" and "base + rand"
    struct IPair
    {
        int base, rand;

        IPair() :
            base(0), rand(0)
        {}

        IPair(const IPair& other) :
            base(other.base), rand(other.rand)
        {}

        IPair(int _base, int _rand) :
            base(_base), rand(_rand)
        {}

        /// @todo Rename to "reset".
        void init()
        {
            base = 0;
            rand = 0;
        }
    };

    /// Specifies a value from "from" to "to"
    struct FRange
    {
        float from, to;

        FRange() :
            from(0), to(0)
        {}
        
        FRange(const FRange& other) :
            from(other.from), to(other.to)
        {}
        
        FRange(float _from, float _to) :
            from(_from), to(_to)
        {}

        /// @todo Rename to "reset".
        void init()
        {
            from = 0.0f;
            to = 0.0f;
        }

        bool isZero() const
        {
            return (from+to) <= std::numeric_limits<float>::epsilon();
        }

    };

    void pair_to_range( IPair pair, FRange * prange );
    void range_to_pair( FRange range, IPair * ppair );

    void ints_to_range( int base, int rand, FRange * prange );
    void floats_to_pair( float vmin, float vmax, IPair * ppair );

//--------------------------------------------------------------------------------------------
// IDSZ
typedef Uint32 IDSZ;

#if !defined(MAKE_IDSZ)
#define MAKE_IDSZ(C0,C1,C2,C3)                 \
    ((IDSZ)(                                   \
             ((((C0)-'A')&0x1F) << 15) |       \
             ((((C1)-'A')&0x1F) << 10) |       \
             ((((C2)-'A')&0x1F) <<  5) |       \
             ((((C3)-'A')&0x1F) <<  0)         \
           ))
#endif

#define IDSZ_NONE MAKE_IDSZ('N','O','N','E')       ///< [NONE]
/**
 * @brief
 *	Convert an integer IDSZ to a text IDSZ.
 * @param idsz
 *	the integer IDSZ
 * @return
 *	a pointer to a text IDSZ
 * @todo
 *	This currently uses a static bufer. Change this.
 */
const char *undo_idsz(IDSZ idsz);

//--------------------------------------------------------------------------------------------
// STRING
    typedef char STRING[256];

//--------------------------------------------------------------------------------------------

/// the "base class" of Egoboo profiles
#   define  EGO_PROFILE_STUFF \
    bool loaded;                    /** Was the data read in? */ \
    STRING name;                    /** Usually the source filename */ \
    int    request_count;           /** the number of attempted spawnx */ \
    int    create_count;            /** the number of successful spawns */

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// References

/// @brief The base reference type, an unsigned integer value.
typedef Uint16 REF_T;

/**
 * @brief
 *	Macro declaring a new reference type.
 * @param name
 *	the name of the reference type
 */
#define DECLARE_REF(name) typedef REF_T name

/**
 * @brief
 *	Convert a reference or derived value to a reference value.
 * @param ref
 *	the reference or derived value
 * @todo
 *	Rename to TO_REF.
 */
#define REF_TO_INT(ref) ((REF_T)(ref))

namespace Ego
{
    /**
     * @brief
     *  A "globally unique" (lol) identifier.
     */
    typedef uint32_t GUID;
    /**
     * @brief
     *  An invalid globally unique identifier.
     */
    #define EGO_GUID_INVALID (~((uint32_t)0))
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	A stack with a fixed capacity specified at compile-time (cfg. std::array).
 * @todo
 *	Rename to @a StaticStack.
 */
template <typename ElementType,size_t Capacity>
struct Stack
{
	
	Ego::GUID update_guid;
	
	int count; ///< @todo Rename to size. @todo Should be of type @a size_t.
	
	ElementType lst[Capacity];
	
	Stack() : update_guid(EGO_GUID_INVALID), count(0)
	{
	}

	/**
	 * @brief
	 *	Get the size of this stack.
	 * @return
	 *	the size of this stack
	 */
	inline size_t get_size() const
	{
		return count;
	}

	/**
	 * @brief
	 *	Get the capacity of this stack.
	 * @return
	 *	the capacity of this stack
	 */
	inline size_t get_capacity() const
	{
		return Capacity;
	}

	/**
	 * @brief
	 *	Get a pointer to the stack element at the specified index.
	 * @param index
	 *	the index
	 * @return
	 *	a pointer to the stack element if @a index is within bounds, @a false otherwise
	 * @todo
	 *	Raise an exception of @a index is greater than or equal to @a capacity.
	 */
	ElementType *get_ptr(size_t index)
	{
		return (index >= Capacity) ? NULL : lst + index;
	}

};

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	An array with a fixed capacity specified at compile-time (cfg. std::array).
 * @todo
 *	Merge with Stack.
 */
template <typename ElementType,size_t Capacity>
struct StaticArray
{
	int count; ///< @todo Rename to size. @todo Should be of type @a size_t.
	ElementType ary[Capacity];
	StaticArray() : count(0)
	{
	}
	ElementType *get_ptr(size_t index)
	{
		return (index >= Capacity) ? NULL : this->ary + index;
	}
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some common data types and enums in egoboo

typedef uint16_t SKIN_T;

/**
 * @brief
 *  Enumeration of "damage modifications" i.e. cancellation or redirections of incoming damage.
 * @todo
 *  Rename to "damage redirections".
 * @todo
 *  It would be perfectly possible to simplify things here in terms of "redirection" and "inversion".
 *  There are three redirections: void, mana and life which redirects damage nowhere (making the
 *  target immune) or redirects damage to mana and/or life. On the second stage we determine if
 *  the damage increases/decreases mana or life by specifying an inversion or identity operation.
 *  E.g. to make damage decrease the mana simply <tt>redirect(Mana)</tt>. To make damage heal
 *  <tt>inverse . redirect(Life)</tt> and so on.
 */
enum DamageModifier : uint8_t
{
    DAMAGEINVICTUS = (1 << 5),  ///< 00x00000 Invictus to this type of damage.
    DAMAGEMANA = (1 << 4),      ///< 000x0000 Deals damage to mana.
    DAMAGECHARGE = (1 << 3),    ///< 0000x000 Converts damage to mana.
    DAMAGEINVERT = (1 << 2),    ///< 00000x00 Makes damage heal.
    NONE = (0),                 ///< 00000000 Do no conversion.
};

#include "egolib/Ref.hpp"

typedef size_t CHR_REF;
#define INVALID_CHR_REF std::numeric_limits<CHR_REF>::max()

DECLARE_REF(TEAM_REF);

typedef Ref<REF_T, 0, ENCHANTPROFILES_MAX, ENCHANTPROFILES_MAX, RefKind::EnchantProfile> EnchantProfileRef;
DECLARE_REF(EVE_REF);
#define INVALID_EVE_REF ((EVE_REF)ENCHANTPROFILES_MAX)

typedef Ref<REF_T, 0, ENCHANTS_MAX, ENCHANTS_MAX, RefKind::Enchant> EnchantRef;
DECLARE_REF(ENC_REF);
#define INVALID_ENC_REF ((ENC_REF)ENCHANTS_MAX)

typedef Ref<REF_T, 0, MAX_PLAYER, MAX_PLAYER, RefKind::Player> PlayerRef;
DECLARE_REF(PLA_REF);
#define INVALID_PLA_REF ((PLA_REF)MAX_PLAYER)

typedef Ref<REF_T, 0, MAX_PIP, MAX_PIP, RefKind::ParticleProfile> ParticleProfileRef;
DECLARE_REF(PIP_REF);
#define INVALID_PIP_REF ((PIP_REF)MAX_PIP)

typedef size_t PRT_REF;
#define INVALID_PRT_REF std::numeric_limits<PRT_REF>::max()

DECLARE_REF(PASS_REF);

typedef Ref<REF_T, 0, OBJECTPROFILES_MAX, OBJECTPROFILES_MAX, RefKind::ObjectProfile> ObjectProfileRef;
DECLARE_REF(PRO_REF);
#define INVALID_PRO_REF ((PRO_REF)OBJECTPROFILES_MAX)

typedef Ref<REF_T, 0, TEXTURES_MAX, TEXTURES_MAX, RefKind::Texture> TextureRef;
DECLARE_REF(TX_REF);
#define INVALID_TX_REF ((TX_REF)TEXTURES_MAX)
