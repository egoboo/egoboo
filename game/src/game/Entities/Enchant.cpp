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

/// @file  game/Entities/Enchant.cpp
/// @brief Enchantment entities.

#define GAME_ENTITIES_PRIVATE 1
#include "Enchant.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Core/GameEngine.hpp"

namespace Ego
{

Enchantment::Enchantment(const std::shared_ptr<eve_t> &enchantmentProfile, PRO_REF spawnerProfile) :
    _isTerminated(false),
    _enchantProfileID(INVALID_ENC_REF),
    _enchantProfile(enchantmentProfile),
    _spawnerProfileID(spawnerProfile),

    _lifeTime(enchantmentProfile->lifetime > 0 ? enchantmentProfile->lifetime * GameEngine::GAME_TARGET_UPS : -1),
    _spawnParticlesTimer(0),

    _target(),
    _owner(),
    _spawner(),
    _overlay(),

    _modifiers(),

    _ownerManaSustain(enchantmentProfile->_owner._manaDrain / static_cast<float>(GameEngine::GAME_TARGET_UPS)),
    _ownerLifeSustain(enchantmentProfile->_owner._lifeDrain / static_cast<float>(GameEngine::GAME_TARGET_UPS)),
    _targetManaDrain(enchantmentProfile->_target._manaDrain / static_cast<float>(GameEngine::GAME_TARGET_UPS)),
    _targetLifeDrain(enchantmentProfile->_target._lifeDrain / static_cast<float>(GameEngine::GAME_TARGET_UPS))
{
    bool doMorph = false;

    // count all the requests for this enchantment type
    _enchantProfile->_spawnRequestCount++;

    //Add all modifiers that this enchantment provides
    for(size_t i = 0; i < eve_t::MAX_ENCHANT_SET; ++i) {
        if(!_enchantProfile->_set[i].apply) continue;

        //Apply Ego::Attribute::MORPH last so that it is always added first in list
        if(i == eve_t::SETMORPH) {
            doMorph = true;
            continue;
        }

        //Translate EnchantSetType to AttributeType
        Ego::Attribute::AttributeType type;
        switch(i)
        {
            case eve_t::SETDAMAGETYPE: type = Ego::Attribute::DAMAGE_TYPE; break;
            case eve_t::SETNUMBEROFJUMPS: type = Ego::Attribute::NUMBER_OF_JUMPS; break;
            case eve_t::SETLIFEBARCOLOR: type = Ego::Attribute::LIFE_BARCOLOR; break;
            case eve_t::SETMANABARCOLOR: type = Ego::Attribute::MANA_BARCOLOR; break;
            case eve_t::SETSLASHMODIFIER: type = Ego::Attribute::SLASH_MODIFIER; break;
            case eve_t::SETCRUSHMODIFIER: type = Ego::Attribute::CRUSH_MODIFIER; break;
            case eve_t::SETPOKEMODIFIER: type = Ego::Attribute::POKE_MODIFIER; break;
            case eve_t::SETHOLYMODIFIER: type = Ego::Attribute::HOLY_MODIFIER; break;
            case eve_t::SETEVILMODIFIER: type = Ego::Attribute::EVIL_MODIFIER; break;
            case eve_t::SETFIREMODIFIER: type = Ego::Attribute::FIRE_MODIFIER; break;
            case eve_t::SETICEMODIFIER: type = Ego::Attribute::ICE_MODIFIER; break;
            case eve_t::SETZAPMODIFIER: type = Ego::Attribute::ZAP_MODIFIER; break;
            case eve_t::SETFLASHINGAND: type = Ego::Attribute::FLASHING_AND; break;
            case eve_t::SETLIGHTBLEND: type = Ego::Attribute::LIGHT_BLEND; break;
            case eve_t::SETALPHABLEND: type = Ego::Attribute::ALPHA_BLEND; break;
            case eve_t::SETSHEEN: type = Ego::Attribute::SHEEN; break;
            case eve_t::SETFLYTOHEIGHT: type = Ego::Attribute::FLY_TO_HEIGHT; break;
            case eve_t::SETWALKONWATER: type = Ego::Attribute::WALK_ON_WATER; break;
            case eve_t::SETCANSEEINVISIBLE: type = Ego::Attribute::SEE_INVISIBLE; break;
            case eve_t::SETMISSILETREATMENT: type = Ego::Attribute::MISSILE_TREATMENT; break;
            case eve_t::SETCOSTFOREACHMISSILE: type = Ego::Attribute::COST_FOR_EACH_MISSILE; break;
            case eve_t::SETCHANNEL: type = Ego::Attribute::CHANNEL_LIFE; break;
            default: throw std::logic_error("Unhandled enchant set type");
        }
        _modifiers.push_front(Ego::EnchantModifier(type, _enchantProfile->_add[i].value));
    }
    for(size_t i = 0; i < eve_t::MAX_ENCHANT_ADD; ++i) {
        if(!_enchantProfile->_add[i].apply) continue;

        //Translate EnchantAddType to AttributeType
        Ego::Attribute::AttributeType type;
        switch(i)
        {
            case eve_t::ADDJUMPPOWER: type = Ego::Attribute::JUMP_POWER; break;
            case eve_t::ADDBUMPDAMPEN: type = Ego::Attribute::BUMP_DAMPEN; break;
            case eve_t::ADDBOUNCINESS: type = Ego::Attribute::BOUNCINESS; break;
            case eve_t::ADDDAMAGE: type = Ego::Attribute::DAMAGE_BONUS; break;
            case eve_t::ADDSIZE: type = Ego::Attribute::SIZE; break;
            case eve_t::ADDACCEL: type = Ego::Attribute::ACCELERATION; break;
            case eve_t::ADDRED: type = Ego::Attribute::RED_SHIFT; break;                        
            case eve_t::ADDGRN: type = Ego::Attribute::GREEN_SHIFT; break;                        
            case eve_t::ADDBLU: type = Ego::Attribute::BLUE_SHIFT; break;                        
            case eve_t::ADDDEFENSE: type = Ego::Attribute::DEFENCE; break;                    
            case eve_t::ADDMANA: type = Ego::Attribute::MAX_MANA; break;
            case eve_t::ADDLIFE: type = Ego::Attribute::MAX_LIFE; break;
            case eve_t::ADDSTRENGTH: type = Ego::Attribute::MIGHT; break;
            case eve_t::ADDWISDOM: log_warning("Spawned enchant with deprecated ADDWISDOM\n"); continue;
            case eve_t::ADDINTELLIGENCE: type = Ego::Attribute::INTELLECT; break;
            case eve_t::ADDDEXTERITY: type = Ego::Attribute::AGILITY; break;
            case eve_t::ADDSLASHRESIST: type = Ego::Attribute::SLASH_RESIST; break;
            case eve_t::ADDCRUSHRESIST: type = Ego::Attribute::CRUSH_RESIST; break;
            case eve_t::ADDPOKERESIST: type = Ego::Attribute::POKE_RESIST; break;
            case eve_t::ADDEVILRESIST: type = Ego::Attribute::EVIL_RESIST; break;
            case eve_t::ADDHOLYRESIST: type = Ego::Attribute::HOLY_RESIST; break;
            case eve_t::ADDFIRERESIST: type = Ego::Attribute::FIRE_RESIST; break;
            case eve_t::ADDICERESIST: type = Ego::Attribute::ICE_RESIST; break;
            case eve_t::ADDZAPRESIST: type = Ego::Attribute::ZAP_RESIST; break;
            default: throw std::logic_error("Unhandled enchant add type");
        }
        _modifiers.push_front(Ego::EnchantModifier(type, _enchantProfile->_add[i].value));
    }
    if(_enchantProfile->seeKurses) {
        _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::SENSE_KURSES, 1.0f));        
    }
    if(_enchantProfile->darkvision) {
        _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::DARKVISION, 1.0f));        
    }

    if(doMorph) {
        _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::MORPH, _spawnerProfileID));
    }

}

Enchantment::~Enchantment()
{
    std::shared_ptr<Object> overlay = _overlay.lock();
    if(overlay) {
        overlay->requestTerminate();
    }
}

void Enchantment::requestTerminate()
{
    _isTerminated = true;
}

bool Enchantment::isTerminated() const
{
    return _isTerminated;
}

void Enchantment::update()
{
    if(isTerminated()) return;

    //Have we lost our target?
    std::shared_ptr<Object> target = _target.lock();
    std::shared_ptr<Object> owner = _owner.lock();
    if(target == nullptr || target->isTerminated()) {
        requestTerminate();
        return;
    }

    //End enchant if owner of the Enchant has died?
    if(!_enchantProfile->_owner._stay) {
        if(!owner || !owner->isAlive()) {
            requestTerminate();
            return;
        }
    }

    //End enchant if target of the enchant has died?
    if(!_enchantProfile->_target._stay && !target->isAlive()) {
        requestTerminate();
        return;
    }

    //Spawn particles?
    if(_spawnParticlesTimer > 0) {
        _spawnParticlesTimer--;
        if(_spawnParticlesTimer == 0) {
            _spawnParticlesTimer = _enchantProfile->contspawn._delay;

            FACING_T facing = target->ori.facing_z;
            for (uint8_t i = 0; i < _enchantProfile->contspawn._amount; ++i)
            {
                ParticleHandler::get().spawnLocalParticle(target->getPosition(), facing, _spawnerProfileID, _enchantProfile->contspawn._lpip,
                                                          INVALID_CHR_REF, GRIP_LAST, 
                                                          owner != nullptr ? owner->getTeam().toRef() : Team::TEAM_DAMAGE, 
                                                          owner != nullptr ? owner->getCharacterID() : INVALID_CHR_REF,
                                                          INVALID_PRT_REF, i, INVALID_CHR_REF);

                facing += _enchantProfile->contspawn._facingAdd;
            }
        }
    }

    //Do drains
    if(target->isAlive()) {
        // Change life
        if (0 != _targetLifeDrain) {
            target->life += _targetLifeDrain;
            if (target->getLife() <= 0.0f) {
                target->kill(owner, false);
            }
            else if (target->getLife() > target->getAttribute(Ego::Attribute::MAX_LIFE)) {
                target->life = FLOAT_TO_FP8(target->getAttribute(Ego::Attribute::MAX_LIFE));
            }
        }

        // Change mana
        if (0 != _targetManaDrain) {
            target->costMana(-_targetManaDrain, owner->getCharacterID());
        }

    }

    // Do enchantment sustain cost (owner)
    if (owner && owner->isAlive()) {
        // Change life
        if (0 != _ownerLifeSustain) {
            owner->life += (_ownerLifeSustain / GameEngine::GAME_TARGET_UPS);

            //Drained to death?
            if ( owner->getLife() <= 0.0f ) {
                owner->kill(target, false);
            }
            else if (owner->getLife() > owner->getAttribute(Ego::Attribute::MAX_LIFE)) {
                owner->life = FLOAT_TO_FP8(owner->getAttribute(Ego::Attribute::MAX_LIFE));
            }
        }

        // Cost mana to sustain
        if (0 != _ownerManaSustain) {
            bool manaPaid = owner->costMana(-_ownerManaSustain, target->getCharacterID());
            if (_enchantProfile->endIfCannotPay && !manaPaid) {
                requestTerminate();
            }
        }
    }

    //Decrement the lifetimer
    if(_lifeTime > 0) {
        if(_lifeTime == 0) {
            requestTerminate();
        }        
    }
}

const std::shared_ptr<eve_t>& Enchantment::getProfile() const
{
    return _enchantProfile;
}

void Enchantment::applyEnchantment(std::shared_ptr<Object> target)
{
    //Invalid target?
    if( target->isTerminated() || (!target->isAlive() && !_enchantProfile->_target._stay) ) {
        requestTerminate();
        return;
    }

    //Already added to a target?
    if(_target.lock()) {
        throw std::logic_error("Enchantment::applyEnchantment - Already applied");
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if (_enchantProfile->retarget) {
        // Left, right, or both are valid
        if (target->getRightHandItem()) {
            // Only right hand is valid
            target = target->getRightHandItem();
        }
        else if (target->getLeftHandItem()) {
            // Pick left hand
            target = target->getLeftHandItem();
        }
        else {
            // No weapons to pick, make it fail
            log_debug("applyEnchantment() failed because target has no valid items in hand\n");
            requestTerminate();
            return;
        }
    }

    //Set our target, stored as a weak_ptr
    _target = target;

    // Check damage type, 90% damage resistance is enough to resist the enchant
    if (_enchantProfile->required_damagetype < DAMAGE_COUNT) {
        if (target->getDamageReduction(_enchantProfile->required_damagetype) >= 0.90f) {
            log_debug("spawn_one_enchant() - failed because the target is immune to the enchant.\n");
            requestTerminate();
            return;
        }
    }

    // Check if target has the required damage type we need
    if (_enchantProfile->require_damagetarget_damagetype < DAMAGE_COUNT) {
        if (target->damagetarget_damagetype != _enchantProfile->require_damagetarget_damagetype) {
            log_warning("spawn_one_enchant() - failed because the target not have the right damagetarget_damagetype.\n");
            requestTerminate();
            return;
        }
    }

    //modify enchant duration with damage resistance (bad resistance actually *increases* duration!)
    if ( _lifeTime > 0 && _enchantProfile->required_damagetype < DAMAGE_COUNT && target ) {
        _lifeTime -= std::ceil(target->getDamageReduction(_enchantProfile->required_damagetype) * _enchantProfile->lifetime);
    }

    // Create an overlay character?
    if (_enchantProfile->spawn_overlay)
    {
        std::shared_ptr<Object> overlay = _currentModule->spawnObject(target->getPosition(), _spawnerProfileID, target->team, 0, target->ori.facing_z, NULL, INVALID_CHR_REF );
        if (overlay)
        {
            overlay = overlay;                             //Kill this character on end...
            overlay->ai.target   = target->getCharacterID();
            overlay->is_overlay  = true;
            chr_set_ai_state(overlay.get(), _enchantProfile->spawn_overlay);  // ??? WHY DO THIS ???

            // Start out with ActionMJ...  Object activated
            int action = overlay->getProfile()->getModel()->getAction(ACTION_MJ);
            if ( !ACTION_IS_TYPE( action, D ) )
            {
                chr_start_anim( overlay.get(), action, false, true );
            }

            // Assume it's transparent...
            overlay->setLight(254);
            overlay->setAlpha(128);
        }
    }

    //Loop through all active enchants and see if there are any conflicts
    for(const std::shared_ptr<Ego::Enchantment> &enchant : target->getActiveEnchants())
    {
        //Scan through all set modifiers of that active enchant
        enchant->_modifiers.remove_if([this, &enchant](const EnchantModifier &modifier)
            {
                //Only set types can conflict
                if(Ego::Attribute::isOverrideSetAttribute(modifier._type)) {
                    return false;
                }

                //Check if this enchant conflicts with any of our set values
                bool removeModifier = false;
                _modifiers.remove_if([this, &enchant, &removeModifier](const EnchantModifier &modifier){
                    if(modifier._type == modifier._type) {
                        //Does this enchant override the other one?
                        if(getProfile()->_override) {

                            //Remove Enchants that conflict with this one?
                            if(getProfile()->remove_overridden) {
                                enchant->requestTerminate();
                            }

                            //Remove old value and use new one instead
                            removeModifier = true;
                            return false;
                        }
                        else {
                            removeModifier = false;
                            return true;
                        }
                    }
                    return false;
                });

                return removeModifier;
            });
    }

    //Now apply the values
    for(const EnchantModifier &modifier : _modifiers)
    {
        //These should never occur
        if(modifier._type == Ego::Attribute::NR_OF_PRIMARY_ATTRIBUTES ||
           modifier._type == Ego::Attribute::NR_OF_ATTRIBUTES) 
        {
            throw std::logic_error("Enchant.cpp - Invalid enchant type: meta-type as modifier");
        }

        //Morph is special and handled differently than others
        if(modifier._type == Ego::Attribute::MORPH) {
            change_character(target->getCharacterID(), _spawnerProfileID, 0, ENC_LEAVE_ALL);

            //Store target's original armor
            target->getTempAttributes()[Ego::Attribute::MORPH] = target->skin;
        }

        //Is it a set type?
        else if(Ego::Attribute::isOverrideSetAttribute(modifier._type)) {
            target->getTempAttributes()[modifier._type] = modifier._value;
        }

        //It's a cumulative addition
        else {
            target->getTempAttributes()[modifier._type] += modifier._value;            
        }
    }

    //Insert this enchantment into the Objects list of active enchants
    target->getActiveEnchants().push_front(shared_from_this());    
}

std::shared_ptr<Object> Enchantment::getTarget() const
{
    return _target.lock();   
}

CHR_REF Enchantment::getOwnerID() const
{
    std::shared_ptr<Object> owner = _owner.lock();
    if(!owner || owner->isTerminated()) {
        return INVALID_CHR_REF;
    }
    return owner->getCharacterID();
}

void Enchantment::setBoostValues(float ownerManaSustain, float ownerLifeSustain, float targetManaDrain, float targetLifeDrain)
{
    _ownerManaSustain = ownerManaSustain;
    _ownerLifeSustain = ownerLifeSustain;
    _targetManaDrain = targetManaDrain;
    _targetLifeDrain = targetLifeDrain;
}

} //Ego
