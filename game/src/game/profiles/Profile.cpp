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
#include "egolib/math/Random.hpp"

ObjectProfile::ObjectProfile() :
    requestCount(0),
    spawnCount(0),
    _fileName("*NONE*"),
    _imad(INVALID_MAD_REF),
    _ieve(INVALID_EVE_REF),
    _slotNumber(-1),

    _randomName(),
    _aiScript(),
    _particleProfiles(),

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
    _levelOverride(0),
    _stateOverride(0),
    _contentOverride(0),

    _idsz(),

    // inventory
    _maxAmmo(0),
    _ammo(0),
    _money(0),

    // characer stats
    _gender(GENDER_OTHER),

    // life
    _startingLife(),
    _startingLifeRegeneration(),
    _spawnLife(PERFECTBIG),

    // mana
    _startingMana(),
    _startingManaRegeneration(),
    _spawnMana(PERFECTBIG),

    _startingManaFlow(),

    _startingStrength(),
    _startingWisdom(),
    _startingIntelligence(),
    _startingDexterity(),

    // physics
    _weight(1),
    _bounciness(0.0f),
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
    _textureMovementRateX(0),
    _textureMovementRateY(0),
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

    // xp
    _experienceForLevel(),
    _startingExperience(),
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

    _damageTargetDamageType(DAMAGE_CRUSH),
    _slotsValid(),
    _riderCanAttack(false),
    _kurseChance(0),
    _hideState(NOHIDE),
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
    _attachedParticleReaffirmDamageType(DAMAGE_FIRE),
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
    _experienceRate.fill(0.0f);
    _idsz.fill(IDSZ_NONE);
    _experienceForLevel.fill(UINT32_MAX);

    memset(&_aiScript, 0, sizeof(script_info_t));
}

ObjectProfile::~ObjectProfile()
{
    // release all of the sub-profiles
    MadStack_release_one( _imad );
    //EveStack_release_one( pobj->ieve );

    //Release particle profiles
    for(const auto &element : _particleProfiles) {
        PipStack_release_one(element.second);
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

uint32_t ObjectProfile::getXPNeededForLevel(uint8_t level) const
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
    const auto &result = _soundMap.find(index);

    //Not found?
    if(result == _soundMap.end()) {
        return INVALID_SOUND_ID;
    }

    return (*result).second;
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
    const auto &result = _particleProfiles.find(index);

    //Not found in map?
    if(result == _particleProfiles.end()) {
        return INVALID_PIP_REF;
    }

    return (*result).second;
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

bool ObjectProfile::loadDataFile(const std::string &filePath)
{
    // Open the file
    vfs_FILE* fileRead = vfs_openRead( filePath.c_str() );
    if (!fileRead) {
        return false;
    }

    //read slot number (ignored for now)
    vfs_get_next_int(fileRead);
    //_slotNumber = vfs_get_next_int(fileRead);

    // Read in the class name
    char buffer[256];
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
    _size = vfs_get_next_float(fileRead);
    _sizeGainPerLevel = vfs_get_next_float(fileRead );
    _shadowSize = vfs_get_next_int(fileRead);
    _bumpSize = vfs_get_next_int(fileRead);
    _bumpHeight = vfs_get_next_int(fileRead);
    _bumpDampen = std::max(INV_FF, vfs_get_next_float(fileRead));    //0 == bumpdampenmeans infinite mass, and causes some problems
    _weight = vfs_get_next_int(fileRead);
    _jumpPower = vfs_get_next_float(fileRead );
    _jumpNumber = vfs_get_next_int(fileRead);
    _animationSpeedSneak = vfs_get_next_float(fileRead );
    _animationSpeedWalk = vfs_get_next_float(fileRead );
    _animationSpeedRun = vfs_get_next_float(fileRead );
    _flyHeight = vfs_get_next_int(fileRead);
    _flashAND = vfs_get_next_int(fileRead);
    _alpha = vfs_get_next_int(fileRead);
    _light = vfs_get_next_int(fileRead);
    _transferBlending = vfs_get_next_bool(fileRead);
    _sheen = vfs_get_next_int(fileRead);
    _phongMapping = vfs_get_next_bool(fileRead);
    _textureMovementRateX = FLOAT_TO_FFFF( vfs_get_next_float(fileRead) );
    _textureMovementRateY = FLOAT_TO_FFFF( vfs_get_next_float(fileRead) );
    _stickyButt = vfs_get_next_bool(fileRead);

    // Invulnerability data
    _isInvincible  = vfs_get_next_bool(fileRead);
    nframefacing   = vfs_get_next_int(fileRead);
    nframeangle    = vfs_get_next_int(fileRead);
    iframefacing   = vfs_get_next_int(fileRead);
    iframeangle    = vfs_get_next_int(fileRead);

    // Resist burning and stuck arrows with nframe angle of 1 or more
    if ( 1 == nframeangle )
    {
        nframeangle = 0;
    }

    // Skin defenses ( 4 skins )
    goto_colon_vfs( NULL, fileRead, false );
    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        int iTmp = 0xFF - vfs_get_int( fileRead );
        _skinInfo[cnt].defence = CLIP( iTmp, 0, 0xFF );
    }

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileRead, false );
        for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            _skinInfo[cnt].damageResistance[damagetype] = vfs_get_damage_resist( fileRead );
        }
    }

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        goto_colon_vfs( NULL, fileRead, false );

        for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
        {
            switch ( char_toupper(vfs_get_first_letter(fileRead)) )
            {
                case 'T': _skinInfo[cnt].damageModifier[damagetype] |= DAMAGEINVERT;   break;
                case 'C': _skinInfo[cnt].damageModifier[damagetype] |= DAMAGECHARGE;   break;
                case 'M': _skinInfo[cnt].damageModifier[damagetype] |= DAMAGEMANA;     break;
                case 'I': _skinInfo[cnt].damageModifier[damagetype] |= DAMAGEINVICTUS; break;

                    //F is nothing
                default: break;
            }
        }
    }

    goto_colon_vfs(NULL, fileRead, false);
    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++)
    {
        _skinInfo[cnt].maxAccel = vfs_get_float( fileRead ) / 80.0f;
    }

    // Experience and level data
    _experienceForLevel[0] = 0;
    for ( size_t level = 1; level < MAXBASELEVEL; level++ )
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
    _damageTargetDamageType = static_cast<DamageType>(vfs_get_next_damage_type(fileRead));
    _weaponAction            = action_which( vfs_get_next_char( fileRead ) );

    // Particle attachments
    _attachedParticleAmount              = vfs_get_next_int( fileRead );
    _attachedParticleReaffirmDamageType = static_cast<DamageType>(vfs_get_next_damage_type(fileRead));
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
    _bludParticleProfile = getParticleProfile(vfs_get_next_int(fileRead));

    // Stuff I forgot
    _waterWalking = vfs_get_next_bool( fileRead );
    _bounciness   = vfs_get_next_float( fileRead );

    // More stuff I forgot
    vfs_get_next_float( fileRead );  //ZF> deprecated value LifeReturn (no longer used)
    vfs_get_next_float( fileRead );  //ZF> deprecated value ManaCost (no longer used)
    _startingLifeRegeneration  = vfs_get_next_int( fileRead );
    _stoppedBy   |= vfs_get_next_int( fileRead );

    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        char skinName[256];
        vfs_get_next_name(fileRead, skinName, 256);
        _skinInfo[cnt].name = skinName;
    }

    for (size_t cnt = 0; cnt < MAX_SKIN; cnt++ )
    {
        _skinInfo[cnt].cost = vfs_get_next_int( fileRead );
    }

    _strengthBonus = vfs_get_next_float( fileRead );          //ZF> Deprecated, but keep here for backwards compatability

    // Another memory lapse
    _riderCanAttack = !vfs_get_next_bool(fileRead);     //ZF> note value is inverted intentionally
    _canBeDazed     =  vfs_get_next_bool(fileRead);
    _canBeGrogged   =  vfs_get_next_bool(fileRead);

    goto_colon_vfs( NULL, fileRead, false );  // Depracated, no longer used (permanent life add)
    goto_colon_vfs( NULL, fileRead, false );  // Depracated, no longer used (permanent mana add)
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
    _canUsePlatforms = !_isPlatform;

    // Read expansions
    while ( goto_colon_vfs(NULL, fileRead, true) )
    {
        const IDSZ idsz = vfs_get_idsz( fileRead );

        switch(idsz)
        {
            case MAKE_IDSZ( 'D', 'R', 'E', 'S' ):
                _skinInfo[vfs_get_int(fileRead)].dressy = true;
            break;

            case MAKE_IDSZ( 'G', 'O', 'L', 'D' ):
                _money = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'S', 'T', 'U', 'K' ):
                _resistBumpSpawn = ( 0 != ( 1 - vfs_get_int( fileRead ) ) );
            break;

            case MAKE_IDSZ( 'P', 'A', 'C', 'K' ):
                _isBigItem = !( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'V', 'A', 'M', 'P' ):
                _hasReflection = !( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'D', 'R', 'A', 'W' ):
                _alwaysDraw = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'R', 'A', 'N', 'G' ):
                _isRanged = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'H', 'I', 'D', 'E' ):
                _hideState = vfs_get_int( fileRead );
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
                _skinOverride = iTmp;  
            }          
            break;

            case MAKE_IDSZ( 'C', 'O', 'N', 'T' ): 
                _contentOverride = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'S', 'T', 'A', 'T' ): 
                _stateOverride = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'L', 'E', 'V', 'L' ): 
                _levelOverride = vfs_get_int( fileRead );
            break;

            case MAKE_IDSZ( 'P', 'L', 'A', 'T' ): 
                _canUsePlatforms = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'R', 'I', 'P', 'P' ): 
                _causesRipples = ( 0 != vfs_get_int( fileRead ) );
            break;

            case MAKE_IDSZ( 'V', 'A', 'L', 'U' ): 
                _isValuable = vfs_get_int( fileRead );
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
                _spellEffectType = iTmp;
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
                _skills[idsz] = vfs_get_int(fileRead);
            break;
        }
    }

    vfs_close( fileRead );
    return true;
}

const SkinInfo& ObjectProfile::getSkinInfo(size_t index)
{
    return _skinInfo[index];
}

float ObjectProfile::getExperienceRate(XPType type) const
{
    if(type >= _experienceRate.size()) {
        return 0.0f;
    }

    return _experienceRate[type];
}

bool ObjectProfile::hasTypeIDSZ(const IDSZ idsz) const
{
    if ( IDSZ_NONE == idsz ) return true;
    if ( idsz == _idsz[IDSZ_TYPE  ] ) return true;
    if ( idsz == _idsz[IDSZ_PARENT] ) return true;

    return false;
}

bool ObjectProfile::hasIDSZ(IDSZ idsz) const
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

std::shared_ptr<ObjectProfile> ObjectProfile::loadFromFile(const std::string &folderPath, const PRO_REF slotNumber)
{
    //Make sure slot number is valid
    if(slotNumber == INVALID_PRO_REF)
    {
        log_warning("ObjectProfile::loadFromFile() - Invalid PRO_REF (%d)\n", slotNumber);
        return nullptr;
    }

    //Allocate memory
    std::shared_ptr<ObjectProfile> profile = std::make_shared<ObjectProfile>();

    //Set some data
    profile->_fileName = folderPath;
    profile->_slotNumber = slotNumber;

    // Load the model for this profile
    profile->_imad = load_one_model_profile_vfs(folderPath.c_str(), profile->_slotNumber);
    if(profile->_imad == INVALID_MAD_REF)
    {
        log_warning("ObjectProfile::loadFromFile() - Unable to load model (%s)\n", folderPath.c_str());
        return nullptr;
    }

    // Load the enchantment for this profile (optional)
    STRING newloadname;
    make_newloadname( folderPath.c_str(), "/enchant.txt", newloadname );
    profile->_ieve = EveStack_losd_one( newloadname, static_cast<EVE_REF>(slotNumber) );

    // Load the messages for this profile, do this before loading the AI script
    // to ensure any dynamic loaded messages get loaded last (optional)
    profile->loadAllMessages(folderPath + "/message.txt");

    // Load the particles for this profile (optional)
    for (size_t cnt = 0; cnt < 30; cnt++ ) //TODO: find better way of listing files
    {
        const std::string particleName = folderPath + "/part" + std::to_string(cnt) + ".txt";
        PIP_REF particleProfile = PipStack_load_one(particleName.c_str(), INVALID_PIP_REF);

        // Make sure it's referenced properly
        if(particleProfile != INVALID_PIP_REF) {
            profile->_particleProfiles[cnt] = particleProfile; 
        }
    }

    //Load profile graphics (optional)
    profile->loadTextures(folderPath);

    // Load the waves for this iobj
    for ( size_t cnt = 0; cnt < 30; cnt++ ) //TODO: make better search than just 30 (list files?)
    {
        const std::string soundName = folderPath + "/sound" + std::to_string(cnt);
        SoundID soundID = _audioSystem.loadSound(soundName);

        if(soundID != INVALID_SOUND_ID) {
            profile->_soundMap[cnt] = soundID;
        }
    }

    // Load the random naming table for this icap (optional)
    profile->_randomName.loadFromFile(folderPath + "/naming.txt");

    // Finally load the character profile
    // Do after loading particle and sound profiles
    try {
        if(!profile->loadDataFile(folderPath + "/data.txt")) {
            log_warning("Unable to load data.txt for profile: %s\n", folderPath.c_str());
            return nullptr;
        }
    }
    catch (const std::runtime_error &ex) {
        log_warning("ProfileSystem::loadFromFile() - Failed to parse (%s/data.txt): (%s)\n", folderPath.c_str(), ex.what());
        return nullptr;
    }

    // Fix lighting if need be
    if (profile->_uniformLit && cfg.gouraud_req)
    {
        mad_make_equally_lit_ref(profile->_imad);
    }

    return profile;
}


bool ObjectProfile::isSlotValid(slot_t slot) const
{
    if(slot >= _slotsValid.size()) {
        return false;
    }

    return _slotsValid[slot];
}