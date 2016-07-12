#include "InventorySlot.hpp"
#include "game/Entities/_Include.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"  //for model action enum
#include "game/game.h" //only for update_wld global var
#include "game/Logic/Player.hpp"
#include "game/ObjectAnimation.h"

namespace Ego {
namespace GUI {

InventorySlot::InventorySlot(const Inventory &inventory, const size_t slotNumber, const std::shared_ptr<Player>& player) :
    _inventory(inventory),
    _slotNumber(slotNumber),
    _player(player) {
    //ctor
}

void InventorySlot::draw() {
    std::shared_ptr<Object> item = _inventory.getItem(_slotNumber);

    // grab the icon reference
    std::shared_ptr<const Texture> icon_ref;


    if (item) {
        icon_ref = item->getIcon();
    } else {
        icon_ref = TextureManager::get().getTexture("mp_data/nullicon");
    }

    bool selected = false;
    if (_player) {
        selected = _player->getSelectedInventorySlot() == _slotNumber;
    }

    //Draw the icon
    draw_game_icon(icon_ref, getX(), getY(), selected ? COLOR_WHITE : NOSPARKLE, update_wld, getWidth());

    //Draw ammo
    if (item) {
        if (0 != item->ammomax && item->ammoknown) {
            if (!item->getProfile()->isStackable() || item->getAmmo() > 1) {
                // Show amount of ammo left
                _gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)->drawTextBox(std::to_string(item->getAmmo()), getX(), getY(), getWidth(), getHeight(), 0);
            }
        }
    }
}

bool InventorySlot::notifyMouseMoved(const Events::MouseMovedEventArgs& e) {
    bool mouseOver = contains(e.getPosition());

    if (mouseOver) {
        if (_player) {
            _player->setSelectedInventorySlot(_slotNumber);
        }
        return true;
    }

    return false;
}


bool InventorySlot::notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) {
    if (!_player || !contains(e.getPosition())) {
        return false;
    }

    if (e.getButton() != SDL_BUTTON_LEFT && e.getButton() != SDL_BUTTON_RIGHT) {
        return false;
    }

    const std::shared_ptr<Object> &pchr = _player->getObject();
    if (pchr->isAlive() && pchr->inst.actionState.action_ready && 0 == pchr->reload_timer) {
        //put it away and swap with any existing item
        Inventory::swap_item(pchr->getObjRef(), _slotNumber, e.getButton() == SDL_BUTTON_LEFT ? SLOT_LEFT : SLOT_RIGHT, false);

        // Make it take a little time
        chr_play_action(pchr.get(), ACTION_MG, false);
        pchr->reload_timer = Inventory::PACKDELAY;
        return true;
    }

    return false;
}

} //GUI
} //Ego