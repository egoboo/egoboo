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
        InventorySlot(const Inventory &inventory, const size_t slotNumber);

        virtual void draw() override;

    private:
        const Inventory& _inventory;
        size_t _slotNumber;
        bool _selected;
};
}
}
