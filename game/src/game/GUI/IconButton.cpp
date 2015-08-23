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
#include "egolib/Renderer/DeferredOpenGLTexture.hpp"

IconButton::IconButton(const std::string &buttonText, const Ego::DeferredOpenGLTexture& icon, int hotkey) : Button(buttonText, hotkey),
	_icon(icon)
{
	//ctor
}

void IconButton::draw()
{
    //Update slidy button effect
    updateSlidyButtonEffect();
    
    auto &renderer = Ego::Renderer::get();

    // Draw the button
    oglx_texture_t::bind(nullptr);

    //Determine button color
    if(!isEnabled())
    {
        renderer.setColour( DISABLED_BUTTON_COLOUR );
    }
    else if(_mouseOver)
    {
        renderer.setColour( HOVER_BUTTON_COLOUR );
    }
    else
    {
        renderer.setColour( DEFAULT_BUTTON_COLOUR );
    }

    struct Vertex
    {
        float x, y;
    };
    auto vb = _gameEngine->getUIManager()->_vertexBuffer;
    Vertex *v = static_cast<Vertex *>(vb->lock());
    v->x = getX(); v->y = getY(); v++;
    v->x = getX(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY() + getHeight(); v++;
    v->x = getX() + getWidth(); v->y = getY();
    vb->unlock();
    renderer.render(*vb, Ego::PrimitiveType::Quadriliterals, 0, 4);

 	//Draw icon
 	int iconSize = getHeight()-4;
	draw_icon_texture(_icon.get_ptr(), getX() + getWidth() - getHeight()-2, getY()+2, 0xFF, 0, iconSize);

    //Draw text on left side in button
    if(!_buttonText.empty())
    {
        int textWidth, textHeight;
        _gameEngine->getUIManager()->getDefaultFont()->getTextSize(_buttonText, &textWidth, &textHeight);

        _gameEngine->getUIManager()->getDefaultFont()->drawText(_buttonText, getX() + 5, getY() + (getHeight()-textHeight)/2);
    }
}

