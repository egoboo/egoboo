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
/// @brief
/// @details

#include "texture.h"

#include "egoboo_fileutil.h"
#include "egoboo_math.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

DECLARE_LIST ( ACCESS_TYPE_NONE, oglx_texture, TxTexture );

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void TxTexture_clear_data()
{
    /// @details BB@> reset the free texture list. Start at TX_LAST so that the global textures/icons are
    ///     can't be allocated by mistake

    int cnt, tnc;

    for ( cnt = TX_LAST, tnc = 0; cnt < TEXTURE_COUNT; cnt++, tnc++ )
    {
        TxTexture.free_ref[tnc] = cnt;
    }
    TxTexture.free_count = tnc;
}

//---------------------------------------------------------------------------------------------
void TxTexture_init_all()
{
    /// @details ZZ@> This function clears out all of the textures

    int cnt;

    for ( cnt = 0; cnt < TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_new( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//---------------------------------------------------------------------------------------------
void TxTexture_release_all()
{
    /// @details ZZ@> This function releases all of the textures

    int cnt;

    for ( cnt = 0; cnt < TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_Release( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//---------------------------------------------------------------------------------------------
void TxTexture_delete_all()
{
    /// @details ZZ@> This function clears out all of the textures

    int cnt;

    for ( cnt = 0; cnt < TEXTURE_COUNT; cnt++ )
    {
        oglx_texture_delete( TxTexture.lst + cnt );
    }

    TxTexture_clear_data();
}

//---------------------------------------------------------------------------------------------
int TxTexture_get_free( int itex )
{

    if ( itex >= 0 && itex < TX_LAST )
    {
        oglx_texture_Release( TxTexture.lst + itex );
    }
    else if ( itex < 0 || itex >= TEXTURE_COUNT )
    {
        if ( TxTexture.free_count > 0 )
        {
            TxTexture.free_count--;
            itex = TxTexture.free_ref[TxTexture.free_count];
        }
        else
        {
            itex = INVALID_TEXTURE;
        }
    }
    else
    {
        int i;

        // grab the specified index
        oglx_texture_Release( TxTexture.lst + itex );

        // if this index is on the free stack, remove it
        for ( i = 0; i < TxTexture.free_count; i++ )
        {
            if ( TxTexture.free_ref[i] == itex )
            {
                if ( TxTexture.free_count > 0 )
                {
                    TxTexture.free_count--;
                    SWAP(int, TxTexture.free_ref[i], TxTexture.free_ref[TxTexture.free_count] );
                }
                break;
            }
        }
    }

    return itex;
}

//---------------------------------------------------------------------------------------------
bool_t TxTexture_free_one( int itex )
{
    if ( itex < 0 || itex >= TEXTURE_COUNT ) return bfalse;

    // release the texture
    oglx_texture_Release( TxTexture.lst + itex );

#if defined(USE_DEBUG)
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

    if ( TxTexture.free_count >= TEXTURE_COUNT )
        return bfalse;

    // do not put anything below TX_LAST back onto the SDL_free stack
    if ( itex >= TX_LAST )
    {
        TxTexture.free_ref[TxTexture.free_count] = itex;
        TxTexture.free_count++;
    }

    return btrue;
}

//---------------------------------------------------------------------------------------------
int TxTexture_load_one( const char *filename, int itex_src, Uint32 key )
{
    /// @details BB@> load a texture into TxTexture.
    ///     If INVALID_TEXTURE == itex, then we just get the next free index

    int    itex;

    // get a texture index.
    itex = TxTexture_get_free(itex_src);

    // handle an error
    if ( itex >= 0 && itex < TEXTURE_COUNT )
    {
        Uint32 txid = ego_texture_load( TxTexture.lst + itex, filename, key );
        if ( INVALID_TX_ID == txid )
        {
            TxTexture_free_one(itex);
            itex = INVALID_TEXTURE;
        }
    }

    return itex;
}

//--------------------------------------------------------------------------------------------
oglx_texture * TxTexture_get_ptr( int itex )
{
    oglx_texture * ptex;

    if ( itex < 0 || itex >= TEXTURE_COUNT )
        return NULL;
    ptex = TxTexture.lst + itex;

    if ( !oglx_texture_Valid(ptex) )
        return NULL;

    return ptex;
}
