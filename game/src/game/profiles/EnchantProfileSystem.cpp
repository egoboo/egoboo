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
#include "game/profiles/EnchantProfileSystem.hpp"
#include "game/audio/AudioSystem.hpp"

_AbstractProfileSystem<eve_t, EVE_REF, INVALID_EVE_REF, MAX_EVE> EveStack("/debug/enchant_profile_usage.txt");

EVE_REF EveStack_load_one(const char *loadName, const EVE_REF _override)
{
    EVE_REF ref = INVALID_EVE_REF;
    if (EveStack.isValidRange(_override))
    {
        EveStack_release_one(_override);
        ref = _override;
    }
    else
    {
        ref = EveStack_get_free();
    }

    if (!EveStack.isValidRange(ref))
    {
        return INVALID_EVE_REF;
    }
    eve_t *eve = EveStack.get_ptr(ref);

    if (!EnchantProfileReader::read(eve, loadName))
    {
        return INVALID_EVE_REF;
    }
    // Limit the endsound_index.
    eve->endsound_index = CLIP<Sint16>(eve->endsound_index, INVALID_SOUND_ID, MAX_WAVE);

    return ref;
}

void EveStack_release_all()
{
    size_t numLoaded = 0;
    int max_request = 0;
    for (EVE_REF ref = 0; ref < MAX_EVE; ref++)
    {
        if (EveStack.isLoaded(ref))
        {
            eve_t *eve = EveStack.get_ptr(ref);

            max_request = std::max(max_request, eve->request_count);
            numLoaded++;
        }
    }
    if (numLoaded > 0 && max_request > 0)
    {
        vfs_FILE * ftmp = vfs_openWriteB("/debug/enchant_profile_usage.txt");
        if (NULL != ftmp)
        {
            vfs_printf(ftmp, "List of used enchant profiles\n\n");

            for (EVE_REF ref = 0; ref < MAX_EVE; ref++)
            {
                if (EveStack.isLoaded(ref))
                {
                    eve_t *eve = EveStack.get_ptr(ref);
                    vfs_printf(ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", REF_TO_INT(ref), eve->name, eve->create_count, eve->request_count);
                }
            }
            vfs_close(ftmp);
        }
        for (EVE_REF ref = 0; ref < MAX_EVE; ++ref)
        {
            EveStack_release_one(ref);
        }
    }
}

void EveStack_init_all()
{
    for (EVE_REF ref = 0; ref < MAX_EVE; ++ref)
    {
        EveStack.get_ptr(ref)->init();
    }
    // Reset the eve stack "pointer".
    EveStack.count = 0;
}

bool EveStack_release_one(const EVE_REF ref)
{
    if (!EveStack.isValidRange(ref)) return false;
    eve_t *eve = EveStack.get_ptr(ref);
    if (eve->loaded) eve->init();
    return true;
}

EVE_REF EveStack_get_free()
{
    int retval = INVALID_EVE_REF;

    if (EveStack.count < MAX_EVE)
    {
        retval = EveStack.count;
        EveStack.count++;
    }

    return CLIP(retval, 0, MAX_EVE);
}
