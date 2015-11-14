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

/// @file game/GUI/MiniMap.cpp
/// @details GUI widget to render that tiny minimap in the corner
/// @author Johan Jansen

#include "MiniMap.hpp"
#include "game/Core/GameEngine.hpp"
#include "egolib/Logic/Team.hpp"
#include "game/Module/Module.hpp"
#include "game/Entities/_Include.hpp"
#include "game/player.h"

static const uint32_t MINIMAP_BLINK_RATE = 500; //milliseconds between each minimap blink

MiniMap::MiniMap() :
    _markerBlinkTimer(0),
    _showPlayerPosition(false),
    _blips(),
    _mouseOver(false),
    _isDragging(false),
    _mouseDragOffset(0.0f, 0.0f),
    _minimapTexture(nullptr)
{
    //The minimap is by default not visible
    setVisible(false);

    if(ego_texture_exists_vfs("mp_data/plan")) {
        _minimapTexture = std::make_shared<Ego::DeferredOpenGLTexture>("mp_data/plan");
    }
    else {
		Log::get().warn("mp_data/plan - Cannot load file!\n");
    }
}

void MiniMap::draw()
{
    if(!_minimapTexture) {
        return;
    }

    //Draw the map image
    _gameEngine->getUIManager()->drawImage(_minimapTexture->get(), getX(), getY(), getWidth(), getHeight(), Ego::Math::Colour4f(1.0f, 1.0f, 1.0f, 0.9f));

    // If one of the players can sense enemies via ESP, draw them as blips on the map
    if (Team::TEAM_MAX != local_stats.sense_enemies_team)
    {
        for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
        {
            if (pchr->isTerminated()) continue;

            const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

            // Show only teams that will attack the player
            if (pchr->getTeam().hatesTeam(_currentModule->getTeamList()[local_stats.sense_enemies_team]))
            {
                // Only if they match the required IDSZ ([NONE] always works)
                if (local_stats.sense_enemies_idsz == IDSZ_NONE ||
                    local_stats.sense_enemies_idsz == profile->getIDSZ(IDSZ_PARENT) ||
                    local_stats.sense_enemies_idsz == profile->getIDSZ(IDSZ_TYPE))
                {
                    // Red blips
                    addBlip(pchr->getPosX(), pchr->getPosY(), COLOR_RED);
                }
            }
        }
    }

    // Show local player position(s)
    if (_showPlayerPosition && SDL_GetTicks() < _markerBlinkTimer)
    {
        for (PLA_REF iplayer = 0; iplayer < MAX_PLAYER; iplayer++)
        {
            // Only valid players
            if (!PlaStack.lst[iplayer].valid) continue;

            const std::shared_ptr<Object> &player = _currentModule->getObjectHandler()[PlaStack.lst[iplayer].index];

            if (!player->isTerminated() && player->isAlive())
            {
                addBlip(player->getPosX(), player->getPosY(), player);
            }
        }
    }
    else
    {
        _markerBlinkTimer = SDL_GetTicks() + MINIMAP_BLINK_RATE;        
    }

    const int BLIP_SIZE = std::min(getWidth(), getHeight()) / 16;

    //Draw all queued blips
    for(const Blip &blip : _blips)
    {
        //Adjust the position values so that they fit inside the minimap
        float x = getX() + (blip.x * getWidth() / _currentModule->getMeshPointer()->_gmem._edge_x);
        float y = getY() + (blip.y * getHeight() / _currentModule->getMeshPointer()->_gmem._edge_y);

        if(blip.icon != nullptr)
        {            
            //Center icon on blip position
            x -= BLIP_SIZE/2;
            y -= BLIP_SIZE/2;

            draw_icon_texture(blip.icon, x, y, 0xFF, 0, BLIP_SIZE, true);
        }
        else
        {
            draw_blip(0.75f, blip.color, x, y);
        }
    }
    _blips.clear();

}

void MiniMap::setShowPlayerPosition(bool show)
{
    _showPlayerPosition = show;
}

void MiniMap::addBlip(const float x, const float y, const HUDColors color)
{
    if (!_currentModule->isInside(x, y)) {
        return;
    }

    _blips.push_back(Blip(x, y, color));
}

void MiniMap::addBlip(const float x, const float y, const std::shared_ptr<Object> &object)
{
    if (!_currentModule->isInside(x, y)) {
        return;
    }

    _blips.push_back(Blip(x, y, chr_get_txtexture_icon_ref(object->getCharacterID())));
}

bool MiniMap::notifyMouseMoved(const int x, const int y)
{
    if(_isDragging) {
        setPosition( Ego::Math::constrain<int>(x+_mouseDragOffset[0], 0, _gameEngine->getUIManager()->getScreenWidth()-getWidth()), 
                     Ego::Math::constrain<int>(y+_mouseDragOffset[1], 0, _gameEngine->getUIManager()->getScreenHeight()-getHeight()) );
    }
    else {
        _mouseOver = contains(x, y);
    }

    return false;
}

bool MiniMap::notifyMouseClicked(const int button, const int x, const int y)
{
    if(_mouseOver && button == SDL_BUTTON_LEFT)
    {
        //Bring the window in front of all other windows
        bringToFront();

        _isDragging = true;
        _mouseDragOffset[0] = getX() - x;
        _mouseDragOffset[1] = getY() - y;

        //Move the window immediatly
        return notifyMouseMoved(x, y);    }
    else if(button == SDL_BUTTON_RIGHT) {
        _isDragging = false;
        return true;
    }

    return false;
}

bool MiniMap::notifyKeyDown(const int keyCode)
{
    //Enlarge minimap
    if(keyCode == SDLK_m)
    {
        if(isVisible())
        {
            const float HALF_SCREEN_WIDTH = _gameEngine->getUIManager()->getScreenWidth() / 2;
            const float HALF_SCREEN_HEIGHT = _gameEngine->getUIManager()->getScreenHeight() / 2;

            if(getWidth() > MiniMap::MAPSIZE) {
                float offsetX = (getX() >= HALF_SCREEN_WIDTH) ? (getWidth()-MiniMap::MAPSIZE) : 0;
                float offsetY = (getY() >= HALF_SCREEN_HEIGHT) ? (getHeight()-MiniMap::MAPSIZE) : 0;

                //Shift position when becoming smaller towards one of the screen corners
                setPosition(getX() + offsetX, getY() + offsetY);
                setSize(MiniMap::MAPSIZE, MiniMap::MAPSIZE);
            }
            else {
                setSize(HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT);
            }

            //Keep minimap inside the screen
            int xPos = Ego::Math::constrain<int>(getX(), 0, _gameEngine->getUIManager()->getScreenWidth() - getWidth());
            int yPos = Ego::Math::constrain<int>(getY(), 0, _gameEngine->getUIManager()->getScreenHeight() - getHeight());
            setPosition(xPos, yPos);
        }

        return true;
    }

    return false;
}

bool MiniMap::notifyMouseReleased(const int button, const int x, const int y)
{
    _isDragging = false;
    return false;
}
