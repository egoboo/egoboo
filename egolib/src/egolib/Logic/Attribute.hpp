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
        MANA_REGEN,       //Mana Regain
        LIFE_REGEN,       //Life Regain
        MAX_MANA,         //Max Mana
        MAX_LIFE,         //Max Life

        WISDOM,           //@TODO Deprecated

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

            case WISDOM:      return "Wisdom"; //TODO: remove

            case NR_OF_ATTRIBUTES: throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
        }
        throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}
}
