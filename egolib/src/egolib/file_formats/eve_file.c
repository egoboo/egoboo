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

/// @file  egolib/file_formats/eve_file.c
/// @brief Data and functions to read and write Egoboo's enchant definition files (<tt>"/modules/*.mod/objects/*.obj/enchant.txt"</tt>).

#include "egolib/file_formats/eve_file.h"
#include "egolib/file_formats/template.h"

#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"

#include "egolib/_math.h"

eve_t *eve_t::init(eve_t *self)
{
    if (!self) return nullptr;

    BLANK_STRUCT_PTR(self);

    self->endsound_index = -1;

    return self;
}

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

bool EnchantProfileWriter::write(eve_t *profile,const char *loadName, const char *templateName)
{
    vfs_FILE* filewrite, *filetemp;

    if (!profile||!loadName) return false;

    filewrite = vfs_openWrite(loadName);
    if (!filewrite) return false;

    filetemp = nullptr;

    // Try the given template file.
    if (VALID_CSTR(templateName))
    {
        filetemp = template_open_vfs(templateName);
    }

    // Try a default template file.
    if (!filetemp)
    {
        filetemp = template_open_vfs("mp_data/templates/enchant.txt");
    }

    // true/false values
    template_put_bool(filetemp, filewrite, profile->retarget);
    template_put_bool(filetemp, filewrite, profile->override);
    template_put_bool(filetemp, filewrite, profile->remove_overridden);
    template_put_bool(filetemp, filewrite, profile->killtargetonend);

    template_put_bool(filetemp, filewrite, profile->poofonend);

    // More stuff
    template_put_int(filetemp, filewrite, std::max(-1, profile->lifetime));
    template_put_int(filetemp, filewrite, profile->endmessage);

    // Drain stuff
    template_put_float(filetemp, filewrite, FP8_TO_FLOAT(profile->owner_mana));
    template_put_float(filetemp, filewrite, FP8_TO_FLOAT(profile->target_mana));
    template_put_bool(filetemp, filewrite, profile->endifcantpay);
    template_put_float(filetemp, filewrite, FP8_TO_FLOAT(profile->owner_life));
    template_put_float(filetemp, filewrite, FP8_TO_FLOAT(profile->target_life));

    // Specifics
    template_put_damage_type(filetemp, filewrite, profile->required_damagetype);
    template_put_damage_type(filetemp, filewrite, profile->require_damagetarget_damagetype);
    template_put_idsz(filetemp, filewrite, profile->removedbyidsz);

    // Now the set values
    template_put_bool(filetemp, filewrite, profile->setyesno[SETDAMAGETYPE]);
    template_put_damage_type(filetemp, filewrite, profile->setvalue[SETDAMAGETYPE]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETNUMBEROFJUMPS]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETNUMBEROFJUMPS]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETLIFEBARCOLOR]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETLIFEBARCOLOR]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETMANABARCOLOR]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETMANABARCOLOR]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETSLASHMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETSLASHMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDSLASHRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETCRUSHMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETCRUSHMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDCRUSHRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETPOKEMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETPOKEMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDPOKERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETHOLYMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETHOLYMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDHOLYRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETEVILMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETEVILMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDEVILRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETFIREMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETFIREMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDFIRERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETICEMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETICEMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDICERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETZAPMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[SETZAPMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDZAPRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETFLASHINGAND]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETFLASHINGAND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETLIGHTBLEND]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETLIGHTBLEND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETALPHABLEND]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETALPHABLEND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETSHEEN]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETSHEEN]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETFLYTOHEIGHT]);
    template_put_int(filetemp, filewrite, profile->setvalue[SETFLYTOHEIGHT]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETWALKONWATER]);
    template_put_bool(filetemp, filewrite, 0 != profile->setvalue[SETWALKONWATER]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETCANSEEINVISIBLE]);
    template_put_bool(filetemp, filewrite, 0 != profile->setvalue[SETCANSEEINVISIBLE]);

    template_put_bool(filetemp, filewrite, profile->setyesno[SETMISSILETREATMENT]);

    switch ((int)profile->setvalue[SETMISSILETREATMENT])
    {
    case MISSILE_NORMAL: template_put_char(filetemp, filewrite, 'N'); break;
    case MISSILE_DEFLECT: template_put_char(filetemp, filewrite, 'D'); break;
    case MISSILE_REFLECT: template_put_char(filetemp, filewrite, 'R'); break;
    }

    template_put_bool(filetemp, filewrite, profile->setyesno[SETCOSTFOREACHMISSILE]);
    template_put_float(filetemp, filewrite, profile->setvalue[SETCOSTFOREACHMISSILE]);
    template_put_bool(filetemp, filewrite, profile->setyesno[SETMORPH]);
    template_put_bool(filetemp, filewrite, profile->setyesno[SETCHANNEL]);

    // Now read in the add values
    template_put_float(filetemp, filewrite, profile->addvalue[ADDJUMPPOWER]);
    template_put_float(filetemp, filewrite, profile->addvalue[ADDBUMPDAMPEN] * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_float(filetemp, filewrite, profile->addvalue[ADDBOUNCINESS] * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDDAMAGE]);                // Used as 8.8-fixed, stored as float
    template_put_float(filetemp, filewrite, profile->addvalue[ADDSIZE]);
    template_put_int(filetemp, filewrite, profile->addvalue[ADDACCEL] * 80.0f);
    template_put_int(filetemp, filewrite, profile->addvalue[ADDRED]);
    template_put_int(filetemp, filewrite, profile->addvalue[ADDGRN]);
    template_put_int(filetemp, filewrite, profile->addvalue[ADDBLU]);
    template_put_int(filetemp, filewrite, -profile->addvalue[ADDDEFENSE]);                // Defense is backwards
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDMANA]);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDLIFE]);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDSTRENGTH]);               // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDWISDOM]);                 // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDINTELLIGENCE]);           // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[ADDDEXTERITY]);              // Used as 8.8-fixed, stored as float

    // copy the template file to the next free output section
    template_seek_free(filetemp, filewrite);

    if (profile->contspawn_amount > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('A', 'M', 'O', 'U'), profile->contspawn_amount);
    }

    if (profile->contspawn_lpip > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('T', 'Y', 'P', 'E'), profile->contspawn_lpip);
    }

    if (profile->contspawn_facingadd > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('T', 'I', 'M', 'E'), profile->contspawn_facingadd);
    }

    if (profile->contspawn_delay > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('F', 'A', 'C', 'E'), profile->contspawn_delay);
    }

    if (-1 != profile->endsound_index)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('S', 'E', 'N', 'D'), profile->endsound_index);
    }

    if (profile->stayifnoowner)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('S', 'T', 'A', 'Y'), 1);
    }

    if (profile->spawn_overlay)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('O', 'V', 'E', 'R'), profile->spawn_overlay);
    }

    if (profile->seekurse)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('C', 'K', 'U', 'R'), profile->seekurse);
    }

    if (profile->darkvision)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('D', 'A', 'R', 'K'), profile->darkvision);
    }

    if (profile->stayiftargetdead)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('D', 'E', 'A', 'D'), 1);
    }

    // dump the rest of the template file
    template_flush(filetemp, filewrite);

    // All done ( finally )
    vfs_close(filewrite);
    template_close_vfs(filetemp);

    return true;
}


