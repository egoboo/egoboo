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

/// @file game/Logic/Player.hpp
/// @author Zefz aka Johan Jansen

#pragma once

#include "IdLib/IdLib.hpp"
#include "egolib/IDSZ.hpp"
#include "egolib/input_device.h"

//Forward declarations
class Object;

namespace Ego
{

/// The state of a player
class Player
{
public:
    Player(const std::shared_ptr<Object>& object, input_device_t *pdevice);

    std::shared_ptr<Object> getObject() const;

    input_device_t* getInputDevice();

    std::unordered_map<IDSZ2, int>& getQuestLog();

    void updateLatches();

    void setLatch(size_t latch, bool value);

    void setChargeBar(const uint32_t currentCharge, const uint32_t maxCharge, const uint32_t chargeTick);

    bool hasUnspentLevel() const;

    void setLevelUpIndicator(const bool hasLevelUp);

    void setInventoryMode(const bool inventoryMode);

    uint8_t getSelectedInventorySlot() const;

    void setSelectedInventorySlot(const uint8_t slot);

    static void net_unbuffer_player_latches();

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
    std::unordered_map<IDSZ2, int> _questLog;          ///< lists all the character's quests

    /// the buffered input from the local input devices
    input_device_t* _pdevice;

    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    latch_t _localLatch;

    // Timed latches
    uint32_t     _tlatch_count;
    time_latch_t _tlatch[MAXLAG];

    /// Network latch, set by net_unbuffer_player_latches(), used to set the local character's latch
    latch_t _net_latch;
};

} //namespace Ego
