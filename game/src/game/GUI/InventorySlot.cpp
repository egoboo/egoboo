#include "InventorySlot.hpp"
#include "game/Entities/_Include.hpp"

#include "game/game.h" //only for update_wld global var

namespace Ego
{
namespace GUI
{

InventorySlot::InventorySlot(const Inventory &inventory, const size_t slotNumber) :
    _inventory(inventory),
    _slotNumber(slotNumber),
    _selected(false)
{
    //ctor
}

void InventorySlot::draw()
{
    std::shared_ptr<Object> item = _inventory.getItem(_slotNumber);

    // grab the icon reference
    TX_REF icon_ref;


    if(item) {
        icon_ref = chr_get_txtexture_icon_ref(item->getCharacterID());
    }
    else {
        icon_ref = static_cast<TX_REF>(TX_ICON_NULL);
    }

    //Draw the icon
    draw_game_icon(icon_ref, getX(), getY(), _selected ? COLOR_WHITE : NOSPARKLE, update_wld, getWidth());

    //Draw ammo
    if(item) 
    {
        if (0 != item->ammomax && item->ammoknown)
        {
            if (!item->getProfile()->isStackable() || item->getAmmo() > 1)
            {
                // Show amount of ammo left
                _gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)->drawTextBox(std::to_string(item->getAmmo()), getX(), getY(), getWidth(), getHeight(), 0);
            }
        }
    }
}

} //GUI
} //Ego