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

struct Inventory
{
public:
    /*
     * @brief
     *  Remove an item from an inventory slot.
     * @details
     *  Note that you still have to handle it falling out.
     */
    static bool remove_item(const CHR_REF ichr, const size_t inventory_slot, const bool ignorekurse);
    /**
     * @brief
     *  Add an item to an inventory slot.
     * @details
     *  This fails if there already is an item there.
     *  If the specified inventory slot is MAXINVENTORY,
     *  it will find the first free inventory slot.
     */
    static bool add_item(const CHR_REF ichr, const CHR_REF item, Uint8 inventory_slot, const bool ignorekurse);
    /**
     * @brief
     *  Swap item between inventory slot and grip slot.
     * @remark
     *  This swaps an item between the specified inventory slot and the specified grip
     *  If the specified inventory slot is MAXINVENTORY,
     *  the function will swap with the first item found in the inventory.
     */
    static bool swap_item(const CHR_REF ichr, Uint8 inventory_slot, const slot_t grip_off, const bool ignorekurse);

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
    static CHR_REF findItem(const Object *pobj, IDSZ idsz, bool equippedOnly);
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
    static CHR_REF findItem(const CHR_REF iobj, IDSZ idsz, bool equippedOnly);

private:

	/**
    * @author ZZ
    * @details This function looks in the character's pack for an item similar
    *    to the one given.  If it finds one, it returns the similar item's
    *    index number, otherwise it returns INVALID_CHR_REF.
	**/
	static CHR_REF hasStack(const CHR_REF item, const CHR_REF character);
};
