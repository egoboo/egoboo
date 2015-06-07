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

bool EnchantProfileWriter::write(std::shared_ptr<eve_t> profile, const std::string& pathname, const char *templateName)
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
    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETDAMAGETYPE].apply);
    template_put_damage_type(filetemp, filewrite, profile->_set[eve_t::SETDAMAGETYPE].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETNUMBEROFJUMPS].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETNUMBEROFJUMPS].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETLIFEBARCOLOR].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETLIFEBARCOLOR].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETMANABARCOLOR].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETMANABARCOLOR].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETSLASHMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETSLASHMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDSLASHRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETCRUSHMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETCRUSHMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDCRUSHRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETPOKEMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETPOKEMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDPOKERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETHOLYMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETHOLYMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDHOLYRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETEVILMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETEVILMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDEVILRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETFIREMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETFIREMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDFIRERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETICEMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETICEMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDICERESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETZAPMODIFIER].apply);
    template_put_damage_modifier(filetemp, filewrite, profile->_set[eve_t::SETZAPMODIFIER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDZAPRESIST].value * 100.0f);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETFLASHINGAND].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETFLASHINGAND].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETLIGHTBLEND].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETLIGHTBLEND].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETALPHABLEND].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETALPHABLEND].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETSHEEN].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETSHEEN].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETFLYTOHEIGHT].apply);
    template_put_int(filetemp, filewrite, profile->_set[eve_t::SETFLYTOHEIGHT].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETWALKONWATER].apply);
    template_put_bool(filetemp, filewrite, 0 != profile->_set[eve_t::SETWALKONWATER].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETCANSEEINVISIBLE].apply);
    template_put_bool(filetemp, filewrite, 0 != profile->_set[eve_t::SETCANSEEINVISIBLE].value);

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETMISSILETREATMENT].apply);

    switch ((e_missle_treatment)(int)profile->_set[eve_t::SETMISSILETREATMENT].value)
    {
    case MISSILE_NORMAL: template_put_char(filetemp, filewrite, 'N'); break;
    case MISSILE_DEFLECT: template_put_char(filetemp, filewrite, 'D'); break;
    case MISSILE_REFLECT: template_put_char(filetemp, filewrite, 'R'); break;
    }

    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETCOSTFOREACHMISSILE].apply);
    template_put_float(filetemp, filewrite, profile->_set[eve_t::SETCOSTFOREACHMISSILE].value);
    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETMORPH].apply);
    template_put_bool(filetemp, filewrite, profile->_set[eve_t::SETCHANNEL].apply);

    // Now read in the add values
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDJUMPPOWER].value);
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDBUMPDAMPEN].value * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDBOUNCINESS].value * 256.0f);  // Used as float, stored as 8.8-fixed
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDDAMAGE].value);                // Used as 8.8-fixed, stored as float
    template_put_float(filetemp, filewrite, profile->_add[eve_t::ADDSIZE].value);
    template_put_int(filetemp, filewrite, profile->_add[eve_t::ADDACCEL].value * 80.0f);
    template_put_int(filetemp, filewrite, profile->_add[eve_t::ADDRED].value);
    template_put_int(filetemp, filewrite, profile->_add[eve_t::ADDGRN].value);
    template_put_int(filetemp, filewrite, profile->_add[eve_t::ADDBLU].value);
    template_put_int(filetemp, filewrite, -profile->_add[eve_t::ADDDEFENSE].value);                // Defense is backwards
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDMANA].value);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDLIFE].value);                   // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDSTRENGTH].value);               // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDWISDOM].value);                 // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDINTELLIGENCE].value);           // Used as 8.8-fixed, stored as float
    template_put_sfp8(filetemp, filewrite, profile->_add[eve_t::ADDDEXTERITY].value);              // Used as 8.8-fixed, stored as float

    // copy the template file to the next free output section
    template_seek_free(filetemp, filewrite);

    if (profile->contspawn._amount > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('A', 'M', 'O', 'U'), profile->contspawn._amount);
    }

    /// @warning The original test was profile->contspawn._lpip > 0 but this is an error.
    /// Question is which parts of this program rely on this error again.
    if (LocalParticleProfileRef::Invalid != profile->contspawn._lpip)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('T', 'Y', 'P', 'E'), profile->contspawn._lpip);
    }

    if (profile->contspawn._facingAdd > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('T', 'I', 'M', 'E'), profile->contspawn._facingAdd);
    }

    if (profile->contspawn._delay > 0)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('F', 'A', 'C', 'E'), profile->contspawn._delay);
    }

    if (-1 != profile->endsound_index)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('S', 'E', 'N', 'D'), profile->endsound_index);
    }

    if (profile->_owner._stay)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('S', 'T', 'A', 'Y'), 1);
    }

    if (profile->spawn_overlay)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('O', 'V', 'E', 'R'), profile->spawn_overlay);
    }

    if (profile->seeKurses)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('C', 'K', 'U', 'R'), profile->seeKurses);
    }

    if (profile->darkvision)
    {
        vfs_put_expansion(filewrite, "", MAKE_IDSZ('D', 'A', 'R', 'K'), profile->darkvision);
    }

    if (profile->_target._stay)
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
