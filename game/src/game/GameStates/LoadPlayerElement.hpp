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

/// @file game/GameStates/LoadPlayerElement.hpp
/// @details Used by the menu system to handle the loading of player characters
/// @author Johan Jansen

#pragma once

#include "game/egoboo.h"
#include "game/Logic/QuestLog.hpp"

//Forward declarations
class ObjectProfile;

class LoadPlayerElement
{
public:
    LoadPlayerElement(std::shared_ptr<ObjectProfile> profile);

    /**
     * @brief
     *   Return the name of this character
     */
    inline const std::string& getName() const {return _name;}

    const Ego::DeferredTexture& getIcon() const;

    inline const std::shared_ptr<ObjectProfile>& getProfile() const {return _profile;}

    /**
     * @return
     *  which player number has selected this character (@a -1 for nobody)
     */
    int getSelectedByPlayer() const {return _selectedByPlayer;}

    /**
     * @return
     *  @a true if the player meets the specified requirements for this quest
     */
    bool hasQuest(const IDSZ2& idsz, const int requiredLevel);

    void setSelectedByPlayer(int selected) {_selectedByPlayer = selected;}

    /**
     * @return
     *  @a true if this character has been picked by a player
     */
    bool isSelected() const;

    /**
     * @brief
     *  Set whether this LoadPlayerElement has been picked to be played or not
     */
    void setSelected(bool selected);

private:
    std::string                     _name;
    TX_REF                          _icon;
    std::shared_ptr<ObjectProfile>  _profile;
    uint16_t                        _skinRef;
    Ego::QuestLog                   _questLog;          ///< all the quests this player has
    int                             _selectedByPlayer;  ///< ID of player who has selected this character (-1 for none)
    bool                            _isSelected;
};
