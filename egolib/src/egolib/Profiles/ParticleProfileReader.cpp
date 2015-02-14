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

#include "egolib/file_formats/template.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/vfs.h"
#include "egolib/_math.h"

bool ParticleProfileReader::read(pip_t *profile, const char *loadName)
{
    vfs_FILE* fileread;
    IDSZ idsz;
    char cTmp;

    fileread = vfs_openRead(loadName);
    if (!fileread)
    {
        return false;
    }

    pip_t::init(profile);

    // set up the EGO_PROFILE_STUFF
    strncpy(profile->name, loadName, SDL_arraysize(profile->name));
    profile->loaded = true;

    // read the 1 line comment at the top of the file
    vfs_gets(profile->comment, SDL_arraysize(profile->comment) - 1, fileread);

    // rewind the file
    vfs_seek(fileread, 0);

    // General data
    profile->force = vfs_get_next_bool(fileread);

    cTmp = vfs_get_next_char(fileread);
    if ('L' == char_toupper((unsigned)cTmp))  profile->type = SPRITE_LIGHT;
    else if ('S' == char_toupper((unsigned)cTmp))  profile->type = SPRITE_SOLID;
    else if ('T' == char_toupper((unsigned)cTmp))  profile->type = SPRITE_ALPHA;

    profile->image_base = vfs_get_next_int(fileread);
    profile->numframes = vfs_get_next_int(fileread);
    profile->image_add.base = vfs_get_next_int(fileread);
    profile->image_add.rand = vfs_get_next_int(fileread);
    profile->rotate_pair.base = vfs_get_next_int(fileread);
    profile->rotate_pair.rand = vfs_get_next_int(fileread);
    profile->rotate_add = vfs_get_next_int(fileread);
    profile->size_base = vfs_get_next_int(fileread);
    profile->size_add = vfs_get_next_int(fileread);
    profile->spdlimit = vfs_get_next_float(fileread);
    profile->facingadd = vfs_get_next_int(fileread);

    // override the base rotation
    if (profile->image_base < 256 && prt_u != prt_direction[profile->image_base])
    {
        profile->rotate_pair.base = prt_direction[profile->image_base];
    }

    // Ending conditions
    profile->end_water = vfs_get_next_bool(fileread);
    profile->end_bump = vfs_get_next_bool(fileread);
    profile->end_ground = vfs_get_next_bool(fileread);
    profile->end_lastframe = vfs_get_next_bool(fileread);
    profile->end_time = vfs_get_next_int(fileread);

    // Collision data
    profile->dampen = vfs_get_next_float(fileread);
    profile->bump_money = vfs_get_next_int(fileread);
    profile->bump_size = vfs_get_next_int(fileread);
    profile->bump_height = vfs_get_next_int(fileread);

    vfs_get_next_range(fileread, &(profile->damage));
    profile->damagetype = vfs_get_next_damage_type(fileread);

    // Lighting data
    cTmp = vfs_get_next_char(fileread);
    if ('T' == char_toupper((unsigned)cTmp)) profile->dynalight.mode = DYNA_MODE_ON;
    else if ('L' == char_toupper((unsigned)cTmp)) profile->dynalight.mode = DYNA_MODE_LOCAL;
    else profile->dynalight.mode = DYNA_MODE_OFF;

    profile->dynalight.level = vfs_get_next_float(fileread);
    profile->dynalight.falloff = vfs_get_next_int(fileread);

    // Initial spawning of this particle
    profile->facing_pair.base = vfs_get_next_int(fileread);
    profile->facing_pair.rand = vfs_get_next_int(fileread);
    profile->spacing_hrz_pair.base = vfs_get_next_int(fileread);
    profile->spacing_hrz_pair.rand = vfs_get_next_int(fileread);
    profile->spacing_vrt_pair.base = vfs_get_next_int(fileread);
    profile->spacing_vrt_pair.rand = vfs_get_next_int(fileread);
    profile->vel_hrz_pair.base = vfs_get_next_int(fileread);
    profile->vel_hrz_pair.rand = vfs_get_next_int(fileread);
    profile->vel_vrt_pair.base = vfs_get_next_int(fileread);
    profile->vel_vrt_pair.rand = vfs_get_next_int(fileread);

    // Continuous spawning of other particles
    profile->contspawn_delay = vfs_get_next_int(fileread);
    profile->contspawn_amount = vfs_get_next_int(fileread);
    profile->contspawn_facingadd = vfs_get_next_int(fileread);
    profile->contspawn_lpip = vfs_get_next_int(fileread);

    // End spawning of other particles
    profile->endspawn_amount = vfs_get_next_int(fileread);
    profile->endspawn_facingadd = vfs_get_next_int(fileread);
    profile->endspawn_lpip = vfs_get_next_int(fileread);

    // Bump spawning of attached particles
    profile->bumpspawn_amount = vfs_get_next_int(fileread);
    profile->bumpspawn_lpip = vfs_get_next_int(fileread);

    // Random stuff  !!!BAD!!! Not complete
    profile->daze_time = vfs_get_next_int(fileread);
    profile->grog_time = vfs_get_next_int(fileread);
    profile->spawnenchant = vfs_get_next_bool(fileread);

    profile->cause_roll = vfs_get_next_bool(fileread);  // !!Cause roll
    profile->cause_pancake = vfs_get_next_bool(fileread);

    profile->needtarget = vfs_get_next_bool(fileread);
    profile->targetcaster = vfs_get_next_bool(fileread);
    profile->startontarget = vfs_get_next_bool(fileread);
    profile->onlydamagefriendly = vfs_get_next_bool(fileread);

    profile->soundspawn = vfs_get_next_int(fileread);

    profile->end_sound = vfs_get_next_int(fileread);

    profile->friendlyfire = vfs_get_next_bool(fileread);

    profile->hateonly = vfs_get_next_bool(fileread);

    profile->newtargetonspawn = vfs_get_next_bool(fileread);

    profile->targetangle = vfs_get_next_int(fileread) >> 1;
    profile->homing = vfs_get_next_bool(fileread);

    profile->homingfriction = vfs_get_next_float(fileread);
    profile->homingaccel = vfs_get_next_float(fileread);
    profile->rotatetoface = vfs_get_next_bool(fileread);

    goto_colon_vfs(NULL, fileread, false);  // !!Respawn on hit is unused

    profile->manadrain = vfs_get_next_ufp8(fileread);
    profile->lifedrain = vfs_get_next_ufp8(fileread);

    // assume default end_wall
    profile->end_wall = profile->end_ground;

    // assume default damfx
    if (profile->homing)  profile->damfx = DAMFX_NONE;

    // Read expansions
    while (goto_colon_vfs(NULL, fileread, true))
    {
        idsz = vfs_get_idsz(fileread);

        if (idsz == MAKE_IDSZ('T', 'U', 'R', 'N'))       SET_BIT(profile->damfx, DAMFX_NONE);        //ZF> This line doesnt do anything?
        else if (idsz == MAKE_IDSZ('A', 'R', 'M', 'O'))  SET_BIT(profile->damfx, DAMFX_ARMO);
        else if (idsz == MAKE_IDSZ('B', 'L', 'O', 'C'))  SET_BIT(profile->damfx, DAMFX_NBLOC);
        else if (idsz == MAKE_IDSZ('A', 'R', 'R', 'O'))  SET_BIT(profile->damfx, DAMFX_ARRO);
        else if (idsz == MAKE_IDSZ('T', 'I', 'M', 'E'))  SET_BIT(profile->damfx, DAMFX_TIME);
        else if (idsz == MAKE_IDSZ('Z', 'S', 'P', 'D'))  profile->zaimspd = vfs_get_float(fileread);
        else if (idsz == MAKE_IDSZ('F', 'S', 'N', 'D'))  profile->end_sound_floor = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('W', 'S', 'N', 'D'))  profile->end_sound_wall = vfs_get_int(fileread);
        else if (idsz == MAKE_IDSZ('W', 'E', 'N', 'D'))  profile->end_wall = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('P', 'U', 'S', 'H'))  profile->allowpush = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('D', 'L', 'E', 'V'))  profile->dynalight.level_add = vfs_get_int(fileread) / 1000.0f;
        else if (idsz == MAKE_IDSZ('D', 'R', 'A', 'D'))  profile->dynalight.falloff_add = vfs_get_int(fileread) / 1000.0f;
        else if (idsz == MAKE_IDSZ('I', 'D', 'A', 'M'))  profile->intdamagebonus = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('W', 'D', 'A', 'M'))  profile->wisdamagebonus = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('G', 'R', 'A', 'V'))  profile->ignore_gravity = (0 != vfs_get_int(fileread));
        else if (idsz == MAKE_IDSZ('O', 'R', 'N', 'T'))
        {
            char cTmp = vfs_get_first_letter(fileread);
            switch (char_toupper((unsigned)cTmp))
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

    vfs_close(fileread);

    return true;
}
