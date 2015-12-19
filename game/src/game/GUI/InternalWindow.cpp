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

static const int BORDER_PIXELS = 5;

InternalWindow::TitleBar::TitleBar(const std::string &title) :
    _titleBarTexture("mp_data/titlebar"),
    _titleSkull("mp_data/gui-skull"),
    _font(_gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)),
    _title(title),
    _textWidth(0),
    _textHeight(0)
{
    //Make title upper case
    std::transform(_title.begin(), _title.end(), _title.begin(), ::toupper);

    //Set size depending on title string
    _font->getTextSize(_title, &_textWidth, &_textHeight);
    _textWidth = std::max<int>(32, _textWidth);
    _textHeight = std::max<int>(32, _textHeight);
    setSize(_textWidth + 20, _textHeight+5);
}

void InternalWindow::TitleBar::draw()
{
    //Background
    _gameEngine->getUIManager()->drawImage(_titleBarTexture.get(), getX()-BORDER_PIXELS*2, getY(), getWidth()+BORDER_PIXELS*4, getHeight());

    //Title String
    _font->drawText(_title, getX() + getWidth()/2 - _textWidth/2, getY() + 12, Ego::Colour4f(0.28f, 0.16f, 0.07f, 1.0f));

    //Draw the skull icon on top
    const int skullWidth = _titleSkull.get_ptr()->getWidth()/2;
    const int skullHeight = _titleSkull.get_ptr()->getHeight()/2;
    _gameEngine->getUIManager()->drawImage(_titleSkull.get(), getX()+getWidth()/2 - skullWidth/2, getY() - skullHeight/2, skullWidth, skullHeight);
}

InternalWindow::InternalWindow(const std::string &title) :
    _titleBar(new TitleBar(title)),
    _background("mp_data/guiwindow"),
    _mouseOver(false),
    _mouseOverCloseButton(false),
    _isDragging(false),
    _mouseDragOffset(0.0f, 0.0f),
    _transparency(0.33f),
    _firstDraw(true)
{
    setPosition(20, 20);
}

void InternalWindow::drawContainer()
{
    //Draw background first
    _gameEngine->getUIManager()->drawImage(_background.get(), getX()-BORDER_PIXELS, getY()-BORDER_PIXELS, getWidth()+BORDER_PIXELS*2, getHeight()+BORDER_PIXELS*2, Ego::Colour4f(1.0f, 1.0f, 1.0f, 0.9f));

    //Draw window title
    _titleBar->draw();

    //Draw an X in top right corner (TODO: Make into a proper button)
    Ego::Math::Colour4f X_HOVER = Ego::Math::Colour4f::white();
    Ego::Math::Colour4f X_DEFAULT(.56f, .56f, .56f, 1.0f);
    _gameEngine->getUIManager()->getFont(UIManager::FONT_GAME)->drawText("X", getX() + getWidth() - 20, getY() + 20, _mouseOverCloseButton ? X_HOVER : X_DEFAULT);
}

bool InternalWindow::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        setPosition( Ego::Math::constrain<int>(x+_mouseDragOffset[0], 0, _gameEngine->getUIManager()->getScreenWidth()-getWidth()), 
                     Ego::Math::constrain<int>(y+_mouseDragOffset[1], _titleBar->getHeight()/2, _gameEngine->getUIManager()->getScreenHeight()-getHeight()) );
        return true;
    }
    else {
        _mouseOver = InternalWindow::contains(x, y) || _titleBar->contains(x, y);

        //Check if mouse is hovering over the close button (TODO: Make into a proper button)
        if(_mouseOver) {
            Ego::Rectangle<int> closeButton = Ego::Rectangle<int>(getX() + getWidth() - 20 - BORDER_PIXELS*2, getY() + 25 + BORDER_PIXELS*2, getX() + getWidth()-BORDER_PIXELS, getY()+15);
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
            _mouseDragOffset[0] = getX() - x;
            _mouseDragOffset[1] = getY() - y;

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

bool InternalWindow::notifyMouseReleased(const int button, const int x, const int y)
{
    _isDragging = false;
    return false;
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

void InternalWindow::addComponent(std::shared_ptr<GUIComponent> component)
{
    //Make sure that all components added to this window are placed relative to 
    //our position so that (0,0) is topleft corner in this InternalWindow
    if(!_firstDraw) {
        component->setPosition(component->getX()+getX(), component->getY()+getY());
    }
    ComponentContainer::addComponent(component);
}

void InternalWindow::setSize(const int width, const int height)
{
    //Also update the width of the title bar if our with changes
    _titleBar->setSize(width, _titleBar->getHeight());

    GUIComponent::setSize(width, height);
}
