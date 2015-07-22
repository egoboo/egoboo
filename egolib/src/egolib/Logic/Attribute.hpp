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
/// @file game/Logic/Attribute.hpp
/// @brief Type for Object attributes (Strength, Dexterity, etc.)
/// @author Johan Jansen

#pragma once

#include "egolib/Core/UnhandledSwitchCaseException.hpp"

namespace Ego
{
namespace Attribute
{
    enum AttributeType : uint8_t
    {
        //Might = increases Life, melee damage, carry capacity, block skill, reduces movement penality for heavy armor, throw distance, brute weapon damage
        MIGHT,            //Strength
    
        //Agility = increases movement speed, attack speed, jump height, ranged aim, agile weapon damage, ranged weapon damage
        AGILITY,          //Dexterity
    
        //Intellect = increases Max Mana, identify items, detect traps, increases XP gain
        INTELLECT,        //Intelligence

        //Spell Power = mana flow, improves spell aim, increases damage with spells
        SPELL_POWER,      //Mana Flow

        //Mana regeneration per second
        MANA_REGEN,       //Mana Regain

        //Life regeneration per second
        LIFE_REGEN,       //Life Regain

        //Maximum mana
        MAX_MANA,         //Max Mana

        //Maximum life
        MAX_LIFE,         //Max Life

        NR_OF_PRIMARY_ATTRIBUTES,   //These represent the primary base attributes of the character (Might, Agility, Intellect, etc.)

        MORPH,

        DAMAGE_TYPE,        //What kind of damage this Object deals with DamageTarget script function
        NUMBER_OF_JUMPS,    //Maximum number of jumps this character can do
        LIFE_BARCOLOR,
        MANA_BARCOLOR,

        SLASH_MODIFIER,
        CRUSH_MODIFIER,
        POKE_MODIFIER,
        HOLY_MODIFIER,
        EVIL_MODIFIER,
        FIRE_MODIFIER,
        ICE_MODIFIER,
        ZAP_MODIFIER,

        FLASHING_AND,       //Flashing graphical effect
        LIGHT_BLEND,        
        ALPHA_BLEND,        //Transparency
        SHEEN,              //Shininess effect
        FLY_TO_HEIGHT,      //Object levitates this high
        WALK_ON_WATER,      //Object walks on top of water
        CAN_SEE_INVISIBLE,  //Can see other invisible Objects?
        MISSILE_TREATMENT,  //Incoming projectiles are NORMAL, DEFLECTED or REFLECTED
        COST_FOR_EACH_MISSILE, //Mana cost for each projectile DEFLECTED or REFLECTED
        CHANNEL_LIFE,       //Can spend life as mana?
        DARKVISION,         //Can see in darkness?

        NR_OF_SET_ATTRIBUTES, //TODO

        JUMP_POWER,
        BUMP_DAMPEN,
        BOUNCINESS,
        DAMAGE_BONUS,        //Magical damage bonus added to all attack particles spawned by this Object
        SIZE,
        ACCELERATION,        //Acceleration

        RED_SHIFT,                        ///< Red shift
        GREEN_SHIFT,                      ///< Green shift
        BLUE_SHIFT,                       ///< Blue shift

        ///< Defensive attributes
        DEFENCE,
        SLASH_RESIST,
        CRUSH_RESIST,
        POKE_RESIST,
        EVIL_RESIST,
        HOLY_RESIST,
        FIRE_RESIST,
        ICE_RESIST,
        ZAP_RESIST,

        NR_OF_ATTRIBUTES            //Always last
    };


    inline std::string toString(const AttributeType type)
    {
        switch(type)
        {
            case MIGHT:       return "Might"; 
            case AGILITY:     return "Agility"; 
            case INTELLECT:   return "Intellect"; 
            case SPELL_POWER: return "Spell Power";
            case MANA_REGEN:  return "Mana Regeneration";
            case LIFE_REGEN:  return "Life Regeneration";
            case MAX_MANA:    return "Mana";
            case MAX_LIFE:    return "Life";

            default:
            case NR_OF_ATTRIBUTES: throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
        }
        throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}
}
