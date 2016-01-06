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

/// @file game/Shop.cpp
/// @brief Shop interaction
#include "game/Shop.hpp"
#include "game/Module/Module.hpp"
#include "game/Module/Passage.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h"

bool Shop::drop(const std::shared_ptr<Object>& dropper, const std::shared_ptr<Object>& item)
{
    if (dropper->isTerminated() || item->isTerminated()) return false;

    bool inShop = false;
    if (item->isitem)
    {
        ObjectRef ownerRef = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
        if (_currentModule->getObjectHandler().exists(ownerRef))
        {
            Object *owner = _currentModule->getObjectHandler().get(ownerRef);

            inShop = true;

            int price = item->getPrice();

            // Are they are trying to sell junk or quest items?
            if (0 == price)
            {
                ai_state_t::add_order(owner->ai, (Uint32)price, Passage::SHOP_BUY);
            }
            else
            {
                dropper->giveMoney(price);
                owner->giveMoney(-price);

                ai_state_t::add_order(owner->ai, (Uint32)price, Passage::SHOP_BUY);
            }
        }
    }

    return inShop;
}

bool Shop::buy(const std::shared_ptr<Object>& buyer, const std::shared_ptr<Object>& item)
{
    if (buyer->isTerminated() || item->isTerminated()) return false;

    bool canGrab = true;
    if (item->isitem)
    {
        ObjectRef ownerRef = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
        if (_currentModule->getObjectHandler().exists(ownerRef))
        {
            Object *owner = _currentModule->getObjectHandler().get(ownerRef);

            //in_shop = true;
            int price = item->getPrice();

            if (buyer->getMoney() >= price)
            {
                // Okay to sell
                ai_state_t::add_order(owner->ai, (Uint32)price, Passage::SHOP_SELL);

                buyer->giveMoney(-price);
                owner->giveMoney(price);

                canGrab = true;
            }
            else
            {
                // Don't allow purchase
                ai_state_t::add_order(owner->ai, price, Passage::SHOP_NOAFFORD);
                canGrab = false;
            }
        }
    }

    /// @note some of these are handled in scripts, so they could be disabled
    // print some feedback messages
    /*
    if (can_grab)
    {
        if (in_shop)
        {
            if (can_pay)
            {
                DisplayMsg_printf("%s bought %s", chr_get_name(ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name(iitem, CHRNAME_ARTICLE));
            }
            else
            {
                DisplayMsg_printf("%s can't afford %s", chr_get_name(ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name(iitem, CHRNAME_ARTICLE));
            }
        }
        else
        {
            DisplayMsg_printf("%s picked up %s", chr_get_name(ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE));
        }
    }
    */

    return canGrab;
}

bool Shop::steal(const std::shared_ptr<Object>& thief, const std::shared_ptr<Object>& item)
{
    if (thief->isTerminated() || item->isTerminated()) return false;

    bool canSteal = true;
    if (item->isitem)
    {
        ObjectRef ownerRef = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
        if (_currentModule->getObjectHandler().exists(ownerRef))
        {
            int detection = Random::getPercent();
            Object *owner = _currentModule->getObjectHandler().get(ownerRef);

            canSteal = true;
            if (owner->canSeeObject(thief) || detection <= 5 || (detection - thief->getAttribute(Ego::Attribute::AGILITY) + owner->getAttribute(Ego::Attribute::INTELLECT)) > 50)
            {
                ai_state_t::add_order(owner->ai, Passage::SHOP_STOLEN, Passage::SHOP_THEFT);
                owner->ai.setTarget(thief->getObjRef());
                canSteal = false;
            }
        }
    }

    return canSteal;
}

bool Shop::canGrabItem(const std::shared_ptr<Object>& grabber, const std::shared_ptr<Object>& item)
{
    if (grabber->isTerminated() || item->isTerminated()) return false;
    // Assume there is no shop so that the character can grab anything.
    bool canGrab = true;

    // check if we are doing this inside a shop
    ObjectRef iShopKeeper = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
    Object *shopKeeper = _currentModule->getObjectHandler().get(iShopKeeper);
    if (INGAME_PCHR(shopKeeper))
    {
        // check for a stealthy pickup
        bool isInvisible = !shopKeeper->canSeeObject(grabber);

        // pets are automatically stealthy
        bool canSteal = isInvisible || grabber->isItem();

        if (canSteal)
        {
            canGrab = Shop::steal(grabber, item);

            if (!canGrab)
            {
                DisplayMsgs::get().printf("%s was detected!!", grabber->getName().c_str());
            }
            else
            {
                DisplayMsgs::get().printf("%s stole %s", grabber->getName().c_str(), item->getName(true, false, false).c_str());
            }
        }
        else
        {
            canGrab = Shop::buy(grabber, item);
        }
    }

    return canGrab;
}

bool Shop::canGrabItem(ObjectRef igrabber, ObjectRef iitem)
{
    auto grabber = _currentModule->getObjectHandler()[igrabber],
         item = _currentModule->getObjectHandler()[iitem];
    if (!grabber || !item) return false;
    return canGrabItem(grabber, item);
}