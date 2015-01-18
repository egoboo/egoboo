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

/// @file  game/Profile.cpp
/// @brief ObjectProfile handling
/// @details
/// @author Johan Jansen

//ZF> TODO: check which headers can be removed
#include "egolib/bsp.h"
#include "game/profiles/Profile.hpp"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/ChrList.h"
#include "game/PrtList.h"
#include "game/mesh.h"
#include "game/particle.h"
#include "game/mad.h"       //for loading md2
#include "game/audio/AudioSystem.hpp"
#include "egolib/file_formats/template.h"

ObjectProfile::ObjectProfile() :
    _requestCount(0),
    _spawnCount(0),
    _fileName("*NONE*"),
    _imad(INVALID_MAD_REF),
    _ieve(INVALID_EVE_REF),
    _slotNumber(-1),

    _randomName(),
    _aiScript(),
    _particleProfiles{},

    _texturesLoaded(),
    _iconsLoaded(),

    _messageList(),
    _soundMap(),

    //-------------------
    //Data.txt
    //-------------------
    _className("*NONE*"),

    // skins
     _skinInfo(),

    // overrides
    _skinOverride(NO_SKIN_OVERRIDE),
    _levelOverride(0)
    _stateOverride(0),
    _contentOverride(0),

    _idsz(),

    // inventory
    _maxAmmo(0)
    _ammo(0)
    _money(0)

    // characer stats
    _gender(GENDER_OTHER),

    // life
    _startingLife(),
    _spawnLife(PERFECTBIG),

    // mana
    _startingMana(),
    _startingManaRegeneration(),
    _spawnMana(PERFECTBIG),

    _startingLifeRegeneration(),
    _startingManaFlow(),

    _startingStrength(),
    _startingWisdom(),
    _startingIntelligence(),
    _startingDexterity(),

    // physics
    _weight(0),
    _dampen(0.0f),
    _bumpDampen(INV_FF),

    _size(1.0f),
    _sizeGainPerLevel(0.0f),
    _shadowSize(0),
    _bumpSize(0),
    _bumpOverrideSize(false),
    _bumpSizeBig(0),
    _bumpOverrideSizeBig(false),
    _bumpHeight(0),
    _bumpOverrideHeight(false),
    _stoppedBy(MAPFX_IMPASS),

    // movement
    _jumpPower(0.0f),            
    _jumpNumber(0),         
    _animationSpeedSneak(0.0f),
    _animationSpeedWalk(0.0f), 
    _animationSpeedRun(0.0f),  
    _flyHeight(0),          
    _waterWalking(false),
    _jumpSound(-1),
    _footFallSound(-1),

    // status graphics
    _lifeColor(0),
    _manaColor(0),
    _drawIcon(true),

    // model graphics
    _flashAND(0),
    _alpha(0),
    _light(0),
    _transferBlending(false),
    _sheen(0),
    _phongMapping(false),
    uoffvel(0),
    voffvel(0),
    _uniformLit(false),
    _hasReflection(false),
    _alwaysDraw(false),
    _forceShadow(false),
    _causesRipples(false),
    _dontCullBackfaces(false),
    _skinHasTransparency(false),

    // attack blocking info
    iframefacing(0), 
    iframeangle(0),
    nframefacing(0),
    nframeangle(0),

    // defense
    _resistBumpSpawn(false),
    _defence(),
    _damageModifier(),
    _damageResistance(),

    // xp
    _experienceForLevel(),
    _startingExperience()
    _experienceWorth(0),
    _experienceExchange(0.0f),
    _experienceRate(),

    // flags
    _isEquipment(false),
    _isItem(false),
    _isMount(false),
    _isStackable(false),
    _isInvincible(false),
    _isPlatform(false),
    _canUsePlatforms(false),
    _canGrabMoney(false),
    _canOpenStuff(false),
    _canBeDazed(false),
    _canBeGrogged(false),
    _isBigItem(false),
    _isRanged(false),
    _nameIsKnown(false),
    _usageIsKnown(false),
    _canCarryToNextModule(false),

    _damageTargetDamageType(0),
    _slotsValid(),
    _riderCanAttack(false),
    _kurseChance(0),
    _hideState(NO_HIDE),
    _isValuable(-1),
    _spellEffectType(NO_SKIN_OVERRIDE),

    // item usage
    _needSkillIDToUse(false),
    _weaponAction(0),
    _attackAttached(0),
    _attackParticleProfile(INVALID_PIP_REF),
    _attackFast(false),

    _strengthBonus(0.0f),
    _wisdomBonus(0.0f),
    _intelligenceBonus(0.0f),
    _dexterityBonus(0.0f),

    // special particle effects
    _attachedParticleAmount(0),
    _attachedParticleReaffirmDamagetype(0),
    _attachedParticleProfile(INVALID_PIP_REF),

    _goPoofParticleAmount(0),
    _goPoofParticleFacingAdd(0),
    _goPoofParticleProfile(INVALID_PIP_REF),

    //Blood
    _bludValid(0),
    _bludParticleProfile(INVALID_PIP_REF),

    // skill system
    _skills(),
    _seeInvisibleLevel(0),

    // random stuff
    _stickyButt(false)
{
    _particleProfiles.fill(INVALID_PIP_REF);

    _experienceRate.fill(0.0f);
    _idsz.fill(IDSZ_NONE);
    _experienceForLevel.fill(UINT32_MAX);

    memset(&_aiScript, 0, sizeof(script_info_t));
}

ObjectProfile::ObjectProfile(const std::string &folderPath, size_t slotNumber) : ObjectProfile()
{
    //Set some data
    _fileName = folderPath;
    _slotNumber = slotNumber;

    // load the character profile
    loadDataFile(folderPath + "/data.txt");

    // Load the model for this profile
    _imad = load_one_model_profile_vfs(folderPath.c_str(), slotNumber);

    // Load the enchantment for this profile
    STRING newloadname;
    make_newloadname( folderPath.c_str(), "/enchant.txt", newloadname );
    _ieve = EveStack_losd_one( newloadname, ( EVE_REF )_slotNumber );

    // Load the messages for this profile, do this before loading the AI script
    // to ensure any dynamic loaded messages get loaded last
    loadAllMessages(folderPath + "/message.txt");

    // Load the particles for this profile
    for (size_t cnt = 0; cnt < _particleProfiles.size(); cnt++ )
    {
        const std::string particleName = folderPath + "/part" + std::to_string(cnt) + ".txt";

        // Make sure it's referenced properly
        _particleProfiles[cnt] = PipStack_load_one(particleName.c_str(), INVALID_PIP_REF);
    }

    loadTextures(folderPath);

    // Load the waves for this iobj
    for ( size_t cnt = 0; cnt < 30; cnt++ ) //TODO: make better search than just 30 (list files?)
    {
        const std::string soundName = folderPath + "/sound" + std::to_string(cnt);
        SoundID soundID = _audioSystem.loadSound(soundName);

        if(soundID != INVALID_SOUND_ID) {
            _soundMap[cnt] = soundID
        }
    }

    // Load the random naming table for this icap
    _randomName.loadFromFile(folderPath + "/naming.txt");

    // Fix lighting if need be
    if (_uniformLit && cfg.gouraud_req)
    {
        mad_make_equally_lit_ref( _imad );
    }
}

ObjectProfile::~ObjectProfile()
{
    // release all of the sub-profiles
    MadStack_release_one( _imad );
    //EveStack_release_one( pobj->ieve );

    //Release particle profiles
    for(PRO_REF particle : _particleProfiles) {
        PipStack_release_one(particle);
    }

    // release whatever textures are being used
    for(const auto &element : _texturesLoaded)
    {
        if ( element.second > TX_SPECIAL_LAST )
        {
            TxList_free_one( element.second );
        }
    }

    for(const auto &element : _iconsLoaded)
    {
        if ( element.second > TX_SPECIAL_LAST )
        {
            TxList_free_one( element.second );
        }
    }
}

uint32_t ObjectProfile::getXPNeededForLevel(size_t level)
{
    if(level >= _experienceForLevel.size()) {
        return UINT32_MAX;
    }

    return _experienceForLevel[level];
}

void ObjectProfile::loadTextures(const std::string &folderPath)
{
    //Clear texture references
    _texturesLoaded.clear();
    _iconsLoaded.clear();

    // Load the skins and icons
    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        STRING newloadname;

        // do the texture
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/tris%d", folderPath.c_str(), cnt );

        TX_REF skin = TxList_load_one_vfs( newloadname, INVALID_TX_REF, TRANSCOLOR );
        if ( VALID_TX_RANGE( skin ) )
        {
            _texturesLoaded[cnt] = skin;
        }

        // do the icon
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/icon%d", folderPath.c_str(), cnt );

        TX_REF icon = TxList_load_one_vfs( newloadname, INVALID_TX_REF, INVALID_KEY );
        if ( VALID_TX_RANGE( icon ) )
        {
            _iconsLoaded[cnt] = icon;
        }
    }

    // If we didn't get a skin, set it to the water texture
    if ( _texturesLoaded.empty() )
    {
        _texturesLoaded[0] = TX_WATER_TOP;
        log_warning( "Object is missing a skin (%s)!\n", _fileName.c_str() );
    }

    // If we didn't get a icon, set it to the NULL icon
    if ( _iconsLoaded.empty())
    {
        _iconsLoaded[0] = TX_ICON_NULL;
        log_debug( "Object is missing an icon (%s)!\n", _fileName.c_str() );
    }
}

void ObjectProfile::addMessage(const std::string &message, const bool filterDuplicates)
{
    std::string parsedMessage = message;

    //replace underscore with whitespace
    std::replace(parsedMessage.begin(), parsedMessage.end(), '_', ' ');

    //Don't add the same message twice
    if(filterDuplicates) {
        for(const std::string& msg : _messageList) {
            if(msg == parsedMessage) {
                return;
            }
        }
    }

    //Add it to the list!
    _messageList.push_back(parsedMessage);
}

const std::string& ObjectProfile::getMessage(size_t index) const
{
    static const std::string EMPTY_STRING;

    if(index > _messageList.size()) {
        return EMPTY_STRING;
    }

    return _messageList[index];
}

void ObjectProfile::loadAllMessages(const std::string &filePath)
{
    /// @author ZF
    /// @details This function loads all messages for an object

    vfs_FILE *fileRead = vfs_openRead( filePath.c_str() );
    if ( fileRead )
    {
        STRING line;

        while ( goto_colon_vfs( NULL, fileRead, true ) )
        {
            //Load one line
            vfs_get_string( fileRead, line, SDL_arraysize( line ) );
            addMessage(line);
        }

        vfs_close( fileRead );
    }
}

const std::string ObjectProfile::generateRandomName()
{
    //If no random names loaded, return class name instead
    if (!_randomName.isLoaded()) {
        return _className;
    }

    return _randomName.generateRandomName();
}


SoundID ObjectProfile::getSoundID( int index ) const
{
    if(index < 0) return INVALID_SOUND_ID;

    //Try to find a SoundID that this number is mapped to
    auto result = _soundMap.find(index);

    //Not found?
    if(result == _soundMap.end()) {
        return INVALID_SOUND_ID;
    }

    return *result;
}

IDSZ ObjectProfile::getIDSZ(size_t type) const
{
    if(type >= IDSZ_COUNT) {
        return IDSZ_NONE;
    }

    return _idsz[type];
}

TX_REF ObjectProfile::getSkin(size_t index)
{
    if(_texturesLoaded.find(index) == _texturesLoaded.end()) {
        return _texturesLoaded[0];
    }

    return _texturesLoaded[index];
}

TX_REF ObjectProfile::getIcon(size_t index)
{
    if(_iconsLoaded.find(index) == _iconsLoaded.end()) {
        return _iconsLoaded[0];
    }

    return _iconsLoaded[index];
}

PIP_REF ObjectProfile::getParticleProfile(size_t index) const
{
    if(index > _particleProfiles.size()) {
        return INVALID_PIP_REF;
    }
    return _particleProfiles[index];
}

uint16_t ObjectProfile::getSkinOverride() const
{
    /// @details values of spelleffect_type or skin_override less than zero mean that the values are not valid.
    ///          values >= mean that the value is random.
    uint16_t retval = MAX_SKIN;

    if ( _spellEffectType >= 0 )
    {
        if ( _spellEffectType >= MAX_SKIN )
        {
            retval = Random::next(MAX_SKIN);
        }
        else
        {
            retval = _spellEffectType % MAX_SKIN;
        }
    }
    else if ( _skinOverride >= 0 )
    {
        if ( _skinOverride >= MAX_SKIN )
        {
            retval = Random::next(MAX_SKIN);
        }
        else
        {
            retval = _skinOverride % MAX_SKIN;
        }
    }

    return retval;
}

void ObjectProfile::setupXPTable()
{
    for (size_t level = MAXBASELEVEL; level < MAXLEVEL; level++ )
    {
        Uint32 xpneeded = _experienceForLevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        _experienceForLevel[level] = xpneeded;
    }
}

void ObjectProfile::loadDataFile(const std::string &filePath)
{
    // Open the file
    vfs_FILE* fileRead = vfs_openRead( filePath.c_str() );
    if (!fileRead)
    {
        log_warning("Unable to load data.txt for profile: %s\n", filePath.c_str());
        return;
    }

    //read slot number (ignored for now)
    vfs_get_next_int(fileRead);
    //_slotNumber = vfs_get_next_int(fileRead);

    // Read in the class name
    char buffer[128];
    vfs_get_next_name(fileRead, buffer, SDL_arraysize(buffer));

    // fix class name capitalization
    buffer[0] = char_toupper(buffer[0]);

    _className = buffer;

    // Light cheat
    _uniformLit = vfs_get_next_bool(fileRead);

    // Ammo
    _maxAmmo = vfs_get_next_int(fileRead);
    _ammo = vfs_get_next_int(fileRead);

    // Gender
    switch( char_toupper(vfs_get_next_char(fileRead)) )
    {
        case 'F': _gender = GENDER_FEMALE; break;
        case 'M': _gender = GENDER_MALE; break;
        case 'R': _gender = GENDER_RANDOM; break;
        default:  _gender = GENDER_OTHER; break;
    }

    // Read in the starting stats
    _lifeColor = vfs_get_next_int( fileRead );
    _manaColor = vfs_get_next_int( fileRead );

    vfs_get_next_range(fileRead, &( _startingLife.val));
    vfs_get_next_range(fileRead, &( _startingLife.perlevel));

    vfs_get_next_range(fileRead, &( _startingMana.val));
    vfs_get_next_range(fileRead, &( _startingMana.perlevel));

    vfs_get_next_range(fileRead, &( _startingManaRegeneration.val));
    vfs_get_next_range(fileRead, &( _startingManaRegeneration.perlevel));

    vfs_get_next_range(fileRead, &( _startingManaFlow.val));
    vfs_get_next_range(fileRead, &( _startingManaFlow.perlevel));

    vfs_get_next_range(fileRead, &( _startingStrength.val));
    vfs_get_next_range(fileRead, &( _startingStrength.perlevel));

    vfs_get_next_range(fileRead, &( _startingWisdom.val));
    vfs_get_next_range(fileRead, &( _startingWisdom.perlevel));

    vfs_get_next_range(fileRead, &( _startingIntelligence.val));
    vfs_get_next_range(fileRead, &( _startingIntelligence.perlevel));

    vfs_get_next_range(fileRead, &( _startingDexterity.val));
    vfs_get_next_range(fileRead, &( _startingDexterity.perlevel));

    // More physical attributes
    size = vfs_get_next_float(fileRead);
    _sizeGainPerLevel = vfs_get_next_float(fileRead );
    _shadowSize = vfs_get_next_int(fileRead);
    _bumpSize = vfs_get_next_int(fileRead);
    _bumpHeight = vfs_get_next_int(fileRead);
    _bumpDampen = vfs_get_next_float(fileRead );

    //0 == bumpdampenmeans infinite mass, and causes some problems
    _bumpDampen = std::max( INV_FF, _bumpDampen );

    weight = vfs_get_next_int(fileRead);
    _jumpPower = vfs_get_next_float(fileRead );
    _jumpNumber = vfs_get_next_int(fileRead);
    _animationSpeedSneak = vfs_get_next_float(fileRead );
    _animationSpeedWalk = vfs_get_next_float(fileRead );
    _animationSpeedRun = vfs_get_next_float(fileRead );
    _waterWalking = vfs_get_next_int(fileRead);
    _flashAND = vfs_get_next_int(fileRead);
    _alpha = vfs_get_next_int(fileRead);
    _light = vfs_get_next_int(fileRead);
    _transferBlending = vfs_get_next_bool(fileRead);

    _sheen = vfs_get_next_int(fileRead);
    _phongMapping = vfs_get_next_bool(fileRead);

    uoffvel    = FLOAT_TO_FFFF( vfs_get_next_float(fileRead) );
    voffvel    = FLOAT_TO_FFFF( vfs_get_next_float(fileRead) );
    _stickyButt = vfs_get_next_bool(fileRead);

    // Invulnerability data
    _isInvincible  = vfs_get_next_bool(fileRead);
    nframefacing   = vfs_get_next_int(fileRead);
    nframeangle    = vfs_get_next_int(fileRead);
    iframefacing   = vfs_get_next_int(fileRead);
    iframeangle    = vfs_get_next_int(fileRead);

    // Resist burning and stuck arrows with nframe angle of 1 or more
    if ( nframeangle > 0 )
    {
        if ( 1 == nframeangle )
        {
            nframeangle = 0;
        }
    }

    // Skin defenses ( 4 skins )
    goto_colon_vfs( NULL, fileRead, false );
    for (size_t  cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        iTmp = 0xFF - vfs_get_int( fileRead );
        defense[cnt] = CLIP( iTmp, 0, 0xFF );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileRead, false );
        for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            damage_resistance[damagetype][cnt] = vfs_get_damage_resist( fileRead );
        }
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileRead, false );

        for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            switch ( char_toupper(vfs_get_first_letter(fileRead)) )
            {
                case 'T': damage_modifier[damagetype][cnt] |= DAMAGEINVERT;   break;
                case 'C': damage_modifier[damagetype][cnt] |= DAMAGECHARGE;   break;
                case 'M': damage_modifier[damagetype][cnt] |= DAMAGEMANA;     break;
                case 'I': damage_modifier[damagetype][cnt] |= DAMAGEINVICTUS; break;

                    //F is nothing
                default: break;
            }
        }
    }

    goto_colon_vfs( NULL, fileRead, false );
    for ( cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        skin_info.maxaccel[cnt] = vfs_get_float( fileRead ) / 80.0f;
    }

    // Experience and level data
    _experienceForLevel[0] = 0;
    for ( size_t level = 1; level < _experienceForLevel.size(); level++ )
    {
        _experienceForLevel[level] = vfs_get_next_int(fileRead);
    }
    setupXPTable();

    vfs_get_next_range( fileRead, &( _startingExperience ) );
    _startingExperience.from /= 256.0f;
    _startingExperience.to   /= 256.0f;

    _experienceWorth    = vfs_get_next_int( fileRead );
    _experienceExchange = vfs_get_next_float(fileRead );

    for(size_t i = 0; i < _experienceRate.size(); ++i)
    {
        _experienceRate[i] = vfs_get_next_float(fileRead ) + 0.001f;
    }

    // IDSZ tags
    for (size_t i = 0; i < _idsz.size(); ++i)
    {
        _idsz[i] = vfs_get_next_idsz(fileRead);
    }

    // Item and damage flags
    _isItem              = vfs_get_next_bool(fileRead);
    _isMount             = vfs_get_next_bool(fileRead);
    _isStackable         = vfs_get_next_bool(fileRead);
    _nameIsKnown            = vfs_get_next_bool(fileRead);
    _usageIsKnown           = vfs_get_next_bool(fileRead);
    _canCarryToNextModule = vfs_get_next_bool(fileRead);
    _needSkillIDToUse     = vfs_get_next_bool(fileRead);
    _isPlatform          = vfs_get_next_bool(fileRead);
    _canGrabMoney        = vfs_get_next_bool(fileRead);
    _canOpenStuff        = vfs_get_next_bool(fileRead);

    // More item and damage stuff
    _damageTargetDamageType = vfs_get_next_damage_type( fileRead );
    _weaponAction            = action_which( vfs_get_next_char( fileRead ) );

    // Particle attachments
    _attachedParticleAmount              = vfs_get_next_int( fileRead );
    _attachedParticleReaffirmDamagetype = vfs_get_next_damage_type( fileRead );
    _attachedParticleProfile  = getParticleProfile( vfs_get_next_int(fileRead) );

    // Character hands
    _slotsValid[SLOT_LEFT]  = vfs_get_next_bool( fileRead );
    _slotsValid[SLOT_RIGHT] = vfs_get_next_bool( fileRead );

    // Attack order ( weapon )
    _attackAttached = vfs_get_next_bool( fileRead );
    _attackParticleProfile  = getParticleProfile( vfs_get_next_int(fileRead) );

    // GoPoof
    _goPoofParticleAmount    = vfs_get_next_int( fileRead );
    _goPoofParticleFacingAdd = vfs_get_next_int( fileRead );
    _goPoofParticleProfile   = getParticleProfile( vfs_get_next_int(fileRead) );

    // Blud
    switch( char_toupper(vfs_get_next_char(fileRead)) )
    {
        case 'T': _bludValid = true;        break;
        case 'U': _bludValid = ULTRABLUDY;  break;
        default:  _bludValid = false;       break;
    }
    _bludParticleProfile = vfs_get_next_int(fileRead);

    // Stuff I forgot
    _waterWalking = vfs_get_next_bool( fileRead );
    dampen    = vfs_get_next_float( fileRead );

    // More stuff I forgot
    vfs_get_next_float( fileRead );  //ZF> deprecated value LifeReturn (no longer used)
    fs_get_next_float( fileRead );  //ZF> deprecated value ManaCost (no longer used)
    _startingLifeRegeneration  = vfs_get_next_int( fileRead );
    _stoppedBy   |= vfs_get_next_int( fileRead );

    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        vfs_get_next_name( fileRead, skin_info.name[cnt], sizeof( skin_info.name[cnt] ) );
    }

    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        skin_info.cost[cnt] = vfs_get_next_int( fileRead );
    }

    _strengthBonus = vfs_get_next_float( fileRead );          //ZF> Deprecated, but keep here for backwards compatability

    // Another memory lapse
    _riderCanAttack = !vfs_get_next_bool(fileRead);
    _canBeDazed     =  vfs_get_next_bool(fileRead);
    _canBeGrogged   =  vfs_get_next_bool(fileRead);

    goto_colon_vfs( NULL, fileRead, false );  // Depracated, no longer used (life add)
    goto_colon_vfs( NULL, fileRead, false );  // Depracated, no longer used (mana add)
    if ( vfs_get_next_bool(fileRead) ) {
        _seeInvisibleLevel = 1;
    }

    _kurseChance    = vfs_get_next_int(fileRead);
    _footFallSound  = vfs_get_next_int(fileRead);  // Footfall sound
    _jumpSound      = vfs_get_next_int(fileRead);  // Jump sound

    // assume the normal dependence of _causesRipples on _isItem
    _causesRipples = !_isItem;

    // assume a round object
    _bumpSizeBig = _bumpSize * SQRT_TWO;

    // assume the normal icon usage
    _drawIcon = _usageIsKnown;

    // assume normal platform usage
    _canUsePlatforms = !platform;

    // Read expansions
    while ( goto_colon_vfs(NULL, fileRead, true) )
    {
        const IDSZ idsz = vfs_get_idsz( fileRead );

        switch(idsz)
        {
            case MAKE_IDSZ( 'D', 'R', 'E', 'S' ):
                getSkinInfo(vfs_get_int(fileRead)).dressy = true;
            break;

            case MAKE_IDSZ( 'G', 'O', 'L', 'D' ):
                money = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'S', 'T', 'U', 'K' ):
                _resistBumpSpawn = ( 0 != ( 1 - vfs_get_int( fileRead ) ) );
            break;

            case MAKE_IDSZ( 'P', 'A', 'C', 'K' ):
                _isBigItem = !( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'V', 'A', 'M', 'P' ):
                reflect = !( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'D', 'R', 'A', 'W' ):
                _alwaysDraw = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'R', 'A', 'N', 'G' ):
                _isRanged = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'H', 'I', 'D', 'E' ):
                hidestate = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'E', 'Q', 'U', 'I' ):
                _isEquipment = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'S', 'Q', 'U', 'A' ):
                _bumpSizeBig = _bumpSize * 2;
            break;

            case MAKE_IDSZ( 'I', 'C', 'O', 'N' ):
                _drawIcon = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'S', 'H', 'A', 'D' ):
                _forceShadow = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'S', 'K', 'I', 'N' ):
            {
                /// @note BB@> This is the skin value of a saved character.
                ///            It should(!) correspond to a valid skin for this object,
                ///            but possibly it could have one of two special values (NO_SKIN_OVERRIDE or MAX_SKIN)

                int iTmp = vfs_get_int( fileRead );

                iTmp = ( iTmp < 0 ) ? NO_SKIN_OVERRIDE : iTmp;
                iTmp = ( iTmp > MAX_SKIN ) ? MAX_SKIN : iTmp;
                skin_override = iTmp;  
            }          
            break;

            case MAKE_IDSZ( 'C', 'O', 'N', 'T' ): 
                content_override = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'S', 'T', 'A', 'T' ): 
                state_override = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'L', 'E', 'V', 'L' ): 
                level_override = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'P', 'L', 'A', 'T' ): 
                _canUsePlatforms = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'R', 'I', 'P', 'P' ): 
                _causesRipples = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'V', 'A', 'L', 'U' ): 
                isvaluable = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'L', 'I', 'F', 'E' ): 
                _spawnLife = 0xff * vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'M', 'A', 'N', 'A' ): 
                _spawnMana = 0xff * vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'B', 'O', 'O', 'K' ):
            {
                /// @note BB@> This is the skin value of a saved character.
                ///            It should(!) correspond to a valid skin for this object,
                ///            but possibly it could have one of two special values (NO_SKIN_OVERRIDE or MAX_SKIN)

                int iTmp = vfs_get_int( fileRead );

                iTmp = ( iTmp < 0 ) ? NO_SKIN_OVERRIDE : iTmp;
                iTmp = ( iTmp > MAX_SKIN ) ? MAX_SKIN : iTmp;
                spelleffect_type = iTmp;
            }
            break;

            //Damage bonuses from stats
            case MAKE_IDSZ( 'F', 'A', 'S', 'T' ):
                _attackFast = ( 0 != vfs_get_int(fileRead) );
            break;

            case MAKE_IDSZ( 'S', 'T', 'R', 'D' ):
                _strengthBonus = vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'I', 'N', 'T', 'D' ):
                _intelligenceBonus = vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'W', 'I', 'S', 'D' ):
                _wisdomBonus = vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'D', 'E', 'X', 'D' ):
                _dexterityBonus = vfs_get_float( fileRead );
            break;

            case MAKE_IDSZ( 'M', 'O', 'D', 'L' ):
            {
                STRING tmp_buffer;
                if ( vfs_get_string( fileRead, tmp_buffer, SDL_arraysize( tmp_buffer ) ) )
                {
                    char * ptr;

                    ptr = strpbrk( tmp_buffer, "SBHCT" );
                    while ( NULL != ptr )
                    {
                        if ( 'S' == *ptr )
                        {
                            _bumpOverrideSize = true;
                        }
                        else if ( 'B' == *ptr )
                        {
                            _bumpOverrideSizeBig = true;
                        }
                        else if ( 'H' == *ptr )
                        {
                            _bumpOverrideHeight = true;
                        }
                        else if ( 'C' == *ptr )
                        {
                            _dontCullBackfaces = true;
                        }
                        else if ( 'T' == *ptr )
                        {
                            _skinHasTransparency = true;
                        }

                        // start on the next character
                        ptr++;
                        ptr = strpbrk( ptr, "SBHCT" );
                    }
                }
            }
            break;

            default:
                //If it is none of the predefined IDSZ extensions then add it as a new skill
                _skills.insert(idsz);
            break;
        }

    }

    vfs_close( fileRead );
    return pcap;
}

const SkinInfo& ObjectProfile::getSkinInfo(size_t index) const
{
    return _skinInfo[index];
}

//--------------------------------------------------------------------------------------------
/*
bool ObjectProfile::exportToDataFile( const std::string &filePath, const char * szTemplateName, cap_t * pcap )
{
    /// @author BB
    /// @details export one cap_t struct to a "data.txt" file
    ///     converted to using the template file
    int damagetype, skin;

    // Open the file
    vfs_FILE* fileWrite = vfs_openWrite( filePath.c_str() );
    if ( NULL == fileWrite ) return false;

    // open the template file
    vfs_FILE* filetemp = nullptr;

    // try the given template file
    if ( VALID_CSTR( szTemplateName ) )
    {
        filetemp = template_open_vfs( szTemplateName );
    }

    // try a default template file
    if ( NULL == filetemp )
    {
        filetemp = template_open_vfs( "mp_data/templates/data.txt" );
    }

    //did we find a template file?
    if ( NULL == filetemp )
    {
        vfs_close( fileWrite );
        return false;
    }

    // Real general data
    template_put_int( filetemp, fileWrite, -1 );     // -1 signals a flexible load thing
    template_put_string_under( filetemp, fileWrite, pcap->classname );
    template_put_bool( filetemp, fileWrite, pcap->uniformlit );
    template_put_int( filetemp, fileWrite, pcap->ammomax );
    template_put_int( filetemp, fileWrite, pcap->ammo );
    template_put_gender( filetemp, fileWrite, pcap->gender );

    // Object stats
    template_put_int( filetemp, fileWrite, pcap->life_color );
    template_put_int( filetemp, fileWrite, pcap->mana_color );
    template_put_range( filetemp, fileWrite, pcap->_startingLife.val );
    template_put_range( filetemp, fileWrite, pcap->_startingLife.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingMana.val );
    template_put_range( filetemp, fileWrite, pcap->_startingMana.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingManaRegeneration.val );
    template_put_range( filetemp, fileWrite, pcap->_startingManaRegeneration.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingManaFlow.val );
    template_put_range( filetemp, fileWrite, pcap->_startingManaFlow.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingStrength.val );
    template_put_range( filetemp, fileWrite, pcap->_startingStrength.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingWisdom.val );
    template_put_range( filetemp, fileWrite, pcap->_startingWisdom.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingIntelligence.val );
    template_put_range( filetemp, fileWrite, pcap->_startingIntelligence.perlevel );
    template_put_range( filetemp, fileWrite, pcap->_startingDexterity.val );
    template_put_range( filetemp, fileWrite, pcap->_startingDexterity.perlevel );

    // More physical attributes
    template_put_float( filetemp, fileWrite, pcap->size );
    template_put_float( filetemp, fileWrite, pcap->_sizeGainPerLevel );
    template_put_int( filetemp, fileWrite, pcap->_shadowSize );
    template_put_int( filetemp, fileWrite, pcap->_bumpSize );
    template_put_int( filetemp, fileWrite, pcap->_bumpHeight );
    template_put_float( filetemp, fileWrite, pcap->_bumpDampen );
    template_put_int( filetemp, fileWrite, pcap->weight );
    template_put_float( filetemp, fileWrite, pcap->jump );
    template_put_int( filetemp, fileWrite, pcap->jumpnumber );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedSneak );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedWalk );
    template_put_float( filetemp, fileWrite, pcap->_animationSpeedRun );
    template_put_int( filetemp, fileWrite, pcap->flyheight );
    template_put_int( filetemp, fileWrite, pcap->flashand );
    template_put_int( filetemp, fileWrite, pcap->alpha );
    template_put_int( filetemp, fileWrite, pcap->light );
    template_put_bool( filetemp, fileWrite, pcap->transferblend );
    template_put_int( filetemp, fileWrite, pcap->sheen );
    template_put_bool( filetemp, fileWrite, pcap->enviro );
    template_put_float( filetemp, fileWrite, FFFF_TO_FLOAT( pcap->uoffvel ) );
    template_put_float( filetemp, fileWrite, FFFF_TO_FLOAT( pcap->voffvel ) );
    template_put_bool( filetemp, fileWrite, pcap->_stickyButt );

    // Invulnerability data
    template_put_bool( filetemp, fileWrite, pcap->invictus );
    template_put_int( filetemp, fileWrite, pcap->nframefacing );
    template_put_int( filetemp, fileWrite, pcap->nframeangle );
    template_put_int( filetemp, fileWrite, pcap->iframefacing );
    template_put_int( filetemp, fileWrite, pcap->iframeangle );

    // Skin defenses
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[0] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[1] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[2] );
    template_put_int( filetemp, fileWrite, 255 - pcap->defense[3] );

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][0] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][1] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][2] );
        template_put_float( filetemp, fileWrite, pcap->damage_resistance[damagetype][3] );
    }

    for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        char code;

        for ( skin = 0; skin < MAX_SKIN; skin++ )
        {
            if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEMANA ) )
            {
                code = 'M';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGECHARGE ) )
            {
                code = 'C';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEINVERT ) )
            {
                code = 'T';
            }
            else if ( HAS_SOME_BITS( pcap->damage_modifier[damagetype][skin], DAMAGEINVICTUS ) )
            {
                code = 'I';
            }
            else
            {
                code = 'F';
            }

            template_put_char( filetemp, fileWrite, code );
        }
    }

    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[0]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[1]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[2]*80 );
    template_put_float( filetemp, fileWrite, pcap->skin_info.maxaccel[3]*80 );

    // Experience and level data
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[1] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[2] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[3] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[4] );
    template_put_int( filetemp, fileWrite, pcap->experience_forlevel[5] );
    template_put_float( filetemp, fileWrite, FLOAT_TO_FP8( pcap->experience.from ) );
    template_put_int( filetemp, fileWrite, pcap->experience_worth );
    template_put_float( filetemp, fileWrite, pcap->experience_exchange );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[0] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[1] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[2] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[3] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[4] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[5] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[6] );
    template_put_float( filetemp, fileWrite, pcap->experience_rate[7] );

    // IDSZ identification tags
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_PARENT] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_TYPE] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_SKILL] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_SPECIAL] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_HATE] );
    template_put_idsz( filetemp, fileWrite, pcap->idsz[IDSZ_VULNERABILITY] );

    // Item and damage flags
    template_put_bool( filetemp, fileWrite, pcap->isitem );
    template_put_bool( filetemp, fileWrite, pcap->ismount );
    template_put_bool( filetemp, fileWrite, pcap->isstackable );
    template_put_bool( filetemp, fileWrite, pcap->_nameIsKnown );
    template_put_bool( filetemp, fileWrite, pcap->_usageIsKnown );
    template_put_bool( filetemp, fileWrite, pcap->_canCarryToNextModule );
    template_put_bool( filetemp, fileWrite, pcap->_needSkillIDToUse );
    template_put_bool( filetemp, fileWrite, pcap->platform );
    template_put_bool( filetemp, fileWrite, pcap->cangrabmoney );
    template_put_bool( filetemp, fileWrite, pcap->canopenstuff );

    // Other item and damage stuff
    template_put_damage_type( filetemp, fileWrite, pcap->_damageTargetDamageType );
    template_put_action( filetemp, fileWrite, pcap->_weaponAction );

    // Particle attachments
    template_put_int( filetemp, fileWrite, pcap->_attachedParticleAmount );
    template_put_damage_type( filetemp, fileWrite, pcap->_attachedParticleReaffirmDamagetype );
    template_put_int( filetemp, fileWrite, pcap->_attachedParticleProfile );

    // Character hands
    template_put_bool( filetemp, fileWrite, pcap->_slotsValid[SLOT_LEFT] );
    template_put_bool( filetemp, fileWrite, pcap->_slotsValid[SLOT_RIGHT] );

    // Particle spawning on attack
    template_put_bool( filetemp, fileWrite, 0 != pcap->_attackAttached );
    template_put_int( filetemp, fileWrite, pcap->_attackParticleProfile );

    // Particle spawning for GoPoof
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleAmount );
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleFacingAdd );
    template_put_int( filetemp, fileWrite, pcap->_goPoofParticleProfile );

    // Particle spawning for blud
    template_put_bool( filetemp, fileWrite, 0 != pcap->_bludValid );
    template_put_int( filetemp, fileWrite, pcap->_bludParticleProfile );

    // Extra stuff
    template_put_bool( filetemp, fileWrite, pcap->waterwalk );
    template_put_float( filetemp, fileWrite, pcap->dampen );

    // More stuff
    template_put_float( filetemp, fileWrite, FP8_TO_FLOAT( pcap->_startingLifeRegeneration ) );     // These two are seriously outdated
    template_put_float( filetemp, fileWrite, FP8_TO_FLOAT( pcap->_manaCost ) );     // and shouldnt be used. Use scripts instead.
    template_put_int( filetemp, fileWrite, pcap->_startingLifeRegeneration );
    template_put_int( filetemp, fileWrite, pcap->stoppedby );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[0] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[1] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[2] );
    template_put_string_under( filetemp, fileWrite, pcap->skin_info.name[3] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[0] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[1] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[2] );
    template_put_int( filetemp, fileWrite, pcap->skin_info.cost[3] );
    template_put_float( filetemp, fileWrite, pcap->_strengthBonus );

    // Another memory lapse
    template_put_bool( filetemp, fileWrite, !pcap->_riderCanAttack );
    template_put_bool( filetemp, fileWrite, pcap->_canBeDazed );
    template_put_bool( filetemp, fileWrite, pcap->_canBeGrogged );
    template_put_int( filetemp, fileWrite, 0 );
    template_put_int( filetemp, fileWrite, 0 );
    template_put_bool( filetemp, fileWrite, pcap->_seeInvisibleLevel > 0 );
    template_put_int( filetemp, fileWrite, pcap->kursechance );
    template_put_int( filetemp, fileWrite, pcap->sound_index[SOUND_FOOTFALL] );
    template_put_int( filetemp, fileWrite, pcap->sound_index[SOUND_JUMP] );

    vfs_flush( fileWrite );

    // copy the template file to the next free output section
    template_seek_free( filetemp, fileWrite );

    // Expansions
    if ( pcap->skin_info.dressy&1 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 0 );

    if ( pcap->skin_info.dressy&2 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 1 );

    if ( pcap->skin_info.dressy&4 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 2 );

    if ( pcap->skin_info.dressy&8 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'E', 'S' ), 3 );

    if ( pcap->resistbumpspawn )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'U', 'K' ), 0 );

    if ( pcap->_isBigItem )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'P', 'A', 'C', 'K' ), 0 );

    if ( !pcap->reflect )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'V', 'A', 'M', 'P' ), 1 );

    if ( pcap->alwaysdraw )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'D', 'R', 'A', 'W' ), 1 );

    if ( pcap->_isRanged )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'R', 'A', 'N', 'G' ), 1 );

    if ( pcap->hidestate != NOHIDE )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'H', 'I', 'D', 'E' ), pcap->hidestate );

    if ( pcap->_isEquipment )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'E', 'Q', 'U', 'I' ), 1 );

    if ( pcap->_bumpSizeBig >= pcap->_bumpSize * 2 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'Q', 'U', 'A' ), 1 );

    if ( pcap->draw_icon != pcap->_usageIsKnown )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'I', 'C', 'O', 'N' ), pcap->draw_icon );

    if ( pcap->forceshadow )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'H', 'A', 'D' ), 1 );

    if ( pcap->ripple == pcap->isitem )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'R', 'I', 'P', 'P' ), pcap->ripple );

    if ( -1 != pcap->isvaluable )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'V', 'A', 'L', 'U' ), pcap->isvaluable );

    if ( pcap->spelleffect_type >= 0 )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'B', 'O', 'O', 'K' ), pcap->spelleffect_type );

    if ( pcap->_attackFast )
        vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'F', 'A', 'S', 'T' ), pcap->_attackFast );

    if ( pcap->_strengthBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'R', 'D' ), pcap->_strengthBonus );

    if ( pcap->_intelligenceBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'I', 'N', 'T', 'D' ), pcap->_intelligenceBonus );

    if ( pcap->_dexterityBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'D', 'E', 'X', 'D' ), pcap->_dexterityBonus );

    if ( pcap->_wisdomBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'W', 'I', 'S', 'D' ), pcap->_wisdomBonus );

    if ( pcap->_bumpOverrideSize || pcap->_bumpOverrideSizeBig ||  pcap->_bumpOverrideHeight )
    {
        STRING sz_tmp = EMPTY_CSTR;

        if ( pcap->_bumpOverrideSize ) strcat( sz_tmp, "S" );
        if ( pcap->_bumpOverrideSizeBig ) strcat( sz_tmp, "B" );
        if ( pcap->_bumpOverrideHeight ) strcat( sz_tmp, "H" );
        if ( pcap->dont_cull_backfaces ) strcat( sz_tmp, "C" );
        if ( pcap->skin_has_transparency ) strcat( sz_tmp, "T" );

        if ( CSTR_END != sz_tmp[0] )
        {
            vfs_put_expansion_string( fileWrite, "", MAKE_IDSZ( 'M', 'O', 'D', 'L' ), sz_tmp );
        }
    }

    // Basic stuff that is always written
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'G', 'O', 'L', 'D' ), pcap->money );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'P', 'L', 'A', 'T' ), pcap->canuseplatforms );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'K', 'I', 'N' ), pcap->skin_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'C', 'O', 'N', 'T' ), pcap->content_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'S', 'T', 'A', 'T' ), pcap->state_override );
    vfs_put_expansion( fileWrite, "", MAKE_IDSZ( 'L', 'E', 'V', 'L' ), pcap->level_override );
    vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'L', 'I', 'F', 'E' ), FP8_TO_FLOAT( pcap->_spawnLife ) );
    vfs_put_expansion_float( fileWrite, "", MAKE_IDSZ( 'M', 'A', 'N', 'A' ), FP8_TO_FLOAT( pcap->_spawnMana ) );

    // Copy all skill expansions
    {
        IDSZ_node_t *pidsz;
        int iterator;

        iterator = 0;
        pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        while ( pidsz != NULL )
        {
            //Write that skill into the file
            vfs_put_expansion( fileWrite, "", pidsz->id, pidsz->level );

            //Get the next IDSZ from the map
            pidsz = idsz_map_iterate( pcap->skills, SDL_arraysize( pcap->skills ), &iterator );
        }
    }

    // dump the rest of the template file
    template_flush( filetemp, fileWrite );

    // The end
    vfs_close( fileWrite );
    template_close_vfs( filetemp );

    return true;
}
*/

float ObjectProfile::getExperienceRate(XPType type) const
{
    if(type >= _experienceRate.size()) {
        return 0.0f;
    }

    return _experienceRate[type];
}

bool ObjectProfile::hasTypeIDSZ(const IDSZ idsz)
{
    if ( IDSZ_NONE == idsz ) return true;
    if ( idsz == _idsz[IDSZ_TYPE  ] ) return true;
    if ( idsz == _idsz[IDSZ_PARENT] ) return true;

    return false;
}

bool ObjectProfile::hasIDSZ(IDSZ idsz)
{
    for(IDSZ compare : _idsz)
    {
        if(compare == idsz)
        {
            return true;
        }
    }

    return false;
}
