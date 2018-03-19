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

/// @file egolib/game/Logic/Player.hpp
/// @author Zefz aka Johan Jansen

#pragma once

#include "idlib/idlib.hpp"
#include "egolib/IDSZ.hpp"
#include "egolib/Entities/Forward.hpp"
#include "egolib/InputControl/InputDevice.hpp"
#include "egolib/game/Logic/QuestLog.hpp"

namespace Ego
{

/// The state of a player
class Player
{
public:
    Player(const std::shared_ptr<Object>& object, const Ego::Input::InputDevice &device);

    /**
    * @brief
    *   Get the in-game Object character representing this Player 
    * @return
    *   the Object of this player or nullptr if it no longer exists
    **/
    std::shared_ptr<Object> getObject() const;

    /**
    * @return
    *   the input device controlling the movement of this Player
    **/
    const Ego::Input::InputDevice& getInputDevice() const;

    /**
    * @return
    *   the quest log of this Player which stores game progress
    **/
    Ego::QuestLog& getQuestLog();

    /**
    * @brief
    *   Polls the input device and sets movement and action latches accordingly
    **/
    void updateLatches();

    /**
    * @brief
    *   This makes this player have a channeling progress bar next to it's status indicator
    **/
    void setChargeBar(const uint32_t currentCharge, const uint32_t maxCharge, const uint32_t chargeTick);

    /**
    * @return
    *   true if this player has gain a level up and has not yet selected a new Perk using
    *   the Level Up button in the CharacterWindow
    **/
    bool hasUnspentLevel() const;

    /**
    * @brief
    *   Sets wheter the status indicator should show that the player can gain a new character
    *   level
    **/
    void setLevelUpIndicator(const bool hasLevelUp);

    /**
    * @brief
    *   Sets wheter all input controls from the input device should be consumed by
    *   the inventory selection instead of controlling the player Object.
    **/
    void setInventoryMode(const bool inventoryMode);

    /**
    * @return
    *   the current selected inventory slot
    **/
    uint8_t getSelectedInventorySlot() const;

    /**
    * @brief
    *   changes which inventory slot is currently selected by this player
    **/
    void setSelectedInventorySlot(const uint8_t slot);

    /**
    * @brief
    *   Legacy code. TODO: remove and port to c++
    *   Required for input controls to work
    **/
    static void net_unbuffer_player_latches();

    /**
    * @brief
    *   these functions are for the channeling bar
    **/
    uint32_t getBarPipWidth() const;
    uint32_t getBarCurrentCharge() const;
    uint32_t getBarMaxCharge() const;
    uint32_t getChargeBarFrame() const;

private:
    std::weak_ptr<Object> _object; ///< Which character?
    bool _unspentLevelUp;          ///< Has gained new experience level?

    //Charge bar
    uint32_t _currentCharge;
    uint32_t _maxCharge;
    uint32_t _chargeBarFrame;
    uint32_t _chargeTick;

    // inventory stuff
    bool     _inventoryMode;          //True if input events are consumed by inventory GUI
    uint8_t  _inventorySlot;
    uint32_t _inventoryCooldown;

    // quest log for this player
    Ego::QuestLog _questLog;          ///< lists all the character's quests

    /// the buffered input from the local input devices
    const Ego::Input::InputDevice& _inputDevice;
};

} //namespace Ego
