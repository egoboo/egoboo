#include "game/Inventory.hpp"
#include "game/Entities/_Include.hpp"

#include "game/game.h"
#include "game/renderer_2d.h"

//Class constants
const size_t Inventory::MAXNUMINPACK;


Inventory::Inventory() :
    _items()
{
    //ctor
}

ObjectRef Inventory::findItem(Object *pobj, const IDSZ2& idsz, bool equippedOnly) {
	if (!pobj || pobj->isTerminated()) {
		return ObjectRef::Invalid;
	}

    ObjectRef result = ObjectRef::Invalid;

    for(const std::shared_ptr<Object> pitem : pobj->getInventory().iterate())
    {
        bool matches_equipped = (!equippedOnly || pitem->isequipped);

        if (pitem->getProfile()->hasTypeIDSZ(idsz) && matches_equipped)
        {
            result = pitem->getObjRef();
            break;
        }
    }

    return result;
}

ObjectRef Inventory::findItem(ObjectRef iowner, const IDSZ2& idsz, bool equippedOnly)
{
    if (!_currentModule->getObjectHandler().exists(iowner))
    {
        return ObjectRef::Invalid;
    }
    return findItem(_currentModule->getObjectHandler().get(iowner), idsz, equippedOnly);
}

//--------------------------------------------------------------------------------------------
bool Inventory::add_item( ObjectRef iowner, ObjectRef iitem, uint8_t inventorySlot, bool ignoreKurse )
{
    // Are owner and item valid?
	if (!_currentModule->getObjectHandler().exists(iowner) || !_currentModule->getObjectHandler().exists(iitem)) {
		return false;
	}
	Object *powner = _currentModule->getObjectHandler().get(iowner);
    const std::shared_ptr<Object> &pitem = _currentModule->getObjectHandler()[iitem];

    // Does the owner have free slot in her inventory?
    if (inventorySlot >= powner->getInventory().getMaxItems()) {
        return false;
    }

    // If there is an item in the slot, do nothing.
	if (powner->getInventory().getItem(inventorySlot)) {
		return false;
	}

    // Don't allow sub-inventories.
	if (pitem->isInsideInventory()) {
		return false;
	}

    // Kursed?
	if (pitem->iskursed && !ignoreKurse)
	{
		// Flag the item as not put away.
		SET_BIT(pitem->ai.alert, ALERTIF_NOTPUTAWAY);
		if (powner->isPlayer()) DisplayMsg_printf("%s is sticky...", pitem->getName().c_str());
		return false;
	}

    // too big item?
	if (pitem->getProfile()->isBigItem())
	{
		SET_BIT(pitem->ai.alert, ALERTIF_NOTPUTAWAY);
		if (powner->isPlayer()) DisplayMsg_printf("%s is too big to be put away...", pitem->getName().c_str());
		return false;
	}

    // Check if item can be stacked on other items.
    ObjectRef stack = Inventory::hasStack(iitem, iowner);
    if ( _currentModule->getObjectHandler().exists( stack ) )
    {
        // We found a similar, stackable item in the inventory.
        Object *pstack = _currentModule->getObjectHandler().get( stack );

        // Reveal the name of the item or the stack.
		if (pitem->nameknown || pstack->getProfile()->isNameKnown())
		{
			pitem->nameknown = true;
			pstack->nameknown = true;
		}

        // Reveal the usage of the item or the stack.
		if (pitem->getProfile()->isUsageKnown() || pstack->getProfile()->isUsageKnown())
		{
			pitem->getProfile()->makeUsageKnown();
			pstack->getProfile()->makeUsageKnown();
		}

        // Add the item ammo to the stack.
        int newammo = pitem->ammo + pstack->ammo;
		if (newammo <= pstack->ammomax)
		{
			// All transfered, so kill the in hand item
			pstack->ammo = newammo;

			pitem->requestTerminate();
			return true;
		}
        else
        {
            // Only some were transfered,
            pitem->ammo     = pitem->ammo + pstack->ammo - pstack->ammomax;
            pstack->ammo    = pstack->ammomax;
            SET_BIT( powner->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        }
    }
    else
    {
        //@todo: implement weight check here
        // Make sure we have room for another item
        //if ( pchr_pack->count >= Inventory::MAXNUMINPACK )
        // {
        //    SET_BIT( powner->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        //    return false;
        //}

        // Take the item out of hand
        pitem->detatchFromHolder(true, false);

        // clear the dropped flag
        UNSET_BIT( pitem->ai.alert, ALERTIF_DROPPED );

        //Do not trigger dismount logic on putting items into inventory
        pitem->dismount_object = ObjectRef::Invalid;
        pitem->dismount_timer = 0;

        //now put the item into the inventory
        pitem->attachedto = ObjectRef::Invalid;
        pitem->inwhich_inventory = iowner;
        powner->getInventory()._items[inventorySlot] = pitem;

        // fix the flags
		if (pitem->getProfile()->isEquipment())
		{
			SET_BIT(pitem->ai.alert, ALERTIF_PUTAWAY);  // same as ALERTIF_ATLASTWAYPOINT;
		}

        //@todo: add in the equipment code here
    }

    return true;
}

bool Inventory::swap_item( ObjectRef iobj, uint8_t inventory_slot, const slot_t grip_off, const bool ignorekurse )
{
    //valid character?
    const std::shared_ptr<Object> &pobj = _currentModule->getObjectHandler()[iobj];
    if(!pobj) {
        return false;
    }

    //Validate slot number
    if(inventory_slot >= pobj->getInventory().getMaxItems()) {
        return false;
    }

    // Make sure everything is hunkydori
    if (pobj->isItem() || pobj->isInsideInventory()) return false;

    const std::shared_ptr<Object> &inventory_item = pobj->getInventory().getItem(inventory_slot);
    const std::shared_ptr<Object> &item           = _currentModule->getObjectHandler()[pobj->holdingwhich[grip_off]];

    //Nothing to do?
    if(!item && !inventory_item) {
        return true;
    }

    //Check if either item is kursed first
    if(!ignorekurse) 
    {
        if(item && item->iskursed) {
            // Flag the last found_item as not put away
            SET_BIT( item->ai.alert, ALERTIF_NOTPUTAWAY );  // Same as ALERTIF_NOTTAKENOUT
            if ( pobj->isPlayer() ) DisplayMsg_printf("%s is sticky...", item->getName().c_str());
            return false;

        }

        if(inventory_item && inventory_item->iskursed) {
            // Flag the last found_item as not removed
            SET_BIT( inventory_item->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
            if ( pobj->isPlayer() ) DisplayMsg_printf( "%s won't go out!", inventory_item->getName().c_str() );
            return false;

        }
    }

    //remove existing item from inventory and into the character's hand
    if (inventory_item) {
        pobj->getInventory().removeItem(inventory_item, ignorekurse);

        inventory_item->getObjectPhysics().attachToObject(pobj, grip_off == SLOT_RIGHT ? GRIP_RIGHT : GRIP_LEFT);

        //fix flags
        UNSET_BIT(inventory_item->ai.alert, ALERTIF_GRABBED);
        SET_BIT(inventory_item->ai.alert, ALERTIF_TAKENOUT);
    }

    //put the new item in the inventory
    if (item) {
        add_item(pobj->getObjRef(), item->getObjRef(), inventory_slot, ignorekurse);
    }

    return true;
}

bool Inventory::remove_item( ObjectRef iholder, const size_t inventory_slot, const bool ignorekurse )
{
    // Ignore invalid holders.
	if (!_currentModule->getObjectHandler().exists(iholder)) {
		return false;
	}
	Object *pholder = _currentModule->getObjectHandler().get(iholder);

    // Ignore invalid slots indices.
	if (inventory_slot >= pholder->getInventory().getMaxItems()) {
		return false;
	}

    // Is there an item?
    const std::shared_ptr<Object> &pitem = pholder->getInventory().getItem(inventory_slot);
	if (!pitem) {
		return false;
	}

    // Is the item kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the last found_item as not removed.
        SET_BIT(pitem->ai.alert, ALERTIF_NOTTAKENOUT);  // Same as ALERTIF_NOTPUTAWAY.
		if (pholder->isPlayer()) {
			DisplayMsg_printf("%s won't go out!", pitem->getName().c_str());
		}
		return false;
    }

    // The item is no longer in an inventory.
    pitem->inwhich_inventory = ObjectRef::Invalid;
    pholder->getInventory()._items[inventory_slot].reset();

    return true;
}

ObjectRef Inventory::hasStack( const ObjectRef item, const ObjectRef character )
{
    bool found  = false;
    ObjectRef istack = ObjectRef::Invalid;

    std::shared_ptr<Object> pitem = _currentModule->getObjectHandler()[item];
    if(!pitem) {
        return ObjectRef::Invalid;
    }

    //Only check items that are actually stackable
    if(!pitem->getProfile()->isStackable()) {
        return ObjectRef::Invalid;
    }

    for(const std::shared_ptr<Object> pstack : _currentModule->getObjectHandler().get(character)->getInventory().iterate())
    {

        found = pstack->getProfile()->isStackable();

        if ( pstack->ammo >= pstack->ammomax )
        {
            found = false;
        }

        // you can still stack something even if the profiles don't match exactly,
        // but they have to have all the same IDSZ properties
        if ( found && ( pstack->getProfile()->getSlotNumber() != pitem->getProfileID() ) )
        {
            for ( Uint16 id = 0; id < IDSZ_COUNT && found; id++ )
            {
                if ( pstack->getProfile()->getIDSZ(id) != pitem->getProfile()->getIDSZ(id) )
                {
                    found = false;
                }
            }
        }

        if ( found )
        {
            istack = pstack->getObjRef();
            break;
        }
    }

    return istack;
}

ObjectRef Inventory::getItemID(const size_t slotNumber) const
{
    std::shared_ptr<Object> item = getItem(slotNumber);
    if(!item) {
        return ObjectRef::Invalid;
    }
    return item->getObjRef();
}

std::shared_ptr<Object> Inventory::getItem(const size_t slotNumber) const
{
    if(slotNumber >= _items.size()) {
        return Object::INVALID_OBJECT;
    }

    std::shared_ptr<Object> item = _items[slotNumber].lock();
    if(item && item->isTerminated()) {
        //_items[slotNumber].reset();
        return Object::INVALID_OBJECT;
    }

    return item;
}

void Inventory::setItem(const size_t slotNumber, const std::shared_ptr<Object> &item)
{
    _items[slotNumber] = item;
}

std::vector<std::shared_ptr<Object>> Inventory::iterate() const
{
    std::vector<std::shared_ptr<Object>> result;
    for(const std::weak_ptr<Object> &weak : _items)
    {
        std::shared_ptr<Object> item = weak.lock();
        if(item) {
            result.push_back(item);
        }
    }
    return result;
}

size_t Inventory::getFirstFreeSlotNumber() const
{
    for(size_t i = 0; i < _items.size(); ++i)
    {
        if(!_items[i].lock())
        {
            return i;
        }
    }

    return _items.size();
}

bool Inventory::removeItem(const std::shared_ptr<Object> &item, const bool ignorekurse)
{
    //Empty or invalid items always returns false
    if(!item) {
        return false;
    }

    for(size_t i = 0; i < _items.size(); ++i)
    {
        //Is this the item we are looking for?
        if(_items[i].lock() == item)
        {
            //is it kursed?
            if ( item->iskursed && !ignorekurse )
            {
                //Flag the item as not removed
                SET_BIT( item->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
                DisplayMsg_printf( "%s won't go out!", item->getName().c_str());
                return false;
            }

            //Remove it from the inventory!
            item->inwhich_inventory = ObjectRef::Invalid;
            _items[i].reset();
            return true;
        }
    }
    return false;
}

size_t Inventory::getMaxItems() const
{
    return _items.size();
}
