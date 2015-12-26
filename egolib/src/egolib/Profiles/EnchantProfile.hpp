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

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/AbstractProfile.hpp"
#undef EGBOLIB_PROFILES_PRIVATE

#include "egolib/IDSZ.hpp"
#include "egolib/Logic/MissileTreatment.hpp"

/**
* @brief
*  An enchantment profile, or "eve"
* @details
*  An internal representation of the "enchant.txt" file.
*/
class EnchantProfile : public AbstractProfile
{
public:

    /**
     * @brief
     *  A list of all the properties to which an enchant (of this enchant profile) can apply the "set" modifier to.
     */
    enum SetModifierTargets
    {
        SETMORPH = 0,           ///< Morph character?
                                ///< @details This must be first since the character must
                                ///< be morphed before adding any of the other enchants.

        SETDAMAGETYPE,          ///< Type of damage dealt
        SETNUMBEROFJUMPS,       ///< Max number of jumps
        SETLIFEBARCOLOR,        ///< Color of life bar
        SETMANABARCOLOR,        ///< Color of mana bar
        SETSLASHMODIFIER,       ///< Damage modifiers
        SETCRUSHMODIFIER,
        SETPOKEMODIFIER,
        SETHOLYMODIFIER,
        SETEVILMODIFIER,
        SETFIREMODIFIER,
        SETICEMODIFIER,
        SETZAPMODIFIER,
        SETFLASHINGAND,             ///< Flash rate
        SETLIGHTBLEND,              ///< Transparency
        SETALPHABLEND,              ///< Alpha
        SETSHEEN,                   ///< Shinyness
        SETFLYTOHEIGHT,             ///< Fly to this height
        SETWALKONWATER,             ///< Walk on water?
        SETCANSEEINVISIBLE,         ///< Can it see invisible?
        SETMISSILETREATMENT,        ///< How to treat missiles
        SETCOSTFOREACHMISSILE,      ///< Cost for each missile treat
        SETCHANNEL,                 ///< Can channel life as mana?
        MAX_ENCHANT_SET,

        ENC_SET_FIRST = SETMORPH,
        ENC_SET_LAST = SETCHANNEL

    };

    /**
     * @brief
     *  A list of all the properties to which an enchant (of this enchant profile) can apply the "add" modifier to.
     */
    enum AddModifierTargets
    {
        ADDJUMPPOWER = 0,
        ADDBUMPDAMPEN,
        ADDBOUNCINESS,
        ADDDAMAGE,
        ADDSIZE,
        ADDACCEL,

        ADDRED,                        ///< Red shift
        ADDGRN,                        ///< Green shift
        ADDBLU,                        ///< Blue shift

        ADDDEFENSE,                    ///< Defence adjustments

        ADDMANA,
        ADDLIFE,

        ADDSTRENGTH,
        ADDWISDOM,
        ADDINTELLIGENCE,
        ADDDEXTERITY,

        ADDSLASHRESIST,
        ADDCRUSHRESIST,
        ADDPOKERESIST,
        ADDEVILRESIST,
        ADDHOLYRESIST,
        ADDFIRERESIST,
        ADDICERESIST,
        ADDZAPRESIST,

        MAX_ENCHANT_ADD,

        //these are only for parsing the enchant file
        ENC_ADD_FIRST = ADDJUMPPOWER,
        ENC_ADD_LAST = ADDZAPRESIST
    };

    /// An enchant maintains relations to its "owner" object (if any) and its "target" object (if any).
    /// This structure describes some aspects of this relation.
    struct ObjectRelation
    {

        bool _stay;         ///< Stay even the related object has died?
        float _manaDrain;  ///< Mana drain from related object?
        float _lifeDrain;  ///< Life drain from related object?

        ObjectRelation() :
            _stay(false), 
            _manaDrain(0), 
            _lifeDrain(0)
        {}

        ObjectRelation(bool stay, SFP8_T manaDrain, SFP8_T lifeDrain) :
            _stay(stay), _manaDrain(manaDrain), _lifeDrain(lifeDrain)
        {}
    };

    /**
     * @brief
     *  Construct this enchant profile with default values.
     */
    EnchantProfile();

    /**
     * @brief
     *  Destruct this enchant profile.
     */
    virtual ~EnchantProfile();

    const std::string& getEnchantName() const;
    void setEnchantName(const std::string& name);

    static std::shared_ptr<EnchantProfile> readFromFile(const std::string& pathname);

public:
    // Enchant spawn description.
    bool _override;                         ///< Override other enchants?
    bool remove_overridden;                ///< Remove other enchants?
    bool retarget;                         ///< Pick a weapon?
    DamageType required_damagetype;             ///< Don't enchant if the target is immune to required_damagetype
    DamageType require_damagetarget_damagetype; ///< Only enchant the target if the target damagetarget_damagetype matches this value
    bool  spawn_overlay;                   ///< Spawn an overlay?

    // Enchant despawn conditions.
    int lifetime;                          ///< Time until end in seconds (-1 for infinite).
    bool endIfCannotPay;                   ///< End on out of mana
    IDSZ2 removedByIDSZ;                   ///< By particle or [NONE]

    // Relation of an enchant (of this profile) to the owner. 
    ObjectRelation _owner;

    // Relation of an enchant (of this profile) to the target.
    ObjectRelation _target;

    /// If an enchant (of this enchant profile) applies a "set" or "add" modifier to a property
    /// & the value to set the property to. Related to Enchant::Modification.
    struct Modifier
    {
        bool apply;   /// Does the modifier apply?
        float value;  /// The value to be assigned/added to the property.

        Modifier() :
            apply(false),
            value(0.0f)
        {}

        /// @todo Rename to reset.
        void init()
        {
            apply = false;
            value = 0.0f;
        }
    };

    // The "set" modifiers of this enchant.
    std::array<Modifier, MAX_ENCHANT_SET> _set;

    // The "add" modifiers of this enchant.
    std::array<Modifier, MAX_ENCHANT_ADD> _add;

    // special modifications
    int seeKurses;                       ///< Allows target to see kurses.
    int darkvision;                      ///< Allows target to see in darkness.

    // What/how to spawn continuously.
    ContinuousSpawnDescriptor contspawn;

    // What to do when the enchant ends.
    int endsound_index;                  ///< Sound on end (-1 for none)
    bool killtargetonend;                ///< Kill the target on end?
    bool poofonend;                      ///< Spawn a poof on end?
    int endmessage;                      ///< Message on end (-1 for none)

private:
    std::string _enchantName;
};
