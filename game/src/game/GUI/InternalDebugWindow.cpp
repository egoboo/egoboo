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

/// @file game/GUI/InternalDebugWindow.cpp
/// @details InternalDebugWindow
/// @author Johan Jansen

#include "game/GUI/InternalDebugWindow.hpp"
#include "egolib/Audio/AudioSystem.hpp"

InternalDebugWindow::InternalDebugWindow(const std::string &title) :
	_mouseOver(false),
	_mouseOverCloseButton(false),
	_isDragging(false),
	_title(title),
	_watchedVariables()
{
	//Set window size depending on title string
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_title, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(std::max(getWidth(), 5 + static_cast<int>(textWidth*1.5f)), getY()+textHeight+5);
}

void InternalDebugWindow::addWatchVariable(const std::string &variableName, std::function<std::string()> lambda)
{
	//Add variable to watch list
	_watchedVariables[variableName] = lambda;

	//Make the window bigger
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDebugFont()->getTextSize(variableName, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(std::max(getWidth(), 5 + textWidth*2), getHeight()+textHeight+5);
}

void InternalDebugWindow::draw()
{
	const GLXvector4f BACKDROP_COLOUR = {0.66f, 0.00f, 0.00f, 0.60f};
	const GLXvector4f TITLE_BAR_COLOUR = {0.20f, 0.20f, 0.66f, 0.60f};

    // Draw the backdrop
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    GL_DEBUG( glColor4fv )( BACKDROP_COLOUR );

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    //Rendering variables
    int textWidth, textHeight;
    int xOffset = getX() + 5;
    int yOffset = getY();
    _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_title, &textWidth, &textHeight);

    //Draw title bar
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    GL_DEBUG( glColor4fv )( BACKDROP_COLOUR );
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+textHeight );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+textHeight );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();
    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    //Draw window title first
    _gameEngine->getUIManager()->getDefaultFont()->drawText(_title, xOffset, yOffset);
    yOffset += textHeight + 5;

    //Draw all monitored variables
    for(const auto &element : _watchedVariables)
    {
        _gameEngine->getUIManager()->getDebugFont()->drawText(element.first + ": " + element.second(), xOffset, yOffset);

        _gameEngine->getUIManager()->getDebugFont()->getTextSize(element.first, &textWidth, &textHeight);
        yOffset += textHeight + 5;
    }

    //Draw an X in top right corner
    Ego::Math::Colour4f X_HOVER = Ego::Math::Colour4f::white();
    Ego::Math::Colour4f X_DEFAULT(.56f, .56f, .56f, 1.0f);
    _gameEngine->getUIManager()->getDefaultFont()->drawText("X", getX() + getWidth() - 16, getY(), _mouseOverCloseButton ? X_HOVER : X_DEFAULT);
}

bool InternalDebugWindow::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
    	setPosition(x, y);
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

    return false;
}

bool InternalDebugWindow::notifyMouseClicked(const int button, const int x, const int y)
{
    if(_mouseOver && button == SDL_BUTTON_LEFT)
    {
    	//Check if close button is pressed first
    	if(_mouseOverCloseButton) {
    		_audioSystem.playSoundFull(_audioSystem.getGlobalSound(GSND_BUTTON_CLICK));
    		destroy();
    		return true;
    	}

    	_isDragging = !_isDragging;
        return true;
    }
    else if(button == SDL_BUTTON_RIGHT) {
    	_isDragging = false;
    	return true;
    }

    return false;
}
