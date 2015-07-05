#pragma once

#include "GUIComponent.hpp"
#include "game/Inventory.hpp"

namespace Ego
{
namespace GUI
{
class InventorySlot : public GUIComponent
{
    public:
        InventorySlot(const Inventory &inventory, const size_t slotNumber, const PLA_REF player);

        virtual void draw() override;

        bool notifyMouseMoved(const int x, const int y) override;
        bool notifyMouseClicked(const int button, const int x, const int y) override;

    private:
        const Inventory& _inventory;
        size_t _slotNumber;
        PLA_REF _player;
};
}
}
