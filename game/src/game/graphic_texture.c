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

static TextureManager TxList;

//--------------------------------------------------------------------------------------------
// TxList implementation
//--------------------------------------------------------------------------------------------
TextureManager::TextureManager()
{
	update_guid = INVALID_UPDATE_GUID;
	used_count = 0;
	reset_freelist();
	for (TX_REF i = 0; i < TX_COUNT; i++)
	{
		lst[i] = nullptr;
	}

}

void TextureManager::reset_freelist()
{
	/// Reset the free texture list.
	/// Start at #TX_SPECIAL_LAST so that the global textures/icons are can't be allocated by mistake.
	size_t i, j;
	for (i = 0,j = TX_SPECIAL_LAST; j < TX_COUNT; i++, j++)
	{
		TxList.free_ref[i] = j;
	}
	TxList.free_count = i;
}
//--------------------------------------------------------------------------------------------
TextureManager *TextureManager::getSingleton()
{
	return &TxList;
}
//--------------------------------------------------------------------------------------------
void TextureManager::startUp()
{
	for (TX_REF i = 0; i < TX_COUNT; i++)
    {
		oglx_texture_t *texture = (oglx_texture_t *)malloc(sizeof(oglx_texture_t));
		oglx_texture_t::ctor(texture);
		TxList.lst[i] = texture;
    }

    TxList.reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TextureManager::shutDown()
{
    for (TX_REF i = 0; i < TX_COUNT; ++i)
    {
        oglx_texture_t::dtor(TxList.lst[i]);
		free(TxList.lst[i]);
		TxList.lst[i] = nullptr;
    }
    TxList.reset_freelist();
}
//--------------------------------------------------------------------------------------------
void TextureManager::release_all()
{
	for (TX_REF cnt = 0; cnt < TX_COUNT; cnt++)
	{
		oglx_texture_release(TxList.lst[cnt]);
	}

	reset_freelist();
}
//--------------------------------------------------------------------------------------------
void TextureManager::reload_all()
{
    /// @author BB
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

	for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
    {
        oglx_texture_t *texture = lst[ref];

        if (oglx_texture_Valid(texture))
        {
            oglx_texture_convert(texture, texture->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
TX_REF TextureManager::acquire(const TX_REF itex)
{
    TX_REF newitex = INVALID_TX_REF;

    if (itex >= 0 && itex < TX_SPECIAL_LAST)
    {
		newitex = itex;
        oglx_texture_release(lst[itex]);
    }
    else if (!VALID_TX_RANGE(itex))
    {
        if (free_count > 0)
        {
            free_count--;
            update_guid++;
			newitex = (TX_REF)free_ref[free_count];
        }
        else
        {
			newitex = INVALID_TX_REF;
        }
    }
    else
    {
        // Grab the specified index.
        oglx_texture_release(lst[(TX_REF)itex]);

        // If this index is on the free stack, remove it.
        for (int i = 0; i < free_count; ++i)
        {
            if (free_ref[i] == itex)
            {
                if (free_count > 0)
                {
                    free_count--;
                    update_guid++;

                    SWAP(size_t, free_ref[i], free_ref[free_count]);
                }
                break;
            }
        }
    }

	return newitex;
}

//--------------------------------------------------------------------------------------------
bool TextureManager::relinquish(const TX_REF itex)
{
    if (itex < 0 || itex >= TX_COUNT) return false;

    // Release the texture.
    oglx_texture_release(lst[itex]);

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

	if (free_count >= TX_COUNT)
	{
		return false;
	}
    // Do not put anything below TX_SPECIAL_LAST back onto the free stack.
    if (itex >= TX_SPECIAL_LAST)
    {
        free_ref[free_count] = REF_TO_INT( itex );

        free_count++;
        update_guid++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
TX_REF TextureManager::load(const char *filename, const TX_REF itex_src, Uint32 key)
{
    /// @author BB
    /// @details load a texture into TxList.
    ///     If INVALID_TX_IDX == itex, then we just get the next free index

    TX_REF retval;

    // get a texture index.
	retval = TextureManager::acquire(itex_src);

    // handle an error
    if ( VALID_TX_RANGE( retval ) )
    {
        Uint32 txid = ego_texture_load_vfs( TxList.lst[retval], filename, key );
        if ( INVALID_GL_ID == txid )
        {
			TextureManager::relinquish(retval);
            retval = INVALID_TX_REF;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

oglx_texture_t *TextureManager::get_valid_ptr(const TX_REF itex)
{
	oglx_texture_t *texture = LAMBDA(itex >= TX_COUNT, nullptr, TxList.lst[itex]);
    if (!oglx_texture_Valid(texture))
    {
        return nullptr;
    }
    return texture;
}
