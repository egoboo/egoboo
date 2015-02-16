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
#include "egolib/Profiles/EnchantProfileWriter.hpp"

#include "egolib/file_formats/template.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/_math.h"

bool EnchantProfileWriter::write(eve_t *profile, const char *loadName, const char *templateName)
{
    vfs_FILE* filewrite, *filetemp;

    if (!profile || !loadName) return false;

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
    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETDAMAGETYPE]);
    template_put_damage_type(filetemp, filewrite, profile->setvalue[eve_t::SETDAMAGETYPE]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETNUMBEROFJUMPS]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETNUMBEROFJUMPS]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETLIFEBARCOLOR]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETLIFEBARCOLOR]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETMANABARCOLOR]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETMANABARCOLOR]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETSLASHMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETSLASHMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDSLASHRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETCRUSHMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETCRUSHMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDCRUSHRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETPOKEMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETPOKEMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDPOKERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETHOLYMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETHOLYMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDHOLYRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETEVILMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETEVILMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDEVILRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETFIREMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETFIREMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDFIRERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETICEMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETICEMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDICERESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETZAPMODIFIER]);
    template_put_damage_modifier(filetemp, filewrite, profile->setvalue[eve_t::SETZAPMODIFIER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDZAPRESIST] * 100.0f);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETFLASHINGAND]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETFLASHINGAND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETLIGHTBLEND]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETLIGHTBLEND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETALPHABLEND]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETALPHABLEND]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETSHEEN]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETSHEEN]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETFLYTOHEIGHT]);
    template_put_int(filetemp, filewrite, profile->setvalue[eve_t::SETFLYTOHEIGHT]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETWALKONWATER]);
    template_put_bool(filetemp, filewrite, 0 != profile->setvalue[eve_t::SETWALKONWATER]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETCANSEEINVISIBLE]);
    template_put_bool(filetemp, filewrite, 0 != profile->setvalue[eve_t::SETCANSEEINVISIBLE]);

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETMISSILETREATMENT]);

    switch ((int)profile->setvalue[eve_t::SETMISSILETREATMENT])
    {
    case MISSILE_NORMAL: template_put_char(filetemp, filewrite, 'N'); break;
    case MISSILE_DEFLECT: template_put_char(filetemp, filewrite, 'D'); break;
    case MISSILE_REFLECT: template_put_char(filetemp, filewrite, 'R'); break;
    }

    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETCOSTFOREACHMISSILE]);
    template_put_float(filetemp, filewrite, profile->setvalue[eve_t::SETCOSTFOREACHMISSILE]);
    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETMORPH]);
    template_put_bool(filetemp, filewrite, profile->setyesno[eve_t::SETCHANNEL]);

    // Now read in the add values
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDJUMPPOWER]);
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDBUMPDAMPEN] * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDBOUNCINESS] * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDDAMAGE]);                // Used as 8.8-fixed, stored as float
    template_put_float(filetemp, filewrite, profile->addvalue[eve_t::ADDSIZE]);
    template_put_int(filetemp, filewrite, profile->addvalue[eve_t::ADDACCEL] * 80.0f);
    template_put_int(filetemp, filewrite, profile->addvalue[eve_t::ADDRED]);
    template_put_int(filetemp, filewrite, profile->addvalue[eve_t::ADDGRN]);
    template_put_int(filetemp, filewrite, profile->addvalue[eve_t::ADDBLU]);
    template_put_int(filetemp, filewrite, -profile->addvalue[eve_t::ADDDEFENSE]);                // Defense is backwards
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDMANA]);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDLIFE]);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDSTRENGTH]);               // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDWISDOM]);                 // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDINTELLIGENCE]);           // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->addvalue[eve_t::ADDDEXTERITY]);              // Used as 8.8-fixed, stored as float

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
