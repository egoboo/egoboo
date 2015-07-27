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

CHR_REF Inventory::findItem(Object *pobj, IDSZ idsz, bool equippedOnly)
{
    if (!pobj || pobj->isTerminated())
    {
        return INVALID_CHR_REF;
    }

    CHR_REF result = INVALID_CHR_REF;

    for(const std::shared_ptr<Object> pitem : pobj->getInventory().iterate())
    {
        bool matches_equipped = (!equippedOnly || pitem->isequipped);

        if (chr_is_type_idsz(pitem->getCharacterID(), idsz) && matches_equipped)
        {
            result = pitem->getCharacterID();
            break;
        }
    }

    return result;
}

CHR_REF Inventory::findItem(const CHR_REF iobj, IDSZ idsz, bool equippedOnly)
{
    if (!_currentModule->getObjectHandler().exists(iobj))
    {
        return INVALID_CHR_REF;
    }
    return Inventory::findItem(_currentModule->getObjectHandler().get(iobj), idsz, equippedOnly);
}

//--------------------------------------------------------------------------------------------
bool Inventory::add_item( const CHR_REF ichr, const CHR_REF item, Uint8 inventory_slot, const bool ignorekurse )
{
    Object *pchr;
    int newammo;

    //valid character?
    if ( !_currentModule->getObjectHandler().exists( ichr ) || !_currentModule->getObjectHandler().exists( item ) ) return false;
    pchr = _currentModule->getObjectHandler().get( ichr );
    const std::shared_ptr<Object> &pitem = _currentModule->getObjectHandler()[item];

    //try get the first free slot found?
    if(inventory_slot >= pchr->getInventory().getMaxItems()) {
        return false;
    }

    //don't override existing items
    if ( pchr->getInventory().getItem(inventory_slot) ) return false;

    // don't allow sub-inventories
    if ( pitem->isInsideInventory() ) return false;

    //kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the item as not put away
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->isPlayer() ) DisplayMsg_printf("%s is sticky...", pitem->getName().c_str());
        return false;
    }

    //too big item?
    if ( pitem->getProfile()->isBigItem() )
    {
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->isPlayer() ) DisplayMsg_printf("%s is too big to be put away...", pitem->getName().c_str());
        return false;
    }

    //put away inhand item
    CHR_REF stack = Inventory::hasStack( item, ichr );
    if ( _currentModule->getObjectHandler().exists( stack ) )
    {
        // We found a similar, stackable item in the pack
        Object  * pstack      = _currentModule->getObjectHandler().get( stack );

        // reveal the name of the item or the stack
        if ( pitem->nameknown || pstack->getProfile()->isNameKnown() )
        {
            pitem->nameknown  = true;
            pstack->nameknown = true;
        }

        // reveal the usage of the item or the stack
        if ( pitem->getProfile()->isUsageKnown() || pstack->getProfile()->isUsageKnown() )
        {
            pitem->getProfile()->makeUsageKnown();
            pstack->getProfile()->makeUsageKnown();
        }

        // add the item ammo to the stack
        newammo = pitem->ammo + pstack->ammo;
        if ( newammo <= pstack->ammomax )
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
            SET_BIT( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        }
    }
    else
    {
        //@todo: implement weight check here
        // Make sure we have room for another item
        //if ( pchr_pack->count >= Inventory::MAXNUMINPACK )
        // {
        //    SET_BIT( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        //    return false;
        //}

        // Take the item out of hand
        pitem->detatchFromHolder(true, false);

        // clear the dropped flag
        UNSET_BIT( pitem->ai.alert, ALERTIF_DROPPED );

        //now put the item into the inventory
        pitem->attachedto = INVALID_CHR_REF;
        pitem->inwhich_inventory = ichr;
        pchr->getInventory()._items[inventory_slot] = pitem;


        // fix the flags
        if ( pitem->getProfile()->isEquipment() )
        {
            SET_BIT( pitem->ai.alert, ALERTIF_PUTAWAY );  // same as ALERTIF_ATLASTWAYPOINT;
        }

        //@todo: add in the equipment code here
    }

    return true;
}

bool Inventory::swap_item( const CHR_REF ichr, Uint8 inventory_slot, const slot_t grip_off, const bool ignorekurse )
{
    //valid character?
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[ichr];
    if(!pchr) {
        return false;
    }

    //Validate slot number
    if(inventory_slot >= pchr->getInventory().getMaxItems()) {
        return false;
    }

    // Make sure everything is hunkydori
    if ( pchr->isItem() || pchr->isInsideInventory() ) return false;

    const std::shared_ptr<Object> &inventory_item = pchr->getInventory().getItem(inventory_slot);
    const std::shared_ptr<Object> &item           = _currentModule->getObjectHandler()[pchr->holdingwhich[grip_off]];

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
            if ( pchr->isPlayer() ) DisplayMsg_printf("%s is sticky...", item->getName().c_str());
            return false;

        }

        if(inventory_item && inventory_item->iskursed) {
            // Flag the last found_item as not removed
            SET_BIT( inventory_item->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
            if ( pchr->isPlayer() ) DisplayMsg_printf( "%s won't go out!", inventory_item->getName().c_str() );
            return false;

        }
    }

    //remove existing item
    if (inventory_item)
    {
        pchr->getInventory().removeItem(inventory_item, ignorekurse);
    }

    //put in the new item
    if (item)
    {
        Inventory::add_item(pchr->getCharacterID(), item->getCharacterID(), inventory_slot, ignorekurse);
    }

    //now put the inventory item into the character's hand
    if (inventory_item)
    {
        attach_character_to_mount( inventory_item->getCharacterID(), pchr->getCharacterID(), grip_off == SLOT_RIGHT ? GRIP_RIGHT : GRIP_LEFT );

        //fix flags
        UNSET_BIT( inventory_item->ai.alert, ALERTIF_GRABBED );
        SET_BIT( inventory_item->ai.alert, ALERTIF_TAKENOUT );
    }

    return true;
}

bool Inventory::remove_item( const CHR_REF ichr, const size_t inventory_slot, const bool ignorekurse )
{
    Object *pholder;

    //valid char?
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return false;
    pholder = _currentModule->getObjectHandler().get( ichr );

    //ignore invalid slots
    if ( inventory_slot >= pholder->getInventory().getMaxItems() )  return false;

    //valid item?
    const std::shared_ptr<Object> &pitem = pholder->getInventory().getItem(inventory_slot);
    if ( !pitem ) return false;

    //is it kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the last found_item as not removed
        SET_BIT( pitem->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
        if ( pholder->isPlayer() ) DisplayMsg_printf( "%s won't go out!", pitem->getName().c_str() );
        return false;
    }

    //no longer in an inventory
    pitem->inwhich_inventory = INVALID_CHR_REF;
    pholder->getInventory()._items[inventory_slot].reset();

    return true;
}

CHR_REF Inventory::hasStack( const CHR_REF item, const CHR_REF character )
{
    bool found  = false;
    CHR_REF istack = INVALID_CHR_REF;

    std::shared_ptr<Object> pitem = _currentModule->getObjectHandler()[item];
    if(!pitem) {
        return INVALID_CHR_REF;
    }

    //Only check items that are actually stackable
    if(!pitem->getProfile()->isStackable()) {
        return INVALID_CHR_REF;
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
        if ( found && ( pstack->getProfile()->getSlotNumber() != pitem->profile_ref ) )
        {
            for ( Uint16 id = 0; id < IDSZ_COUNT && found; id++ )
            {
                if ( chr_get_idsz( pstack->getCharacterID(), id ) != chr_get_idsz( item, id ) )
                {
                    found = false;
                }
            }
        }

        if ( found )
        {
            istack = pstack->getCharacterID();
            break;
        }
    }

    return istack;
}

CHR_REF Inventory::getItemID(const size_t slotNumber) const
{
    std::shared_ptr<Object> item = getItem(slotNumber);
    if(!item) {
        return INVALID_CHR_REF;
    }
    return item->getCharacterID();
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
            item->inwhich_inventory = INVALID_CHR_REF;
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
