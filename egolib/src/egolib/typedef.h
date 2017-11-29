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
#include "egolib/Log/_Include.hpp"
#include "egolib/Ref.hpp"
#include <cstdint>
#include <limits>

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

    typedef uint32_t UFP8_T;
    typedef int32_t SFP8_T;

    /// fast version of V1 / 256
#   define UFP8_TO_UINT(V1)   ( ((unsigned)(V1)) >> 8 )
    /// signed version of V1 / 256

    /// fast version of V1 / 256
#   define UINT_TO_UFP8(V1)   ( ((unsigned)(V1)) << 8 )

    /// version of V1 / 256.0f
#   define FP8_TO_FLOAT(V1)   ( (float)(V1) * id::fraction<float, 1, 256>() )
    /// version of V1 * 256.0f
#   define FLOAT_TO_FP8(V1)   ( (uint32_t)((V1) * (float)(0x0100) ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// BIT FIELDS
    typedef uint32_t BIT_FIELD;                            ///< A big string supporting 32 bits

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

    template <typename Type>
    struct Pair {
        Type base, rand;

        Pair() :
            base(0), rand(0) {}

        Pair(const Pair<Type>& other) :
            base(other.base), rand(other.rand) {}

        Pair(const Type& base, const Type& rand) :
            base(base), rand(rand) {}

        const Pair<Type>& operator=(const Pair<Type>& other) {
            base = other.base;
            rand = other.rand;
            return *this;
        }

        bool operator==(const Pair<Type>& other) const {
            return base == other.base
                && rand == other.rand;
        }

        bool operator!=(const Pair<Type>& other) const {
            return base != other.base
                || rand != other.rand;
        }
    };

    /// Specifies a value between "base" and "base + rand"
    using IPair = Pair<int>;

    id::interval<float> pair_to_range(const IPair& source);
    IPair range_to_pair(const id::interval<float>& source);

//--------------------------------------------------------------------------------------------
// STRING
    typedef char STRING[256];

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

using ObjectRef = Ref<size_t, std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::max(), RefKind::Object>;
namespace std {
    template <>
    struct hash<ObjectRef> {
        size_t operator()(const ObjectRef& x) const {
            return hash<ObjectRef::ValueType>()(x.get());
        }
    };
}
Log::Entry& operator<<(Log::Entry& entry, const ObjectRef& ref);

DECLARE_REF(TEAM_REF);

using EnchantProfileRef = Ref<REF_T, 0, ENCHANTPROFILES_MAX, RefKind::EnchantProfile>;
DECLARE_REF(EVE_REF);
#define INVALID_EVE_REF (EnchantProfileRef::InvalidValue)
namespace std {
template <>
struct hash<EnchantProfileRef> {
    size_t operator()(const EnchantProfileRef& x) const {
        return hash<EnchantProfileRef::ValueType>()(x.get());
    }
};
}


using EnchantRef = Ref<REF_T, 0, ENCHANTS_MAX, RefKind::Enchant>;
DECLARE_REF(ENC_REF);

using PlayerRef = Ref<REF_T, 0, MAX_PLAYER, RefKind::Player>;
DECLARE_REF(PLA_REF);
#define INVALID_PLA_REF (PlayerRef::InvalidValue)

using ParticleProfileRef = Ref<REF_T, 0, MAX_PIP, RefKind::ParticleProfile>;
DECLARE_REF(PIP_REF);
#define INVALID_PIP_REF (ParticleProfileRef::InvalidValue)
namespace std {
template <>
struct hash<ParticleProfileRef> {
    size_t operator()(const ParticleProfileRef& x) const {
        return hash<ParticleProfileRef::ValueType>()(x.get());
    }
};
}
Log::Entry& operator<<(Log::Entry& entry, const ParticleProfileRef& ref);

using ParticleRef = Ref<size_t, std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::max(), RefKind::Particle>;
namespace std {
    template <>
    struct hash<ParticleRef> {
        size_t operator()(const ParticleRef& x) const {
            return hash<ParticleRef::ValueType>()(x.get());
        }
    };
}
Log::Entry& operator<<(Log::Entry& entry, const ParticleRef& ref);

DECLARE_REF(PASS_REF);

using ObjectProfileRef = Ref<REF_T, 0, OBJECTPROFILES_MAX, RefKind::ObjectProfile>;
namespace std {
    template <>
    struct hash<ObjectProfileRef> {
        size_t operator()(const ObjectProfileRef& x) const {
            return hash<ObjectProfileRef::ValueType>()(x.get());
        }
    };
}
Log::Entry& operator<<(Log::Entry& entry, const ObjectProfileRef& ref);

DECLARE_REF(PRO_REF);
#define INVALID_PRO_REF (ObjectProfileRef::InvalidValue)

using TextureRef = Ref<REF_T, 0, TEXTURES_MAX, RefKind::Texture>;
DECLARE_REF(TX_REF);

