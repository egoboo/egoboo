#pragma once

#include "game/GUI/Component.hpp"
#include "game/Inventory.hpp"

// Forward declarations.
namespace Ego { class Player; }

namespace Ego {
namespace GUI {

class InventorySlot : public Component {
public:
    InventorySlot(const Inventory &inventory, const size_t slotNumber, const std::shared_ptr<Player>& player);

    virtual void draw() override;

    bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) override;

private:
    const Inventory& _inventory;
    size_t _slotNumber;
    std::shared_ptr<Player> _player;
};

} // namespace GUI
} // namespace Ego

