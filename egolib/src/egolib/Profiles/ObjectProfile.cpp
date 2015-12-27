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

/// @file  egolib/Profiles/ObjectProfile.cpp
/// @brief ObjectProfile handling
/// @details
/// @author Johan Jansen

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/ObjectProfile.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/Entities/_Include.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/FileFormats/template.h"
#include "egolib/Math/Random.hpp"

static const SkinInfo INVALID_SKIN = SkinInfo();

ObjectProfile::ObjectProfile() :
    _spawnRequestCount(0),
    _spawnCount(0),
    _pathname("*NONE*"),
    _model(nullptr),
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

    //for imports
    _spawnLife(PERFECTBIG),
    _spawnMana(PERFECTBIG),

    //Base attributes
    _baseAttribute(),
    _attributeGain(),

    // physics
    _weight(1),
    _bounciness(0.0f),
    _bumpDampen(INV_FF<float>()),

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
    _hasReflection(true),
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
    _blockRating(0),

    // defense
    _resistBumpSpawn(false),

    // xp
    _experienceForLevel(),
    _startingExperience(),
    _experienceWorth(0),
    _experienceExchange(0.0f),
    _experienceRate(),
    _levelUpRandomSeedOverride(0),

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
    _attachAttackParticleToWeapon(false),
    _attackParticle(-1),
    _attackFast(false),

    _strengthBonus(0.0f),
    _intelligenceBonus(0.0f),
    _dexterityBonus(0.0f),

    // special particle effects
    _attachedParticleAmount(0),
    _attachedParticleReaffirmDamageType(DAMAGE_FIRE),
    _attachedParticle(-1),

    _goPoofParticleAmount(0),
    _goPoofParticleFacingAdd(0),
    _goPoofParticle(-1),

    //Blood
    _bludValid(0),
    _bludParticle(-1),

    // skill system
    _seeInvisibleLevel(0),

    // random stuff
    _stickyButt(false),
    _useManaCost(0.0f),

    _startingPerks(),
    _perkPool()
{
    _experienceRate.fill(0.0f);
    _idsz.fill(IDSZ2::None);
    _experienceForLevel.fill(std::numeric_limits<uint32_t>::max());
}

ObjectProfile::~ObjectProfile()
{
    //Release particle profiles
    for(const auto &element : _particleProfiles)
    {
        ParticleProfileSystem::get().release(element.second);
    }
}

uint32_t ObjectProfile::getXPNeededForLevel(uint8_t level) const
{
    if(level >= _experienceForLevel.size()) {
        return std::numeric_limits<uint32_t>::max();
    }

    return _experienceForLevel[level];
}

void ObjectProfile::loadTextures(const std::string &folderPath)
{
    //Clear texture references
    _texturesLoaded.clear();
    _iconsLoaded.clear();

    // Load the skins and icons
    for (int cnt = 0; cnt < SKINS_PEROBJECT_MAX*2; cnt++)
    {
        // do the texture
        const std::string skinPath = folderPath + "/tris" + std::to_string(cnt);
        if(ego_texture_exists_vfs(skinPath))
        {
            _texturesLoaded[cnt] = Ego::DeferredTexture(skinPath);
        }

        // do the icon
        const std::string iconPath = folderPath + "/icon" + std::to_string(cnt);
	    if(ego_texture_exists_vfs(iconPath))
        {
            _iconsLoaded[cnt] = Ego::DeferredTexture(iconPath);
        }
    }

    // If we didn't get a skin, set it to the water texture
    if ( _texturesLoaded.empty() )
    {
        _texturesLoaded[0] = Ego::DeferredTexture("mp_data/waterlow");
		Log::get().warn("Object is missing a skin (%s)!\n", getPathname().c_str());
    }

    // If we didn't get a icon, set it to the NULL icon
    if ( _iconsLoaded.empty())
    {
        _iconsLoaded[0] = Ego::DeferredTexture("mp_data/nullicon");
		Log::get().debug("Object is missing an icon (%s)!\n", getPathname().c_str());
    }
}

size_t ObjectProfile::addMessage(const std::string &message, const bool filterDuplicates)
{
    std::string parsedMessage = message;

    //replace underscore with whitespace
    std::replace(parsedMessage.begin(), parsedMessage.end(), '_', ' ');

    //Don't add the same message twice
    if(filterDuplicates) {
        size_t messageListSize = _messageList.size();
        for(size_t pos = 0; pos < messageListSize; pos++) {
            if(_messageList[pos] == parsedMessage) {
                return pos;
            }
        }
    }

    //Add it to the list!
    _messageList.push_back(parsedMessage);
    return _messageList.size() - 1;
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

    ReadContext ctxt(filePath);
    if (!ctxt.ensureOpen()) return;
    STRING line;

    while (ctxt.skipToColon(true))
    {
        //Load one line
        vfs_read_string_lit(ctxt, line, SDL_arraysize(line));
        addMessage(line);
    }
}

const std::string ObjectProfile::generateRandomName()
{
    //If no random names loaded, return class name instead
    if (!_randomName.isLoaded())
    {
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

const IDSZ2& ObjectProfile::getIDSZ(size_t type) const
{
    if(type >= IDSZ_COUNT) {
        return IDSZ2::None;
    }

    return _idsz[type];
}

const Ego::DeferredTexture& ObjectProfile::getSkin(size_t index)
{
    if(_texturesLoaded.find(index) == _texturesLoaded.end()) {
        return _texturesLoaded[0];
    }

    return _texturesLoaded[index];
}

const Ego::DeferredTexture& ObjectProfile::getIcon(size_t index)
{
    if(_iconsLoaded.find(index) == _iconsLoaded.end()) {
        return _iconsLoaded[0];
    }

    return _iconsLoaded[index];
}

PIP_REF ObjectProfile::getParticleProfile(const LocalParticleProfileRef& lppref) const
{
    if (lppref.get() <= -1) {
        return INVALID_PIP_REF;
    } 

    const auto &result = _particleProfiles.find(lppref);

    //Not found in map?
    if(result == _particleProfiles.end()) {
        return INVALID_PIP_REF;
    }

    return (*result).second;
}

uint16_t ObjectProfile::getSkinOverride() const
{
    //Are we actually a spell book?
    if (_spellEffectType != NO_SKIN_OVERRIDE) {
        return _spellEffectType;
    }

    return _skinOverride;
}

void ObjectProfile::setupXPTable()
{
    for (size_t level = MAXBASELEVEL; level < MAXLEVEL; level++ )
    {
        uint32_t xpneeded = _experienceForLevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        _experienceForLevel[level] = xpneeded;
    }
}

bool ObjectProfile::loadDataFile(const std::string &filePath)
{
    // Open the file
    ReadContext ctxt(filePath);
    if (!ctxt.ensureOpen())
    {
        return false;
    }

    //read slot number (ignored for now)
    vfs_get_next_int(ctxt);
    //_slotNumber = vfs_get_next_int(ctxt);

    // Read in the class name
    char buffer[256];
    vfs_get_next_string_lit(ctxt, buffer, SDL_arraysize(buffer));

    // fix class name capitalization
    buffer[0] = Ego::toupper(buffer[0]);

    _className = buffer;

    // Light cheat
    _uniformLit = vfs_get_next_bool(ctxt);

    // Ammo
    _maxAmmo = vfs_get_next_int(ctxt);
    _ammo = vfs_get_next_int(ctxt);

    // Gender
    switch (Ego::toupper(vfs_get_next_printable(ctxt)))
    {
        case 'F': _gender = GENDER_FEMALE; break;
        case 'M': _gender = GENDER_MALE; break;
        case 'R': _gender = GENDER_RANDOM; break;
        default:  _gender = GENDER_OTHER; break;
    }

    // Read in the starting stats
    _lifeColor = vfs_get_next_int(ctxt);
    _manaColor = vfs_get_next_int(ctxt);

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::MAX_LIFE]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::MAX_LIFE]);

    //@note ZF> All life is doubled because Aaron decided it was a good idea to shift all damage by 1 by default (>> 1)
    //          With the newer damage system, we do it this way instead to keep the same game balance as before
    //          This allows for more granularity and show the player that he is slowly losing health from posion for example
    _baseAttribute[Ego::Attribute::MAX_LIFE].from *= 2;
    _baseAttribute[Ego::Attribute::MAX_LIFE].to *= 2;

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::MAX_MANA]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::MAX_MANA]);

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::MANA_REGEN]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::MANA_REGEN]);

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::SPELL_POWER]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::SPELL_POWER]);

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::MIGHT]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::MIGHT]);

    FRange wisdom, wisdomGain;
    vfs_get_next_range(ctxt, &wisdom);
    vfs_get_next_range(ctxt, &wisdomGain);

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::INTELLECT]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::INTELLECT]);

    //Wisdom used to be an attribute in Egoboo, but now its deprecated. To figure out intellect use average of WIS and INT
    if(!wisdom.isZero()) {
        _baseAttribute[Ego::Attribute::INTELLECT].from = 0.5f*(_baseAttribute[Ego::Attribute::INTELLECT].from + wisdom.from);
        _baseAttribute[Ego::Attribute::INTELLECT].to   = 0.5f*(_baseAttribute[Ego::Attribute::INTELLECT].to   + wisdom.to);        
    }
    if(!wisdomGain.isZero()) {
        _attributeGain[Ego::Attribute::INTELLECT].from = 0.5f*(_attributeGain[Ego::Attribute::INTELLECT].from + wisdomGain.from);
        _attributeGain[Ego::Attribute::INTELLECT].to   = 0.5f*(_attributeGain[Ego::Attribute::INTELLECT].to   + wisdomGain.to);        
    }

    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::AGILITY]);
    vfs_get_next_range(ctxt, &_attributeGain[Ego::Attribute::AGILITY]);

    // More physical attributes
    _size = vfs_get_next_float(ctxt);
    _sizeGainPerLevel = vfs_get_next_float(ctxt);
    _shadowSize = vfs_get_next_int(ctxt);
    _bumpSize = vfs_get_next_int(ctxt);
    _bumpHeight = vfs_get_next_int(ctxt);
    _bumpDampen = std::max(INV_FF<float>(), vfs_get_next_float(ctxt));    //0 == bumpdampenmeans infinite mass, and causes some problems
    _weight = vfs_get_next_int(ctxt);
    _jumpPower = vfs_get_next_float(ctxt);
    _jumpNumber = vfs_get_next_int(ctxt);
    _animationSpeedSneak = 2.0f * vfs_get_next_float(ctxt);
    _animationSpeedWalk = 2.0f * vfs_get_next_float(ctxt);
    _animationSpeedRun = 2.0f * vfs_get_next_float(ctxt);
    _flyHeight = vfs_get_next_int(ctxt);
    _flashAND = vfs_get_next_int(ctxt);
    _alpha = vfs_get_next_int(ctxt);
    _light = vfs_get_next_int(ctxt);
    _transferBlending = vfs_get_next_bool(ctxt);
    _sheen = vfs_get_next_int(ctxt);
    _phongMapping = vfs_get_next_bool(ctxt);
    _textureMovementRateX = FLOAT_TO_FFFF(vfs_get_next_float(ctxt));
    _textureMovementRateY = FLOAT_TO_FFFF(vfs_get_next_float(ctxt));
    _stickyButt = vfs_get_next_bool(ctxt);

    // Invulnerability data
    _isInvincible = vfs_get_next_bool(ctxt);
    nframefacing = vfs_get_next_int(ctxt);
    nframeangle = vfs_get_next_int(ctxt);
    iframefacing = vfs_get_next_int(ctxt);
    iframeangle = vfs_get_next_int(ctxt);

    // Resist burning and stuck arrows with nframe angle of 1 or more
    if ( 1 == nframeangle )
    {
        nframeangle = 0;
    }

    // Skin defenses ( 4 skins )
    ctxt.skipToColon(false);
    for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
    {
        _skinInfo[cnt].defence = Ego::Math::constrain(ctxt.readIntegerLiteral(), 0, 0xFF);
    }

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        ctxt.skipToColon(false);
        for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
        {
            _skinInfo[cnt].damageResistance[damagetype] = vfs_get_damage_resist(ctxt);
        }
    }

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        ctxt.skipToColon(false);

        for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
        {
            _skinInfo[cnt].damageModifier[damagetype] = 0;
            switch (Ego::toupper(ctxt.readPrintable()))
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

    ctxt.skipToColon(false);
    for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
    {
        _skinInfo[cnt].maxAccel = ctxt.readRealLiteral() / 80.0f;
    }

    // Experience and level data
    _experienceForLevel[0] = 0;
    for ( size_t level = 1; level < MAXBASELEVEL; level++ )
    {
        _experienceForLevel[level] = vfs_get_next_int(ctxt);
    }
    setupXPTable();

    vfs_get_next_range(ctxt, &_startingExperience);

    _experienceWorth = vfs_get_next_int(ctxt);
    _experienceExchange = vfs_get_next_float(ctxt);

    for(size_t i = 0; i < _experienceRate.size(); ++i)
    {
        _experienceRate[i] = vfs_get_next_float(ctxt) + 0.001f;
    }

    // IDSZ tags
    for (size_t i = 0; i < _idsz.size(); ++i)
    {
        _idsz[i] = vfs_get_next_idsz(ctxt);
    }

    // Item and damage flags
    _isItem = vfs_get_next_bool(ctxt);
    _isMount = vfs_get_next_bool(ctxt);
    _isStackable = vfs_get_next_bool(ctxt);
    _nameIsKnown = vfs_get_next_bool(ctxt);
    _usageIsKnown = vfs_get_next_bool(ctxt);
    _canCarryToNextModule = vfs_get_next_bool(ctxt);
    _needSkillIDToUse = vfs_get_next_bool(ctxt);
    _isPlatform = vfs_get_next_bool(ctxt);
    _canGrabMoney = vfs_get_next_bool(ctxt);
    _canOpenStuff = vfs_get_next_bool(ctxt);

    // More item and damage stuff
    _damageTargetDamageType = vfs_get_next_damage_type(ctxt);
    _weaponAction = Ego::ModelDescriptor::charToAction(vfs_get_next_printable(ctxt));

    // Particle attachments
    _attachedParticleAmount = vfs_get_next_int(ctxt);
    _attachedParticleReaffirmDamageType = vfs_get_next_damage_type(ctxt);
    _attachedParticle = vfs_get_next_local_particle_profile_ref(ctxt);

    // Character hands
    _slotsValid[SLOT_LEFT] = vfs_get_next_bool(ctxt);
    _slotsValid[SLOT_RIGHT] = vfs_get_next_bool(ctxt);

    // Attack order ( weapon )
    _attachAttackParticleToWeapon = vfs_get_next_bool(ctxt);
    _attackParticle = vfs_get_next_local_particle_profile_ref(ctxt);

    // GoPoof
    _goPoofParticleAmount = vfs_get_next_int(ctxt);
    _goPoofParticleFacingAdd = vfs_get_next_int(ctxt);
    _goPoofParticle = vfs_get_next_local_particle_profile_ref(ctxt);

    // Blud
    switch (Ego::toupper(vfs_get_next_printable(ctxt)))
    {
        case 'T': _bludValid = true;        break;
        case 'U': _bludValid = ULTRABLUDY;  break;
        default:  _bludValid = false;       break;
    }
    _bludParticle = vfs_get_next_local_particle_profile_ref(ctxt);

    // Stuff I forgot
    _waterWalking = vfs_get_next_bool(ctxt);
    _bounciness = vfs_get_next_float(ctxt);

    // More stuff I forgot
    vfs_get_next_float(ctxt);  //ZF> deprecated value LifeReturn (no longer used)
    _useManaCost = vfs_get_next_float(ctxt);
    vfs_get_next_range(ctxt, &_baseAttribute[Ego::Attribute::LIFE_REGEN]);
    _attributeGain[Ego::Attribute::LIFE_REGEN].from = _attributeGain[Ego::Attribute::LIFE_REGEN].to = 0;    //ZF> TODO: regen gain per level not implemented
    _baseAttribute[Ego::Attribute::LIFE_REGEN].from /= 256.0f;
    _baseAttribute[Ego::Attribute::LIFE_REGEN].to /= 256.0f;
    _stoppedBy |= vfs_get_next_int(ctxt);

    for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
    {
        char skinName[256];
        vfs_get_next_string_lit(ctxt, skinName, 256);
        _skinInfo[cnt].name = skinName;
    }

    for (size_t cnt = 0; cnt < SKINS_PEROBJECT_MAX; cnt++)
    {
        _skinInfo[cnt].cost = vfs_get_next_int(ctxt);
    }

    _strengthBonus = vfs_get_next_float(ctxt);          //ZF> Deprecated, but keep here for backwards compatability

    // Another memory lapse
    _riderCanAttack = !vfs_get_next_bool(ctxt);     //ZF> note value is inverted intentionally
    _canBeDazed = vfs_get_next_bool(ctxt);
    _canBeGrogged = vfs_get_next_bool(ctxt);

    ctxt.skipToColon(false);  // Depracated, no longer used (permanent life add)
    ctxt.skipToColon(false);  // Depracated, no longer used (permanent mana add)
    if (vfs_get_next_bool(ctxt)) {
        _seeInvisibleLevel = 1;
    }

    _kurseChance = vfs_get_next_int(ctxt);
    _footFallSound = vfs_get_next_int(ctxt);  // Footfall sound
    _jumpSound = vfs_get_next_int(ctxt);  // Jump sound

    // assume the normal dependence of _causesRipples on _isItem
    _causesRipples = !_isItem;

    // assume a round object
    _bumpSizeBig = _bumpSize * Ego::Math::sqrtTwo<float>();

    // assume the normal icon usage
    _drawIcon = _usageIsKnown;

    // assume normal platform usage
    _canUsePlatforms = !_isPlatform;

    // Read expansions
    while (ctxt.skipToColon(true))
    {
        const IDSZ2 idsz = ctxt.readIDSZ();

        switch(idsz.toUint32())
        {
            case IDSZ2::caseLabel( 'D', 'R', 'E', 'S' ):
                _skinInfo[ctxt.readIntegerLiteral()].dressy = true;
            break;

            case IDSZ2::caseLabel( 'G', 'O', 'L', 'D' ):
                _money = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'S', 'T', 'U', 'K' ):
                _resistBumpSpawn = (0 != (1 - ctxt.readIntegerLiteral()));
            break;

            case IDSZ2::caseLabel( 'P', 'A', 'C', 'K' ):
                _isBigItem = !(0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'V', 'A', 'M', 'P' ):
                _hasReflection = (0 == ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'D', 'R', 'A', 'W' ):
                _alwaysDraw = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'R', 'A', 'N', 'G' ):
                _isRanged = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'H', 'I', 'D', 'E' ):
                _hideState = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'E', 'Q', 'U', 'I' ):
                _isEquipment = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'S', 'Q', 'U', 'A' ):
                _bumpSizeBig = _bumpSize * 2;
            break;

            case IDSZ2::caseLabel( 'I', 'C', 'O', 'N' ):
                _drawIcon = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'S', 'H', 'A', 'D' ):
                _forceShadow = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'S', 'K', 'I', 'N' ):
            {
                /// @note BB@> This is the skin value of a saved character.
                ///            It should(!) correspond to a valid skin for this object,
                ///            but possibly it could have one of two special values (NO_SKIN_OVERRIDE or SKINS_PEROBJECT_MAX)

                int iTmp = ctxt.readIntegerLiteral();

                iTmp = ( iTmp < 0 ) ? NO_SKIN_OVERRIDE : iTmp;
                _skinOverride = iTmp;  
            }          
            break;

            case IDSZ2::caseLabel( 'C', 'O', 'N', 'T' ): 
                _contentOverride = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'S', 'T', 'A', 'T' ): 
                _stateOverride = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'L', 'E', 'V', 'L' ): 
                _levelOverride = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'P', 'L', 'A', 'T' ): 
                _canUsePlatforms = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'R', 'I', 'P', 'P' ): 
                _causesRipples = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'V', 'A', 'L', 'U' ): 
                _isValuable = ctxt.readIntegerLiteral();
            break;

            case IDSZ2::caseLabel( 'L', 'I', 'F', 'E' ): 
                _spawnLife = 0xff * ctxt.readRealLiteral();
            break;

            case IDSZ2::caseLabel( 'M', 'A', 'N', 'A' ): 
                _spawnMana = 0xff * ctxt.readRealLiteral();
            break;

            case IDSZ2::caseLabel( 'B', 'O', 'O', 'K' ):
            {
                _spellEffectType = ctxt.readIntegerLiteral();
            }
            break;

            //Damage bonuses from stats
            case IDSZ2::caseLabel( 'F', 'A', 'S', 'T' ):
                _attackFast = (0 != ctxt.readIntegerLiteral());
            break;

            case IDSZ2::caseLabel( 'S', 'T', 'R', 'D' ):
                _strengthBonus = ctxt.readRealLiteral();
            break;

            case IDSZ2::caseLabel( 'I', 'N', 'T', 'D' ):
                _intelligenceBonus = ctxt.readRealLiteral();
            break;

            case IDSZ2::caseLabel( 'D', 'E', 'X', 'D' ):
                _dexterityBonus = ctxt.readRealLiteral();
            break;

            case IDSZ2::caseLabel( 'M', 'O', 'D', 'L' ):
            {
                char tmp_buffer[1024+1];
                vfs_read_string_lit(ctxt, tmp_buffer, 1024);
                if (strlen(tmp_buffer) > 0)
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

            case IDSZ2::caseLabel('B', 'L', 'O', 'C'):
            {
                _blockRating = ctxt.readIntegerLiteral();
            }
            break;

            //Random Seed for level ups
            case  IDSZ2::caseLabel( 'S', 'E', 'E', 'D' ):
                _levelUpRandomSeedOverride = ctxt.readIntegerLiteral();
            break;

            //Perks known
            case IDSZ2::caseLabel( 'P', 'E', 'R', 'K' ):
            {
                std::string perkName = ctxt.readName();
                std::replace(perkName.begin(), perkName.end(), '_', ' '); //replace underscore with spaces
                Ego::Perks::PerkID id = Ego::Perks::PerkHandler::get().fromString(perkName);
                if(id != Ego::Perks::NR_OF_PERKS)
                {
                    _startingPerks[id] = true;
                }
                else
                {
					Log::get().warn("Unknown [PERK] parsed: %s (%s)\n", perkName.c_str(), filePath.c_str());
                }
            }
            break;

            //Perk Pool (perks that we can learn in the future)
            case IDSZ2::caseLabel( 'P', 'O', 'O', 'L' ):
            {
                std::string perkName = ctxt.readName();
                std::replace(perkName.begin(), perkName.end(), '_', ' '); //replace underscore with spaces
                Ego::Perks::PerkID id = Ego::Perks::PerkHandler::get().fromString(perkName);
                if(id != Ego::Perks::NR_OF_PERKS)
                {
                    _perkPool[id] = true;
                }
                else
                {
					Log::get().warn("Unknown [POOL] perk parsed: %s (%s)\n", perkName.c_str(), filePath.c_str());
                }
            }
            break;

            //Backwards compatability with old skill system (for older data files)
            case IDSZ2::caseLabel('A', 'W', 'E', 'P'): _startingPerks[Ego::Perks::WEAPON_PROFICIENCY] = true; break;
            case IDSZ2::caseLabel('P', 'O', 'I', 'S'): _startingPerks[Ego::Perks::POISONRY] = true; break;
            case IDSZ2::caseLabel('C', 'K', 'U', 'R'): _startingPerks[Ego::Perks::SENSE_KURSES] = true; break;
            case IDSZ2::caseLabel('R', 'E', 'A', 'D'): _startingPerks[Ego::Perks::LITERACY] = true; break;
            case IDSZ2::caseLabel('W', 'M', 'A', 'G'): _startingPerks[Ego::Perks::ARCANE_MAGIC] = true; break;
            case IDSZ2::caseLabel('H', 'M', 'A', 'G'): _startingPerks[Ego::Perks::DIVINE_MAGIC] = true; break;
            case IDSZ2::caseLabel('T', 'E', 'C', 'H'): _startingPerks[Ego::Perks::USE_TECHNOLOGICAL_ITEMS] = true; break;
            case IDSZ2::caseLabel('D', 'I', 'S', 'A'): _startingPerks[Ego::Perks::TRAP_LORE] = true; break;
            case IDSZ2::caseLabel('S', 'T', 'A', 'B'): _startingPerks[Ego::Perks::BACKSTAB] = true; break;
            case IDSZ2::caseLabel('D', 'A', 'R', 'K'): _startingPerks[Ego::Perks::NIGHT_VISION] = true; break;
            case IDSZ2::caseLabel('J', 'O', 'U', 'S'): _startingPerks[Ego::Perks::JOUSTING] = true; break;

            default:
				Log::get().warn("Unknown IDSZ parsed: [%4s] (%s)\n", idsz.toString().c_str(), filePath.c_str());
            break;
        }
    }
    return true;
}

const SkinInfo& ObjectProfile::getSkinInfo(size_t index) const
{
    const auto &result = _skinInfo.find(index);
    if(result == _skinInfo.end()) {
        return INVALID_SKIN; //empty skin (dont construct new element in map)
    }

    return (*result).second;
}

bool ObjectProfile::isValidSkin(size_t index) const
{
    return _skinInfo.find(index) != _skinInfo.end();
}

float ObjectProfile::getExperienceRate(XPType type) const
{
    if(type >= _experienceRate.size()) {
        return 0.0f;
    }

    return _experienceRate[type];
}

bool ObjectProfile::hasTypeIDSZ(const IDSZ2& idsz) const
{
    if ( IDSZ2::None == idsz ) return true;
    if ( idsz == _idsz[IDSZ_TYPE  ] ) return true;
    if ( idsz == _idsz[IDSZ_PARENT] ) return true;

    return false;
}

bool ObjectProfile::hasIDSZ(const IDSZ2& idsz) const
{
    for(const IDSZ2& compare : _idsz)
    {
        if(compare == idsz)
        {
            return true;
        }
    }

    return false;
}

std::shared_ptr<ObjectProfile> ObjectProfile::loadFromFile(const std::string &folderPath, const PRO_REF slotNumber, const bool lightWeight)
{
    //Make sure slot number is valid
    if(slotNumber == INVALID_PRO_REF)
    {
		Log::get().warn("ObjectProfile::loadFromFile() - Invalid PRO_REF (%d)\n", slotNumber);
        return nullptr;
    }

    //Allocate memory
    std::shared_ptr<ObjectProfile> profile = std::make_shared<ObjectProfile>();

    //Set some data
    profile->_pathname = folderPath;
    profile->_slotNumber = slotNumber;

    //Don't load 3d model, enchant, messages, sounds or particle effects for lightweight profiles
    if(!lightWeight)
    {
        // Load the model for this profile
        try {
            profile->_model = std::make_shared<Ego::ModelDescriptor>(folderPath.c_str());
        }
        catch (const std::runtime_error &ex) {
			Log::get().warn("ObjectProfile::loadFromFile() - Unable to load model (%s)\n", folderPath.c_str());
            return nullptr;
        }

        // Load the enchantment for this profile (optional)
        STRING newloadname;
        make_newloadname( folderPath.c_str(), "/enchant.txt", newloadname );
        profile->_ieve = EnchantProfileSystem.load_one( newloadname, static_cast<EVE_REF>(slotNumber) );

        // Load the messages for this profile, do this before loading the AI script
        // to ensure any dynamic loaded messages get loaded last (optional)
        profile->loadAllMessages(folderPath + "/message.txt");

        // Load the particles for this profile (optional)
        for (LocalParticleProfileRef cnt(0); cnt.get() < 30; ++cnt) //TODO: find better way of listing files
        {
            const std::string particleName = folderPath + "/part" + std::to_string(cnt.get()) + ".txt";
            PIP_REF particleProfile = ParticleProfileSystem::get().load_one(particleName.c_str(), INVALID_PIP_REF);

            // Make sure it's referenced properly
            if(particleProfile != INVALID_PIP_REF) {
                profile->_particleProfiles[cnt] = particleProfile; 
            }
        }

        // Load the waves for this iobj
        for ( size_t cnt = 0; cnt < 30; cnt++ ) //TODO: make better search than just 30 (list files?)
        {
            const std::string soundName = folderPath + "/sound" + std::to_string(cnt);
            SoundID soundID = AudioSystem::get().loadSound(soundName);

            if(soundID != INVALID_SOUND_ID) {
                profile->_soundMap[cnt] = soundID;
            }
        }
    }

    //Load profile graphics (optional)
    profile->loadTextures(folderPath);

    // Load the random naming table for this icap (optional)
    profile->_randomName.loadFromFile(folderPath + "/naming.txt");

    // Finally load the character profile
    // Do after loading particle and sound profiles
    try {
        if(!profile->loadDataFile(folderPath + "/data.txt")) {
			Log::get().warn("Unable to load data.txt for profile: %s\n", folderPath.c_str());
            return nullptr;
        }
    }
    catch (const std::runtime_error &ex) {
		Log::get().warn("ProfileSystem::loadFromFile() - Failed to parse (%s/data.txt): (%s)\n", folderPath.c_str(), ex.what());
        return nullptr;
    }

    // Fix lighting if need be
    if (profile->_uniformLit && egoboo_config_t::get().graphic_gouraudShading_enable.getValue())
    {
        profile->getModel()->makeEquallyLit();
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


bool ObjectProfile::exportCharacterToFile(const std::string &filePath, const Object *character)
{
    if (nullptr == character) {
        return false;
    }

    // Open the file
    vfs_FILE *fileWrite = vfs_openWrite(filePath);
    if (!fileWrite) {
        return false;  
    } 

    // open the template file
    vfs_FILE *fileTemp = template_open_vfs( "mp_data/templates/data.txt" );

    //did we find a template file?
    if (!fileTemp)
    {
        vfs_close( fileWrite );
        return false;
    }

    const std::shared_ptr<ObjectProfile> &profile = character->getProfile();

    // Real general data
    template_put_int( fileTemp, fileWrite, -1 );     // -1 signals a flexible load thing
    template_put_string_under( fileTemp, fileWrite, profile->_className.c_str() );
    template_put_bool( fileTemp, fileWrite, profile->_uniformLit );
    template_put_int( fileTemp, fileWrite, character->ammomax );     //Note: overridden by chr
    template_put_int( fileTemp, fileWrite, character->ammo );        //Note: overridden by chr
    template_put_gender( fileTemp, fileWrite, character->gender );   //Note: overridden by chr

     //Attributes (TODO: can be easily converted into a for loop if order does not matter)
    template_put_int( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::LIFE_BARCOLOR) );              //Note: overriden by chr
    template_put_int( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::MANA_BARCOLOR) );              //Note: overriden by chr
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::MAX_LIFE)*0.5f ); //Note: overriden by chr (ZF> Halved hp because it is doubled on parse)
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::MAX_LIFE));
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::MAX_MANA) ); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::MAX_MANA));
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::MANA_REGEN)); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::MANA_REGEN));
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::SPELL_POWER) ); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::SPELL_POWER));
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::MIGHT) ); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::MIGHT));
    template_put_float( fileTemp, fileWrite, 0.0f); //Note: deprecated
    template_put_float( fileTemp, fileWrite, 0.0f); //Note: deprecated
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::INTELLECT) ); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::INTELLECT));
    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::AGILITY) ); //Note: overriden by chr
    template_put_range( fileTemp, fileWrite, profile->getAttributeGain(Ego::Attribute::AGILITY));

    // More physical attributes
    template_put_float( fileTemp, fileWrite, character->fat_goto );                   //Note: overriden by chr
    template_put_float( fileTemp, fileWrite, profile->_sizeGainPerLevel );
    template_put_int( fileTemp, fileWrite, profile->_shadowSize );
    template_put_int( fileTemp, fileWrite, profile->_bumpSize );
    template_put_int( fileTemp, fileWrite, profile->_bumpHeight );
    template_put_float( fileTemp, fileWrite, character->phys.bumpdampen );           //Note: overriden by chr

    //Weight
    if ( CHR_INFINITE_WEIGHT == character->phys.weight || 0.0f == character->fat )
    {
        template_put_int( fileTemp, fileWrite, CAP_INFINITE_WEIGHT );           //Note: overriden by chr
    }
    else
    {
        uint32_t weight = character->phys.weight / character->fat / character->fat / character->fat;
        template_put_int( fileTemp, fileWrite, std::min(weight, static_cast<uint32_t>(CAP_MAX_WEIGHT)) );   //Note: overriden by chr
    }

    template_put_float( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::JUMP_POWER) );    //Note: overriden by chr
    template_put_int( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); //Note: overriden by chr
    template_put_float( fileTemp, fileWrite, profile->_animationSpeedSneak);
    template_put_float( fileTemp, fileWrite, profile->_animationSpeedWalk);
    template_put_float( fileTemp, fileWrite, profile->_animationSpeedRun);
    template_put_int( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::FLY_TO_HEIGHT) ); //Note: overriden by chr
    template_put_int(fileTemp, fileWrite, profile->_flashAND);
    template_put_int(fileTemp, fileWrite, profile->_alpha);
    template_put_int(fileTemp, fileWrite, profile->_light);
    template_put_bool(fileTemp, fileWrite, profile->_transferBlending);
    template_put_int( fileTemp, fileWrite, profile->_sheen );
    template_put_bool( fileTemp, fileWrite, profile->_phongMapping );
    template_put_float( fileTemp, fileWrite, FFFF_TO_FLOAT( profile->_textureMovementRateX ) );
    template_put_float( fileTemp, fileWrite, FFFF_TO_FLOAT( profile->_textureMovementRateY ) );
    template_put_bool(fileTemp, fileWrite, profile->_stickyButt);

    // Invulnerability data
    template_put_bool( fileTemp, fileWrite, character->invictus );
    template_put_int( fileTemp, fileWrite, profile->nframefacing );
    template_put_int( fileTemp, fileWrite, profile->nframeangle );
    template_put_int( fileTemp, fileWrite, profile->iframefacing );
    template_put_int( fileTemp, fileWrite, profile->iframeangle );

    // Skin defenses (TODO: add support for more than 4)
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(0).defence );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(1).defence );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(2).defence );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(3).defence );

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        //TODO: add support for more than 4
        for(int i = 0; i < 4; ++i) {
            //ZF> Another small hack to prevent 0 damage resist to be parsed as 0 damage shift
            float damageResist = profile->getSkinInfo(i).damageResistance[damagetype];
            template_put_float( fileTemp, fileWrite, damageResist == 0.0f ? 1 : damageResist);
        }
    }

    for (size_t damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
    {
        char code;

        for (size_t skin = 0; skin < SKINS_PEROBJECT_MAX; skin++)
        {
            if ( HAS_SOME_BITS( profile->getSkinInfo(skin).damageModifier[damagetype], DAMAGEMANA ) )
            {
                code = 'M';
            }
            else if ( HAS_SOME_BITS( profile->getSkinInfo(skin).damageModifier[damagetype], DAMAGECHARGE ) )
            {
                code = 'C';
            }
            else if ( HAS_SOME_BITS( profile->getSkinInfo(skin).damageModifier[damagetype], DAMAGEINVERT ) )
            {
                code = 'T';
            }
            else if ( HAS_SOME_BITS( profile->getSkinInfo(skin).damageModifier[damagetype], DAMAGEINVICTUS ) )
            {
                code = 'I';
            }
            else
            {
                code = 'F';
            }

            template_put_char( fileTemp, fileWrite, code );
        }
    }

    template_put_float( fileTemp, fileWrite, profile->getSkinInfo(0).maxAccel*80 );
    template_put_float( fileTemp, fileWrite, profile->getSkinInfo(1).maxAccel*80 );
    template_put_float( fileTemp, fileWrite, profile->getSkinInfo(2).maxAccel*80 );
    template_put_float( fileTemp, fileWrite, profile->getSkinInfo(3).maxAccel*80 );

    // Experience and level data
    template_put_int( fileTemp, fileWrite, profile->_experienceForLevel[1] );
    template_put_int( fileTemp, fileWrite, profile->_experienceForLevel[2] );
    template_put_int( fileTemp, fileWrite, profile->_experienceForLevel[3] );
    template_put_int( fileTemp, fileWrite, profile->_experienceForLevel[4] );
    template_put_int( fileTemp, fileWrite, profile->_experienceForLevel[5] );
    template_put_float( fileTemp, fileWrite, FLOAT_TO_FP8( character->experience ) );    //Note overriden by chr
    template_put_int( fileTemp, fileWrite, profile->_experienceWorth );
    template_put_float( fileTemp, fileWrite, profile->_experienceExchange );
    for(size_t i = 0; i < profile->_experienceRate.size(); ++i) {
        template_put_float( fileTemp, fileWrite, profile->_experienceRate[i] );        
    }

    // IDSZ identification tags
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_PARENT] );
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_TYPE] );
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_SKILL] );
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_SPECIAL] );
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_HATE] );
    template_put_idsz( fileTemp, fileWrite, profile->_idsz[IDSZ_VULNERABILITY] );

    // Item and damage flags
    template_put_bool( fileTemp, fileWrite, character->isitem);  //Note overriden by chr
    template_put_bool( fileTemp, fileWrite, profile->_isMount );
    template_put_bool( fileTemp, fileWrite, profile->_isStackable );
    template_put_bool( fileTemp, fileWrite, character->nameknown || character->ammoknown); // make sure that identified items are saved as identified );
    template_put_bool( fileTemp, fileWrite, profile->_usageIsKnown );
    template_put_bool( fileTemp, fileWrite, profile->_canCarryToNextModule );
    template_put_bool( fileTemp, fileWrite, profile->_needSkillIDToUse );
    template_put_bool( fileTemp, fileWrite, character->platform );       //Note overriden by chr
    template_put_bool(fileTemp, fileWrite, profile->_canGrabMoney);
    template_put_bool(fileTemp, fileWrite, profile->_canOpenStuff);

    // Other item and damage stuff
    template_put_damage_type( fileTemp, fileWrite, character->damagetarget_damagetype ); //Note overriden by chr
    template_put_action( fileTemp, fileWrite, profile->_weaponAction );

    // Particle attachments
    template_put_int( fileTemp, fileWrite, profile->_attachedParticleAmount );
    template_put_damage_type(fileTemp, fileWrite, character->reaffirm_damagetype);
    template_put_local_particle_profile_ref( fileTemp, fileWrite, profile->_attachedParticle );

    // Character hands
    template_put_bool( fileTemp, fileWrite, profile->_slotsValid[SLOT_LEFT] );
    template_put_bool( fileTemp, fileWrite, profile->_slotsValid[SLOT_RIGHT] );

    // Particle spawning on attack
    template_put_bool( fileTemp, fileWrite, 0 != profile->_attachAttackParticleToWeapon );
    template_put_local_particle_profile_ref( fileTemp, fileWrite, profile->_attackParticle );

    // Particle spawning for GoPoof
    template_put_int( fileTemp, fileWrite, profile->_goPoofParticleAmount );
    template_put_int( fileTemp, fileWrite, profile->_goPoofParticleFacingAdd );
    template_put_local_particle_profile_ref(fileTemp, fileWrite, profile->_goPoofParticle);

    // Particle spawning for blud
    template_put_bool( fileTemp, fileWrite, 0 != profile->_bludValid );
    template_put_local_particle_profile_ref( fileTemp, fileWrite, profile->_bludParticle );

    // Extra stuff
    template_put_bool(fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::WALK_ON_WATER) > 0); //Note: overriden by chr
    template_put_float( fileTemp, fileWrite, character->phys.dampen );   //Note: overriden by chr

    // More stuff
    template_put_float(fileTemp, fileWrite, 0); //unused
    template_put_float(fileTemp, fileWrite, profile->_useManaCost);
    template_put_float(fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::LIFE_REGEN) * 256.0f);   //Note: overridden by chr
    template_put_int( fileTemp, fileWrite, character->stoppedby );   //Note: overridden by chr
    template_put_string_under( fileTemp, fileWrite, profile->getSkinInfo(0).name.c_str() );
    template_put_string_under( fileTemp, fileWrite, profile->getSkinInfo(1).name.c_str() );
    template_put_string_under( fileTemp, fileWrite, profile->getSkinInfo(2).name.c_str() );
    template_put_string_under( fileTemp, fileWrite, profile->getSkinInfo(3).name.c_str() );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(0).cost );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(1).cost );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(2).cost );
    template_put_int( fileTemp, fileWrite, profile->getSkinInfo(3).cost );
    template_put_float( fileTemp, fileWrite, 0); //unused

    // Another memory lapse
    template_put_bool( fileTemp, fileWrite, !profile->_riderCanAttack );
    template_put_bool( fileTemp, fileWrite, profile->_canBeDazed );
    template_put_bool( fileTemp, fileWrite, profile->_canBeGrogged );
    template_put_int( fileTemp, fileWrite, 0 );
    template_put_int( fileTemp, fileWrite, 0 );
    template_put_bool( fileTemp, fileWrite, character->getBaseAttribute(Ego::Attribute::SEE_INVISIBLE) > 0 ); //Note: Overridden by chr
    template_put_int( fileTemp, fileWrite, character->iskursed ? 100 : 0 );  //Note: overridden by chr
    template_put_int( fileTemp, fileWrite, profile->_footFallSound);
    template_put_int( fileTemp, fileWrite, profile->_jumpSound);

    vfs_flush( fileWrite );

    // copy the template file to the next free output section
    template_seek_free( fileTemp, fileWrite );

    // Expansions
    for(int i = 0; i < 4; ++i) {
        if (profile->getSkinInfo(i).dressy) {
            vfs_put_expansion(fileWrite, "", IDSZ2( 'D', 'R', 'E', 'S' ), i);
        }
    }

    if ( profile->_resistBumpSpawn )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'T', 'U', 'K' ), 0 );

    if ( profile->_isBigItem )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'P', 'A', 'C', 'K' ), 0 );

    if ( !profile->_hasReflection )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'V', 'A', 'M', 'P' ), 1 );

    if ( profile->_alwaysDraw )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'D', 'R', 'A', 'W' ), 1 );

    if ( profile->_isRanged )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'R', 'A', 'N', 'G' ), 1 );

    if ( profile->_hideState != NOHIDE )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'H', 'I', 'D', 'E' ), profile->_hideState );

    if ( profile->_isEquipment )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'E', 'Q', 'U', 'I' ), 1 );

    if ( profile->_bumpSizeBig >= profile->_bumpSize * 2 )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'Q', 'U', 'A' ), 1 );

    if ( profile->_drawIcon != profile->_usageIsKnown )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'I', 'C', 'O', 'N' ), character->draw_icon ); //note: overridden by chr

    if ( profile->_forceShadow )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'H', 'A', 'D' ), 1 );

    if ( profile->_causesRipples == profile->_isItem )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'R', 'I', 'P', 'P' ), profile->_causesRipples );

    if ( -1 != profile->_isValuable )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'V', 'A', 'L', 'U' ), profile->_isValuable );

    if ( profile->_spellEffectType != NO_SKIN_OVERRIDE )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'B', 'O', 'O', 'K' ), profile->_spellEffectType );

    if ( profile->_attackFast )
        vfs_put_expansion( fileWrite, "", IDSZ2( 'F', 'A', 'S', 'T' ), profile->_attackFast );

    if ( profile->_strengthBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", IDSZ2( 'S', 'T', 'R', 'D' ), profile->_strengthBonus );

    if ( profile->_intelligenceBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", IDSZ2( 'I', 'N', 'T', 'D' ), profile->_intelligenceBonus );

    if ( profile->_dexterityBonus > 0 )
        vfs_put_expansion_float( fileWrite, "", IDSZ2( 'D', 'E', 'X', 'D' ), profile->_dexterityBonus );

    if ( profile->_bumpOverrideSize || profile->_bumpOverrideSizeBig ||  profile->_bumpOverrideHeight )
    {
        STRING sz_tmp = EMPTY_CSTR;

        if ( profile->_bumpOverrideSize ) strcat( sz_tmp, "S" );
        if ( profile->_bumpOverrideSizeBig ) strcat( sz_tmp, "B" );
        if ( profile->_bumpOverrideHeight ) strcat( sz_tmp, "H" );
        if ( profile->_dontCullBackfaces ) strcat( sz_tmp, "C" );
        if ( profile->_skinHasTransparency ) strcat( sz_tmp, "T" );

        if ( CSTR_END != sz_tmp[0] )
        {
            vfs_put_expansion_string( fileWrite, "", IDSZ2( 'M', 'O', 'D', 'L' ), sz_tmp );
        }
    }

    // Basic stuff that is always written
    vfs_put_expansion( fileWrite, "", IDSZ2( 'G', 'O', 'L', 'D' ), character->money );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'P', 'L', 'A', 'T' ), character->canuseplatforms );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'K', 'I', 'N' ), character->skin );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'C', 'O', 'N', 'T' ), character->ai.content );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'T', 'A', 'T' ), character->ai.state );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'L', 'E', 'V', 'L' ), character->experiencelevel );
    vfs_put_expansion( fileWrite, "", IDSZ2( 'S', 'E', 'E', 'D' ), character->getLevelUpSeed() );
    vfs_put_expansion_float( fileWrite, "", IDSZ2( 'L', 'I', 'F', 'E' ), character->getLife() );
    vfs_put_expansion_float( fileWrite, "", IDSZ2( 'M', 'A', 'N', 'A' ), character->getMana() );

    // write down any perks that have been mastered
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i) {
        const Ego::Perks::Perk& perk = Ego::Perks::PerkHandler::get().getPerk(static_cast<Ego::Perks::PerkID>(i));
        if(!character->hasPerk(perk.getID())) continue;
        std::string name = perk.getName();
        std::replace(name.begin(), name.end(), ' ', '_'); //replace space with underscore
        vfs_put_expansion_string(fileWrite, "", IDSZ2( 'P', 'E', 'R', 'K' ), name.c_str() );
    }

    // write down all perks that we can still learn
    for(size_t i = 0; i < Ego::Perks::NR_OF_PERKS; ++i) {
        const Ego::Perks::Perk& perk = Ego::Perks::PerkHandler::get().getPerk(static_cast<Ego::Perks::PerkID>(i));
        if(!profile->_perkPool[i] || character->hasPerk(perk.getID())) continue;
        std::string name = perk.getName();
        std::replace(name.begin(), name.end(), ' ', '_'); //replace space with underscore
        vfs_put_expansion_string(fileWrite, "", IDSZ2( 'P', 'O', 'O', 'L' ), name.c_str() );
    }

    // dump the rest of the template file
    template_flush( fileTemp, fileWrite );

    // The end
    vfs_close( fileWrite );
    template_close_vfs( fileTemp );

    return true;
}

size_t ObjectProfile::getRandomSkinID() const
{
    if(_skinInfo.empty()) {
        return 0;
    }

    auto element = _skinInfo.begin();
    std::advance(element, Random::next(_skinInfo.size()-1));
    return element->first;
}

const FRange& ObjectProfile::getAttributeGain(Ego::Attribute::AttributeType type) const 
{
    EGOBOO_ASSERT(type < _attributeGain.size()); 
    return _attributeGain[type];
}

const FRange& ObjectProfile::getAttributeBase(Ego::Attribute::AttributeType type) const
{
    EGOBOO_ASSERT(type < _baseAttribute.size()); 
    return _baseAttribute[type];    
}

bool ObjectProfile::canLearnPerk(const Ego::Perks::PerkID id) const
{
    if(id == Ego::Perks::NR_OF_PERKS) return false;
    return _perkPool[id];
}

bool ObjectProfile::beginsWithPerk(const Ego::Perks::PerkID id) const
{
    if(id == Ego::Perks::NR_OF_PERKS) return false;
    return _startingPerks[id];
}
