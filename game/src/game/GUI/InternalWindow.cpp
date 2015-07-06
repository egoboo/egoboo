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

InternalWindow::TitleBar::TitleBar(const std::string &title) :
    _titleBarTexture(std::unique_ptr<oglx_texture_t>(new oglx_texture_t())),
    _titleSkull(std::unique_ptr<oglx_texture_t>(new oglx_texture_t())),
    _font(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)),
    _title(title),
    _textWidth(0),
    _textHeight(0)
{
    ego_texture_load_vfs(_titleBarTexture.get(), "mp_data/titlebar", TRANSCOLOR);
    ego_texture_load_vfs(_titleSkull.get(), "mp_data/gui-skull", TRANSCOLOR);

    //Make title upper case
    std::transform(_title.begin(), _title.end(), _title.begin(), ::toupper);

    //Set size depending on title string
    _font->getTextSize(_title, &_textWidth, &_textHeight);
    _textWidth = std::max<int>(32, _textWidth);
    _textHeight = std::max<int>(32, _textHeight);
    setSize(_textWidth + 20, _textHeight);
}

void InternalWindow::TitleBar::draw()
{
    //Background
    _gameEngine->getUIManager()->drawImage(*_titleBarTexture.get(), getX()-5, getY(), getWidth()+10, getHeight());

    //Title String
    _font->drawText(_title, getX() + getWidth()/2 - _textWidth/2, getY() - _textHeight/2, Ego::Colour4f(0.28f, 0.16f, 0.07f, 1.0f));

    //Draw the skull icon on top
    const int skullWidth = _titleSkull->getWidth()/2;
    const int skullHeight = _titleSkull->getHeight()/2;
    _gameEngine->getUIManager()->drawImage(*_titleSkull.get(), getX()+getWidth()/2 - skullWidth/2, getY() - skullHeight/2, skullWidth, skullHeight);
}

InternalWindow::InternalWindow(const std::string &title) :
    _titleBar(new TitleBar(title)),
    _background(std::unique_ptr<oglx_texture_t>(new oglx_texture_t())),
    _mouseOver(false),
    _mouseOverCloseButton(false),
    _isDragging(false),
    _mouseDragOffset{0, 0},
    _transparency(0.33f),
    _firstDraw(true)
{
    //Load background
    ego_texture_load_vfs(_background.get(), "mp_data/guiwindow", TRANSCOLOR);

    setPosition(20, 20);
}

void InternalWindow::drawContainer()
{
    //Draw background first
    _gameEngine->getUIManager()->drawImage(*_background.get(), getX(), getY(), getWidth(), getHeight(), Ego::Colour4f(1.0f, 1.0f, 1.0f, 0.9f));

    //Draw window title
    _titleBar->draw();

    //Draw an X in top right corner
    Ego::Math::Colour4f X_HOVER = Ego::Math::Colour4f::white();
    Ego::Math::Colour4f X_DEFAULT(.56f, .56f, .56f, 1.0f);
    _gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)->drawText("X", getX() + getWidth() - 20, getY() + 20, _mouseOverCloseButton ? X_HOVER : X_DEFAULT);
}

bool InternalWindow::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        setPosition(x+_mouseDragOffset.x, y+_mouseDragOffset.y);
        return true;
    }
    else {
        _mouseOver = InternalWindow::contains(x, y) || _titleBar->contains(x, y);

        //Check if mouse is hovering over the close button
        if(_mouseOver) {
            Ego::Rectangle<int> closeButton = Ego::Rectangle<int>(getX() + getWidth()-32, getY(), getX() + getWidth(), getY()-32);
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
        if(_titleBar->contains(x, y)) {
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

    //Update titlebar position
    _titleBar->setPosition(x, y - _titleBar->getHeight()/2);

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
 
    //Dynamically resize window to fit added components
    //TODO

    ComponentContainer::addComponent(component);
}

void InternalWindow::setSize(const int width, const int height)
{
    //Also update the width of the title bar if our with changes
    _titleBar->setSize(width, _titleBar->getHeight());

    GUIComponent::setSize(width, height);
}
