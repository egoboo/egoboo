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

/// @file game/GUI/MiniMap.hpp
/// @details GUI widget to render that tiny minimap in the corner
/// @author Johan Jansen

#pragma once

#include "game/GUI/Component.hpp"

namespace Ego { class DeferredTexture; }

namespace Ego {
namespace GUI {

class MiniMap : public Component {
public:
    /// @brief The size of the minimap.
    /// @default Default is @a 128.
    static const uint32_t MAPSIZE;

    MiniMap();

    virtual void draw(DrawingContext& drawingContext) override;

    void setShowPlayerPosition(bool show);

    void addBlip(const float x, const float y, const HUDColors color);

    void addBlip(const float x, const float y, const std::shared_ptr<Object> &object);

    bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) override;
    bool notifyKeyboardKeyPressed(const Events::KeyboardKeyPressedEvent& e) override;
    bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e) override;

private:

    //Private blip class
    class Blip {
    public:
        Blip(const float setX, const float setY, const HUDColors setColor) :
            x(setX),
            y(setY),
            color(setColor),
            icon(nullptr) {
            //ctor
        }

        Blip(const float setX, const float setY, const std::shared_ptr<const Texture>& setIcon) :
            x(setX),
            y(setY),
            color(COLOR_WHITE),
            icon(setIcon) {
            //ctor
        }

        float x;
        float y;
        HUDColors color;
        std::shared_ptr<const Texture> icon;
    };

private:
    uint32_t _markerBlinkTimer;     //< Ticks until next minimap blink is shown
    bool _showPlayerPosition;
    std::vector<Blip> _blips;
    bool _mouseOver;
    bool _isDragging;
    Vector2f _mouseDragOffset;
    std::shared_ptr<DeferredTexture> _minimapTexture;
};

} // namespace GUI
} // namespace Ego
