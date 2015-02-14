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
    vfs_FILE* fileread;
    char cTmp;
    IDSZ idsz;

    if (!profile) return nullptr;

    eve_t::init(profile);

    fileread = vfs_openRead(loadName);
    if (!fileread) return nullptr;

    // true/false values
    profile->retarget = vfs_get_next_bool(fileread);
    profile->override = vfs_get_next_bool(fileread);
    profile->remove_overridden = vfs_get_next_bool(fileread);
    profile->killtargetonend = vfs_get_next_bool(fileread);

    profile->poofonend = vfs_get_next_bool(fileread);

    // More stuff
    profile->lifetime = vfs_get_next_int(fileread);
    profile->endmessage = vfs_get_next_int(fileread);

    // Drain stuff
    profile->owner_mana = vfs_get_next_sfp8(fileread);
    profile->target_mana = vfs_get_next_sfp8(fileread);
    profile->endifcantpay = vfs_get_next_bool(fileread);
    profile->owner_life = vfs_get_next_sfp8(fileread);
    profile->target_life = vfs_get_next_sfp8(fileread);

    // Specifics
    profile->required_damagetype = vfs_get_next_damage_type(fileread);
    profile->require_damagetarget_damagetype = vfs_get_next_damage_type(fileread);
    profile->removedbyidsz = vfs_get_next_idsz(fileread);

    // Now the set values
    profile->setyesno[SETDAMAGETYPE] = vfs_get_next_bool(fileread);
    profile->setvalue[SETDAMAGETYPE] = vfs_get_damage_type(fileread);

    profile->setyesno[SETNUMBEROFJUMPS] = vfs_get_next_bool(fileread);
    profile->setvalue[SETNUMBEROFJUMPS] = vfs_get_int(fileread);

    profile->setyesno[SETLIFEBARCOLOR] = vfs_get_next_bool(fileread);
    profile->setvalue[SETLIFEBARCOLOR] = vfs_get_int(fileread);

    profile->setyesno[SETMANABARCOLOR] = vfs_get_next_bool(fileread);
    profile->setvalue[SETMANABARCOLOR] = vfs_get_int(fileread);

    profile->setyesno[SETSLASHMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETSLASHMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDSLASHRESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETCRUSHMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETCRUSHMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDCRUSHRESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETPOKEMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETPOKEMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDPOKERESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETHOLYMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETHOLYMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDHOLYRESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETEVILMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETEVILMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDEVILRESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETFIREMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETFIREMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDFIRERESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETICEMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETICEMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDICERESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETZAPMODIFIER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETZAPMODIFIER] = vfs_get_damage_modifier(fileread);
    profile->addvalue[ADDZAPRESIST] = vfs_get_damage_resist(fileread);

    profile->setyesno[SETFLASHINGAND] = vfs_get_next_bool(fileread);
    profile->setvalue[SETFLASHINGAND] = vfs_get_int(fileread);

    profile->setyesno[SETLIGHTBLEND] = vfs_get_next_bool(fileread);
    profile->setvalue[SETLIGHTBLEND] = vfs_get_int(fileread);

    profile->setyesno[SETALPHABLEND] = vfs_get_next_bool(fileread);
    profile->setvalue[SETALPHABLEND] = vfs_get_int(fileread);

    profile->setyesno[SETSHEEN] = vfs_get_next_bool(fileread);
    profile->setvalue[SETSHEEN] = vfs_get_int(fileread);

    profile->setyesno[SETFLYTOHEIGHT] = vfs_get_next_bool(fileread);
    profile->setvalue[SETFLYTOHEIGHT] = vfs_get_int(fileread);

    profile->setyesno[SETWALKONWATER] = vfs_get_next_bool(fileread);
    profile->setvalue[SETWALKONWATER] = vfs_get_bool(fileread);

    profile->setyesno[SETCANSEEINVISIBLE] = vfs_get_next_bool(fileread);
    profile->setvalue[SETCANSEEINVISIBLE] = vfs_get_bool(fileread);

    profile->setyesno[SETMISSILETREATMENT] = vfs_get_next_bool(fileread);
    cTmp = vfs_get_first_letter(fileread);
    if ('R' == char_toupper((unsigned)cTmp)) profile->setvalue[SETMISSILETREATMENT] = MISSILE_REFLECT;
    else if ('D' == char_toupper((unsigned)cTmp)) profile->setvalue[SETMISSILETREATMENT] = MISSILE_DEFLECT;
    else profile->setvalue[SETMISSILETREATMENT] = MISSILE_NORMAL;

    profile->setyesno[SETCOSTFOREACHMISSILE] = vfs_get_next_bool(fileread);
    profile->setvalue[SETCOSTFOREACHMISSILE] = vfs_get_float(fileread);

    profile->setyesno[SETMORPH] = vfs_get_next_bool(fileread);
    profile->setvalue[SETMORPH] = true;  // vfs_get_bool( fileread );        //ZF> huh? why always channel and morph?

    profile->setyesno[SETCHANNEL] = vfs_get_next_bool(fileread);
    profile->setvalue[SETCHANNEL] = true;  // vfs_get_bool( fileread );

    // Now read in the add values
    profile->addvalue[ADDJUMPPOWER] = vfs_get_next_float(fileread);
    profile->addvalue[ADDBUMPDAMPEN] = vfs_get_next_int(fileread) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[ADDBOUNCINESS] = vfs_get_next_int(fileread) / 256.0f;    // Stored as 8.8-fixed, used as float
    profile->addvalue[ADDDAMAGE] = vfs_get_next_sfp8(fileread);            // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDSIZE] = vfs_get_next_float(fileread);           // Stored as float, used as float
    profile->addvalue[ADDACCEL] = vfs_get_next_int(fileread) / 80.0f;   // Stored as int, used as float
    profile->addvalue[ADDRED] = vfs_get_next_int(fileread);
    profile->addvalue[ADDGRN] = vfs_get_next_int(fileread);
    profile->addvalue[ADDBLU] = vfs_get_next_int(fileread);
    profile->addvalue[ADDDEFENSE] = -vfs_get_next_int(fileread);    // Defense is backwards
    profile->addvalue[ADDMANA] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDLIFE] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDSTRENGTH] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDWISDOM] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDINTELLIGENCE] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed
    profile->addvalue[ADDDEXTERITY] = vfs_get_next_sfp8(fileread);    // Stored as float, used as 8.8-fixed

    // Determine which entries are not important
    for (size_t cnt = 0; cnt < MAX_ENCHANT_ADD; cnt++)
    {
        profile->addyesno[cnt] = (0.0f != profile->addvalue[cnt]);
    }

    // Read expansions
    while (goto_colon_vfs(NULL, fileread, true))
    {
        idsz = vfs_get_idsz(fileread);

        if (idsz == MAKE_IDSZ('A', 'M', 'O', 'U')) profile->contspawn_amount = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('T', 'Y', 'P', 'E')) profile->contspawn_lpip = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('T', 'I', 'M', 'E')) profile->contspawn_delay = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('F', 'A', 'C', 'E')) profile->contspawn_facingadd = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('S', 'E', 'N', 'D')) profile->endsound_index = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('S', 'T', 'A', 'Y')) profile->stayifnoowner = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('O', 'V', 'E', 'R')) profile->spawn_overlay = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('D', 'E', 'A', 'D')) profile->stayiftargetdead = (0 != vfs_get_int(fileread));

        else if (idsz == MAKE_IDSZ('C', 'K', 'U', 'R')) profile->seekurse = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('D', 'A', 'R', 'K')) profile->darkvision = vfs_get_int(fileread);
    }

    // All done ( finally )
    vfs_close(fileread);

    strncpy(profile->name, loadName, SDL_arraysize(profile->name));
    profile->loaded = true;

    return true;
}
