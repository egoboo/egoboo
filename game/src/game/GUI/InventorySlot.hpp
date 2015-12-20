#pragma once

#include "GUIComponent.hpp"
#include "game/Inventory.hpp"

//Forward declarations
namespace Ego { class Player; }

namespace Ego
{
namespace GUI
{
class InventorySlot : public GUIComponent
{
    public:
        InventorySlot(const Inventory &inventory, const size_t slotNumber, const std::shared_ptr<Ego::Player>& player);

        virtual void draw() override;

        bool notifyMouseMoved(const int x, const int y) override;
        bool notifyMouseClicked(const int button, const int x, const int y) override;

    private:
        const Inventory& _inventory;
        size_t _slotNumber;
        std::shared_ptr<Ego::Player> _player;
};
}
}
