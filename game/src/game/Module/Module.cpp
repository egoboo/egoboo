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
#include "egolib/Logic/TreasureTables.hpp"

#include "game/Module/Passage.hpp"
#include "game/game.h"
#include "game/graphic.h"
#include "game/Logic/Player.hpp"
#include "game/Entities/_Include.hpp"
#include "game/CharacterMatrix.h"

/// @todo Remove this global.
std::unique_ptr<GameModule> _currentModule = nullptr;

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
    Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, "loading module ", "`", profile->getPath(), "`", Log::EndOfEntry);

    // set up the virtual file system for the module (Do before loading the module)
    if (!setup_init_module_vfs_paths(getPath().c_str())) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "Failed to setup module vfs"); 
    }

    //Initialize random seeds
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
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to load wawalite.txt for ", "`", profile->getPath(), "`", Log::EndOfEntry);
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

    // log debug info for every object loaded into the module
    if (egoboo_config_t::get().debug_developerMode_enable.getValue()) {
        logSlotUsage("/debug/slotused.txt");
    }

    // reset some counters
    timeron = false;
    update_wld = 0;
    clock_chr_stat = 0;    
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
                import_data.slot_lst[cnt] = ProfileSystem::get().loadOneProfile(importPath).get();
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
    std::unique_ptr<ReadContext> ctxt = nullptr;
    try {
        ctxt = std::make_unique<ReadContext>("mp_data/passage.txt");
    } catch (...) {
        return;
    }
    //Load all passages in file
    while (ctxt->skipToColon(true))
    {
        //read passage area and constrain passage area within the level
        int x0 = Ego::Math::constrain<int>(ctxt->readIntegerLiteral(), 0, _mesh->_info.getTileCountX() - 1);
        int y0 = Ego::Math::constrain<int>(ctxt->readIntegerLiteral(), 0, _mesh->_info.getTileCountY() - 1);
        int x1 = Ego::Math::constrain<int>(ctxt->readIntegerLiteral(), 0, _mesh->_info.getTileCountX() - 1);
        int y1 = Ego::Math::constrain<int>(ctxt->readIntegerLiteral(), 0, _mesh->_info.getTileCountY() - 1);

        //Read if open by default
        bool open = ctxt->readBool();

        //Read mask (optional)
        uint8_t mask = MAPFX_IMPASS | MAPFX_WALL;
        if (ctxt->readBool()) mask = MAPFX_IMPASS;
        if (ctxt->readBool()) mask = MAPFX_SLIPPY;

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

std::shared_ptr<Object> GameModule::spawnObject(const Vector3f& pos, ObjectProfileRef profile, const TEAM_REF team, const int skin,
                                                const Facing& facing, const std::string &name, const ObjectRef override)
{
    const std::shared_ptr<ObjectProfile> &ppro = ProfileSystem::get().getProfile(profile);
    if (!ppro)
    {
        if ( profile.get() > getImportAmount() * MAX_IMPORT_PER_PLAYER )
        {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "attempt to spawn object from an invalid object profile ", "`", profile, "`", Log::EndOfEntry);
        }
        return Object::INVALID_OBJECT;
    }

    // count all the requests for this character type
    ppro->_spawnRequestCount++;

    // allocate a new character
    std::shared_ptr<Object> pchr = getObjectHandler().insert(profile, override);
    if (!pchr) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to spawn character", Log::EndOfEntry);
        return Object::INVALID_OBJECT;
    }

    // just set the spawn info
    pchr->spawn_data.pos = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = Facing(FACING_T(facing));
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

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

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

    //Set our position
    pchr->setPosition(pos);
    pchr->setSpawnPosition(pos);

    // AI stuff
    ai_state_t::spawn( pchr->ai, pchr->getObjRef(), pchr->getProfileID().get(), getTeamList()[team].getMorale() );

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

    //Facing
    pchr->ori.facing_z     = Facing(FACING_T(facing));
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

    // Particle attachments
    for ( uint8_t tnc = 0; tnc < ppro->getAttachedParticleAmount(); tnc++ )
    {
        ParticleHandler::get().spawnParticle( pchr->getPosition(), pchr->ori.facing_z, ppro->getSlotNumber(), ppro->getAttachedParticleProfile(),
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

    chr_update_matrix( pchr.get(), true );

    // start the character out in the "dance" animation
    pchr->inst.startAnimation(ACTION_DA, true, true);

    // count all the successful spawns of this character
    ppro->_spawnCount++;

#if defined(DEBUG_OBJECT_SPAWN) && defined(_DEBUG)
    log_debug( "spawnObject() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( pchr->getCharacterID() ), name.c_str(), ppro->getClassName().c_str() );
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

        //Update model animation
        object->inst.updateAnimation();

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

bool GameModule::addPlayer(const std::shared_ptr<Object>& object, const Ego::Input::InputDevice &device)
{
    if(!object || object->isTerminated()) {
        return false;
    }

    std::shared_ptr<Ego::Player> player = std::make_shared<Ego::Player>(object, device);
    _playerList.push_back(player);

    // set the reference to the player
    object->is_which_player = _playerList.size()-1;

    // download the quest info
    player->getQuestLog().loadFromFile(object->getProfile()->getPathname());

    //Local player added
    local_stats.noplayers = false;
    object->islocalplayer = true;
    local_stats.player_count++;

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
                pchr->setVelocity({0.0f, 0.0f, pchr->getVelocity().z()});

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
                    pchr->setVelocity({0.0f, 0.0f, pchr->getVelocity().z()});
                }
                else {
                    // Stop movement
                    pchr->setVelocity(Vector3f::zero());

                    // Play sound effect
                    if (pchr->isPlayer()) {
                        AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_PITFALL));
                    }
                    else {
                        AudioSystem::get().playSound(pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_PITFALL));
                    }

                    // Do some damage (same as damage tile)
                    pchr->damage(Facing::ATK_BEHIND, _damageTile.amount, static_cast<DamageType>(_damageTile.damagetype), Team::TEAM_DAMAGE, 
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
            int actual_damage = pchr->damage(Facing::ATK_BEHIND, _damageTile.amount,
                                             static_cast<DamageType>(_damageTile.damagetype), 
                                             Team::TEAM_DAMAGE, nullptr, true, false, false);

            pchr->damage_timer = DAMAGETILETIME;

            if ((actual_damage > 0) && (LocalParticleProfileRef::Invalid != _damageTile.part_gpip) && 0 == (update_wld & _damageTile.partand)) {
                ParticleHandler::get().spawnGlobalParticle( pchr->getPosition(), Facing::ATK_FRONT, _damageTile.part_gpip, 0 );
            }
        }
    }
}

void GameModule::loadTeamAlliances()
{
    std::unique_ptr<ReadContext> ctxt = nullptr;
    // Load the file if it exists
    try {
        ctxt = std::make_unique<ReadContext>("mp_data/alliance.txt");
    } catch (...) {
        return;
    }
    //Found the file, parse the contents
    while (ctxt->skipToColon(true))
    {
        std::string buffer;
        vfs_read_string_lit(*ctxt, buffer);
        if (buffer.length() < 1) {
            throw CompilationErrorException(__FILE__, __LINE__, CompilationErrorKind::Syntactical, Id::Location(ctxt->getFileName(), ctxt->getLineNumber()),
                                            "empty string literal");
        }
        TEAM_REF teama = (buffer[0] - 'A') % Team::TEAM_MAX;

        vfs_read_string_lit(*ctxt, buffer);
        if (buffer.length() < 1) {
            throw CompilationErrorException(__FILE__, __LINE__, CompilationErrorKind::Syntactical, Id::Location(ctxt->getFileName(), ctxt->getLineNumber()),
                                            "empty string literal");
        }
        TEAM_REF teamb = (buffer[0] - 'A') % Team::TEAM_MAX;
        _teamList[teama].makeAlliance(_teamList[teamb]);
    }
}

void GameModule::logSlotUsage(const std::string& savename)
{
    /// @author ZZ
    /// @details This is a debug function for checking model loads

    vfs_FILE* hFileWrite = vfs_openWrite(savename);
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        //vfs_printf( hFileWrite, "%d of %d frames used...\n", Md2FrameList_index, MAXFRAME );

        ObjectProfileRef lastSlotNumber(0);
        for (const auto &element : ProfileSystem::get().getLoadedProfiles())
        {
            const std::shared_ptr<ObjectProfile> &profile = element.second;

            //ZF> ugh, import objects are currently handled in a weird special way.
            for(ObjectProfileRef i = lastSlotNumber; i < profile->getSlotNumber() && i <= ObjectProfileRef(36); ++i)
            {
                if (!ProfileSystem::get().isLoaded(i))
                {
                    vfs_printf( hFileWrite, "%3" PRIuZ " %32s.\n", i.get(), "Slot reserved for import players" );
                }
            }
            lastSlotNumber = profile->getSlotNumber();

            vfs_printf(hFileWrite, "%3d %32s %s\n", profile->getSlotNumber().get(), profile->getClassName().c_str(), profile->getModel()->getName().c_str());
        }

        vfs_close( hFileWrite );
    }
}

void GameModule::spawnAllObjects()
{
    std::unordered_map<int, std::string> reservedSlots; //Keep track of which slot numbers are reserved by their load name
    std::unordered_set<std::string> dynamicObjectList;  //references to slots that need to be dynamically loaded later
    std::vector<spawn_file_info_t> objectsToSpawn;      //The full list of objects to be spawned 

    //First load treasure tables
    Ego::TreasureTables treasureTables("mp_data/randomtreasure.txt");

    // Turn some back on
    auto entries = spawn_file_read("mp_data/spawn.txt");
    {
        std::shared_ptr<Object> parent = nullptr;

        for (auto& entry : entries)
        {
            //Spit out a warning if they break the limit
            if ( objectsToSpawn.size() >= OBJECTS_MAX )
            {
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "too many objects in file ", "`",
                                                 "mp_data/spawn,txt", "`", ". Maximum number of objects is ", OBJECTS_MAX,
                                                 Log::EndOfEntry);
                break;
            }

            // check to see if the slot is valid
            if ( entry.slot >= INVALID_PRO_REF )
            {
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "invalid slot ", entry.slot,
                                                 " for ", "`", entry.spawn_comment, "`", " in file ", "`", "mp_data/spawn,txt", "`",
                                                 Log::EndOfEntry);
                continue;
            }

            //convert the spawn name into a format we like
            convert_spawn_file_load_name(entry, treasureTables);

            // If it is a dynamic slot, remember to dynamically allocate it for later
            if ( entry.slot <= -1 )
            {
                dynamicObjectList.insert(entry.spawn_comment);
            }

            //its a static slot number, mark it as reserved if it isnt already
            else if (reservedSlots[entry.slot].empty())
            {
                reservedSlots[entry.slot] = entry.spawn_comment;
            }

            //Finished with this object for now
            objectsToSpawn.push_back(entry);
        }

        //Next we dynamically find slot numbers for each of the objects in the dynamic list
        for(const std::string &spawnName : dynamicObjectList)
        {
            ObjectProfileRef profileSlot;

            //Find first free slot that is not the spellbook slot
            for (profileSlot = ObjectProfileRef(1 + MAX_IMPORT_PER_PLAYER * MAX_PLAYER); profileSlot < ObjectProfileRef::Invalid; ++profileSlot)
            {
                //don't try to grab loaded profiles
                if (ProfileSystem::get().isLoaded(profileSlot)) continue;

                //the slot already dynamically loaded by a different spawn object of the same type that we are, no need to reload in a new slot
                if(reservedSlots[profileSlot.get()] == spawnName) {
                     break;
                }

                //found a completely free slot
                if (reservedSlots[profileSlot.get()].empty())
                {
                    //Reserve this one for us
                    reservedSlots[profileSlot.get()] = spawnName;
                    break;
                }
            }

            //If all slots are reserved, spit out a warning (very unlikely unless there is a bug somewhere)
            if ( profileSlot == ObjectProfileRef::Invalid ) {
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to acquire free dynamic slot for object ", spawnName, ". All slots in use?", Log::EndOfEntry);
            }
        }

        //Now spawn each object in order
        for(spawn_file_info_t &spawnInfo : objectsToSpawn)
        {
            //Dynamic slot number? Then figure out what slot number is assigned to us
            if(spawnInfo.slot <= -1) {
                for(const auto &element : reservedSlots)
                {
                    if(element.second == spawnInfo.spawn_comment)
                    {
                        spawnInfo.slot = element.first;
                        break;
                    }
                }
            }

            // If nothing is already in that slot, try to load it.
            if (!ProfileSystem::get().isLoaded(spawnInfo.slot))
            {
                bool import_object = spawnInfo.slot > (getImportAmount() * MAX_IMPORT_PER_PLAYER);

                if ( !activate_spawn_file_load_object( spawnInfo ) )
                {
                    // no, give a warning if it is useful
                    if ( import_object )
                    {
                        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__,
                                                         "the object ", "`", spawnInfo.spawn_comment, "`", " in slot ",
                                                         spawnInfo.slot, " in file ", "`", "mp_data/spawn,txt", "`",
                                                         "does not exist on this machine", Log::EndOfEntry);
                    }
                    continue;
                }
            }

            // we only reach this if everything was loaded properly
            std::shared_ptr<Object> spawnedObject = spawnObjectFromFileEntry(spawnInfo, parent);

            //We might become the new parent
            if (spawnedObject != nullptr && spawnInfo.attach == ATTACH_NONE) {
                parent = spawnedObject;
            }
        }
    }

    // Fix tilting trees problem
    tiltCharactersToTerrain();

    //now load the profile AI, do last so that all reserved slot numbers are initialized
    game_load_profile_ai();    
}

std::shared_ptr<Object> GameModule::spawnObjectFromFileEntry(const spawn_file_info_t& psp_info, const std::shared_ptr<Object> &parent)
{
    if (!psp_info.do_spawn || psp_info.slot < 0) {
        return nullptr;  
    }

    auto iprofile = ObjectProfileRef(static_cast<PRO_REF>(psp_info.slot));

    //Require a valid parent?
    if(psp_info.attach != ATTACH_NONE && !parent) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "failed to spawn ", "`", psp_info.spawn_name, "`", " due to missing parent", Log::EndOfEntry);
        return nullptr;
    }

    // Spawn the character
    std::shared_ptr<Object> pobject = spawnObject(psp_info.pos, iprofile, psp_info.team, psp_info.skin, psp_info.facing, psp_info.pname == nullptr ? "" : *psp_info.pname, ObjectRef::Invalid);

    //Failed to spawn?
    if (!pobject) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to spawn ", "`", psp_info.spawn_name, "`", Log::EndOfEntry);
        return nullptr;
    }

    //Add money
    pobject->giveMoney(psp_info.money);

    //Set AI stuff
    pobject->ai.content = psp_info.content;
    pobject->ai.passage = psp_info.passage;

    // determine the attachment
    switch(psp_info.attach)
    {
        case ATTACH_NONE:
            make_one_character_matrix(pobject->getObjRef());
        break;

        case ATTACH_INVENTORY:
            // Inventory character
            Inventory::add_item(parent->getObjRef(), pobject->getObjRef(), parent->getInventory().getFirstFreeSlotNumber(), true);

            //If the character got merged into a stack, then it will be marked as terminated
            if(pobject->isTerminated()) {
                return nullptr;
            }

            // Make spellbooks change
            SET_BIT(pobject->ai.alert, ALERTIF_GRABBED);
        break;

        case ATTACH_LEFT:
        case ATTACH_RIGHT:
            // Wielded character
            grip_offset_t grip_off = (ATTACH_LEFT == psp_info.attach) ? GRIP_LEFT : GRIP_RIGHT;

            if(pobject->getObjectPhysics().attachToObject(parent, grip_off)) {
                // Handle the "grabbed" messages
                //scr_run_chr_script(pobject);
                UNSET_BIT(pobject->ai.alert, ALERTIF_GRABBED);
            }
        break;

    }

    // Set the starting pinfo->level
    if (psp_info.level > 0) {
        if (pobject->experiencelevel < psp_info.level) {
            pobject->experience = pobject->getProfile()->getXPNeededForLevel(psp_info.level);
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if (!isImportValid() && nullptr != parent && parent->isPlayer()) {
        pobject->nameknown = true;
        pobject->iskursed = false;
    }

    // Turn on input devices
    if ( psp_info.stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == getImportAmount() && getPlayerList().size() < getPlayerAmount() )
        {
            // a single player module

            bool player_added = addPlayer(pobject, Ego::Input::InputDevice::DeviceList[local_stats.player_count]);

            if ( getImportAmount() == 0 && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = true;
            }
        }
        else if ( getPlayerList().size() < getImportAmount() && getPlayerList().size() < getPlayerAmount() && getPlayerList().size() < g_importList.count )
        {
            // A multiplayer module

            int local_index = -1;
            for ( size_t tnc = 0; tnc < g_importList.count; tnc++ )
            {
                if (pobject->getProfileID().get() <= import_data.max_slot && ProfileSystem::get().isLoaded(pobject->getProfileID()))
                {
                    int islot = pobject->getProfileID().get();

                    if ( import_data.slot_lst[islot] == g_importList.lst[tnc].slot )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            if ( -1 != local_index )
            {
                // It's a local input
                addPlayer(pobject, Ego::Input::InputDevice::DeviceList[g_importList.lst[local_index].local_player_num]);
            }
            else
            {
                // It's a remote input
                std::logic_error("Remote input control no longer supported");
                //module.addPlayer(pobject, nullptr);
            }
        }
    }

    return pobject;
}

void GameModule::tiltCharactersToTerrain()
{
    for(const std::shared_ptr<Object> &object : getObjectHandler().iterator())
    {
        if (object->isTerminated()) {
            continue;  
        }

        if ( object->getProfile()->hasStickyButt() )
        {
            uint8_t twist = getMeshPointer()->get_twist( object->getTile() );
            object->ori.map_twist_facing_y = Facing(g_meshLookupTables.twist_facing_y[twist]);
            object->ori.map_twist_facing_x = Facing(g_meshLookupTables.twist_facing_x[twist]);
        }
        else
        {
            object->ori.map_twist_facing_y = orientation_t::MAP_TURN_OFFSET;
            object->ori.map_twist_facing_x = orientation_t::MAP_TURN_OFFSET;
        }
    }
}
