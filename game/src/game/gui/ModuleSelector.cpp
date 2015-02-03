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

/// @file game/gui/ModuleSelector.cpp
/// @details GUI widget to select which module to play
/// @author Johan Jansen

#include "game/gui/ModuleSelector.hpp"
#include "game/gui/Button.hpp"

ModuleSelector::ModuleSelector() :
	_startIndex(0)
{
    // Figure out at what offset we want to draw the module menu.
    int moduleMenuOffsetX = ( GFX_WIDTH  - 640 ) / 2;
    moduleMenuOffsetX = std::max(0, moduleMenuOffsetX);

    int moduleMenuOffsetY = ( GFX_HEIGHT - 480 ) / 2;
    moduleMenuOffsetY = std::max(0, moduleMenuOffsetY);

	//Set backdrop size and position
	setSize(330, 250);
	setPosition(moduleMenuOffsetX + 21, moduleMenuOffsetY + 173);

	std::shared_ptr<Button> nextModuleButton = std::make_shared<Button>("->", SDLK_RIGHT);
	nextModuleButton->setPosition(moduleMenuOffsetX + 590, moduleMenuOffsetY + 74);
	nextModuleButton->setSize(30, 30);
	addComponent(nextModuleButton);

	std::shared_ptr<Button> previousModuleButton = std::make_shared<Button>("<-", SDLK_LEFT);
	previousModuleButton->setPosition(moduleMenuOffsetX + 20, moduleMenuOffsetY + 74);
	previousModuleButton->setSize(30, 30);
	addComponent(previousModuleButton);	
}

void ModuleSelector::drawContainer()
{
	const GLXvector4f buttonColour = {0.66f, 0.0f, 0.0f, 0.6f};

    //Draw backdrop
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    
    GL_DEBUG( glColor4fv )( buttonColour );
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( getX(), getY() );
        GL_DEBUG( glVertex2f )( getX(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY()+getHeight() );
        GL_DEBUG( glVertex2f )( getX()+getWidth(), getY() );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );
}
