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
    _selectedByPlayer(-1),
    _inputDevice(INPUT_DEVICE_UNKNOWN),
    _isSelected(false)
{
    // load the quest info from "quest.txt" so we can determine the valid modules
    quest_log_download_vfs(_questLog, SDL_arraysize(_questLog), profile->getPathname().c_str());

    // load the chop data from "naming.txt" to generate the character name (kinda silly how it's done currently)
    RandomName randomName;
    randomName.loadFromFile(profile->getPathname() + "/naming.txt");
    
    // generate the name from the chop
    _name = randomName.generateRandomName();
}

bool LoadPlayerElement::hasQuest(const IDSZ idsz, const int requiredLevel)
{
    int quest_level = quest_log_get_level(_questLog, SDL_arraysize(_questLog), idsz);

    // find beaten quests or quests with proper level
    if ( quest_level <= QUEST_BEATEN || requiredLevel <= quest_level )
    {
        return true;
    }

    return false;
}


const Ego::DeferredOpenGLTexture& LoadPlayerElement::getIcon() const 
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