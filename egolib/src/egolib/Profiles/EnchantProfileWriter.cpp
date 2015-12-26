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

#include "egolib/FileFormats/template.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/_math.h"

bool EnchantProfileWriter::write(std::shared_ptr<EnchantProfile> profile, const std::string& pathname, const char *templateName)
{
    vfs_FILE* filewrite, *filetemp;

    if (!profile) return false;

    filewrite = vfs_openWrite(pathname.c_str());
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
    template_put_bool(filetemp, filewrite, profile->_override);
    template_put_bool(filetemp, filewrite, profile->remove_overridden);
    template_put_bool(filetemp, filewrite, profile->killtargetonend);

    template_put_bool(filetemp, filewrite, profile->poofonend);

    // More stuff
    template_put_int(filetemp, filewrite, std::max(-1, profile->lifetime));
    template_put_int(filetemp, filewrite, profile->endmessage);

    // Drain stuff
    template_put_sfp8(filetemp, filewrite, profile->_owner._manaDrain);
    template_put_sfp8(filetemp, filewrite, profile->_target._manaDrain);
    template_put_bool(filetemp, filewrite, profile->endIfCannotPay);
    template_put_sfp8(filetemp, filewrite, profile->_owner._lifeDrain);
    template_put_sfp8(filetemp, filewrite, profile->_target._lifeDrain);

    // Specifics
    template_put_damage_type(filetemp, filewrite, profile->required_damagetype);
    template_put_damage_type(filetemp, filewrite, profile->require_damagetarget_damagetype);
    template_put_idsz(filetemp, filewrite, profile->removedByIDSZ);

    // Now the set values
    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETDAMAGETYPE].apply);
    template_put_damage_type(filetemp, filewrite, profile->_set[EnchantProfile::SETDAMAGETYPE].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETNUMBEROFJUMPS].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETNUMBEROFJUMPS].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETLIFEBARCOLOR].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETLIFEBARCOLOR].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETMANABARCOLOR].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETMANABARCOLOR].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETSLASHMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETSLASHMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDSLASHRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETCRUSHMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETCRUSHMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDCRUSHRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETPOKEMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETPOKEMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDPOKERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETHOLYMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETHOLYMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDHOLYRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETEVILMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETEVILMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDEVILRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETFIREMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETFIREMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDFIRERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETICEMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETICEMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDICERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETZAPMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[EnchantProfile::SETZAPMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDZAPRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETFLASHINGAND].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETFLASHINGAND].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETLIGHTBLEND].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETLIGHTBLEND].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETALPHABLEND].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETALPHABLEND].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETSHEEN].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETSHEEN].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETFLYTOHEIGHT].apply);
    template_put_int(filetemp, filewrite, profile->_set[EnchantProfile::SETFLYTOHEIGHT].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETWALKONWATER].apply);
    template_put_bool(filetemp, filewrite, 0 != profile->_set[EnchantProfile::SETWALKONWATER].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETCANSEEINVISIBLE].apply);
    template_put_bool(filetemp, filewrite, 0 != profile->_set[EnchantProfile::SETCANSEEINVISIBLE].value);

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETMISSILETREATMENT].apply);

    switch ( (MissileTreatment)(long)(profile->_set[eve_t::SETMISSILETREATMENT].value) )
    {
        case MissileTreatment_Normal:  template_put_char(filetemp, filewrite, 'N'); break;
        case MissileTreatment_Deflect: template_put_char(filetemp, filewrite, 'D'); break;
        case MissileTreatment_Reflect: template_put_char(filetemp, filewrite, 'R'); break;
    }

    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETCOSTFOREACHMISSILE].apply);
    template_put_float(filetemp, filewrite, profile->_set[EnchantProfile::SETCOSTFOREACHMISSILE].value);
    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETMORPH].apply);
    template_put_bool(filetemp, filewrite, profile->_set[EnchantProfile::SETCHANNEL].apply);

    // Now read in the add values
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDJUMPPOWER].value);
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDBUMPDAMPEN].value * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDBOUNCINESS].value * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDDAMAGE].value);                // Used as 8.8-fixed, stored as float
    template_put_float(filetemp, filewrite, profile->_add[EnchantProfile::ADDSIZE].value);
    template_put_int(filetemp, filewrite, profile->_add[EnchantProfile::ADDACCEL].value * 80.0f);
    template_put_int(filetemp, filewrite, profile->_add[EnchantProfile::ADDRED].value);
    template_put_int(filetemp, filewrite, profile->_add[EnchantProfile::ADDGRN].value);
    template_put_int(filetemp, filewrite, profile->_add[EnchantProfile::ADDBLU].value);
    template_put_int(filetemp, filewrite, -profile->_add[EnchantProfile::ADDDEFENSE].value);                // Defense is backwards
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDMANA].value);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDLIFE].value);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDSTRENGTH].value);               // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDWISDOM].value);                 // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDINTELLIGENCE].value);           // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[EnchantProfile::ADDDEXTERITY].value);              // Used as 8.8-fixed, stored as float

    // copy the template file to the next free output section
    template_seek_free(filetemp, filewrite);

    if (profile->contspawn._amount > 0)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('A', 'M', 'O', 'U'), profile->contspawn._amount);
    }

    /// @warning The original test was profile->contspawn._lpip > 0 but this is an error.
    /// Question is which parts of this program rely on this error again.
    if (LocalParticleProfileRef::Invalid != profile->contspawn._lpip)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('T', 'Y', 'P', 'E'), profile->contspawn._lpip);
    }

    if (profile->contspawn._facingAdd > 0)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('T', 'I', 'M', 'E'), profile->contspawn._facingAdd);
    }

    if (profile->contspawn._delay > 0)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('F', 'A', 'C', 'E'), profile->contspawn._delay);
    }

    if (-1 != profile->endsound_index)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('S', 'E', 'N', 'D'), profile->endsound_index);
    }

    if (profile->_owner._stay)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('S', 'T', 'A', 'Y'), 1);
    }

    if (profile->spawn_overlay)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('O', 'V', 'E', 'R'), profile->spawn_overlay);
    }

    if (profile->seeKurses)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('C', 'K', 'U', 'R'), profile->seeKurses);
    }

    if (profile->darkvision)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('D', 'A', 'R', 'K'), profile->darkvision);
    }

    if (profile->_target._stay)
    {
        vfs_put_expansion(filewrite, "", IDSZ2('D', 'E', 'A', 'D'), 1);
    }

    if (!profile->getEnchantName().empty())
    {
        vfs_put_expansion_string(filewrite, "", IDSZ2('N', 'A', 'M', 'E'), profile->getEnchantName().c_str());
    }

    // dump the rest of the template file
    template_flush(filetemp, filewrite);

    // All done ( finally )
    vfs_close(filewrite);
    template_close_vfs(filetemp);

    return true;
}
