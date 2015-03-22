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

TextureManager *TextureManager::_singleton = nullptr;

TextureManager::TextureManager()
    : _lst(), _free()
{
    // Fill the _free set with all texture references
    // from TX_SPECIAL_LAST (inclusive) to TX_COUNT (exclusive).
    for (TX_REF ref = TX_SPECIAL_LAST; ref < TX_COUNT; ++ref)
    {
        _free.insert(ref);
    }
	
    // Initialize the actual list of textures.
	for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
	{
        oglx_texture_t *texture;
        try
        {
            texture = static_cast<oglx_texture_t *>(malloc(sizeof(oglx_texture_t)));
            if (!texture)
            {
                throw std::bad_alloc();
            }
            if (!oglx_texture_t::ctor(texture))
            {
                free(texture);
                throw std::bad_alloc();
            }
        }
        catch (std::exception& ex)
        {
            while (ref > 0)
            {
                oglx_texture_t::dtor(_lst[--ref]);
                free(_lst[ref]);
                _lst[ref] = nullptr;
            }
            throw std::bad_alloc();
        }
        _lst[ref] = texture;
    }
}

TextureManager::~TextureManager()
{
    for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
    {
        oglx_texture_t::dtor(_lst[ref]);
        free(_lst[ref]);
        _lst[ref] = nullptr;
    }
}

void TextureManager::freeAll()
{
    // Fill the _free set with all texture references
    // from TX_SPECIAL_LAST (inclusive) to TX_COUNT (exclusive).
    for (TX_REF ref = TX_SPECIAL_LAST; ref < TX_COUNT; ++ref)
    {
        _free.insert(ref);
    }
}

TextureManager& TextureManager::get()
{
    if (!_singleton)
    {
        throw std::logic_error("texture manager not initialized");
    }
	return *_singleton;
}

void TextureManager::initialize()
{
    if (_singleton)
    {
        log_warning("%s:%d: texture manager already initialized - ignoring\n", __FILE__, __LINE__);
        return;
    }
    _singleton = new TextureManager();
}

void TextureManager::uninitialize()
{
    if (!_singleton)
    {
        log_warning("%s:%d: texture manager not initialized - ignoring\n", __FILE__, __LINE__);
        return;
    }
    delete _singleton;
    _singleton = nullptr;
}

void TextureManager::release_all()
{
	for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
	{
		oglx_texture_release(_lst[ref]);
	}

	freeAll();
}

void TextureManager::reload_all()
{
	for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
    {
        oglx_texture_t *texture = _lst[ref];

        if (oglx_texture_Valid(texture))
        {
            oglx_texture_convert(texture, texture->surface, INVALID_KEY );
        }
    }
}

TX_REF TextureManager::acquire(const TX_REF ref)
{
    if (ref >= 0 && ref < TX_SPECIAL_LAST)
    {
        oglx_texture_release(_lst[ref]);
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
        oglx_texture_release(_lst[ref]);
        // Remove this reference from the free set.
        _free.erase(_free.find(ref));
        return ref;
    }
}

//--------------------------------------------------------------------------------------------
bool TextureManager::relinquish(const TX_REF ref)
{
    if (ref < 0 || ref >= TX_COUNT) return false;

    // Release the texture.
    oglx_texture_release(_lst[ref]);
#if 0
#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < free_count; cnt++ )
        {
            if ( itex == free_ref[cnt] ) return false;
        }
    }
#endif
#endif
#if 0
 	if (free_count >= TX_COUNT)
	{
		return false;
	}
#endif
    // Do not put anything below TX_SPECIAL_LAST back onto the free stack.
    if (ref >= TX_SPECIAL_LAST)
    {
        _free.insert(ref);
#if 0
        free_ref[free_count] = REF_TO_INT( itex );

        free_count++;
        update_guid++;
#endif
    }

    return true;
}

//--------------------------------------------------------------------------------------------
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
	oglx_texture_t *texture = LAMBDA(ref >= TX_COUNT, nullptr, _lst[ref]);
    if (!oglx_texture_Valid(texture))
    {
        return nullptr;
    }
    return texture;
}
