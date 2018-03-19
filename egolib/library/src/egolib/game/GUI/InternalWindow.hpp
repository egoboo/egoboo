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

/// @file egolib/game/GUI/InternalWindow.hpp
/// @details InternalWindow
/// @author Johan Jansen
#pragma once

#include "egolib/game/GUI/Container.hpp"
#include "egolib/Renderer/DeferredTexture.hpp"

namespace Ego::GUI {
// Forward declarations.
class Image;
} // namespace Ego::GUI

namespace Ego::GUI {

class InternalWindow : public Container {
protected:
    class TitleBar : public Component {
    public:
        TitleBar(const std::string &titleBar);

        void draw(DrawingContext& drawingContext) override;

        int getTextHeight() const { return _textHeight; }

    protected:
        DeferredTexture _titleBarTexture;
        DeferredTexture _titleSkullTexture;
        std::shared_ptr<Font> _font;
        std::string _title;
        int _textWidth;
        int _textHeight;
    };

public:
    InternalWindow(const std::string &title);

    bool notifyMousePointerMoved(const Events::MousePointerMovedEvent& e) override;
    bool notifyMouseButtonPressed(const Events::MouseButtonPressedEvent& e) override;
    bool notifyMouseButtonReleased(const Events::MouseButtonReleasedEvent& e) override;

    void draw(DrawingContext& drawingContext) override;

public:
    virtual void setSize(const Vector2f& size) override;

    void setTransparency(float alpha);

    void addComponent(const std::shared_ptr<Component>& component) override;

protected:
    void drawContainer(DrawingContext& drawingContext) override;

protected:
    std::unique_ptr<TitleBar> _titleBar;
    std::shared_ptr<Image> _closeButton;
    DeferredTexture _background;
    bool _mouseOver;
    bool _mouseOverCloseButton;
    bool _isDragging;
    Vector2f _mouseDragOffset;
    float _transparency;
protected:
    bool _firstDraw;
};

} // namespace Ego::GUI
