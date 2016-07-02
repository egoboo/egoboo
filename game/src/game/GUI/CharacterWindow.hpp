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

    bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;

private:
    int addResistanceLabel(const int x, const int y, const DamageType type);
    int addAttributeLabel(const int x, const int y, const Attribute::AttributeType type);

    void buildCharacterStatisticTab();
    void buildKnownPerksTab();
    void buildActiveEnchantsTab();

    void describeEnchantEffects(const std::vector<std::shared_ptr<Enchantment>> &enchants, std::shared_ptr<ScrollableList> list);

private:
    std::shared_ptr<Object> _character;
    std::shared_ptr<Button> _levelUpButton;
    std::weak_ptr<InternalWindow> _levelUpWindow;

    std::vector<std::shared_ptr<Component>> _characterStatisticsTab;
    std::vector<std::shared_ptr<Component>> _knownPerksTab;
    std::vector<std::shared_ptr<Component>> _activeEnchantsTab;
};

} // namespace GUI
} // namespace Ego
