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

/// @file game/GUI/IconButton.cpp
/// @details A button with an small icon on the right side
/// @author Johan Jansen

#include "game/GUI/IconButton.hpp"

IconButton::IconButton(const std::string &buttonText, TX_REF icon, int hotkey) : Button(buttonText, hotkey),
	_icon(icon)
{
	//ctor
}

void IconButton::draw()
{
    //Update slidy button effect
    updateSlidyButtonEffect();

    // Draw the button
    oglx_texture_t::bind(nullptr);

    //Determine button color
    if(!isEnabled())
    {
        GL_DEBUG( glColor4fv )( DISABLED_BUTTON_COLOUR );
    }
    else if(_mouseOver)
    {
        GL_DEBUG( glColor4fv )( HOVER_BUTTON_COLOUR );
    }
    else
    {
        GL_DEBUG( glColor4fv )( DEFAULT_BUTTON_COLOUR );
    }

    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

 	//Draw icon
 	int iconSize = getHeight()-4;
 	if(_icon != INVALID_TX_REF) {
    	draw_icon_texture(TextureManager::get().get_valid_ptr(_icon), getX() + getWidth() - getHeight()-2, getY()+2, 0xFF, 0, iconSize);
 	}

    //Draw text on left side in button
    if(!_buttonText.empty())
    {
        int textWidth, textHeight;
        _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_buttonText, &textWidth, &textHeight);

        _gameEngine->getUIManager()->getDefaultFont()->drawText(_buttonText, getX() + 5, getY() + (getHeight()-textHeight)/2);
    }
}

