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
#include "egolib/Debug.hpp"

//--------------------------------------------------------------------------------------------
// place the definition of the lambda operator in a macro
#define LAMBDA(AA,BB,CC) ((AA) ? (BB) : (CC))

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
#   define FP8_TO_FLOAT(V1)   ( (float)(V1) * INV_0100<float>() )
    /// version of V1 * 256.0f
#   define FLOAT_TO_FP8(V1)   ( (Uint32)((V1) * (float)(0x0100) ) )

#   define FP8_MUL(V1, V2)    ( ((V1)*(V2)) >> 8 )               ///< this may overflow if V1 or V2 have non-zero bits in their upper 8 bits
#   define FP8_DIV(V1, V2)    ( ((V1)<<8) / (V2) )               ///< this  will fail if V1 has bits in the upper 8 bits

//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// 16.16 fixed point types

    typedef Uint32 UFP16_T;
    typedef Sint32 SFP16_T;

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


//--------------------------------------------------------------------------------------------
// STRING
    typedef char STRING[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// References

/// @brief The base reference type, an unsigned integer value.
typedef uint16_t REF_T;

/**
 * @brief
 *	Macro declaring a new reference type.
 * @param name
 *	the name of the reference type
 */
#define DECLARE_REF(name) using name = REF_T

/**
 * @brief
 *	Convert a reference or derived value to a reference value.
 * @param ref
 *	the reference or derived value
 * @todo
 *	Rename to TO_REF.
 */
#define REF_TO_INT(ref) ((REF_T)(ref))

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
    /**
     * @brief
     *  A "globally unique" (lol) identifier.
     */
    using Guid = uint32_t;
    /**
     * @brief
     *  An invalid globally unique identifier.
     */
    static constexpr Guid InvalidGuid() {
        return std::numeric_limits<Guid>::max();
    }
	
	Guid updateGuid;
	
	int count; ///< @todo Rename to size. @todo Should be of type @a size_t.
	
	ElementType lst[Capacity];
	
	Stack() : updateGuid(InvalidGuid), count(0)
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
	 *  Get a reference to the stack element at the specified index.
	 * @param index
	 *  the index
	 * @return
	 *  a reference to the stack element if @a index is within bounds
	 * @throw Id::RuntimeErrorException
	 *  if @a index is out of bounds
	 */
	ElementType& get_ref(size_t index)
	{
		if (index >= Capacity) throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		return lst[index];
	}

	/**
	 * @brief
	 *	Get a pointer to the stack element at the specified index.
	 * @param index
	 *	the index
	 * @return
	 *	a pointer to the stack element if @a index is within bounds, a null pointer otherwise
	 * @todo
	 *	Raise an exception of @a index is greater than or equal to @a capacity.
	 */
	ElementType *get_ptr(size_t index)
	{
		if (index >= Capacity) throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		return &(lst[index]);
	}

};

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

using ObjectRef = Ref<size_t, std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), RefKind::Object>;
namespace std {
    template <>
    struct hash<ObjectRef> {
        size_t operator()(const ObjectRef& x) const {
            return hash<ObjectRef::Type>()(x.get());
        }
    };
}

DECLARE_REF(TEAM_REF);

using EnchantProfileRef = Ref<REF_T, 0, ENCHANTPROFILES_MAX, ENCHANTPROFILES_MAX, RefKind::EnchantProfile>;
DECLARE_REF(EVE_REF);
#define INVALID_EVE_REF ((EVE_REF)ENCHANTPROFILES_MAX)

using EnchantRef = Ref<REF_T, 0, ENCHANTS_MAX, ENCHANTS_MAX, RefKind::Enchant>;
DECLARE_REF(ENC_REF);
#define INVALID_ENC_REF ((ENC_REF)ENCHANTS_MAX)

using PlayerRef = Ref<REF_T, 0, MAX_PLAYER, MAX_PLAYER, RefKind::Player>;
DECLARE_REF(PLA_REF);
#define INVALID_PLA_REF ((PLA_REF)MAX_PLAYER)

using ParticleProfileRef = Ref<REF_T, 0, MAX_PIP, MAX_PIP, RefKind::ParticleProfile>;
DECLARE_REF(PIP_REF);
#define INVALID_PIP_REF ((PIP_REF)MAX_PIP)

using ParticleRef = Ref<size_t, std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max(), RefKind::Particle>;
namespace std {
    template <>
    struct hash<ParticleRef> {
        size_t operator()(const ParticleRef& x) const {
            return hash<ParticleRef::Type>()(x.get());
        }
    };
}

DECLARE_REF(PASS_REF);

using ObjectProfileRef = Ref<REF_T, 0, OBJECTPROFILES_MAX, OBJECTPROFILES_MAX, RefKind::ObjectProfile>;
namespace std {
    template <>
    struct hash<ObjectProfileRef> {
        size_t operator()(const ObjectProfileRef& x) const {
            return hash<ObjectProfileRef::Type>()(x.get());
        }
    };
}
DECLARE_REF(PRO_REF);
#define INVALID_PRO_REF ((PRO_REF)OBJECTPROFILES_MAX)

using TextureRef = Ref<REF_T, 0, TEXTURES_MAX, TEXTURES_MAX, RefKind::Texture>;
DECLARE_REF(TX_REF);
#define INVALID_TX_REF ((TX_REF)TEXTURES_MAX)
