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

/// @file game/GUI/InternalWindow.cpp
/// @details InternalWindow
/// @author Johan Jansen

#include "game/GUI/InternalWindow.hpp"

InternalWindow::InternalWindow(const std::string &title) :
    _mouseOver(false),
    _mouseOverCloseButton(false),
    _isDragging(false),
    _title(title)
{
    //Set window size depending on title string
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_title, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(std::max(getWidth(), 5 + static_cast<int>(textWidth*1.5f)), getY()+textHeight+5);
}

void InternalWindow::drawContainer()
{
    const Ego::Math::Colour4f BACKDROP_COLOUR = {0.66f, 0.00f, 0.00f, 0.60f};
    const Ego::Math::Colour4f TITLE_BAR_COLOUR = {0.20f, 0.20f, 0.66f, 0.60f};
    
    auto &renderer = Ego::Renderer::get();

    struct Vertex
    {
        float x, y;
    };
    auto vb = _gameEngine->getUIManager()->_vertexBuffer;
    Vertex *v;

    // Draw the backdrop
    oglx_texture_t::bind(nullptr);
    renderer.setColour(BACKDROP_COLOUR);

    v = static_cast<Vertex *>(vb->lock());
    v->x = getX(); v->y = getY(); v++;
    v->x = getX(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY();
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

    //Rendering variables
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_title, &textWidth, &textHeight);

    //Draw title bar
    renderer.setColour(BACKDROP_COLOUR);

    v = static_cast<Vertex *>(vb->lock());
    v->x = getX(); v->y = getY(); v++;
    v->x = getX(); v->y = getY() + textHeight; v++;
    v->x = getX() + getWidth(); v->y = getY() + textHeight; v++;
    v->x = getX() + getWidth(); v->y = getY();
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

    //Draw window title first
    _gameEngine->getUIManager()->getDefaultFont()->drawText(_title, getX() + 5, getY());

    //Draw an X in top right corner
    Ego::Math::Colour4f X_HOVER = Ego::Math::Colour4f::white();
    Ego::Math::Colour4f X_DEFAULT(.56f, .56f, .56f, 1.0f);
    _gameEngine->getUIManager()->getDefaultFont()->drawText("X", getX() + getWidth() - 16, getY(), _mouseOverCloseButton ? X_HOVER : X_DEFAULT);
}

bool InternalWindow::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        setPosition(x, y);
        return true;
    }
    else {
        _mouseOver = contains(x, y);

        //Check if mouse is hovering over the close button
        if(_mouseOver) {
            Ego::Rectangle<int> closeButton = Ego::Rectangle<int>(getX() + getWidth()-32, getY()+32, getX() + getWidth(), getY());
            _mouseOverCloseButton = closeButton.point_inside(x, y);
        }
        else {
            _mouseOverCloseButton = false;
        }
    }

    return ComponentContainer::notifyMouseMoved(x, y);
}

bool InternalWindow::notifyMouseClicked(const int button, const int x, const int y)
{
    if(_mouseOver && button == SDL_BUTTON_LEFT)
    {
        //Check if close button is pressed first
        if(_mouseOverCloseButton) {
            AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_BUTTON_CLICK));
            destroy();
            return true;
        }

        _isDragging = !_isDragging;

        if(_isDragging) {
            return notifyMouseMoved(x, y);
        }

        return true;
    }
    else if(button == SDL_BUTTON_RIGHT) {
        _isDragging = false;
        return true;
    }

    return ComponentContainer::notifyMouseClicked(button, x, y);
}

void InternalWindow::draw()
{
    drawAll();
}

void InternalWindow::setPosition(const int x, const int y)
{
    //Calculate offsets in position change
    int translateX = x - getX();
    int translateY = y - getY();

    //Shift window position
    GUIComponent::setPosition(x, y);

    //Shift all child components as well
    _componentListMutex.lock();
    for(const std::shared_ptr<GUIComponent> &component : _componentList)
    {
        component->setPosition(component->getX() + translateX, component->getY() + translateY);
    }
    _componentListMutex.unlock();
}
