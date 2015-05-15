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

eve_t::eve_t() : AbstractProfile(),

    // Enchant spawn description.
    _override(false),
    remove_overridden(false),
    retarget(false),
    required_damagetype(DamageType::DAMAGE_NONE),
    require_damagetarget_damagetype(DamageType::DAMAGE_NONE),
    spawn_overlay(false),

    // Enchant despawn conditions.
    lifetime(0),
    endIfCannotPay(false),
    removedByIDSZ(IDSZ_NONE),

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
    endmessage(-1)
{
    //ctor
}

eve_t::~eve_t()
{
    //dtor
}

eve_t *eve_t::init()
{
    this->AbstractProfile::init();

    for (size_t i = 0; i < MAX_ENCHANT_SET; ++i)
    {
        this->_set[i].init();
    }
    for (size_t i = 0; i < MAX_ENCHANT_ADD; ++i)
    {
        this->_add[i].init();
    }
    contspawn.reset();
    _owner.init();
    _target.init();
    this->seeKurses = false;
    this->darkvision = false;


    // What to do when the enchant ends.
    endsound_index = -1;
    killtargetonend = false;
    poofonend = false;
    endmessage = -1;

    // Enchant spawn description.
    _override = false;
    remove_overridden = false;
    retarget = false;
    required_damagetype = DamageType::DAMAGE_NONE;
    require_damagetarget_damagetype = DamageType::DAMAGE_NONE;
    spawn_overlay = false;
    // Enchant despawn conditions.
    lifetime = 0;
    endIfCannotPay = false;
    removedByIDSZ = IDSZ_NONE;

    return this;
}
