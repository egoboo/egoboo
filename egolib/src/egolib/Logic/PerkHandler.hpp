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

#include "egolib/Logic/Perk.hpp"

namespace Ego
{
namespace Perks
{

class PerkHandler
{
public:
    const Perk& getPerk(const PerkID type) const;

    /**
    * @brief
    *   Converts a string into a PerkID
    * @return
    *   A PerkID of the specified Perk if successful, NR_OF_PERKS otherwise (eg. on parse failure)
    **/
    PerkID fromString(const std::string &name) const;

    static inline const PerkHandler& get() { return *_singleton.get(); }

private:
    /**
    * @brief
    *   Private constructor
    **/
    PerkHandler();

    void initializePerk(const PerkID id, const Ego::Attribute::AttributeType type, const std::string &iconPath, const std::string &name, const std::string &description, const PerkID perkRequirement = NR_OF_PERKS);

private:
    std::array<std::unique_ptr<Perk>, NR_OF_PERKS> _perkList;

    static std::unique_ptr<PerkHandler> _singleton;
};

} //Perks
} //Ego
