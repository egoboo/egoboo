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
/**
 * @brief
 *  A Perk is a special advantage, ability or intrinsic (see http://www.roguebasin.com/index.php?title=Intrinsics)
 *  Perks are gained by achieving experience levels
 * @author
 *  Johan Jansen
 */
#pragma once

#include "egolib/Renderer/DeferredOpenGLTexture.hpp"
#include "egolib/Logic/Attribute.hpp"
#include "egolib/Math/Colour4f.hpp"

namespace Ego
{
namespace Perks
{

enum PerkID : uint8_t
{
    //Might perks
    TOUGHNESS,
    WEAPON_PROFICIENCY,
    GIGANTISM,
    SOLDIERS_FORTITUDE,
    BRUTE,
    DEFENDER,
    MOBILE_DEFENCE,
    HOLD_THE_LINE,
    STALWART,
    JOUSTING,
    PERFECTION,
    ENDURANCE,
    ATHLETICS,
    ANCIENT_BLUD,
    BLUNT_WEAPONS_MASTERY,
    BRUTAL_STRIKE,
    POLEARM_MASTERY,
    SWORD_MASTERY,
    AXE_MASTERY,
    GRIM_REAPER,
    WOLVERINE,
    BERSERKER,

    //Intellect perks
    CARTOGRAPHY,
    SENSE_KURSES,
    KURSE_IMMUNITY,
    DRAGON_BLOOD,
    FAST_LEARNER,
    NIGHT_VISION,
    LITERACY,
    ARCANE_MAGIC,
    DIVINE_MAGIC,
    POISONRY,
    TRAP_LORE,
    USE_TECHNOLOGICAL_ITEMS,
    READ_GNOMISH,
    DANGER_SENSE,
    NAVIGATION,
    SENSE_UNDEAD,
    PERCEPTIVE,  
    SENSE_INVISIBLE,
    THAUMATURGY,
    WAND_MASTERY,
    ELEMENTAL_RESISTANCE,
    FIRE_WARD,
    ICE_WARD,
    ZAP_WARD,
    POWER,
    JACK_OF_ALL_TRADES,
    SORCERY,
    DISINTEGRATE,
    TELEPORT_MASTERY,
    SPELL_MASTERY,
    MYSTIC_INTELLECT,
    MEDITATION,
    BOOKWORM,
    MERCENARY,
    DARK_ARTS_MASTERY,
    MAGIC_ATTUNEMENT,
    CRUSADER,
    LORE_MASTER,
    TOO_SILLY_TO_DIE,
    //DARKVISION,
    
    //Agility perks
    ACROBATIC,
    MASTER_ACROBAT,
    SPRINT,
    DASH,
    MOBILITY,
    DODGE,
    MASTERFUL_DODGE,
    BACKSTAB,
    CRACKSHOT,
    SHARPSHOOTER,
    DEADLY_STRIKE,
    CRITICAL_HIT,
    LUCKY,
    QUICK_STRIKE,
    BOW_MASTERY,
    CROSSBOW_MASTERY,
    WHIP_MASTERY,
    DOUBLE_SHOT,
    IMPROVISED_WEAPONS,

    NR_OF_PERKS    //Always last
};

class Perk
{
public:

    /**
    * @brief
    *   Default constructor initializes to invalid perk
    **/
    Perk();

    const std::string& getName() const;

    const std::string& getDescription() const;

    PerkID getRequirement() const;

    const Ego::DeferredOpenGLTexture& getIcon() const;

    /**
    * @brief
    *   Get the theme colour associated with this Perk
    *   which depends on what attribute type it is linked with
    **/
    const Ego::Math::Colour4f& getColour() const;

    Ego::Attribute::AttributeType getType() const;

    PerkID getID() const;

private:
    PerkID _id;
    Ego::Attribute::AttributeType _perkType;
    std::string _name;
    std::string _description;
    PerkID _perkRequirement;
    Ego::DeferredOpenGLTexture _icon;

    friend class PerkHandler;
};

} //Perks
} //Ego
