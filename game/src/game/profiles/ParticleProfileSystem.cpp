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

#define GAME_PROFILES_PRIVATE 1
#include "game/profiles/ParticleProfileSystem.hpp"
#include "game/audio/AudioSystem.hpp"

_AbstractProfileSystem<pip_t, PIP_REF, INVALID_PIP_REF, MAX_PIP> PipStack("/debug/particle_profile_usage.txt");

PIP_REF PipStack_load_one(const char *loadName, const PIP_REF _override)
{
    PIP_REF ref = INVALID_PIP_REF;
    if (PipStack.isValidRange(_override))
    {
        PipStack_release_one(_override);
        ref = _override;
    }
    else
    {
        ref = PipStack_get_free();
    }

    if (!PipStack.isValidRange(ref))
    {
        return INVALID_PIP_REF;
    }
    pip_t *pip = PipStack.get_ptr(ref);

    if (!ParticleProfileReader::read(pip, loadName))
    {
        return INVALID_PIP_REF;
    }

    pip->end_sound = CLIP<Sint8>(pip->end_sound, INVALID_SOUND_ID, MAX_WAVE);
    pip->soundspawn = CLIP<Sint8>(pip->soundspawn, INVALID_SOUND_ID, MAX_WAVE);

    return ref;
}

void PipStack_release_all()
{
    size_t numLoaded = 0;
    int max_request = 0;
    for (PIP_REF ref = 0; ref < MAX_PIP; ref++)
    {
        if (PipStack.isLoaded(ref))
        {
            pip_t *pip = PipStack.get_ptr(ref);

            max_request = std::max(max_request, pip->request_count);
            numLoaded++;
        }
    }

    if (numLoaded > 0 && max_request > 0)
    {
        vfs_FILE * ftmp = vfs_openWriteB("/debug/particle_profile_usage.txt");
        if (NULL != ftmp)
        {
            vfs_printf(ftmp, "List of used particle profiles\n\n");
            for (PIP_REF ref = 0; ref < MAX_PIP; ref++)
            {
                if (PipStack.isLoaded(ref))
                {
                    pip_t *pip = PipStack.get_ptr(ref);
                    vfs_printf(ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", REF_TO_INT(ref), pip->name, pip->create_count, pip->request_count);
                }
            }
            vfs_close(ftmp);
        }
        for (PIP_REF ref = 0; ref < MAX_PIP; ref++)
        {
            PipStack_release_one(ref);
        }
    }
}

void PipStack_init_all()
{
    for (PIP_REF ref = 0; ref < MAX_PIP; ++ref)
    {
        PipStack.get_ptr(ref)->init();
    }
    // Reset the pip stack "pointer".
    PipStack.count = 0;
}

bool PipStack_release_one(const PIP_REF ref)
{
    if (!PipStack.isValidRange(ref)) return false;
    pip_t *pip = PipStack.get_ptr(ref);
    if (pip->loaded) pip->init();
    return true;
}

PIP_REF PipStack_get_free()
{
    int retval = INVALID_PIP_REF;

    if (PipStack.count < MAX_PIP)
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return CLIP(retval, 0, MAX_PIP);
}
