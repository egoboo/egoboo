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
#include "egolib/Entities/Enchant.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Core/GameEngine.hpp"

namespace Ego
{

Enchantment::Enchantment(const std::shared_ptr<EnchantProfile> &enchantmentProfile, ObjectProfileRef spawnerProfile, const std::shared_ptr<Object> &owner) :
    _isTerminated(false),
    _enchantProfile(enchantmentProfile),
    _spawnerProfileID(spawnerProfile),

    _lifeTime(enchantmentProfile->lifetime > 0 ? enchantmentProfile->lifetime * GameEngine::GAME_TARGET_UPS : -1),
    _spawnParticlesTimer(0),

    _target(),
    _owner(owner),
    _spawner(),
    _overlay(),

    _modifiers(),
    _missileTreatment(MissileTreatment_Normal),
    _missileTreatmentCost(0.0f),

    _ownerManaSustain(enchantmentProfile->_owner._manaDrain),
    _ownerLifeSustain(enchantmentProfile->_owner._lifeDrain),
    _targetManaDrain(enchantmentProfile->_target._manaDrain),
    _targetLifeDrain(enchantmentProfile->_target._lifeDrain)
{
    bool doMorph = false;

    // count all the requests for this enchantment type
    _enchantProfile->_spawnRequestCount++;

    //Add all modifiers that this enchantment provides
    for(size_t i = 0; i < EnchantProfile::MAX_ENCHANT_SET; ++i) {
        if(!_enchantProfile->_set[i].apply) continue;

        //Apply Ego::Attribute::MORPH last so that it is always added first in list
        if(i == EnchantProfile::SETMORPH) {
            doMorph = true;
            continue;
        }

        //Translate EnchantSetType to AttributeType
        Ego::Attribute::AttributeType type;
        switch(i)
        {
            case EnchantProfile::SETDAMAGETYPE: type = Ego::Attribute::DAMAGE_TYPE; break;
            case EnchantProfile::SETNUMBEROFJUMPS: type = Ego::Attribute::NUMBER_OF_JUMPS; break;
            case EnchantProfile::SETLIFEBARCOLOR: type = Ego::Attribute::LIFE_BARCOLOR; break;
            case EnchantProfile::SETMANABARCOLOR: type = Ego::Attribute::MANA_BARCOLOR; break;
            case EnchantProfile::SETSLASHMODIFIER: type = Ego::Attribute::SLASH_MODIFIER; break;
            case EnchantProfile::SETCRUSHMODIFIER: type = Ego::Attribute::CRUSH_MODIFIER; break;
            case EnchantProfile::SETPOKEMODIFIER: type = Ego::Attribute::POKE_MODIFIER; break;
            case EnchantProfile::SETHOLYMODIFIER: type = Ego::Attribute::HOLY_MODIFIER; break;
            case EnchantProfile::SETEVILMODIFIER: type = Ego::Attribute::EVIL_MODIFIER; break;
            case EnchantProfile::SETFIREMODIFIER: type = Ego::Attribute::FIRE_MODIFIER; break;
            case EnchantProfile::SETICEMODIFIER: type = Ego::Attribute::ICE_MODIFIER; break;
            case EnchantProfile::SETZAPMODIFIER: type = Ego::Attribute::ZAP_MODIFIER; break;
            case EnchantProfile::SETFLASHINGAND: type = Ego::Attribute::FLASHING_AND; break;
            case EnchantProfile::SETLIGHTBLEND: type = Ego::Attribute::LIGHT_BLEND; break;
            case EnchantProfile::SETALPHABLEND: type = Ego::Attribute::ALPHA_BLEND; break;
            case EnchantProfile::SETSHEEN: type = Ego::Attribute::SHEEN; break;
            case EnchantProfile::SETFLYTOHEIGHT: type = Ego::Attribute::FLY_TO_HEIGHT; break;
            case EnchantProfile::SETWALKONWATER: type = Ego::Attribute::WALK_ON_WATER; break;
            case EnchantProfile::SETCANSEEINVISIBLE: type = Ego::Attribute::SEE_INVISIBLE; break;
            case EnchantProfile::SETCHANNEL: type = Ego::Attribute::CHANNEL_LIFE; break;

            //These are not object attributes but enchant attributes
            case EnchantProfile::SETMISSILETREATMENT:    _missileTreatment = (MissileTreatment)(long)(_enchantProfile->_set[i].value); continue;
            case EnchantProfile::SETCOSTFOREACHMISSILE:  _missileTreatmentCost = _enchantProfile->_set[i].value; continue;

            default: throw std::logic_error("Unhandled enchant set type");
        }

        _modifiers.push_front(Ego::EnchantModifier(type, _enchantProfile->_set[i].value));
    }
    for(size_t i = 0; i < EnchantProfile::MAX_ENCHANT_ADD; ++i) {
        if(!_enchantProfile->_add[i].apply) continue;

        //Translate EnchantAddType to AttributeType
        Ego::Attribute::AttributeType type;
        switch(i)
        {
            case EnchantProfile::ADDJUMPPOWER: type = Ego::Attribute::JUMP_POWER; break;
            case EnchantProfile::ADDBUMPDAMPEN: type = Ego::Attribute::BUMP_DAMPEN; break;
            case EnchantProfile::ADDBOUNCINESS: type = Ego::Attribute::BOUNCINESS; break;
            case EnchantProfile::ADDDAMAGE: type = Ego::Attribute::DAMAGE_BONUS; break;
            case EnchantProfile::ADDSIZE: type = Ego::Attribute::SIZE; break;
            case EnchantProfile::ADDACCEL: type = Ego::Attribute::ACCELERATION; break;
            case EnchantProfile::ADDRED: type = Ego::Attribute::RED_SHIFT; break;                        
            case EnchantProfile::ADDGRN: type = Ego::Attribute::GREEN_SHIFT; break;                        
            case EnchantProfile::ADDBLU: type = Ego::Attribute::BLUE_SHIFT; break;                        
            case EnchantProfile::ADDDEFENSE: type = Ego::Attribute::DEFENCE; break;                    
            case EnchantProfile::ADDMANA: type = Ego::Attribute::MAX_MANA; break;
            case EnchantProfile::ADDLIFE: type = Ego::Attribute::MAX_LIFE; break;
            case EnchantProfile::ADDSTRENGTH: type = Ego::Attribute::MIGHT; break;
            case EnchantProfile::ADDWISDOM: Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "spawned enchant with deprecated ADDWISDOM", Log::EndOfEntry); continue;
            case EnchantProfile::ADDINTELLIGENCE: type = Ego::Attribute::INTELLECT; break;
            case EnchantProfile::ADDDEXTERITY: type = Ego::Attribute::AGILITY; break;
            case EnchantProfile::ADDSLASHRESIST: type = Ego::Attribute::SLASH_RESIST; break;
            case EnchantProfile::ADDCRUSHRESIST: type = Ego::Attribute::CRUSH_RESIST; break;
            case EnchantProfile::ADDPOKERESIST: type = Ego::Attribute::POKE_RESIST; break;
            case EnchantProfile::ADDEVILRESIST: type = Ego::Attribute::EVIL_RESIST; break;
            case EnchantProfile::ADDHOLYRESIST: type = Ego::Attribute::HOLY_RESIST; break;
            case EnchantProfile::ADDFIRERESIST: type = Ego::Attribute::FIRE_RESIST; break;
            case EnchantProfile::ADDICERESIST: type = Ego::Attribute::ICE_RESIST; break;
            case EnchantProfile::ADDZAPRESIST: type = Ego::Attribute::ZAP_RESIST; break;
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

    _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::LIFE_REGEN, enchantmentProfile->_target._lifeDrain));
    _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::MANA_REGEN, enchantmentProfile->_target._manaDrain));

    if(doMorph) {
        _modifiers.push_front(Ego::EnchantModifier(Ego::Attribute::MORPH, _spawnerProfileID.get()));
    }

}

Enchantment::~Enchantment()
{
    std::shared_ptr<Object> overlay = _overlay.lock();
    if(overlay) {
        overlay->requestTerminate();
    }

    //Remove enchantment modifiers from target
    std::shared_ptr<Object> target = _target.lock();
    if(target != nullptr && !target->isTerminated()) {
        for(const EnchantModifier &modifier : _modifiers)
        {
            if(modifier._type == Ego::Attribute::MORPH) {
                //change back into original form
                target->polymorphObject(target->basemodel_ref, modifier._value);
            }
            else if(Ego::Attribute::isOverrideSetAttribute(modifier._type)) {
                //remove effect completely
                target->getTempAttributes().erase(modifier._type);
            }
            else {
                //remove cumulative bonus/penality
                target->getTempAttributes()[modifier._type] -= modifier._value;
            }
        }
    }

    //Remove boost effects from owner
    std::shared_ptr<Object> owner = _owner.lock();
    if(owner != nullptr && !owner->isTerminated()) {
        owner->getTempAttributes()[Ego::Attribute::MANA_REGEN] -= _ownerManaSustain;
        owner->getTempAttributes()[Ego::Attribute::LIFE_REGEN] -= _ownerLifeSustain;
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

            Facing facing = target->ori.facing_z;
            for (uint8_t i = 0; i < _enchantProfile->contspawn._amount; ++i)
            {
                ParticleHandler::get().spawnLocalParticle(target->getPosition(), facing, ObjectProfileRef(_spawnerProfileID), _enchantProfile->contspawn._lpip,
                                                          ObjectRef::Invalid, GRIP_LAST, 
                                                          owner != nullptr ? owner->getTeam().toRef() : static_cast<TEAM_REF>(Team::TEAM_DAMAGE), 
                                                          owner != nullptr ? owner->getObjRef() : ObjectRef::Invalid,
                                                          ParticleRef::Invalid, i, ObjectRef::Invalid);

                facing += Facing(_enchantProfile->contspawn._facingAdd);
            }
        }
    }

    //Can we kill the target by draining life?
    if(target->isAlive()) {
        if (target->getLife() + _targetLifeDrain < 0.0f) {
            target->kill(owner, false);
        }
    }

    //Can the owner still sustain us?
    if (owner && owner->isAlive()) {

        //Killed by draining life?
        if(owner->getLife() + _ownerLifeSustain < 0.0f) {
            owner->kill(target, false);

            if(_enchantProfile->endIfCannotPay || !_enchantProfile->_target._stay) {
                requestTerminate();
                return;
            }
        }

        //Owner has no longer enough mana to sustain the enchant?
        if(_enchantProfile->endIfCannotPay) {
            if(owner->getMana() + _ownerManaSustain < 0.0f) {
                requestTerminate();
                return;
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

const std::shared_ptr<EnchantProfile>& Enchantment::getProfile() const
{
    return _enchantProfile;
}

void Enchantment::applyEnchantment(std::shared_ptr<Object> target)
{
    //Invalid target?
    if( target->isTerminated() || (!target->isAlive() && !_enchantProfile->_target._stay) ) {
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to apply enchant: invalid target", Log::EndOfEntry);
        requestTerminate();
        return;
    }

    //Already added to a target?
    if(_target.lock()) {
        auto e = Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to apply enchant: target locked", Log::EndOfEntry);
        Log::get() << e;
        throw std::logic_error(e.getText());
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
			Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "unable to apply enachant: target has no valid items in hand", Log::EndOfEntry);
            requestTerminate();
            return;
        }
    }

    //Set our target, stored as a weak_ptr
    _target = target;

    // Check damage type, 90% damage resistance is enough to resist the enchant
    if (_enchantProfile->required_damagetype < DAMAGE_COUNT) {
        if (target->getDamageReduction(_enchantProfile->required_damagetype) >= 0.90f) {
			Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "unable to apply enchant: target is immune to enchant", Log::EndOfEntry);
            requestTerminate();
            return;
        }
    }

    // Check if target has the required damage type we need
    if (_enchantProfile->require_damagetarget_damagetype < DAMAGE_COUNT) {
        if (target->damagetarget_damagetype != _enchantProfile->require_damagetarget_damagetype) {
			Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "unable to apply enchant: target has wrong damage type", Log::EndOfEntry);
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
        std::shared_ptr<Object> overlay = _currentModule->spawnObject(target->getPosition(), _spawnerProfileID, target->team, 0, target->ori.facing_z, "", ObjectRef::Invalid );
        if (overlay)
        {
            _overlay = overlay;                             //Kill this character on end...
            overlay->ai.setTarget(target->getObjRef());
            overlay->is_overlay  = true;
            overlay->ai.state = _enchantProfile->spawn_overlay; // ??? WHY DO THIS ???

            // Start out with ActionMJ...  Object activated
            ModelAction action = overlay->getProfile()->getModel()->getAction(ACTION_MJ);
            if (action >= ACTION_DD)
            {
                overlay->inst.startAnimation(action, false, true);
            }

            // Assume it's transparent...
            overlay->setLight(254);
            overlay->setAlpha(128);
        }
    }

    //Check if this enchant has any set modifiers that conflicts with another enchant
    _modifiers.remove_if([this, &target](const EnchantModifier &modifier) {

        //Only set types can conflict
        if(!Ego::Attribute::isOverrideSetAttribute(modifier._type)) {
            return false;
        }

        //Is there no conflict?
        if(target->getTempAttributes().find(modifier._type) == target->getTempAttributes().end()) {
            return false;
        }

        //Ok there exist a conflict, so now we have to resolve it somehow
        //Does this enchant override other enchants?
        if(getProfile()->_override) {
            bool conflictResolved = false;

            //Find the active enchant that conflicts with us
            for(const std::shared_ptr<Ego::Enchantment> &conflictingEnchant : target->getActiveEnchants()) {
                conflictingEnchant->_modifiers.remove_if([this, &conflictingEnchant, &modifier, &conflictResolved](const EnchantModifier &otherModifier)
                    {
                        //Is this the one?
                        if(modifier._type == otherModifier._type) {
                            conflictResolved = true;

                            //Remove Enchants that conflict with this one?
                            if(getProfile()->remove_overridden) {
                                conflictingEnchant->requestTerminate();
                            }

                            return true;
                        }

                        //Nope, keep looking
                        return false;
                    });

                //Has it been resolved?
                if(conflictResolved) {
                    break;
                }
            }

            //We have higher priority than exiting enchants
            return false;
        }
        else {
            //The existing enchant has higher priority than ours
            return true;
        }
    });

    //Now actually apply the values to the target
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
            //Store target's original armor
            target->getTempAttributes()[Ego::Attribute::MORPH] = target->skin;

            //Transform the object
            target->polymorphObject(ObjectProfileRef(_spawnerProfileID), 0);
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

    //Finally apply boost values to owner as well
    std::shared_ptr<Object> owner = _owner.lock();
    if(owner != nullptr && !owner->isTerminated()) {
        owner->getTempAttributes()[Ego::Attribute::MANA_REGEN] += _ownerManaSustain;
        owner->getTempAttributes()[Ego::Attribute::LIFE_REGEN] += _ownerLifeSustain;
    }

    //Insert this enchantment into the Objects list of active enchants
    target->getActiveEnchants().push_front(shared_from_this());    
}

std::shared_ptr<Object> Enchantment::getTarget() const
{
    return _target.lock();   
}

std::shared_ptr<Object> Enchantment::getOwner() const
{
    return _owner.lock();
}

ObjectRef Enchantment::getOwnerRef() const
{
    std::shared_ptr<Object> owner = _owner.lock();
    if(!owner || owner->isTerminated()) {
        return ObjectRef::Invalid;
    }
    return owner->getObjRef();
}

void Enchantment::setBoostValues(float ownerManaSustain, float ownerLifeSustain, float targetManaDrain, float targetLifeDrain)
{
    //Update boost effects to owner
    std::shared_ptr<Object> owner = _owner.lock();
    if(owner && !owner->isTerminated()) {
        owner->getTempAttributes()[Ego::Attribute::MANA_REGEN] -= _ownerManaSustain;
        owner->getTempAttributes()[Ego::Attribute::LIFE_REGEN] -= _ownerLifeSustain;
        owner->getTempAttributes()[Ego::Attribute::MANA_REGEN] += ownerManaSustain;
        owner->getTempAttributes()[Ego::Attribute::LIFE_REGEN] += ownerLifeSustain;
    }
    _ownerManaSustain = ownerManaSustain;
    _ownerLifeSustain = ownerLifeSustain;

    //Update boost effects to target
    std::shared_ptr<Object> target = _target.lock();  
    if(target != nullptr) {
        for(EnchantModifier &modifier : _modifiers) {
            if(modifier._type == Ego::Attribute::MANA_REGEN) {
                target->getTempAttributes()[Ego::Attribute::MANA_REGEN] -= modifier._value;
                modifier._value = -targetManaDrain;
                target->getTempAttributes()[Ego::Attribute::MANA_REGEN] += modifier._value;
            }
            else if(modifier._type == Ego::Attribute::LIFE_REGEN) {
                target->getTempAttributes()[Ego::Attribute::LIFE_REGEN] -= modifier._value;
                modifier._value = -targetLifeDrain;
                target->getTempAttributes()[Ego::Attribute::LIFE_REGEN] += modifier._value;            
            }
        }        
    }  
    _targetManaDrain = targetManaDrain;
    _targetLifeDrain = targetLifeDrain;
}

void Enchantment::playEndSound() const
{
    std::shared_ptr<Object> target = _target.lock();
    if(target) {
        const std::shared_ptr<ObjectProfile> &spawnerProfile = ProfileSystem::get().getProfile(_spawnerProfileID);
        AudioSystem::get().playSound(target->getPosition(), spawnerProfile->getSoundID(getProfile()->endsound_index));
    }
}

MissileTreatment Enchantment::getMissileTreatment() const
{
    return _missileTreatment;
}

float Enchantment::getMissileTreatmentCost() const
{
    return _missileTreatmentCost;
}

const std::forward_list<EnchantModifier>& Enchantment::getModifiers() const
{
    return _modifiers;
}

} //Ego
