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

/// @file game/GameStates/LoadPlayerElement.cpp
/// @details Used by the menu system to handle the loading of player characters
/// @author Johan Jansen

#include "game/GameStates/LoadPlayerElement.hpp"
#include "egolib/Profiles/_Include.hpp"

LoadPlayerElement::LoadPlayerElement(std::shared_ptr<ObjectProfile> profile) :
    _name("*NONE*"),
    _profile(profile),
    _skinRef(profile->getSkinOverride()),
    _questLog(),
    _selectedByPlayer(-1),
    _isSelected(false)
{
    // load the quest info from "quest.txt" so we can determine the valid modules
    _questLog.loadFromFile(profile->getPathname());

    // load the chop data from "naming.txt" to generate the character name (kinda silly how it's done currently)
    RandomName randomName;
    randomName.loadFromFile(profile->getPathname() + "/naming.txt");
    
    // generate the name from the chop
    _name = randomName.generateRandomName();
}

bool LoadPlayerElement::hasQuest(const IDSZ2& idsz, const int requiredLevel)
{
    // find beaten quests or quests with proper level
    if (_questLog.isBeaten(idsz) || requiredLevel <= _questLog[idsz]) {
        return true;
    }

    return false;
}


const Ego::DeferredTexture& LoadPlayerElement::getIcon() const 
{
	return _profile->getIcon(_skinRef);
}

bool LoadPlayerElement::isSelected() const
{
    return _isSelected;
}

void LoadPlayerElement::setSelected(bool selected)
{
    _isSelected = selected;
}