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

#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Logic/Attribute.hpp"

namespace Ego
{
namespace Perks
{

enum PerkID : uint8_t
{
    //Might perks
    TOUGHNESS,

    //Intellect perks
    CARTOGRAPHY,
    SENSE_KURSES,
    KURSE_IMMUNITY,
    //LITERACY,
    //ARCANE_MAGIC,
    //LORE_MASTER,
    //NAVIGATION,
    //PERCEPTIVE,
    //DANGER_SENSE,
    //NIGHTVISION,
    //DARKVISION,
    //THIEVERY,
    //READ_GNOMISH
    //DIVINE_MAGIC
    //FAST_LEARNER

    //Agility perks
    ACROBATIC,

    NR_OF_PERKS    //Always last
};

class Perk
{
public:
    const std::string& getName() const;

    const std::string& getDescription() const;

    PerkID getRequirement() const;

private:
    PerkID _id;
    Ego::Attribute::AttributeType _perkType;
    std::string _name;
    std::string _description;
    PerkID _perkRequirement;
    std::unique_ptr<oglx_texture_t> _icon;

    /**
    * @brief
    *   Default constructor initializes to invalid perk
    **/
    Perk();

    friend class PerkHandler;
};

} //Perks
} //Ego
