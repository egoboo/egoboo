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

/// @file game/texture.c
/// @brief Implementation of code for controlling object texturing
/// @details

#include "egolib/_math.h"
#include "game/graphic_texture.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

TextureManager::TextureManager()
    : _lst(), _free()
{
    // Fill the _free set with all texture references
    // from TX_SPECIAL_LAST (inclusive) to TEXTURES_MAX (exclusive).
    for (TX_REF ref = TX_SPECIAL_LAST; ref < TEXTURES_MAX; ++ref)
    {
        _free.insert(ref);
    }

    // Initialize the actual list of textures.
    for (TX_REF ref = 0; ref < TEXTURES_MAX; ++ref)
    {
        oglx_texture_t *texture = nullptr;
        try
        {
            texture = oglx_texture_t::create();
        }
        catch (std::exception& ex)
        {
            while (ref > 0)
            {
                oglx_texture_t::destroy(_lst[--ref]);
                _lst[ref] = nullptr;
            }
            throw std::bad_alloc();
        }
        _lst[ref] = texture;
    }
}

TextureManager::~TextureManager()
{
    for (TX_REF ref = 0; ref < TEXTURES_MAX; ++ref)
    {
        oglx_texture_t::dtor(_lst[ref]);
        free(_lst[ref]);
        _lst[ref] = nullptr;
    }
}

void TextureManager::freeAll()
{
    // Fill the _free set with all texture references
    // from TX_SPECIAL_LAST (inclusive) to TEXTURES_MAX (exclusive).
    for (TX_REF ref = TX_SPECIAL_LAST; ref < TEXTURES_MAX; ++ref)
    {
        _free.insert(ref);
    }
}

void TextureManager::release_all()
{
    for (TX_REF ref = 0; ref < TEXTURES_MAX; ++ref)
    {
        oglx_texture_t::release(_lst[ref]);
    }

    freeAll();
}

void TextureManager::reload_all()
{
    for (TX_REF ref = 0; ref < TEXTURES_MAX; ++ref)
    {
        oglx_texture_t *texture = _lst[ref];

        if (oglx_texture_Valid(texture))
        {
            oglx_texture_t::convert(texture, texture->surface, INVALID_KEY);
        }
    }
}

TX_REF TextureManager::acquire(const TX_REF ref)
{
    if (ref >= 0 && ref < TX_SPECIAL_LAST)
    {
        oglx_texture_t::release(_lst[ref]);
        return ref;
    }
    else if (!VALID_TX_RANGE(ref))
    {
        auto head = _free.begin();
        if (head == _free.end())
        {
            return INVALID_TX_REF;
        }
        TX_REF newRef = *head;
        _free.erase(head);
        return newRef;
    }
    else
    {
        // Release the texture under the specified reference.
        oglx_texture_t::release(_lst[ref]);
        // Remove this reference from the free set.
        _free.erase(_free.find(ref));
        return ref;
    }
}

//--------------------------------------------------------------------------------------------
bool TextureManager::relinquish(const TX_REF ref)
{
    if (ref < 0 || ref >= TEXTURES_MAX) return false;

    // Release the texture.
    oglx_texture_t::release(_lst[ref]);

    // Do not put anything below TX_SPECIAL_LAST back onto the free stack.
    if (ref >= TX_SPECIAL_LAST)
    {
        _free.insert(ref);
}

    return true;
}

TX_REF TextureManager::load(const char *filename, const TX_REF ref, Uint32 key)
{
    /// @author BB
    /// @details load a texture into TxList.
    ///     If INVALID_TX_IDX == ref, then we just get the next free index

    // Acquire a texture reference.
    TX_REF newRef = acquire(ref);
    // If no texture reference is available ...
    if (!VALID_TX_RANGE(newRef))
    {
        // ... return INVALID_TX_REF.
        return INVALID_TX_REF;
    }

    // Otherwise load the texture.
    Uint32 id = ego_texture_load_vfs(_lst[newRef], filename, key);
    // If loading fails ...
    if (INVALID_GL_ID == id)
    {
        // ... relinquish the reference and ...
        relinquish(newRef);
        // ... return INVALID_TX_REF.
        return INVALID_TX_REF;
    }

    return newRef;
}

oglx_texture_t *TextureManager::get_valid_ptr(const TX_REF ref)
{
    oglx_texture_t *texture = LAMBDA(ref >= TEXTURES_MAX, nullptr, _lst[ref]);
    if (!oglx_texture_Valid(texture))
    {
        return nullptr;
    }
    return texture;
}
