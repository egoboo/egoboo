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

/// @file game/gui/IconButton.cpp
/// @details A button with an small icon on the right side
/// @author Johan Jansen

#include "game/gui/IconButton.hpp"
#include "game/ui.h"
#include "game/graphic_texture.h"

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
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );

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

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

 	//Draw icon
 	int iconSize = getHeight()-4;
 	if(_icon != INVALID_TX_REF) {
    	draw_icon_texture(TxList_get_valid_ptr(_icon), getX() + getWidth() - getHeight()-2, getY()+2, 0xFF, 0, iconSize);
 	}

    //Draw text on left side in button
    if(!_buttonText.empty())
    {
        int textWidth, textHeight;
        fnt_getTextSize(ui_getFont(), _buttonText.c_str(), &textWidth, &textHeight);

        GL_DEBUG( glColor4fv )(Ego::white_vec);
        fnt_drawText_OGL_immediate(ui_getFont(), {0xFF, 0xFF, 0xFF, 0x00}, getX() + 5, getY() + (getHeight()-textHeight)/2, "%s", _buttonText.c_str());
    }
}

