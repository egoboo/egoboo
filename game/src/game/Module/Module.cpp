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

#include "game/ObjectPhysics.h" //only for move_one_character_get_environment()

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

        Object *pchr = _currentModule->getObjectHandler().get(character);
        if (!pchr->isAlive() || !VALID_PLA(pchr->is_which_player)) continue;

        // Don't do items in hands or inventory.
        if(pchr->isBeingHeld()) continue;

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
                                                const FACING_T facing, const std::string &name, const CHR_REF override)
{
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
    strncpy( pchr->spawn_data.name, name.c_str(), SDL_arraysize( pchr->spawn_data.name ) );
    pchr->spawn_data.override = override;

    // download all the values from the character spawn_ptr->profile
    // Set up model stuff
    pchr->stoppedby = ppro->getStoppedByMask();
    pchr->nameknown = ppro->isNameKnown();
    pchr->ammoknown = ppro->isNameKnown();
    pchr->draw_icon = ppro->isDrawIcon();

    // Starting Perks
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i) {
        Ego::Perks::PerkID id = static_cast<Ego::Perks::PerkID>(i);
        if(ppro->beginsWithPerk(id)) {
            pchr->addPerk(id);
        }
    }

    // Ammo
    pchr->ammomax = ppro->getMaxAmmo();
    pchr->ammo = ppro->getAmmo();

    // Gender
    pchr->gender = ppro->getGender();
    if ( pchr->gender == GENDER_RANDOM )
    {
        //50% male or female
        if(Random::nextBool())
        {
            pchr->gender = GENDER_FEMALE;
        }
        else
        {
            pchr->gender = GENDER_MALE;
        }
    }

    // Life and Mana bars
    pchr->setBaseAttribute(Ego::Attribute::LIFE_BARCOLOR, ppro->getLifeColor());
    pchr->setBaseAttribute(Ego::Attribute::MANA_BARCOLOR, ppro->getManaColor());

    // Flags
    pchr->damagetarget_damagetype = ppro->getDamageTargetType();
    pchr->setBaseAttribute(Ego::Attribute::WALK_ON_WATER, ppro->canWalkOnWater() ? 1.0f : 0.0f);
    pchr->platform        = ppro->isPlatform();
    pchr->canuseplatforms = ppro->canUsePlatforms();
    pchr->isitem          = ppro->isItem();
    pchr->invictus        = ppro->isInvincible();

    // Jumping
    pchr->setBaseAttribute(Ego::Attribute::JUMP_POWER, ppro->getJumpPower());
    pchr->setBaseAttribute(Ego::Attribute::NUMBER_OF_JUMPS, ppro->getJumpNumber());

    // Other junk
    pchr->setBaseAttribute(Ego::Attribute::FLY_TO_HEIGHT, ppro->getFlyHeight());
    pchr->phys.dampen = ppro->getBounciness();

    pchr->phys.bumpdampen = ppro->getBumpDampen();
    if ( CAP_INFINITE_WEIGHT == ppro->getWeight() )
    {
        pchr->phys.weight = CHR_INFINITE_WEIGHT;
    }
    else
    {
        uint32_t itmp = ppro->getWeight() * ppro->getSize() * ppro->getSize() * ppro->getSize();
        pchr->phys.weight = std::min( itmp, CHR_MAX_WEIGHT );
    }

    // Money is added later
    pchr->money = ppro->getStartingMoney();

    // Experience
    int iTmp = Random::next( ppro->getStartingExperience() );
    pchr->experience      = std::min( iTmp, MAXXP );
    pchr->experiencelevel = ppro->getStartingLevel();

    // Particle attachments
    pchr->reaffirm_damagetype = ppro->getReaffirmDamageType();

    // Character size and bumping
    chr_init_size(pchr.get(), ppro);

    // Kurse state
    if ( ppro->isItem() )
    {
        uint16_t kursechance = ppro->getKurseChance();
        if (egoboo_config_t::get().game_difficulty.getValue() >= Ego::GameDifficulty::Hard)
        {
            kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        }
        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Normal && kursechance != 100)
        {
            kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        }
        pchr->iskursed = Random::getPercent() <= kursechance;
    }

    // AI stuff
    ai_state_spawn( &( pchr->ai ), pchr->getCharacterID(), pchr->getProfileID(), getTeamList()[team].getMorale() );

    // Team stuff
    pchr->team     = team;
    pchr->team_base = team;
    if ( !pchr->isInvincible() )  getTeamList()[team].increaseMorale();

    // Firstborn becomes the leader
    if ( !getTeamList()[team].getLeader() )
    {
        getTeamList()[team].setLeader(pchr);
    }

    // getSkinOverride() can return NO_SKIN_OVERRIDE, so we need to check
    // for the "random skin marker" even if that function is called
    if (ppro->getSkinOverride() != ObjectProfile::NO_SKIN_OVERRIDE)
    {
        pchr->spawn_data.skin = ppro->getSkinOverride();
    }

    //Negative skin number means random skin
    else if (pchr->spawn_data.skin < 0)
    {
        // This is a "random" skin.
        // Force it to some specific value so it will go back to the same skin every respawn
        // We are now ensuring that there are skin graphics for all skins, so there
        // is no need to count the skin graphics loaded into the profile.
        // Limiting the available skins to ones that had unique graphics may have been a mistake since
        // the skin-dependent properties in data.txt may exist even if there are no unique graphics.
        pchr->spawn_data.skin = ppro->getRandomSkinID();
    }

    // actually set the character skin
    pchr->setSkin(pchr->spawn_data.skin);

    // override the default behavior for an "easy" game
    if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Normal)
    {
        pchr->setLife(pchr->getAttribute(Ego::Attribute::MAX_LIFE));
        pchr->setMana(pchr->getAttribute(Ego::Attribute::MAX_MANA));
    }
    else {
        pchr->setLife(ppro->getSpawnLife());
        pchr->setMana(ppro->getSpawnMana());        
    }

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    // grab all of the environment information
    move_one_character_get_environment( pchr.get() );

    pchr->setPosition(pos);

    pchr->pos_stt  = pos;
    pchr->pos_old  = pos;

    pchr->ori.facing_z     = facing;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    // Name the character
    if (name.empty())
    {
        // Generate a random name
        pchr->setName(ppro->generateRandomName());
    }
    else
    {
        // A name has been given
        pchr->setName(name);
    }

    // initalize the character instance
    chr_instance_t::spawn(pchr->inst, profile, pchr->spawn_data.skin);
    chr_update_matrix( pchr.get(), true );

    // Particle attachments
    for ( uint8_t tnc = 0; tnc < ppro->getAttachedParticleAmount(); tnc++ )
    {
        ParticleHandler::get().spawnParticle( pchr->getPosition(), pchr->ori.facing_z, ppro->getSlotNumber(), ppro->getAttachedParticleProfile(),
                            pchr->getCharacterID(), GRIP_LAST + tnc, pchr->team, pchr->getCharacterID(), INVALID_PRT_REF, tnc);
    }

    // is the object part of a shop's inventory?
    if ( pchr->isItem() )
    {
        // Items that are spawned inside shop passages are more expensive than normal

        CHR_REF shopOwner = getShopOwner(pchr->getPosX(), pchr->getPosY());
        if(shopOwner != Passage::SHOP_NOOWNER) {
            pchr->isshopitem = true;               // Full value
            pchr->iskursed   = false;              // Shop items are never kursed
            pchr->nameknown  = true;               // identified
            pchr->ammoknown  = true;
        }
        else {
            pchr->isshopitem = false;
        }
    }

    /// @author ZF
    /// @details override the shopitem flag if the item is known to be valuable
    /// @author BB
    /// @details this prevents (essentially) all books from being able to be burned
    //if ( pcap->isvaluable )
    //{
    //    pchr->isshopitem = true;
    //}

    // determine whether the object is hidden
    chr_update_hide( pchr.get() );

    chr_instance_t::update_ref(pchr->inst, pchr->enviro.grid_level, true );

    // start the character out in the "dance" animation
    chr_start_anim(pchr.get(), ACTION_DA, true, true);

    // count all the successful spawns of this character
    ppro->_spawnCount++;

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    log_debug( "spawnObject() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( pchr->getCharacterID() ), name.c_str(), ppro->getClassName().c_str() );
#endif

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    if ( _gameObjects.exists( pchr->attachedto ) && CHR_INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->getPosX(), pchr->getPosY(), nrm[kX], nrm[kY] );
    }
#endif

    return pchr;
}

/// @todo Remove this global.
std::unique_ptr<GameModule> _currentModule = nullptr;
