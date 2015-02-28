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

/// @file game/gui/InternalDebugWindow.cpp
/// @details InternalDebugWindow
/// @author Johan Jansen

#include "game/gui/InternalDebugWindow.hpp"

InternalDebugWindow::InternalDebugWindow(const std::string &title) :
	_mouseOver(false),
	_isDragging(false),
	_title(title),
	_watchedVariables()
{
	//Set window size depending on title string
    int textWidth, textHeight;
    fnt_getTextSize(_gameEngine->getUIManager()->getDefaultFont(), _title.c_str(), &textWidth, &textHeight);
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
    fnt_getTextSize(_gameEngine->getUIManager()->getDebugFont(), variableName.c_str(), &textWidth, &textHeight);
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
    fnt_getTextSize(_gameEngine->getUIManager()->getDefaultFont(), _title.c_str(), &textWidth, &textHeight);

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
	Ego::Renderer::get().setColour(Ego::Colour4f::WHITE);
    fnt_drawText_OGL_immediate(_gameEngine->getUIManager()->getDefaultFont(), {0xFF, 0xFF, 0xFF, 0x00}, xOffset, yOffset, "%s", _title.c_str());
    yOffset += textHeight + 5;

    //Draw all monitored variables
    for(const auto &element : _watchedVariables)
    {
        fnt_drawText_OGL_immediate(_gameEngine->getUIManager()->getDebugFont(), {0xFF, 0xFF, 0xFF, 0x00}, xOffset, yOffset, "%s: %s", element.first.c_str(), element.second().c_str());

        fnt_getTextSize(_gameEngine->getUIManager()->getDebugFont(), element.first.c_str(), &textWidth, &textHeight);
        yOffset += textHeight + 5;
    }

    //Draw an X in top right corner
    fnt_drawText_OGL_immediate(_gameEngine->getUIManager()->getDefaultFont(), {0xFF, 0xFF, 0xFF, 0x00}, getX()+getWidth()-16, getY(), "X");
}

bool InternalDebugWindow::notifyMouseMoved(const int x, const int y)
{

    if(_isDragging) {
    	setPosition(x, y);
    }
    else {
	    _mouseOver = contains(x, y);
    }

    return false;
}

bool InternalDebugWindow::notifyMouseClicked(const int button, const int x, const int y)
{
    if(_mouseOver && button == SDL_BUTTON_LEFT)
    {
    	_isDragging = !_isDragging;
        return true;
    }
    else if(button == SDL_BUTTON_RIGHT) {
    	_isDragging = false;
    }

    return false;
}
