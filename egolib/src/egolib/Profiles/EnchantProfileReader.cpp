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
    profile->setyesno[SETDAMAGETYPE] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETDAMAGETYPE] = vfs_get_damage_type(ctxt);

    profile->setyesno[SETNUMBEROFJUMPS] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETNUMBEROFJUMPS] = vfs_get_int(ctxt);

    profile->setyesno[SETLIFEBARCOLOR] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETLIFEBARCOLOR] = vfs_get_int(ctxt);

    profile->setyesno[SETMANABARCOLOR] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETMANABARCOLOR] = vfs_get_int(ctxt);

    profile->setyesno[SETSLASHMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETSLASHMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDSLASHRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETCRUSHMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETCRUSHMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDCRUSHRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETPOKEMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETPOKEMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDPOKERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETHOLYMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETHOLYMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDHOLYRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETEVILMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETEVILMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDEVILRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETFIREMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETFIREMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDFIRERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETICEMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETICEMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDICERESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETZAPMODIFIER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETZAPMODIFIER] = vfs_get_damage_modifier(ctxt);
    profile->addvalue[ADDZAPRESIST] = vfs_get_damage_resist(ctxt);

    profile->setyesno[SETFLASHINGAND] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETFLASHINGAND] = vfs_get_int(ctxt);

    profile->setyesno[SETLIGHTBLEND] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETLIGHTBLEND] = vfs_get_int(ctxt);

    profile->setyesno[SETALPHABLEND] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETALPHABLEND] = vfs_get_int(ctxt);

    profile->setyesno[SETSHEEN] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETSHEEN] = vfs_get_int(ctxt);

    profile->setyesno[SETFLYTOHEIGHT] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETFLYTOHEIGHT] = vfs_get_int(ctxt);

    profile->setyesno[SETWALKONWATER] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETWALKONWATER] = vfs_get_bool(ctxt);

    profile->setyesno[SETCANSEEINVISIBLE] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETCANSEEINVISIBLE] = vfs_get_bool(ctxt);

    profile->setyesno[SETMISSILETREATMENT] = vfs_get_next_bool(ctxt);
    cTmp = vfs_get_first_letter(ctxt);
    if ('R' == char_toupper(cTmp)) profile->setvalue[SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ('D' == char_toupper(cTmp)) profile->setvalue[SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else profile->setvalue[SETMISSILETREATMENT] = MISSILE_NORMAL;

    profile->setyesno[SETCOSTFOREACHMISSILE] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETCOSTFOREACHMISSILE] = vfs_get_float(ctxt);

    profile->setyesno[SETMORPH] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETMORPH] = true;  // vfs_get_bool( fileread );        //ZF> huh? why always channel and morph?

    profile->setyesno[SETCHANNEL] = vfs_get_next_bool(ctxt);
    profile->setvalue[SETCHANNEL] = true;  // vfs_get_bool( fileread );

    // Now read in the add values
    profile->addvalue[ADDJUMPPOWER] = vfs_get_next_float(ctxt);
    profile->addvalue[ADDBUMPDAMPEN] = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[ADDBOUNCINESS] = vfs_get_next_int(ctxt) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[ADDDAMAGE] = vfs_get_next_sfp8(ctxt);            // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDSIZE] = vfs_get_next_float(ctxt);           // Stored as float, used as float
    profile->addvalue[ADDACCEL] = vfs_get_next_int(ctxt) / 80.0f;   // Stored as int, used as float
    profile->addvalue[ADDRED] = vfs_get_next_int(ctxt);
    profile->addvalue[ADDGRN] = vfs_get_next_int(ctxt);
    profile->addvalue[ADDBLU] = vfs_get_next_int(ctxt);
    profile->addvalue[ADDDEFENSE] = -vfs_get_next_int(ctxt);    // Defense is backwards
    profile->addvalue[ADDMANA] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDLIFE] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDSTRENGTH] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDWISDOM] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDINTELLIGENCE] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDDEXTERITY] = vfs_get_next_sfp8(ctxt);    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for (size_t cnt = 0; cnt < MAX_ENCHANT_ADD; cnt++)
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
