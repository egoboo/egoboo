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

/// @file game/GUI/CharacterWindow.hpp
/// @details CharacterWindow
/// @author Johan Jansen
#pragma once

#include "game/GUI/InternalWindow.hpp"
#include "game/GUI/TabPanel.hpp"

// Forward declarations.
class Object;

namespace Ego {
namespace GUI {
class Button;
class ScrollableList;
} // namespace GUI
class Enchantment;
} // namespace Ego

namespace Ego {
namespace GUI {

class CharacterWindow : public InternalWindow {
public:
    CharacterWindow(const std::shared_ptr<Object> &object);
    ~CharacterWindow();

    void drawContainer(DrawingContext& drawingContext) override;
    void draw(DrawingContext& drawingContext) override;
    void drawAll(DrawingContext& drawingContext) override;
    bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) override;

private:
    int addResistanceLabel(std::shared_ptr<Tab> target, const Point2f& position, const DamageType type);
    int addAttributeLabel(std::shared_ptr<Tab> target, const Point2f& position, const Attribute::AttributeType type);

    void buildCharacterStatisticTab(std::shared_ptr<Tab> target);
    void buildKnownPerksTab(std::shared_ptr<Tab> target);
    void buildActiveEnchantsTab(std::shared_ptr<Tab> target);

    void describeEnchantEffects(const std::vector<std::shared_ptr<Enchantment>> &enchants, std::shared_ptr<ScrollableList> list);

private:
    std::shared_ptr<Object> _character;
    std::shared_ptr<Button> _levelUpButton;
    std::weak_ptr<InternalWindow> _levelUpWindow;

    std::shared_ptr<Tab> _characterStatisticsTab;
    std::shared_ptr<Tab> _knownPerksTab;
    std::shared_ptr<Tab> _activeEnchantsTab;
};

} // namespace GUI
} // namespace Ego
