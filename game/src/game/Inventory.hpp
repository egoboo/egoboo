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

/// @file game/Inventory.hpp
/// @brief Inventory managment for characters
#pragma once

#include "game/egoboo_typedef.h"

class Inventory
{
public:
    static const size_t MAXNUMINPACK = 6;   ///< Max number of items to carry in pack

    Inventory();

    /*
     * @brief
     *  Remove an item from an inventory slot.
     * @details
     *  Note that you still have to handle it falling out.
     */
    static bool remove_item(ObjectRef ichr, const size_t inventory_slot, const bool ignorekurse);

    /**
     * @brief
     *  Add an item to an inventory slot.
     * @details
     *  This fails if there already is an item there.
     */
    static bool add_item(ObjectRef iowner, const ObjectRef iitem, uint8_t inventorySlot, bool ignoreKurse);
    /**
     * @brief
     *  Swap item between inventory slot and grip slot.
     * @remark
     *  This swaps an item between the specified inventory slot and the specified grip
     */
    static bool swap_item(ObjectRef iowner, uint8_t inventorySlot, const slot_t grip_off, bool ignoreKurse);

    /**
     * @brief
     *  Find an item in the pack.
     * @param pobj
     *  the object
     * @param idsz, equippedOnly
     *  the search criteria
     * @return
     *  the character reference of the first item in the pack matching the search criterion,
     *  #INVALID_CHR_REF if no item in the pack matches the search criterion
     * @remark
     *  This function searches the characters pack for an item matching the search criterion.
     *  An item matches the search criterion if it has the specified IDSZ. If @a equipped is
     *  @a true, then in addition the item must be equipped in order to match the search
     *  criterion.
     */
    static ObjectRef findItem(Object *powner, IDSZ idsz, bool equippedOnly);
    /**
     * @brief
     *  Find an item in the pack.
     * @param iobj
     *  the object
     * @param idsz, equippedOnly
     *  the search criteria
     * @return
     *  the character reference of the first item in the pack matching the search criterion,
     *  #INVALID_CHR_REF if no item in the pack matches the search criterion
     * @remark
     *  This function searches the characters pack for an item matching the search criterion.
     *  An item matches the search criterion if it has the specified IDSZ. If @a equipped is
     *  @a true, then in addition the item must be equipped in order to match the search
     *  criterion.
     */
    static ObjectRef findItem(ObjectRef iowner, IDSZ idsz, bool equippedOnly);

    /**
    * @brief
    *   Retrieve the object reference of an item in this inventory
    * @param slotNumber
    *   Index number of the inventory slot to get
    * @return
    *   The object reference of the object in the specified slot number in this inventory
    *   Returns the invalid object reference if slot number is out of bounds or if slot is empty
    **/
    ObjectRef getItemID(const size_t slotNumber) const;

    /**
    * @brief
    *   Retrieve the shared_ptr of an item in this inventory
    * @param slotNumber
    *   Index number of the inventory slot to get
    * @return
    *   A shared_ptr to the Object in the specified slot number in this inventory
    *   Returns nullptr if slotNumber is out of bounds or if slot is empty
    **/
    std::shared_ptr<Object> getItem(const size_t slotNumber) const;

    void setItem(const size_t slotNumber, const std::shared_ptr<Object> &item);

    /**
    * @return
    *   The index number of the first free unused slot number in this inventory.
    *   Returns getMaxItems() if there is no free unused slots.
    **/
    size_t getFirstFreeSlotNumber() const;

    /**
    * @return
    *   The maximum number of different items that is allowed in this inventory
    **/
    size_t getMaxItems() const;

    /**
    * @brief
    *   Returns a vector of all shared_ptr<Object> contained in this Inventory
    **/
    std::vector<std::shared_ptr<Object>> iterate() const;

    /*
     * @brief
     *  Remove an item from this inventory.
     * @param item
     *  The item to remove
     * @param ignorekurse
     *  Remove items that normally cannot be removed due to a kurse
     * @return
     *  true if an item was removed from the inventory
     * @details
     *  Note that you still have to handle it falling out.
     */
    bool removeItem(const std::shared_ptr<Object> &item, const bool ignorekurse);

private:

	/**
    * @author ZZ
    * @details This function looks in the character's pack for an item similar
    *    to the one given.  If it finds one, it returns the similar item's
    *    index number, otherwise it returns INVALID_CHR_REF.
	**/
	static ObjectRef hasStack(const ObjectRef item, const ObjectRef character);

    std::array<std::weak_ptr<Object>, MAXNUMINPACK> _items;
};
