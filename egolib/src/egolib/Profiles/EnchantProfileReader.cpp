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
#include "egolib/Profiles/EnchantProfileReader.hpp"

#include "egolib/file_formats/template.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/_math.h"

bool EnchantProfileReader::read(eve_t *profile, const char *loadName)
{
    char cTmp;
    IDSZ idsz;

    if (!profile) return nullptr;

    eve_t::init(profile);

    ReadContext ctxt(loadName);
    if (!ctxt.ensureOpen()) return nullptr;

    // true/false values
    profile->retarget = vfs_get_next_bool(ctxt);
    profile->override = vfs_get_next_bool(ctxt);
    profile->remove_overridden = vfs_get_next_bool(ctxt);
    profile->killtargetonend = vfs_get_next_bool(ctxt);

    profile->poofonend = vfs_get_next_bool(ctxt);

    // More stuff
    profile->lifetime = vfs_get_next_int(ctxt);
    profile->endmessage = vfs_get_next_int(ctxt);

    // Drain stuff
    profile->owner_mana = vfs_get_next_sfp8(ctxt);
    profile->target_mana = vfs_get_next_sfp8(ctxt);
    profile->endifcantpay = vfs_get_next_bool(ctxt);
    profile->owner_life = vfs_get_next_sfp8(ctxt);
    profile->target_life = vfs_get_next_sfp8(ctxt);

    // Specifics
    profile->required_damagetype = vfs_get_next_damage_type(ctxt);
    profile->require_damagetarget_damagetype = vfs_get_next_damage_type(ctxt);
    profile->removedbyidsz = vfs_get_next_idsz(ctxt);

    // Now the set values
    profile->setyesno[eve_t::SETDAMAGETYPE] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETDAMAGETYPE] = vfs_get_damage_type(ctxt);

    profile->setyesno[eve_t::SETNUMBEROFJUMPS] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETNUMBEROFJUMPS] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETLIFEBARCOLOR] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETLIFEBARCOLOR] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETMANABARCOLOR] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETMANABARCOLOR] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETSLASHMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETSLASHMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDSLASHRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETCRUSHMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETCRUSHMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDCRUSHRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETPOKEMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETPOKEMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDPOKERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETHOLYMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETHOLYMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDHOLYRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETEVILMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETEVILMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDEVILRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETFIREMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETFIREMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDFIRERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETICEMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETICEMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDICERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETZAPMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETZAPMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[eve_t::ADDZAPRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[eve_t::SETFLASHINGAND] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETFLASHINGAND] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETLIGHTBLEND] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETLIGHTBLEND] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETALPHABLEND] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETALPHABLEND] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETSHEEN] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETSHEEN] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETFLYTOHEIGHT] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETFLYTOHEIGHT] = vfs_get_int(ctxt);

    profile->setyesno[eve_t::SETWALKONWATER] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETWALKONWATER] = vfs_get_bool(ctxt);

    profile->setyesno[eve_t::SETCANSEEINVISIBLE] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETCANSEEINVISIBLE] = vfs_get_bool(ctxt);

    profile->setyesno[eve_t::SETMISSILETREATMENT] = vfs_get_next_bool(ctxt);
    cTmp = vfs_get_first_letter(ctxt);
    if ('R' == char_toupper(cTmp)) profile->setvalue[eve_t::SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ('D' == char_toupper(cTmp)) profile->setvalue[eve_t::SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else profile->setvalue[eve_t::SETMISSILETREATMENT] = MISSILE_NORMAL;

    profile->setyesno[eve_t::SETCOSTFOREACHMISSILE] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETCOSTFOREACHMISSILE] = vfs_get_float(ctxt);

    profile->setyesno[eve_t::SETMORPH] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETMORPH] = true;  // vfs_get_bool( fileread );        //ZF> huh? why always channel and morph?

    profile->setyesno[eve_t::SETCHANNEL] = vfs_get_next_bool(ctxt);
    profile->setvalue[eve_t::SETCHANNEL] = true;  // vfs_get_bool( fileread );

    // Now read in the add values
    profile->addvalue[eve_t::ADDJUMPPOWER] = vfs_get_next_float(ctxt);
    profile->addvalue[eve_t::ADDBUMPDAMPEN] = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[eve_t::ADDBOUNCINESS] = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[eve_t::ADDDAMAGE] = vfs_get_next_sfp8(ctxt);            // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDSIZE] = vfs_get_next_float(ctxt);           // Stored as float, used as float
    profile->addvalue[eve_t::ADDACCEL] = vfs_get_next_int(ctxt) / 80.0f;   // Stored as int, used as float
    profile->addvalue[eve_t::ADDRED] = vfs_get_next_int(ctxt);
    profile->addvalue[eve_t::ADDGRN] = vfs_get_next_int(ctxt);
    profile->addvalue[eve_t::ADDBLU] = vfs_get_next_int(ctxt);
    profile->addvalue[eve_t::ADDDEFENSE] = -vfs_get_next_int(ctxt);    // Defense is backwards
    profile->addvalue[eve_t::ADDMANA] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDLIFE] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDSTRENGTH] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDWISDOM] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDINTELLIGENCE] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[eve_t::ADDDEXTERITY] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for (size_t cnt = 0; cnt < eve_t::MAX_ENCHANT_ADD; cnt++)
    {
        profile->addyesno[cnt] = (0.0f != profile->addvalue[cnt]);
    }

    // Read expansions
    while (goto_colon_vfs(NULL, ctxt._file, true))
    {
        idsz = vfs_get_idsz(ctxt);

        if (idsz == MAKE_IDSZ('A', 'M', 'O', 'U')) profile->contspawn_amount = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('T', 'Y', 'P', 'E')) profile->contspawn_lpip = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('T', 'I', 'M', 'E')) profile->contspawn_delay = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('F', 'A', 'C', 'E')) profile->contspawn_facingadd = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('S', 'E', 'N', 'D')) profile->endsound_index = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('S', 'T', 'A', 'Y')) profile->stayifnoowner = (0 != vfs_get_int(ctxt));
        else if (idsz == MAKE_IDSZ('O', 'V', 'E', 'R')) profile->spawn_overlay = (0 != vfs_get_int(ctxt));
        else if (idsz == MAKE_IDSZ('D', 'E', 'A', 'D')) profile->stayiftargetdead = (0 != vfs_get_int(ctxt));

        else if (idsz == MAKE_IDSZ('C', 'K', 'U', 'R')) profile->seekurse = vfs_get_int(ctxt);
        else if (idsz == MAKE_IDSZ('D', 'A', 'R', 'K')) profile->darkvision = vfs_get_int(ctxt);
    }

    strncpy(profile->name, loadName, SDL_arraysize(profile->name));
    profile->loaded = true;

    return true;
}
