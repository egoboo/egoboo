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
    _background(std::unique_ptr<oglx_texture_t>(new oglx_texture_t())),
    _mouseOver(false),
    _mouseOverCloseButton(false),
    _isDragging(false),
    _mouseDragOffset{0, 0},
    _title(title),
    _transparency(0.33f),
    _firstDraw(true)
{
    //Load background
    ego_texture_load_vfs(_background.get(), "mp_data/tooltip", TRANSCOLOR);

    //Set window size depending on title string
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_title, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(std::max(getWidth(), 5 + static_cast<int>(textWidth*1.5f)), getY()+textHeight+5);
    setPosition(20, 20);
}

void InternalWindow::drawContainer()
{
    //Draw background first
    _gameEngine->getUIManager()->drawImage(*_background.get(), getX(), getY(), getWidth(), getHeight(), Ego::Colour4f(1.0f, 1.0f, 1.0f, 0.9f));

    //Draw window title
    _gameEngine->getUIManager()->getDefaultFont()->drawText(_title, getX() + 5, getY());

    //Draw an X in top right corner
    Ego::Math::Colour4f X_HOVER = Ego::Math::Colour4f::white();
    Ego::Math::Colour4f X_DEFAULT(.56f, .56f, .56f, 1.0f);
    _gameEngine->getUIManager()->getDefaultFont()->drawText("X", getX() + getWidth() - 16, getY(), _mouseOverCloseButton ? X_HOVER : X_DEFAULT);
}

bool InternalWindow::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        setPosition(x+_mouseDragOffset.x, y+_mouseDragOffset.y);
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

        //Bring the window in front of all other windows
        bringToFront();

        //Only the top title bar triggers dragging
        if(y-getY() < _gameEngine->getUIManager()->getFont(UIManager::FONT_DEFAULT)->getFontHeight()) {
            _isDragging = true;
            _mouseDragOffset.x = getX() - x;
            _mouseDragOffset.y = getY() - y;

            //Move the window immediatly
            return notifyMouseMoved(x, y);
        }
        else {
            _isDragging = false;
        }
    }
    else if(button == SDL_BUTTON_RIGHT) {
        _isDragging = false;
    }

    return ComponentContainer::notifyMouseClicked(button, x, y);
}

void InternalWindow::draw()
{
    if(_firstDraw) {
        _firstDraw = false;

        //Make sure that all components added to this window are placed relative to 
        //our position so that (0,0) is topleft corner in this InternalWindow
        for(const std::shared_ptr<GUIComponent> &component : ComponentContainer::iterator())
        {
            component->setPosition(component->getX()+getX(), component->getY()+getY());
        }
    }
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
    for(const std::shared_ptr<GUIComponent> &component : ComponentContainer::iterator())
    {
        component->setPosition(component->getX() + translateX, component->getY() + translateY);
    }
}

void InternalWindow::setTransparency(float alpha)
{
    _transparency = Ego::Math::constrain(alpha, 0.0f, 1.0f);
}

bool InternalWindow::notifyMouseReleased(const int button, const int x, const int y)
{
    _isDragging = false;
    return false;
}

void InternalWindow::addComponent(std::shared_ptr<GUIComponent> component)
{
    //Make sure that all components added to this window are placed relative to 
    //our position so that (0,0) is topleft corner in this InternalWindow
    if(!_firstDraw) {
        component->setPosition(component->getX()+getX(), component->getY()+getY());
    }
    ComponentContainer::addComponent(component);
}
