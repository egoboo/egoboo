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

/// @file texture.c
/// @brief Implementation of code for controlling object texturing
/// @details

#include "texture.h"

#include "egoboo_fileutil.h"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, oglx_texture_t, TxTexture, TX_TEXTURE_COUNT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void TxTexture_clear_data()
{
    /// @details BB@> reset the free texture list. Start at TX_LAST so that the global textures/icons are
    ///     can't be allocated by mistake

    int cnt, tnc;

    for ( cnt = TX_LAST, tnc = 0; cnt < TX_TEXTURE_COUNT; cnt++, tnc++ )
    {
        TxTexture.free_ref[tnc] = cnt;
    }
    TxTexture.free_count = tnc;
}

//--------------------------------------------------------------------------------------------
void TxTexture_init_all()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_ctor( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTexture_release_all()
{
    /// @details ZZ@> This function releases all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_Release( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTexture_delete_all()
{
    /// @details ZZ@> This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_dtor( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//--------------------------------------------------------------------------------------------
void TxTexture_reload_all()
{
    /// @details ZZ@> This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_t * ptex = TxTexture.lst + cnt;

        if ( ptex->valid )
        {
            oglx_texture_Convert( ptex, ptex->surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
TX_REF TxTexture_get_free( const TX_REF itex )
{
    TX_REF retval = ( TX_REF )INVALID_TX_TEXTURE;

    if ( itex >= 0 && itex < TX_LAST )
    {
        retval = itex;
        oglx_texture_Release( TxTexture.lst + itex );
    }
    else if ( itex < 0 || itex >= TX_TEXTURE_COUNT )
    {
        if ( TxTexture.free_count > 0 )
        {
            TxTexture.free_count--;
            TxTexture.update_guid++;

            retval = TxTexture.free_ref[TxTexture.free_count];
        }
        else
        {
            retval = ( TX_REF )INVALID_TX_TEXTURE;
        }
    }
    else
    {
        int i;

        // grab the specified index
        oglx_texture_Release( TxTexture.lst + ( TX_REF )itex );

        // if this index is on the free stack, remove it
        for ( i = 0; i < TxTexture.free_count; i++ )
        {
            if ( TxTexture.free_ref[i] == itex )
            {
                if ( TxTexture.free_count > 0 )
                {
                    TxTexture.free_count--;
                    TxTexture.update_guid++;

                    SWAP( size_t, TxTexture.free_ref[i], TxTexture.free_ref[TxTexture.free_count] );
                }
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t TxTexture_free_one( const TX_REF itex )
{
    if ( itex < 0 || itex >= TX_TEXTURE_COUNT ) return bfalse;

    // release the texture
    oglx_texture_Release( TxTexture.lst + itex );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < TxTexture.free_count; cnt++ )
        {
            if ( itex == TxTexture.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    if ( TxTexture.free_count >= TX_TEXTURE_COUNT )
        return bfalse;

    // do not put anything below TX_LAST back onto the SDL_free stack
    if ( itex >= TX_LAST )
    {
        TxTexture.free_ref[TxTexture.free_count] = REF_TO_INT( itex );

        TxTexture.free_count++;
        TxTexture.update_guid++;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
TX_REF TxTexture_load_one_vfs( const char *filename, const TX_REF itex_src, Uint32 key )
{
    /// @details BB@> load a texture into TxTexture.
    ///     If INVALID_TX_TEXTURE == itex, then we just get the next free index

    TX_REF retval;

    // get a texture index.
    retval = TxTexture_get_free( itex_src );

    // handle an error
    if ( retval >= 0 && retval < TX_TEXTURE_COUNT )
    {
        Uint32 txid = ego_texture_load_vfs( TxTexture.lst + retval, filename, key );
        if ( INVALID_GL_ID == txid )
        {
            TxTexture_free_one( retval );
            retval = INVALID_TX_TEXTURE;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * TxTexture_get_ptr( const TX_REF itex )
{
    oglx_texture_t * ptex;

    if ( itex < 0 || itex >= TX_TEXTURE_COUNT ) return NULL;
    ptex = TxTexture.lst + itex;

    if ( !oglx_texture_Valid( ptex ) )
        return NULL;

    return ptex;
}
