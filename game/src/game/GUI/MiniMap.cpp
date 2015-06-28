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
    _blips()
{

}

void MiniMap::draw()
{
    //Make sure map texture is valid
    oglx_texture_t *miniMapTexture = TextureManager::get().get_valid_ptr((TX_REF)TX_MAP);
    if(!miniMapTexture) {
        return;
    }

    //Draw the map image
    _gameEngine->getUIManager()->drawImage(*miniMapTexture, getX(), getY(), getWidth(), getHeight());

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
                addBlip(player->getPosX(), player->getPosY(), COLOR_WHITE);
            }
        }
    }
    else
    {
        _markerBlinkTimer = SDL_GetTicks() + MINIMAP_BLINK_RATE;        
    }

    //Draw all queued blips
    for(const Blip &blip : _blips)
    {
        //Adjust the position values so that they fit inside the minimap
        float x = getX() + (blip.x * MAPSIZE / PMesh->gmem.edge_x);
        float y = getY() + (blip.y * MAPSIZE / PMesh->gmem.edge_y);

        draw_blip(0.75f, blip.color, x, y);
    }
    _blips.clear();

}

void MiniMap::setShowPlayerPosition(bool show)
{
    _showPlayerPosition = show;
}

void MiniMap::addBlip(const float x, const float y, const uint8_t color)
{
    if (!_currentModule->isInside(x, y)) {
        return;
    }

    _blips.push_back(Blip(x, y, color));
}
