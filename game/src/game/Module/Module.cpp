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

/// @file game/Module/Module.cpp
/// @details Code handling a game module
/// @author Johan Jansen

#include "game/Module/Module.hpp"
#include "egolib/Math/Random.hpp"
#include "egolib/Logic/Team.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Module/Passage.hpp"
#include "game/game.h"
#include "game/network.h"
#include "game/player.h"
#include "game/mesh.h"
#include "game/char.h"

GameModule::GameModule(const std::shared_ptr<ModuleProfile> &profile, const uint32_t seed) :
    _moduleProfile(profile),
    _gameObjects(),
    _playerList(),
    _teamList(),
    _name(profile->getName()),
    _exportValid(profile->isExportAllowed()),
    _exportReset(profile->isExportAllowed()),
    _isRespawnValid(profile->isRespawnValid()),
    _isBeaten(false),
    _seed(seed),
    _passages()
{
    srand( _seed );
    Random::setSeed(_seed);

    //Initialize all teams
    for(int i = 0; i < Team::TEAM_MAX; ++i) {
        _teamList.push_back(Team(i));
    }
}

GameModule::~GameModule()
{

}

void GameModule::loadAllPassages()
{
    // Reset all of the old passages
    _passages.clear();

    // Load the file
    ReadContext ctxt("mp_data/passage.txt");
    if (!ctxt.ensureOpen()) return;

    //Load all passages in file
    while (ctxt.skipToColon(true))
    {
        //read passage area
        irect_t area;
        area._left = ctxt.readInt();
        area._top = ctxt.readInt();
        area._right = ctxt.readInt();
        area._bottom = ctxt.readInt();

        //constrain passage area within the level
        area._left = CLIP(area._left, 0, _currentModule->getMeshPointer()->info.tiles_x - 1);
        area._top = CLIP(area._top, 0, _currentModule->getMeshPointer()->info.tiles_y - 1);
        area._right = CLIP(area._right, 0, _currentModule->getMeshPointer()->info.tiles_x - 1);
        area._bottom = CLIP(area._bottom, 0, _currentModule->getMeshPointer()->info.tiles_y - 1);

        //Read if open by default
        bool open = ctxt.readBool();

        //Read mask (optional)
        uint8_t mask = MAPFX_IMPASS | MAPFX_WALL;
        if (ctxt.readBool()) mask = MAPFX_IMPASS;
        if (ctxt.readBool()) mask = MAPFX_SLIPPY;

        std::shared_ptr<Passage> passage = std::make_shared<Passage>(area, mask);

        //check if we need to close the passage
        if (!open) {
            passage->close();
        }

        //finished loading this one!
        _passages.push_back(passage);
    }
}

void GameModule::checkPassageMusic()
{
    // Look at each player
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++)
    {
        CHR_REF character = PlaStack.lst[ipla].index;
        if (!_currentModule->getObjectHandler().exists(character)) continue;

        // Don't do items in hands or inventory.
        if (IS_ATTACHED_CHR(character)) continue;

        Object *pchr = _currentModule->getObjectHandler().get(character);
        if (!pchr->isAlive() || !VALID_PLA(pchr->is_which_player)) continue;

        //Loop through every passage
        for (const std::shared_ptr<Passage>& passage : _passages)
        {
            if (passage->checkPassageMusic(pchr))
            {
                return;
            }
        }
    }
}

CHR_REF GameModule::getShopOwner(const float x, const float y)
{
    // Loop through every passage.
    for(const std::shared_ptr<Passage>& passage : _passages)
    {
        // Only check actual shops.
        if(!passage->isShop()) 
        {
            continue;
        }

        // Is item inside this shop?
        if(passage->isPointInside(x, y))
        {
            return passage->getShopOwner();
        }
    }

    return Passage::SHOP_NOOWNER;       
}

void GameModule::removeShopOwner(CHR_REF owner)
{
    // Loop through every passage:
    for(const std::shared_ptr<Passage> &passage : _passages)
    {
        // Only check actual shops:
        if(!passage->isShop())
        {
            continue;
        }

        if(passage->getShopOwner() == owner)
        {
            passage->removeShop();
        }

        // TODO: Mark all items in shop as normal items again.
    }
}

int GameModule::getPassageCount()
{
    return _passages.size();
}

std::shared_ptr<Passage> GameModule::getPassageByID(int id)
{
    if(id < 0 || id >= _passages.size())
    {
        return nullptr;
    }

    return _passages[id];
}

uint8_t GameModule::getImportAmount() const 
{
    return _moduleProfile->getImportAmount();
}

uint8_t GameModule::getPlayerAmount() const 
{
    return _moduleProfile->getMaxPlayers();
}

bool GameModule::isImportValid() const 
{
    return _moduleProfile->getImportAmount() > 0;
}

const std::string& GameModule::getPath() const 
{
    return _moduleProfile->getFolderName();
}

bool GameModule::canRespawnAnyTime() const 
{
    return _moduleProfile->hasRespawnAnytime();
}

uint8_t GameModule::getMaxPlayers() const
{
    return _moduleProfile->getMaxPlayers();
}

uint8_t GameModule::getMinPlayers() const
{
    return _moduleProfile->getMinPlayers();
}

bool GameModule::isInside(const float x, const float y) const
{
     return x > 0 && x < _currentModule->getMeshPointer()->gmem.edge_x && y > 0 && y < _currentModule->getMeshPointer()->gmem.edge_y;
}

std::shared_ptr<Object> GameModule::spawnObject(const fvec3_t& pos, const PRO_REF profile, const TEAM_REF team, const int skin, 
                                                const FACING_T facing, const char *name, const CHR_REF override)
{
    // fix a "bad" name
    if ( NULL == name ) name = "";

    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(profile);
    if (!ppro)
    {
        if ( profile > getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
            log_warning( "spawnObject() - trying to spawn using invalid profile %d\n", REF_TO_INT( profile ) );
        }
        return Object::INVALID_OBJECT;
    }

    // count all the requests for this character type
    ppro->_spawnRequestCount++;

    // allocate a new character
    std::shared_ptr<Object> pchr = getObjectHandler().insert(profile, override);
    if (!pchr) {
        log_warning( "spawnObject() - failed to spawn character\n" );
        return Object::INVALID_OBJECT;
    }

    // just set the spawn info
    pchr->spawn_data.pos = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = facing;
    strncpy( pchr->spawn_data.name, name, SDL_arraysize( pchr->spawn_data.name ) );
    pchr->spawn_data.override = override;

    chr_config_do_init(pchr.get());

    // start the character out in the "dance" animation
    chr_start_anim(pchr.get(), ACTION_DA, true, true);

    // count all the successful spawns of this character
    ppro->_spawnCount++;

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    log_debug( "spawnObject() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( pchr->getCharacterID() ), name, ppro->getClassName().c_str() );
#endif

    return pchr;
}


/// @todo Remove this global.
std::unique_ptr<GameModule> _currentModule = nullptr;
