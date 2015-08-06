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

/// @file  game/Entities/Enchant.hpp
/// @brief Enchantment entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "egolib/typedef.h"
#include "egolib/Logic/Attribute.hpp"
#include "egolib/Profiles/_Include.hpp"
#include "game/Entities/_Include.hpp"

//Forward declarations
class Object;

namespace Ego
{

struct EnchantModifier
{
    Ego::Attribute::AttributeType _type;
    float _value;

    EnchantModifier(Ego::Attribute::AttributeType type, float value) :
        _type(type),
        _value(value)
    {
        //ctor
    }
};

/**
 * @brief
 *  The definition of an enchantment entity.
 */
class Enchantment : public std::enable_shared_from_this<Enchantment>
{
public:
    Enchantment(const std::shared_ptr<eve_t> &enchantmentProfile, PRO_REF spawnerProfile, const std::shared_ptr<Object> &owner);

    ~Enchantment();

    void requestTerminate();

    bool isTerminated() const;

    /**
    * @brief
    *   Update one game logic loop tick for this enchant. This will
    *   check if this enchant can kill the owner or target through drains,
    *   spawns any enchant particle effects and checks if the enchantment itself should die.
    **/
    void update();

    const std::shared_ptr<eve_t>& getProfile() const;

    /**
    * @brief
    *   Applies this Enchantment to the specified target. It will stay there and affect the target until
    *   it expires or is removed.
    **/
    void applyEnchantment(std::shared_ptr<Object> target);

    /**
    * @return
    *   The target of this enchant, or nullptr if it no longer has a valid target
    **/
    std::shared_ptr<Object> getTarget() const;

    /**
    * @return
    *   character ID of the Owner of this enchant or INVALID_CHR_REF if there is no valid owner
    **/
    CHR_REF getOwnerID() const;

    /**
    * @return
    *   The owner of this enchant, or nullptr if it no longer has a valid owner
    **/
    std::shared_ptr<Object> getOwner() const;

    float getOwnerManaSustain() const {return _ownerManaSustain;}
    float getOwnerLifeSustain() const {return _ownerLifeSustain;}
    float getTargetManaDrain()  const {return _targetManaDrain;}
    float getTargetLifeDrain()  const {return _targetLifeDrain;}

    void setBoostValues(float ownerManaSustain, float ownerLifeSustain, float targetManaDrain, float targetLifeDrain);

    /**
    * @brief
    *   Plays the ending sound of this enchant
    **/
    void playEndSound() const;

    /**
    * @brief
    *   Returns what kind of deflection handling this enchantment provides.
    * @return
    *   not MISSILE_NORMAL if it provides some special kind of missile protection
    **/
    MissileTreatmentType getMissileTreatment() const;

    /**
    * @return
    *   How much the owner of the enchant must pay in mana for each missile deflected or reflected.
    *   If the owner cannot pay the cost, then the enchantment provides no special missile protection
    **/
    float getMissileTreatmentCost() const;

private:
    bool _isTerminated;

    std::shared_ptr<eve_t> _enchantProfile;

    PRO_REF _spawnerProfileID;        ///< The object  profile index that spawned this enchant

    int _lifeTime;                  ///< Time before end (in game logic frames)
    int _spawnParticlesTimer;       ///< Time before spawning particle effects (in game logic frames)

    std::weak_ptr<Object> _target;  ///< Who it enchants
    std::weak_ptr<Object> _owner;   ///< Who cast the enchant
    std::weak_ptr<Object> _spawner; ///< The spellbook character
    std::weak_ptr<Object> _overlay; ///< The overlay character

    //Missile deflection enchant?
    MissileTreatmentType _missileTreatment;
    float _missileTreatmentCost;

    /// List to remember if properties were subjected to modifications by this enchant
    std::forward_list<EnchantModifier> _modifiers;

    float _ownerManaSustain;               ///< Boost values
    float _ownerLifeSustain;
    float _targetManaDrain;
    float _targetLifeDrain;
};

} //Ego