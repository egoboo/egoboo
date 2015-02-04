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

TxListTy TxList;



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void TxList_reset_freelist();

//--------------------------------------------------------------------------------------------
// TxList implementation
//--------------------------------------------------------------------------------------------
void TxList_reset_freelist()
{
    /// @author BB
    /// @details reset the free texture list. Start at TX_SPECIAL_LAST so that the global textures/icons are
    ///     can't be allocated by mistake

    int cnt, tnc;

    for ( cnt = TX_SPECIAL_LAST, tnc = 0; cnt < TX_COUNT; cnt++, tnc++ )
    {
        TxList.free_ref[tnc] = cnt;
    }
    TxList.free_count = tnc;
}

//--------------------------------------------------------------------------------------------
void TxList_init_all()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

	for (TX_REF cnt = 0; cnt < TX_COUNT; cnt++)
    {
		oglx_texture_t *texture = (oglx_texture_t *)malloc(sizeof(oglx_texture_t));
		oglx_texture_t::ctor(texture);
		TxList.lst[cnt] = texture;
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_release_all()
{
    /// @author ZZ
    /// @details This function releases all of the textures

    for (TX_REF cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_release( TxList.lst[cnt] );
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_delete_all()
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    for (TX_REF cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_t::dtor( TxList.lst[cnt] );
		free(TxList.lst[cnt]);
		TxList.lst[cnt] = nullptr;
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_reload_all()
{
    /// @author BB
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

	for (TX_REF ref = 0; ref < TX_COUNT; ++ref)
    {
        oglx_texture_t *texture = TxList.lst[ref];

        if (oglx_texture_Valid(texture))
        {
            oglx_texture_convert(texture, texture->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
TX_REF TxList_get_free( const TX_REF itex )
{
    TX_REF retval = INVALID_TX_REF;

    if ( itex >= 0 && itex < TX_SPECIAL_LAST )
    {
        retval = itex;
        oglx_texture_release( TxList.lst[itex] );
    }
    else if ( !VALID_TX_RANGE( itex ) )
    {
        if ( TxList.free_count > 0 )
        {
            TxList.free_count--;
            TxList.update_guid++;

            retval = ( TX_REF )TxList.free_ref[TxList.free_count];
        }
        else
        {
            retval = INVALID_TX_REF;
        }
    }
    else
    {
        // grab the specified index
        oglx_texture_release(TxList.lst[(TX_REF)itex]);

        // if this index is on the free stack, remove it
        for (int i = 0; i < TxList.free_count; i++ )
        {
            if ( TxList.free_ref[i] == itex )
            {
                if ( TxList.free_count > 0 )
                {
                    TxList.free_count--;
                    TxList.update_guid++;

                    SWAP( size_t, TxList.free_ref[i], TxList.free_ref[TxList.free_count] );
                }
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool TxList_free_one( const TX_REF itex )
{
    if ( itex < 0 || itex >= TX_COUNT ) return false;

    // release the texture
    oglx_texture_release(TxList.lst[itex]);

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < TxList.free_count; cnt++ )
        {
            if ( itex == TxList.free_ref[cnt] ) return false;
        }
    }
#endif

    if ( TxList.free_count >= TX_COUNT )
        return false;

    // do not put anything below TX_SPECIAL_LAST back onto the free stack
    if ( itex >= TX_SPECIAL_LAST )
    {
        TxList.free_ref[TxList.free_count] = REF_TO_INT( itex );

        TxList.free_count++;
        TxList.update_guid++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
TX_REF TxList_load_one_vfs( const char *filename, const TX_REF itex_src, Uint32 key )
{
    /// @author BB
    /// @details load a texture into TxList.
    ///     If INVALID_TX_IDX == itex, then we just get the next free index

    TX_REF retval;

    // get a texture index.
    retval = TxList_get_free( itex_src );

    // handle an error
    if ( VALID_TX_RANGE( retval ) )
    {
        Uint32 txid = ego_texture_load_vfs( TxList.lst[retval], filename, key );
        if ( INVALID_GL_ID == txid )
        {
            TxList_free_one( retval );
            retval = INVALID_TX_REF;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

oglx_texture_t * TxList_get_valid_ptr( const TX_REF itex )
{
	oglx_texture_t *texture = LAMBDA(itex >= TX_COUNT, nullptr, TxList.lst[itex]);
    if (!oglx_texture_Valid(texture))
    {
        return nullptr;
    }
    return texture;
}
