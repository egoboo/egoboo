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
#include "game/Logic/Player.hpp"
#include "game/Entities/_Include.hpp"
#include "game/CharacterMatrix.h"
#include "game/ObjectAnimation.h"

GameModule::GameModule(const std::shared_ptr<ModuleProfile> &profile, const uint32_t seed) :
    _moduleProfile(profile),
    _gameObjects(),
    _playerNameList(),
    _playerList(),    
    _teamList(),
    _name(profile->getName()),
    _exportValid(profile->isExportAllowed()),
    _exportReset(profile->isExportAllowed()),
    _isRespawnValid(profile->isRespawnValid()),
    _isBeaten(false),
    _seed(seed),

    _water(),
    _damageTile(),

    _passages(),
    _mesh(std::make_shared<ego_mesh_t>()),
    _tileTextures(),
    _waterTextures(),

    _pitsClock(PIT_CLOCK_RATE),
    _pitsKill(false),
    _pitsTeleport(false),
    _pitsTeleportPos()
{
    Log::get().info("Loading module \"%s\"\n", profile->getPath().c_str());

    srand( _seed );
    Random::setSeed(_seed);

    //Initialize all teams
    for(int i = 0; i < Team::TEAM_MAX; ++i) {
        _teamList.push_back(Team(i));
    }

    //Load tile textures
    for(size_t i = 0; i < _tileTextures.size(); ++i) {
        _tileTextures[i] = Ego::DeferredTexture("mp_data/tile" + std::to_string(i));
    }

    //Load water textures
    _waterTextures[0] = Ego::DeferredTexture("mp_data/waterlow");
    _waterTextures[1] = Ego::DeferredTexture("mp_data/watertop");

    // load a bunch of assets that are used in the module
    AudioSystem::get().loadGlobalSounds();
    ProfileSystem::get().loadGlobalParticleProfiles();

    //Load wavalite data
    wawalite_data_t *wavalite = read_wawalite_vfs();
    if (wavalite != nullptr) {
        _water.upload(wavalite->water);
        _damageTile.upload(wavalite->damagetile);
    }
    else {
        Log::get().warn( "wawalite.txt not loaded for %s.\n", profile->getPath().c_str() );
    }
    upload_wawalite();

    //Load all the profiles required by this module
    loadProfiles();

    //Load mesh
    MeshLoader meshLoader;
    _mesh = meshLoader(profile->getPath());

    //Load passage.txt
    loadAllPassages();

    //Load alliance.txt
    loadTeamAlliances();
}

GameModule::~GameModule()
{
    //free all particles
    ParticleHandler::get().clear();

    //Free all profiles loaded by the module
    ProfileSystem::get().reset();

    //Free all textures
    gfx_system_release_all_graphics();
}

void GameModule::loadProfiles()
{
    //Load the spell book profile
    ProfileSystem::get().loadOneProfile("mp_data/globalobjects/book.obj", SPELLBOOK);

    // Clear the import slots...
    import_data.slot_lst.fill(INVALID_PRO_REF);
    import_data.max_slot = -1;

    // This overwrites existing loaded slots that are loaded globally
    overrideslots = true;
    import_data.player = -1;
    import_data.slot   = -100;

    //Load any saved player characters from disk (if needed)
    if (isImportValid()) {
        for (int cnt = 0; cnt < getImportAmount()*MAX_IMPORT_PER_PLAYER; cnt++ )
        {
            std::ostringstream pathFormat;
            pathFormat << "mp_import/temp" << std::setw(4) << std::setfill('0') << cnt << ".obj";

            // Make sure the object exists...
            const std::string importPath = pathFormat.str();
            const std::string dataFilePath = importPath + "/data.txt";

            if ( vfs_exists(dataFilePath.c_str()) )
            {
                // new player found
                if (0 == (cnt % MAX_IMPORT_PER_PLAYER)) import_data.player++;

                // store the slot info
                import_data.slot = cnt;

                // load it
                import_data.slot_lst[cnt] = ProfileSystem::get().loadOneProfile(importPath);
                import_data.max_slot      = std::max(import_data.max_slot, cnt);
            }
        }
    }

    // return this to the normal value
    overrideslots = false;

    // load all module-specific object profiels
    game_load_module_profiles(_moduleProfile->getPath());   // load the objects from the module's directory    
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
        //read passage area and constrain passage area within the level
        int x0 = Ego::Math::constrain<int>(ctxt.readIntegerLiteral(), 0, _mesh->_info.getTileCountX() - 1);
        int y0 = Ego::Math::constrain<int>(ctxt.readIntegerLiteral(), 0, _mesh->_info.getTileCountY() - 1);
        int x1 = Ego::Math::constrain<int>(ctxt.readIntegerLiteral(), 0, _mesh->_info.getTileCountX() - 1);
        int y1 = Ego::Math::constrain<int>(ctxt.readIntegerLiteral(), 0, _mesh->_info.getTileCountY() - 1);

        //Read if open by default
        bool open = ctxt.readBool();

        //Read mask (optional)
        uint8_t mask = MAPFX_IMPASS | MAPFX_WALL;
        if (ctxt.readBool()) mask = MAPFX_IMPASS;
        if (ctxt.readBool()) mask = MAPFX_SLIPPY;

        std::shared_ptr<Passage> passage = std::make_shared<Passage>(*this, x0, y0, x1, y1, mask);

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
    for(const std::shared_ptr<Ego::Player> &player : _playerList)
    {
        const std::shared_ptr<Object> &pchr = player->getObject();
        if (!pchr || pchr->isTerminated()) continue;

        if (!pchr->isAlive()) continue;

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

ObjectRef GameModule::getShopOwner(const float x, const float y) {
    // Loop through every passage.
    for(const std::shared_ptr<Passage>& passage : _passages) {
        // Only check actual shops.
        if(!passage->isShop()) {
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

void GameModule::removeShopOwner(ObjectRef owner)
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
    return x >= 0 && x < _mesh->_tmem._edge_x && y >= 0 && y < _mesh->_tmem._edge_y;
}

std::shared_ptr<Object> GameModule::spawnObject(const Vector3f& pos, const PRO_REF profile, const TEAM_REF team, const int skin,
                                                const Facing& facing, const std::string &name, const ObjectRef override)
{
    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(profile);
    if (!ppro)
    {
        if ( profile > getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
			Log::get().warn( "spawnObject() - trying to spawn using invalid profile %d\n", REF_TO_INT( profile ) );
        }
        return Object::INVALID_OBJECT;
    }

    // count all the requests for this character type
    ppro->_spawnRequestCount++;

    // allocate a new character
    std::shared_ptr<Object> pchr = getObjectHandler().insert(profile, override);
    if (!pchr) {
		Log::get().warn( "spawnObject() - failed to spawn character\n" );
        return Object::INVALID_OBJECT;
    }

    // just set the spawn info
    pchr->spawn_data.pos = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = Facing(uint16_t(facing));
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
    switch (ppro->getGender()) {
        case GenderProfile::Male:
            pchr->gender = Gender::Male;
            break;
        case GenderProfile::Female:
            pchr->gender = Gender::Female;
            break;
        case GenderProfile::Neuter:
            pchr->gender = Gender::Neuter;
            break;
        case GenderProfile::Random:
            /// 50% male or female.
            /// @todo And what about Neuter?
            if (Random::nextBool()) {
                pchr->gender = Gender::Female;
            } else {
                pchr->gender = Gender::Male;
            }
            break;
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
        pchr->phys.weight = Ego::Physics::CHR_INFINITE_WEIGHT;
    }
    else
    {
        uint32_t itmp = ppro->getWeight() * ppro->getSize() * ppro->getSize() * ppro->getSize();
        pchr->phys.weight = std::min(itmp, Ego::Physics::CHR_MAX_WEIGHT);
    }

    // Extra spawn money is added later
    pchr->giveMoney(ppro->getStartingMoney());

    // Experience
    pchr->experience = Random::next( ppro->getStartingExperience() );
    pchr->experiencelevel = ppro->getStartingLevel();

    // Particle attachments
    pchr->reaffirm_damagetype = ppro->getReaffirmDamageType();

    // Character size and bumping
    pchr->fat_stt           = ppro->getSize();
    pchr->shadow_size_stt   = ppro->getShadowSize();
    pchr->bump_stt.size     = ppro->getBumpSize();
    pchr->bump_stt.size_big = ppro->getBumpSizeBig();
    pchr->bump_stt.height   = ppro->getBumpHeight();

    //Initialize size and collision box
    pchr->fat                = pchr->fat_stt;
    pchr->shadow_size_save   = pchr->shadow_size_stt;
    pchr->bump_save.size     = pchr->bump_stt.size;
    pchr->bump_save.size_big = pchr->bump_stt.size_big;
    pchr->bump_save.height   = pchr->bump_stt.height;
    pchr->recalculateCollisionSize();

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
    ai_state_t::spawn( pchr->ai, pchr->getObjRef(), pchr->getProfileID(), getTeamList()[team].getMorale() );

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
    if (pchr->spawn_data.skin < 0 || !ppro->isValidSkin(pchr->spawn_data.skin))
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

    pchr->setPosition(pos);
    pchr->setSpawnPosition(pos);

    pchr->ori.facing_z     = Facing(facing);
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
        ParticleHandler::get().spawnParticle( pchr->getPosition(), Facing(uint16_t(pchr->ori.facing_z)), ppro->getSlotNumber(), ppro->getAttachedParticleProfile(),
                                              pchr->getObjRef(), GRIP_LAST + tnc, pchr->team, pchr->getObjRef(), ParticleRef::Invalid, tnc);
    }

    // is the object part of a shop's inventory?
    if ( pchr->isItem() )
    {
        // Items that are spawned inside shop passages are more expensive than normal

        ObjectRef shopOwner = getShopOwner(pchr->getPosX(), pchr->getPosY());
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

    chr_instance_t::update_ref(pchr->inst, pchr->getPosition(), true);

    // start the character out in the "dance" animation
    chr_start_anim(pchr.get(), ACTION_DA, true, true);

    // count all the successful spawns of this character
    ppro->_spawnCount++;

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    log_debug( "spawnObject() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( pchr->getCharacterID() ), name.c_str(), ppro->getClassName().c_str() );
#endif

#if defined(_DEBUG) && defined(DEBUG_WAYPOINTS)
    if ( _gameObjects.exists( pchr->attachedto ) && Ego::Physics::CHR_INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->getPosX(), pchr->getPosY(), nrm[kX], nrm[kY] );
    }
#endif

    return pchr;
}

std::shared_ptr<const Ego::Texture> GameModule::getTileTexture(const size_t index)
{
    if(index >= _tileTextures.size()) return nullptr;
    return _tileTextures[index].get_ptr();
}

std::shared_ptr<const Ego::Texture> GameModule::getWaterTexture(const uint8_t layer)
{
    if(layer > _waterTextures.size()) return nullptr;
    return _waterTextures[layer].get_ptr();
}

void GameModule::updateAllObjects()
{
   for(const std::shared_ptr<Object> &object : getObjectHandler().iterator())
    {
        //Skip terminated objects
        if(object->isTerminated()) {
            continue;
        }

        //Update object logic
        object->update();

        //Generate movement and attacks from input latches
        chr_do_latch_button(object.get());

        //Update model animation
        move_one_character_do_animation(object.get());

        // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
        chr_instance_t::update_ref(object->inst, object->getPosition(), true);

        //Check if this object should be poofed (destroyed)
        bool timeOut = ( object->ai.poof_time > 0 ) && ( object->ai.poof_time <= static_cast<int32_t>(update_wld) );
        if (timeOut) {
            object->requestTerminate();
        }
    }

    // Reset the clock
    if (clock_chr_stat >= ONESECOND) {
        clock_chr_stat -= ONESECOND;
    }
}

water_instance_t& GameModule::getWater()
{
    return _water;
}

std::shared_ptr<Ego::Player>& GameModule::getPlayer(size_t index)
{
    return _playerList[index];
}

const std::vector<std::shared_ptr<Ego::Player>>& GameModule::getPlayerList() const
{
    return _playerList;    
}

bool GameModule::addPlayer(const std::shared_ptr<Object>& object, input_device_t *pdevice)
{
    if(!object || object->isTerminated()) {
        return false;
    }

    std::shared_ptr<Ego::Player> player = std::make_shared<Ego::Player>(object, pdevice);
    _playerList.push_back(player);

    // set the reference to the player
    object->is_which_player = _playerList.size()-1;

    // download the quest info
    player->getQuestLog().loadFromFile(object->getProfile()->getPathname());

    if (pdevice != nullptr)
    {
        local_stats.noplayers = false;
        object->islocalplayer = true;
        local_stats.player_count++;
    }

    return true;
}

void GameModule::updatePits()
{
    //Are pits enabled?
    if (!_pitsKill && !_pitsTeleport) {
        return;
    }

    //Decrease the timer
    if (_pitsClock > 0) {
        _pitsClock--;
    }

    if (0 == _pitsClock)
    {
        //Reset timer
        _pitsClock = PIT_CLOCK_RATE;

        // Kill any particles that fell in a pit, if they die in water...
        for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator()) {
            if ( particle->getPosZ() < PITDEPTH && particle->getProfile()->end_water )
            {
                particle->requestTerminate();
            }
        }

        // Kill or teleport any characters that fell in a pit...
        for(const std::shared_ptr<Object> &pchr : _gameObjects.iterator()) {
            // Is it a valid character?
            if ( pchr->isInvincible() || !pchr->isAlive() ) continue;
            if ( pchr->isBeingHeld() ) continue;

            // Do we kill it?
            if ( _pitsKill && pchr->getPosZ() < PITDEPTH )
            {
                // Got one!
                pchr->kill(Object::INVALID_OBJECT, false);
                pchr->vel.x() = 0;
                pchr->vel.y() = 0;

                /// @note ZF@> Disabled, the pitfall sound was intended for pits.teleport only
                // Play sound effect
                // sound_play_chunk( pchr->pos, g_wavelist[GSND_PITFALL] );
            }

            // Do we teleport it?
            else if (_pitsTeleport && pchr->getPosZ() < PITDEPTH * 4)
            {
                // Teleport them back to a "safe" spot
                if (!pchr->teleport(_pitsTeleportPos, Facing(pchr->ori.facing_z))) {
                    // Kill it instead
                    pchr->kill(Object::INVALID_OBJECT, false);
                    pchr->vel.x() = 0;
                    pchr->vel.y() = 0;
                }
                else {
                    // Stop movement
                    pchr->vel = Vector3f::zero();

                    // Play sound effect
                    if (pchr->isPlayer()) {
                        AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_PITFALL));
                    }
                    else {
                        AudioSystem::get().playSound(pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_PITFALL));
                    }

                    // Do some damage (same as damage tile)
                    pchr->damage(Facing(ATK_BEHIND), _damageTile.amount, static_cast<DamageType>(_damageTile.damagetype), Team::TEAM_DAMAGE, 
                                 _gameObjects[pchr->ai.getBumped()], true, false, false);
                }
            }
        }
    }
}

void GameModule::enablePitsTeleport(const Vector3f &location)
{
    _pitsTeleportPos = location;
    _pitsTeleport = true;
    _pitsKill = false;
}

void GameModule::enablePitsKill()
{
    _pitsTeleport = false;
    _pitsKill = true;
}

void GameModule::updateDamageTiles()
{
    // do the damage tile stuff
    for(const std::shared_ptr<Object> &pchr : _gameObjects.iterator()) {
        // if the object is not really in the game, do nothing
        if (pchr->isHidden() || !pchr->isAlive()) continue;

        // if you are being held by something, you are protected
        if (pchr->isInsideInventory()) continue;

        // are we on a damage tile?
        if ( !_mesh->grid_is_valid( pchr->getTile() ) ) continue;
        if ( 0 == _mesh->test_fx( pchr->getTile(), MAPFX_DAMAGE ) ) continue;

        // are we low enough?
        if (pchr->getPosZ() > pchr->getObjectPhysics().getGroundElevation() + DAMAGERAISE) continue;

        // allow reaffirming damage to things like torches, even if they are being held,
        // but make the tolerance closer so that books won't burn so easily
        if (!pchr->isBeingHeld() || pchr->getPosZ() < pchr->getObjectPhysics().getGroundElevation() + DAMAGERAISE)
        {
            if (pchr->reaffirm_damagetype == _damageTile.damagetype)
            {
                if (0 == (update_wld & TILE_REAFFIRM_AND))
                {
                    reaffirm_attached_particles(pchr->getObjRef());
                }
            }
        }

        // do not do direct damage to items that are being held
        if (pchr->isBeingHeld()) continue;

        // don't do direct damage to invulnerable objects
        if (pchr->isInvincible()) continue;

        if (0 == pchr->damage_timer)
        {
            int actual_damage = pchr->damage(Facing(ATK_BEHIND), _damageTile.amount,
                                             static_cast<DamageType>(_damageTile.damagetype), 
                                             Team::TEAM_DAMAGE, nullptr, true, false, false);

            pchr->damage_timer = DAMAGETILETIME;

            if ((actual_damage > 0) && (LocalParticleProfileRef::Invalid != _damageTile.part_gpip) && 0 == (update_wld & _damageTile.partand)) {
                ParticleHandler::get().spawnGlobalParticle( pchr->getPosition(), Facing(ATK_FRONT), _damageTile.part_gpip, 0 );
            }
        }
    }
}

void GameModule::loadTeamAlliances()
{
    // Load the file if it exists
    ReadContext ctxt("mp_data/alliance.txt");
    if (!ctxt.ensureOpen()) {
        return;
    }

    //Found the file, parse the contents
    while (ctxt.skipToColon(true))
    {
        char buffer[1024 + 1];
        vfs_read_string_lit(ctxt, buffer, 1024);
        if (strlen(buffer) < 1) {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt.getLoadName(), ctxt.getLineNumber()),
                                                "empty string literal");
        }
        TEAM_REF teama = (buffer[0] - 'A') % Team::TEAM_MAX;

        vfs_read_string_lit(ctxt, buffer, 1024);
        if (strlen(buffer) < 1) {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt.getLoadName(), ctxt.getLineNumber()),
                                                "empty string literal");
        }
        TEAM_REF teamb = (buffer[0] - 'A') % Team::TEAM_MAX;
        _teamList[teama].makeAlliance(_teamList[teamb]);
    }
}

/// @todo Remove this global.
std::unique_ptr<GameModule> _currentModule = nullptr;
