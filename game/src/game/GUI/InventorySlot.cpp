#include "InventorySlot.hpp"
#include "game/Entities/_Include.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"  //for model action enum
#include "game/game.h" //only for update_wld global var
#include "game/player.h"

namespace Ego
{
namespace GUI
{

InventorySlot::InventorySlot(const Inventory &inventory, const size_t slotNumber, const PLA_REF player) :
    _inventory(inventory),
    _slotNumber(slotNumber),
    _player(player)
{
    //ctor
}

void InventorySlot::draw()
{
    std::shared_ptr<Object> item = _inventory.getItem(_slotNumber);

    // grab the icon reference
    const oglx_texture_t* icon_ref;


    if(item) {
        icon_ref = chr_get_txtexture_icon_ref(item->getObjRef().get());
    }
    else {
        icon_ref = TextureManager::get().getTexture("mp_data/nullicon").get();
    }

    bool selected = false;
    if (VALID_PLA(_player))  
    {
        selected = PlaStack.get_ptr(_player)->inventory_slot ==_slotNumber;
    }

    //Draw the icon
    draw_game_icon(icon_ref, getX(), getY(), selected ? COLOR_WHITE : NOSPARKLE, update_wld, getWidth());

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

bool InventorySlot::notifyMouseMoved(const int x, const int y)
{
    bool mouseOver = contains(x, y);

    if(mouseOver) 
    {
        if (VALID_PLA(_player))  
        {
            PlaStack.get_ptr(_player)->inventory_slot = _slotNumber;
        }
        return true;
    }

    return false;
}


bool InventorySlot::notifyMouseClicked(const int button, const int x, const int y)
{
    if (!VALID_PLA(_player) || !contains(x, y)) {
        return false;
    }

    if(button != SDL_BUTTON_LEFT && button != SDL_BUTTON_RIGHT) {
        return false;
    }

    const player_t* player = PlaStack.get_ptr(_player);
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[player->index];
    if(pchr->isAlive() && pchr->inst.action_ready && 0 == pchr->reload_timer)
    {
        //put it away and swap with any existing item
        Inventory::swap_item( pchr->getObjRef(), _slotNumber, button == SDL_BUTTON_LEFT ? SLOT_LEFT : SLOT_RIGHT, false );

        // Make it take a little time
        chr_play_action( pchr.get(), ACTION_MG, false );
        pchr->reload_timer = PACKDELAY;
        return true;
    }

    return false;
}

} //GUI
} //Ego