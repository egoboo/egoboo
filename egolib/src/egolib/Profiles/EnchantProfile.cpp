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

#define EGOLIB_PROFILES_PRIVATE 1
#include "egolib/Profiles/EnchantProfile.hpp"
#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/fileutil.h"

EnchantProfile::EnchantProfile() : AbstractProfile(),

    // Enchant spawn description.
    _override(false),
    remove_overridden(false),
    retarget(false),
    required_damagetype(DamageType::DAMAGE_DIRECT),
    require_damagetarget_damagetype(DamageType::DAMAGE_DIRECT),
    spawn_overlay(false),

    // Enchant despawn conditions.
    lifetime(0),
    endIfCannotPay(false),
    removedByIDSZ(IDSZ2::None),

    _owner(), 
    _target(),

    _set(), 
    _add(), 

    seeKurses(0), 
    darkvision(0),

    contspawn(),

    // What to do when the enchant ends.
    endsound_index(-1),
    killtargetonend(false),
    poofonend(false),
    endmessage(-1),

    _enchantName()
{
    for (size_t i = 0; i < MAX_ENCHANT_SET; ++i)
    {
        this->_set[i].init();
    }
    for (size_t i = 0; i < MAX_ENCHANT_ADD; ++i)
    {
        this->_add[i].init();
    }
    contspawn.reset();
}

EnchantProfile::~EnchantProfile()
{
    //dtor
}

const std::string& EnchantProfile::getEnchantName() const
{
    return _enchantName;
}

void EnchantProfile::setEnchantName(const std::string& name)
{
    _enchantName = name;
}

std::shared_ptr<EnchantProfile> EnchantProfile::readFromFile(const std::string& pathname)
{
    std::shared_ptr<EnchantProfile> profile = std::make_shared<EnchantProfile>();

    ReadContext ctxt(pathname);
    if (!ctxt.ensureOpen())
    {
        return nullptr;
    }

    // true/false values
    profile->retarget = vfs_get_next_bool(ctxt);
    profile->_override = vfs_get_next_bool(ctxt);
    profile->remove_overridden = vfs_get_next_bool(ctxt);
    profile->killtargetonend = vfs_get_next_bool(ctxt);

    profile->poofonend = vfs_get_next_bool(ctxt);

    // More stuff
    profile->lifetime = vfs_get_next_int(ctxt);
    profile->endmessage = vfs_get_next_int(ctxt);

    // Drain stuff
    profile->_owner._manaDrain = vfs_get_next_float(ctxt);
    profile->_target._manaDrain = vfs_get_next_float(ctxt);
    profile->endIfCannotPay = vfs_get_next_bool(ctxt);
    profile->_owner._lifeDrain = vfs_get_next_float(ctxt);
    profile->_target._lifeDrain = vfs_get_next_float(ctxt);

    // Specifics
    profile->required_damagetype = vfs_get_next_damage_type(ctxt);
    profile->require_damagetarget_damagetype = vfs_get_next_damage_type(ctxt);
    profile->removedByIDSZ = vfs_get_next_idsz(ctxt);

    // Now the set values
    profile->_set[EnchantProfile::SETDAMAGETYPE].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETDAMAGETYPE].value = vfs_get_damage_type(ctxt);

    profile->_set[EnchantProfile::SETNUMBEROFJUMPS].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETNUMBEROFJUMPS].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETLIFEBARCOLOR].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETLIFEBARCOLOR].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETMANABARCOLOR].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETMANABARCOLOR].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETSLASHMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETSLASHMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDSLASHRESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETCRUSHMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETCRUSHMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDCRUSHRESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETPOKEMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETPOKEMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDPOKERESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETHOLYMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETHOLYMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDHOLYRESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETEVILMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETEVILMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDEVILRESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETFIREMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETFIREMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDFIRERESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETICEMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETICEMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDICERESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETZAPMODIFIER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETZAPMODIFIER].value = vfs_get_damage_modifier(ctxt);
    profile->_add[EnchantProfile::ADDZAPRESIST].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETFLASHINGAND].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETFLASHINGAND].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETLIGHTBLEND].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETLIGHTBLEND].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETALPHABLEND].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETALPHABLEND].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETSHEEN].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETSHEEN].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETFLYTOHEIGHT].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETFLYTOHEIGHT].value = ctxt.readIntegerLiteral();

    profile->_set[EnchantProfile::SETWALKONWATER].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETWALKONWATER].value = ctxt.readBool();

    profile->_set[EnchantProfile::SETCANSEEINVISIBLE].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETCANSEEINVISIBLE].value = ctxt.readBool();

    Ego::Script::EnumDescriptor<MissileTreatment> enumDescriptor
        (
            "MissileTreatment",
            {
                // Normal.
                {"Normal", MissileTreatment_Normal},
                {"NORMAL", MissileTreatment_Normal},
                {"N", MissileTreatment_Normal},
            // Reflect.
                {"Reflect", MissileTreatment_Reflect},
                {"REFLECT", MissileTreatment_Reflect},
                {"R", MissileTreatment_Reflect},
            // Deflect.
                {"Deflect", MissileTreatment_Deflect},
                {"DEFLECT", MissileTreatment_Deflect},
                {"D", MissileTreatment_Deflect},
            }
    );

    profile->_set[EnchantProfile::SETMISSILETREATMENT].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETMISSILETREATMENT].value = ctxt.readEnum(enumDescriptor);

    profile->_set[EnchantProfile::SETCOSTFOREACHMISSILE].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETCOSTFOREACHMISSILE].value = ctxt.readRealLiteral();

    profile->_set[EnchantProfile::SETMORPH].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETMORPH].value = true;  // vfs_get_bool( fileread );        //ZF> huh? why always channel and morph?

    profile->_set[EnchantProfile::SETCHANNEL].apply = vfs_get_next_bool(ctxt);
    profile->_set[EnchantProfile::SETCHANNEL].value = true;  // vfs_get_bool( fileread );

    // Now read in the add values
    profile->_add[EnchantProfile::ADDJUMPPOWER].value = vfs_get_next_float(ctxt);
    profile->_add[EnchantProfile::ADDBUMPDAMPEN].value = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->_add[EnchantProfile::ADDBOUNCINESS].value = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->_add[EnchantProfile::ADDDAMAGE].value = vfs_get_next_float(ctxt);            // Stored as float, used as 8.8-fixed
    profile->_add[EnchantProfile::ADDSIZE].value = vfs_get_next_float(ctxt);           // Stored as float, used as float
    profile->_add[EnchantProfile::ADDACCEL].value = vfs_get_next_int(ctxt) / 80.0f;   // Stored as int, used as float
    profile->_add[EnchantProfile::ADDRED].value = vfs_get_next_int(ctxt);
    profile->_add[EnchantProfile::ADDGRN].value = vfs_get_next_int(ctxt);
    profile->_add[EnchantProfile::ADDBLU].value = vfs_get_next_int(ctxt);
    profile->_add[EnchantProfile::ADDDEFENSE].value = -vfs_get_next_int(ctxt);    // Defense is backwards
    profile->_add[EnchantProfile::ADDMANA].value = vfs_get_next_float(ctxt);    // Stored as float, used as 8.8-fixed
    profile->_add[EnchantProfile::ADDLIFE].value = vfs_get_next_float(ctxt);    // Stored as float, used as 8.8-fixed
    profile->_add[EnchantProfile::ADDSTRENGTH].value = vfs_get_next_float(ctxt);    // Stored as float, used as 8.8-fixed
    profile->_add[EnchantProfile::ADDWISDOM].value = vfs_get_next_float(ctxt);    // Deprecated (not used)
    profile->_add[EnchantProfile::ADDINTELLIGENCE].value = vfs_get_next_float(ctxt);    // Stored as float, used as 8.8-fixed
    profile->_add[EnchantProfile::ADDDEXTERITY].value = vfs_get_next_float(ctxt);    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for (size_t cnt = 0; cnt < EnchantProfile::MAX_ENCHANT_ADD; cnt++)
    {
        profile->_add[cnt].apply = (0.0f != profile->_add[cnt].value);
    }
    profile->_add[EnchantProfile::ADDFIRERESIST].apply = profile->_set[EnchantProfile::SETFIREMODIFIER].apply;
    profile->_add[EnchantProfile::ADDEVILRESIST].apply = profile->_set[EnchantProfile::SETEVILMODIFIER].apply;
    profile->_add[EnchantProfile::ADDZAPRESIST].apply = profile->_set[EnchantProfile::SETZAPMODIFIER].apply;
    profile->_add[EnchantProfile::ADDICERESIST].apply = profile->_set[EnchantProfile::SETICEMODIFIER].apply;
    profile->_add[EnchantProfile::ADDHOLYRESIST].apply = profile->_set[EnchantProfile::SETHOLYMODIFIER].apply;
    profile->_add[EnchantProfile::ADDPOKERESIST].apply = profile->_set[EnchantProfile::SETPOKEMODIFIER].apply;
    profile->_add[EnchantProfile::ADDSLASHRESIST].apply = profile->_set[EnchantProfile::SETSLASHMODIFIER].apply;
    profile->_add[EnchantProfile::ADDCRUSHRESIST].apply = profile->_set[EnchantProfile::SETCRUSHMODIFIER].apply;

    // Read expansions
    while (ctxt.skipToColon(true))
    {
        switch(ctxt.readIDSZ().toUint32())
        {
            case IDSZ2::caseLabel('A', 'M', 'O', 'U'): profile->contspawn._amount = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('T', 'Y', 'P', 'E'): profile->contspawn._lpip = vfs_get_local_particle_profile_ref(ctxt); break;
            case IDSZ2::caseLabel('T', 'I', 'M', 'E'): profile->contspawn._delay = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('F', 'A', 'C', 'E'): profile->contspawn._facingAdd = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('S', 'E', 'N', 'D'): profile->endsound_index = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('S', 'T', 'A', 'Y'): profile->_owner._stay = (0 != ctxt.readIntegerLiteral()); break;
            case IDSZ2::caseLabel('O', 'V', 'E', 'R'): profile->spawn_overlay = (0 != ctxt.readIntegerLiteral()); break;
            case IDSZ2::caseLabel('D', 'E', 'A', 'D'): profile->_target._stay = (0 != ctxt.readIntegerLiteral()); break;
            case IDSZ2::caseLabel('C', 'K', 'U', 'R'): profile->seeKurses = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('D', 'A', 'R', 'K'): profile->darkvision = ctxt.readIntegerLiteral(); break;
            case IDSZ2::caseLabel('N', 'A', 'M', 'E'): profile->setEnchantName(ctxt.readName()); break;
            default: /*TODO: log error*/ break;
        }
    }

    profile->_name = pathname;

    // Limit the endsound_index.
    profile->endsound_index = Ego::Math::constrain<Sint16>(profile->endsound_index, INVALID_SOUND_ID, MAX_WAVE);

    return profile;
}
