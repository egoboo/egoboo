#include "game/Inventory.hpp"
#include "game/Entities/_Include.hpp"

#include "game/game.h"
#include "game/renderer_2d.h"

CHR_REF Inventory::findItem(const Object *pobj, IDSZ idsz, bool equippedOnly)
{
    if (!pobj || pobj->isTerminated())
    {
        return INVALID_CHR_REF;
    }

    CHR_REF result = INVALID_CHR_REF;

    PACK_BEGIN_LOOP(pobj->inventory, pitem, item)
    {
        bool matches_equipped = (!equippedOnly || pitem->isequipped);

        if (chr_is_type_idsz(item, idsz) && matches_equipped)
        {
            result = item;
            break;
        }
    }
    PACK_END_LOOP();

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
    Object *pchr, *pitem;
    int newammo;

    //valid character?
    if ( !_currentModule->getObjectHandler().exists( ichr ) || !_currentModule->getObjectHandler().exists( item ) ) return false;
    pchr = _currentModule->getObjectHandler().get( ichr );
    pitem = _currentModule->getObjectHandler().get( item );

    //try get the first free slot found?
    if ( inventory_slot >= MAXINVENTORY )
    {
        int i;
        for ( i = 0; i < Object::MAXNUMINPACK; i++ )
        {
            if ( !_currentModule->getObjectHandler().exists( pchr->inventory[i] ) )
            {
                //found a free slot
                inventory_slot = i;
                break;
            }
        }

        //did we find one?
        if ( i == MAXINVENTORY ) return false;
    }

    //don't override existing items
    if ( _currentModule->getObjectHandler().exists( pchr->inventory[inventory_slot] ) ) return false;

    // don't allow sub-inventories
    if ( _currentModule->getObjectHandler().exists( pitem->inwhich_inventory ) ) return false;

    //kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the item as not put away
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->islocalplayer ) DisplayMsg_printf("%s is sticky...", pitem->getName().c_str());
        return false;
    }

    //too big item?
    if ( pitem->getProfile()->isBigItem() )
    {
        SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
        if ( pchr->islocalplayer ) DisplayMsg_printf("%s is too big to be put away...", pitem->getName().c_str());
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
        //if ( pchr_pack->count >= Object::MAXNUMINPACK )
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
        pchr->inventory[inventory_slot] = item;

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
    CHR_REF item, inventory_item;
    Object *pchr;
    bool success = false;
    bool inventory_rv;

    //valid character?
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return false;
    pchr = _currentModule->getObjectHandler().get( ichr );

    //try get the first used slot found?
    if ( inventory_slot >= MAXINVENTORY )
    {
        int i;
        for ( i = 0; i < Object::MAXNUMINPACK; i++ )
        {
            if ( !_currentModule->getObjectHandler().exists( pchr->inventory[i] ) )
            {
                //found a free slot
                inventory_slot = i;
                break;
            }
        }
    }

    inventory_item = pchr->inventory[inventory_slot];
    item           = pchr->holdingwhich[grip_off];

    // Make sure everything is hunkydori
    if ( pchr->isitem || _currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) ) return false;

    //remove existing item
    if ( _currentModule->getObjectHandler().exists( inventory_item ) )
    {
        inventory_rv = Inventory::remove_item( ichr, inventory_slot, ignorekurse );
        if ( inventory_rv ) success = true;
    }

    //put in the new item
    if ( _currentModule->getObjectHandler().exists( item ) )
    {
        inventory_rv = Inventory::add_item( ichr, item, inventory_slot, ignorekurse );
        if ( inventory_rv ) success = true;
    }

    //now put the inventory item into the character's hand
    if ( _currentModule->getObjectHandler().exists( inventory_item ) && success )
    {
        Object *pitem = _currentModule->getObjectHandler().get( inventory_item );
        attach_character_to_mount( inventory_item, ichr, grip_off == SLOT_RIGHT ? GRIP_RIGHT : GRIP_LEFT );

        //fix flags
        UNSET_BIT( pitem->ai.alert, ALERTIF_GRABBED );
        SET_BIT( pitem->ai.alert, ALERTIF_TAKENOUT );
    }

    return success;
}

bool Inventory::remove_item( const CHR_REF ichr, const size_t inventory_slot, const bool ignorekurse )
{
    CHR_REF item;
    Object *pitem;
    Object *pholder;

    //ignore invalid slots
    if ( inventory_slot >= MAXINVENTORY )  return false;

    //valid char?
    if ( !_currentModule->getObjectHandler().exists( ichr ) ) return false;
    pholder = _currentModule->getObjectHandler().get( ichr );
    item = pholder->inventory[inventory_slot];

    //valid item?
    if ( !_currentModule->getObjectHandler().exists( item ) ) return false;
    pitem = _currentModule->getObjectHandler().get( item );

    //is it kursed?
    if ( pitem->iskursed && !ignorekurse )
    {
        // Flag the last found_item as not removed
        SET_BIT( pitem->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
        if ( pholder->islocalplayer ) DisplayMsg_printf( "%s won't go out!", pitem->getName().c_str() );
        return false;
    }

    //no longer in an inventory
    pitem->inwhich_inventory = INVALID_CHR_REF;
    pholder->inventory[inventory_slot] = INVALID_CHR_REF;

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

    PACK_BEGIN_LOOP( _currentModule->getObjectHandler().get(character)->inventory, pstack, istack_new )
    {
        const std::shared_ptr<ObjectProfile> &stackProfile = _currentModule->getObjectHandler()[istack_new]->getProfile();

        found = stackProfile->isStackable();

        if ( pstack->ammo >= pstack->ammomax )
        {
            found = false;
        }

        // you can still stack something even if the profiles don't match exactly,
        // but they have to have all the same IDSZ properties
        if ( found && ( stackProfile->getSlotNumber() != pitem->profile_ref ) )
        {
            for ( Uint16 id = 0; id < IDSZ_COUNT && found; id++ )
            {
                if ( chr_get_idsz( istack_new, id ) != chr_get_idsz( item, id ) )
                {
                    found = false;
                }
            }
        }

        if ( found )
        {
            istack = istack_new;
            break;
        }
    }
    PACK_END_LOOP();

    return istack;
}
