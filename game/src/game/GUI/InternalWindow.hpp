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

/// @file game/GUI/InternalWindow.hpp
/// @details InternalWindow
/// @author Johan Jansen
#pragma once

#include "game/GUI/Component.hpp"
#include "game/GUI/ComponentContainer.hpp"

namespace Ego {
namespace GUI {
// Forward declarations.
class Image;
} // namespace GUI
} // namespace Ego

namespace Ego {
namespace GUI {

class InternalWindow : public Component, public ComponentContainer {
protected:
    class TitleBar : public Component {
    public:
        TitleBar(const std::string &titleBar);

        void draw() override;

        int getTextHeight() const { return _textHeight; }

    private:
        DeferredTexture _titleBarTexture;
        DeferredTexture _titleSkull;
        std::shared_ptr<Font> _font;
        std::string _title;
        int _textWidth;
        int _textHeight;
    };

public:
    InternalWindow(const std::string &title);

    bool notifyMouseMoved(const Events::MouseMovedEventArgs& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEventArgs& e) override;
    bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEventArgs& e) override;

    void draw() override;

    virtual void setPosition(float x, float y) override;
    virtual void setSize(float width, float height) override;

    void setTransparency(float alpha);

    void addComponent(std::shared_ptr<Component> component) override;

protected:
    void drawContainer() override;

private:
    std::unique_ptr<TitleBar> _titleBar;
    std::shared_ptr<Image> _closeButton;
    Ego::DeferredTexture _background;
    bool _mouseOver;
    bool _mouseOverCloseButton;
    bool _isDragging;
    Vector2f _mouseDragOffset;
    float _transparency;
    bool _firstDraw;
};

} // namespace GUI
} // namespace Ego
