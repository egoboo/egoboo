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

#include "egolib/Math/_Include.hpp"
#include "egolib/integrations/color.hpp"

/**
 * @brief
 *  The possible damage types.
 * @todo
 *  MH: This will become an enum class.
 */
enum DamageType : uint8_t
{
    
    DAMAGE_SLASH = 0,                   /// A cutting type of attack.
    DAMAGE_CRUSH,                       /// A blunt type of attack.
    DAMAGE_POKE,                        /// A focused type of attack.
    DAMAGE_HOLY,                        /// Most invert Holy damage so that it heals
    DAMAGE_EVIL,                        /// Poisonous, necrotic, profane damage
    DAMAGE_FIRE,                        /// Damage by heat, fire, lava, etc.
    DAMAGE_ICE,                         /// Very cold damage, frost, ice water, etc.
    DAMAGE_ZAP,                         /// Electric shock damage
    DAMAGE_COUNT,

    DAMAGE_DIRECT = 0xFF                /// Special damage type (armor & resistance does not apply)
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
inline const Ego::Colour3f& DamageType_getColour(DamageType damageType)
{
    using Colour3f = Ego::Colour3f;
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
