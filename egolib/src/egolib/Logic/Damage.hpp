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
#pragma once

#include "egolib/Logic/StaticEnumDescriptor.hpp"
#include "egolib/Math/_Include.hpp"

/**
 * @brief
 *  The possible damage types.
 * @todo
 *  MH: This will become an enum class.
 */
enum DamageType : uint8_t
{
    /// A cutting type of attack.
    DAMAGE_SLASH = 0,
    /// A blunt type of attack.
    DAMAGE_CRUSH,
    /// A focused type of attack.
    DAMAGE_POKE,
    DAMAGE_HOLY,                             ///< (Most invert Holy damage )
    DAMAGE_EVIL,
    DAMAGE_FIRE,
    DAMAGE_ICE,
    DAMAGE_ZAP,
    DAMAGE_COUNT,

    DAMAGE_NONE = 255
};

/**
 * @brief
 *  Get if a damage type is a "physical" damage type.
 * @param damageType
 *  the damage type
 * @return
 *  @a true if the damage type is a "physical" damage type,
 *  @a false otherwise
 * @remark
 *  The damage types "crush", "poke" and "slash" are "physical" damage types.
 * @todo
 *  MH: The term "physical" does not quite nail it. May I recommend the term "kinetic"?
 *  For instance, fire or ice are quite "physical" entities, despite of their magical
 *  origin in most - but eventually not all - cases. For instance damage when touching
 *  a brazier or lava is certainly fire and not of magical origin.
 */
inline bool DamageType_isPhysical(DamageType damageType)
{
    return DAMAGE_CRUSH == damageType
        || DAMAGE_POKE == damageType
        || DAMAGE_SLASH == damageType;
}

/**
 * @brief
 *  Get the colour associated with a damage type.
 * @param damageType
 *  the damage type
 * @return
 *  the colour associated with the damage type
 */
inline const Ego::Math::Colour3f& DamageType_getColour(DamageType damageType)
{
    using Colour3f = Ego::Math::Colour3f;
    static const auto& zapColour = Colour3f::yellow();
    static const auto& fireColour = Colour3f::red();
    static const auto& evilColour = Colour3f::green();
    static const auto& holyColour = Colour3f::mauve();
    static const auto& iceColour = Colour3f::blue();
    /// @remark Physical damage types all map to the same colour "white".
    static const auto& pokeColour = Colour3f::white();
    static const auto& crushColour = Colour3f::white();
    static const auto& slashColour = Colour3f::white();
    switch (damageType)
    {
        case DAMAGE_ZAP:
            return zapColour;
        case DAMAGE_FIRE:
            return fireColour;
        case DAMAGE_EVIL:
            return evilColour;
        case DAMAGE_HOLY:
            return holyColour;
        case DAMAGE_ICE:
            return iceColour;
        case DAMAGE_POKE:
            return pokeColour;
        case DAMAGE_SLASH:
            return slashColour;
        case DAMAGE_CRUSH:
            return crushColour;
        default:
            throw std::runtime_error("unreachable code reached");
    };
}

namespace Ego
{
namespace Game
{
#if 0
/// Convert an enumeration value into a value of its underlying type.
template <typename Type>
typename std::enable_if<std::is_enum<Type>::value, typename std::underlying_type<Type>::type>::type
toUnderlying(Type value)
{
    return static_cast<std::underlying_type<Type>::type>(value);
}

// Information about the Ego::Game::DamageType enumeration.
template <>
struct Info<DamageType>
{
    static const std::array<DamageType, 8>& values()
    {
        static const std::array<DamageType, 8> _array =
            {
                DamageType::Crush,
                DamageType::Poke,
                DamageType::Holy,
                DamageType::Evil,
                DamageType::Fire,
                DamageType::Ice,
                DamageType::Zap,
            };
        return _array;
    }
    static const size_t size = 8;
};

/** 
 * @brief
 *  Get the descriptor for the damage type enumeration.
 * @return
 *  the descriptor
 */
const Ego::Script::EnumDescriptor<DamageType>& getDamageTypeDescriptor();

struct DamageInfo
{
private:
    /**
     * @brief
     *  The damage between "base" and "base + rand".
     *  Both "base" and "rand" must be non-negative.
     */
    IPair _amount;

    /**
     * @brief
     *  The type of damage dealt.
     */
    DamageType _type;

    /**
     * @brief
     *  The life drain as 8.8 fixed point.
     */
    UFP8_T _lifeDrain;

    /**
     * @brief
     *  The mana drain as 8.8f fixed point.
     */
    UFP8_T _manaDrain;

public:

    void setType(DamageType type)
    {
        _type = type;
    }

    DamageType getType() const
    {
        return _type;
    }

    void setLifeDrain(UFP8_T lifeDrain)
    {
        _lifeDrain = lifeDrain;
    }

    UFP8_T getLifeDrain() const
    {
        return _lifeDrain;
    }

    void setManaDrain(UFP8_T manaDrain)
    {
        _manaDrain = manaDrain;
    }

    UFP8_T getManaDrain() const
    {
        return _manaDrain;
    }

    void reset()
    {
        _type = DamageType::None;
        _amount.base = 0; _amount.rand = 0;
        _lifeDrain = 0;
        _manaDrain = 0;
    }

    int getBase() const
    {
        return _amount.base;
    }

    void setBase(int base)
    {
        if (base < 0)
        {
            throw std::invalid_argument("base damage amount can't be negative");
        }
        _amount.base = base;
    }

    void setRandom(int random)
    {
        if (random < 0)
        {
            throw std::invalid_argument("random damage amount can't be negative");
        }
        _amount.rand = random;
    }

    int getMin() const
    {
        return _amount.base;
    }

    int getMax() const
    {
        return _amount.base + _amount.rand;
    }

    const IPair& getRange() const
    {
        return _amount;
    }

};
#endif
} // namespace Game
} // namespace Ego
