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
#include "egolib/Profiles/ParticleProfileReader.hpp"

#include "egolib/Audio/AudioSystem.hpp"
#include "egolib/FileFormats/template.h"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/_math.h"

bool ParticleProfileReader::read(pip_t *profile, const char *loadName)
{
    IDSZ idsz;
    char cTmp;

    ReadContext ctxt(loadName);
    if (!ctxt.ensureOpen())
    {
        return false;
    }

    profile->init();

    // set up the EGO_PROFILE_STUFF
    strncpy(profile->_name, loadName, SDL_arraysize(profile->_name));
    profile->_loaded = true;

    // read the 1 line comment at the top of the file
    std::string comment = ctxt.readSingleLineComment();
    strncpy(profile->comment,comment.c_str(),SDL_arraysize(profile->comment));

    // General data
    profile->force = vfs_get_next_bool(ctxt);

    cTmp = vfs_get_next_printable(ctxt);
    if ('L' == Ego::toupper(cTmp))  profile->type = SPRITE_LIGHT;
    else if ('S' == Ego::toupper(cTmp))  profile->type = SPRITE_SOLID;
    else if ('T' == Ego::toupper(cTmp))  profile->type = SPRITE_ALPHA;

    profile->image_base = vfs_get_next_int(ctxt);
    profile->numframes = vfs_get_next_int(ctxt);
    profile->image_add.base = vfs_get_next_int(ctxt);
    profile->image_add.rand = vfs_get_next_int(ctxt);
    profile->rotate_pair.base = vfs_get_next_int(ctxt);
    profile->rotate_pair.rand = vfs_get_next_int(ctxt);
    profile->rotate_add = vfs_get_next_int(ctxt);
    profile->size_base = vfs_get_next_int(ctxt);
    profile->size_add = vfs_get_next_int(ctxt);
    profile->spdlimit = vfs_get_next_float(ctxt);
    profile->facingadd = vfs_get_next_int(ctxt);

    // override the base rotation
    if (profile->image_base < 256 && prt_u != prt_direction[profile->image_base])
    {
        profile->rotate_pair.base = prt_direction[profile->image_base];
    }

    // Ending conditions
    profile->end_water = vfs_get_next_bool(ctxt);
    profile->end_bump = vfs_get_next_bool(ctxt);
    profile->end_ground = vfs_get_next_bool(ctxt);
    profile->end_lastframe = vfs_get_next_bool(ctxt);
    profile->end_time = vfs_get_next_int(ctxt);

    // Collision data
    profile->dampen = vfs_get_next_float(ctxt);
    profile->bump_money = vfs_get_next_int(ctxt);
    profile->bump_size = vfs_get_next_int(ctxt);
    profile->bump_height = vfs_get_next_int(ctxt);

    vfs_get_next_range(ctxt, &(profile->damage));
    profile->damageType = vfs_get_next_damage_type(ctxt);

    // Lighting data
    cTmp = vfs_get_next_printable(ctxt);
    if ('T' == Ego::toupper(cTmp)) profile->dynalight.mode = DYNA_MODE_ON;
    else if ('L' == Ego::toupper(cTmp)) profile->dynalight.mode = DYNA_MODE_LOCAL;
    else profile->dynalight.mode = DYNA_MODE_OFF;

    profile->dynalight.level = vfs_get_next_float(ctxt);
    profile->dynalight.falloff = vfs_get_next_int(ctxt);

    // Initial spawning of this particle
    profile->facing_pair.base = vfs_get_next_int(ctxt);
    profile->facing_pair.rand = vfs_get_next_int(ctxt);

    profile->spacing_hrz_pair.base = vfs_get_next_int(ctxt);
    profile->spacing_hrz_pair.rand = vfs_get_next_int(ctxt);

    profile->spacing_vrt_pair.base = vfs_get_next_int(ctxt);
    profile->spacing_vrt_pair.rand = vfs_get_next_int(ctxt);

    profile->vel_hrz_pair.base = vfs_get_next_int(ctxt);
    profile->vel_hrz_pair.rand = vfs_get_next_int(ctxt);

    profile->vel_vrt_pair.base = vfs_get_next_int(ctxt);
    profile->vel_vrt_pair.rand = vfs_get_next_int(ctxt);

    // Continuous spawning of other particles
    profile->contspawn._delay = vfs_get_next_int(ctxt);
    profile->contspawn._amount = vfs_get_next_int(ctxt);
    profile->contspawn._facingAdd = vfs_get_next_int(ctxt);
    profile->contspawn._lpip = vfs_get_next_int(ctxt);

    // End spawning of other particles
    profile->endspawn._amount = vfs_get_next_int(ctxt);
    profile->endspawn._facingAdd = vfs_get_next_int(ctxt);
    profile->endspawn._lpip = vfs_get_next_int(ctxt);

    // Bump spawning of attached particles
    profile->bumpspawn._amount = vfs_get_next_int(ctxt);
    profile->bumpspawn._facingAdd = 0; // @to add.
    profile->bumpspawn._lpip = vfs_get_next_int(ctxt);

    // Random stuff  !!!BAD!!! Not complete
    profile->dazeTime = vfs_get_next_nat(ctxt);
    profile->grogTime = vfs_get_next_nat(ctxt);
    profile->spawnenchant = vfs_get_next_bool(ctxt);

    profile->cause_roll = vfs_get_next_bool(ctxt);  // !!Cause roll
    profile->cause_pancake = vfs_get_next_bool(ctxt);

    profile->needtarget = vfs_get_next_bool(ctxt);
    profile->targetcaster = vfs_get_next_bool(ctxt);
    profile->startontarget = vfs_get_next_bool(ctxt);
    profile->onlydamagefriendly = vfs_get_next_bool(ctxt);

    profile->soundspawn = vfs_get_next_int(ctxt);

    profile->end_sound = vfs_get_next_int(ctxt);

    profile->friendlyfire = vfs_get_next_bool(ctxt);

    profile->hateonly = vfs_get_next_bool(ctxt);

    profile->newtargetonspawn = vfs_get_next_bool(ctxt);

    profile->targetangle = vfs_get_next_int(ctxt) >> 1;
    profile->homing = vfs_get_next_bool(ctxt);

    profile->homingfriction = vfs_get_next_float(ctxt);
    profile->homingaccel = vfs_get_next_float(ctxt);
    profile->rotatetoface = vfs_get_next_bool(ctxt);

    ctxt.skipToColon(false);  // !!Respawn on hit is unused

    profile->manaDrain = vfs_get_next_ufp8(ctxt);
    profile->lifeDrain = vfs_get_next_ufp8(ctxt);

    // assume default end_wall
    profile->end_wall = profile->end_ground;

    // assume default damfx
    if (profile->homing)  profile->damfx = DAMFX_NONE;

    // Read expansions
    while (ctxt.skipToColon(true))
    {
        idsz = ctxt.readIDSZ();

        if (idsz == MAKE_IDSZ('N', 'O', 'N', 'E'))       SET_BIT(profile->damfx, DAMFX_NONE);
        else if (idsz == MAKE_IDSZ('T', 'U', 'R', 'N'))  SET_BIT(profile->damfx, DAMFX_TURN);
        else if (idsz == MAKE_IDSZ('A', 'R', 'M', 'O'))  SET_BIT(profile->damfx, DAMFX_ARMO);
        else if (idsz == MAKE_IDSZ('B', 'L', 'O', 'C'))  SET_BIT(profile->damfx, DAMFX_NBLOC);
        else if (idsz == MAKE_IDSZ('A', 'R', 'R', 'O'))  SET_BIT(profile->damfx, DAMFX_ARRO);
        else if (idsz == MAKE_IDSZ('T', 'I', 'M', 'E'))  SET_BIT(profile->damfx, DAMFX_TIME);
        else if (idsz == MAKE_IDSZ('Z', 'S', 'P', 'D'))  profile->zaimspd = ctxt.readReal();
        else if (idsz == MAKE_IDSZ('F', 'S', 'N', 'D'))  profile->end_sound_floor = ctxt.readInt();
        else if (idsz == MAKE_IDSZ('W', 'S', 'N', 'D'))  profile->end_sound_wall = ctxt.readInt();
        else if (idsz == MAKE_IDSZ('W', 'E', 'N', 'D'))  profile->end_wall = (0 != ctxt.readInt());
        else if (idsz == MAKE_IDSZ('P', 'U', 'S', 'H'))  profile->allowpush = (0 != ctxt.readInt());
        else if (idsz == MAKE_IDSZ('D', 'L', 'E', 'V'))  profile->dynalight.level_add = ctxt.readInt() / 1000.0f;
        else if (idsz == MAKE_IDSZ('D', 'R', 'A', 'D'))  profile->dynalight.falloff_add = ctxt.readInt() / 1000.0f;
        else if (idsz == MAKE_IDSZ('I', 'D', 'A', 'M'))  profile->damageBoni._intelligence = (0 != ctxt.readInt());
        else if (idsz == MAKE_IDSZ('W', 'D', 'A', 'M'))  profile->damageBoni._wisdom = (0 != ctxt.readInt());
        else if (idsz == MAKE_IDSZ('G', 'R', 'A', 'V'))  profile->ignore_gravity = (0 != ctxt.readInt());
        else if (idsz == MAKE_IDSZ('O', 'R', 'N', 'T'))
        {
            switch (Ego::toupper(ctxt.readPrintable()))
            {
            case 'X': profile->orientation = ORIENTATION_X; break;  // put particle up along the world or body-fixed x-axis
            case 'Y': profile->orientation = ORIENTATION_Y; break;  // put particle up along the world or body-fixed y-axis
            case 'Z': profile->orientation = ORIENTATION_Z; break;  // put particle up along the world or body-fixed z-axis
            case 'V': profile->orientation = ORIENTATION_V; break;  // vertical, like a candle
            case 'H': profile->orientation = ORIENTATION_H; break;  // horizontal, like a plate
            case 'B': profile->orientation = ORIENTATION_B; break;  // billboard
            }
        }
    }

    // Limit the end_sound index.
    profile->end_sound = CLIP<Sint8>(profile->end_sound, INVALID_SOUND_ID, MAX_WAVE);
    // Limit the soundspawn index.
    profile->soundspawn = CLIP<Sint8>(profile->soundspawn, INVALID_SOUND_ID, MAX_WAVE);

    return true;
}
