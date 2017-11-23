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

    virtual void draw(Ego::GUI::DrawingContext& drawingContext) override;

    bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) override;

private:
    const Inventory& _inventory;
    size_t _slotNumber;
    std::shared_ptr<Player> _player;
};

} // namespace GUI
} // namespace Ego

