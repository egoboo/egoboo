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

#include "../egolib/fileutil.h"
#include "../egolib/_math.inl"

#include "graphic_texture.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, oglx_texture_t, TxList, TX_COUNT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void TxList_reset_freelist( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( oglx_texture_t, TxList, TX_COUNT );

//--------------------------------------------------------------------------------------------
// TxList implementation
//--------------------------------------------------------------------------------------------
void TxList_reset_freelist( void )
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
void TxList_init_all( void )
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_ctor( TxList.lst + cnt );
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_release_all( void )
{
    /// @author ZZ
    /// @details This function releases all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_Release( TxList.lst + cnt );
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_delete_all( void )
{
    /// @author ZZ
    /// @details This function clears out all of the textures

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_dtor( TxList.lst + cnt );
    }

    TxList_reset_freelist();
}

//--------------------------------------------------------------------------------------------
void TxList_reload_all( void )
{
    /// @author ZZ
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    TX_REF cnt;

    for ( cnt = 0; cnt < TX_COUNT; cnt++ )
    {
        oglx_texture_t * ptex = TxList.lst + cnt;

        if ( oglx_texture_Valid( ptex ) )
        {
            oglx_texture_Convert( ptex, ptex->surface, INVALID_KEY );
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
        oglx_texture_Release( TxList.lst + itex );
    }
    else if ( !VALID_TX_RANGE( itex ) )
    {
        if ( TxList.free_count > 0 )
        {
            TxList.free_count--;
            TxList.update_guid++;

            retval = TxList.free_ref[TxList.free_count];
        }
        else
        {
            retval = INVALID_TX_REF;
        }
    }
    else
    {
        int i;

        // grab the specified index
        oglx_texture_Release( TxList.lst + ( TX_REF )itex );

        // if this index is on the free stack, remove it
        for ( i = 0; i < TxList.free_count; i++ )
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
bool_t TxList_free_one( const TX_REF itex )
{
    if ( itex < 0 || itex >= TX_COUNT ) return bfalse;

    // release the texture
    oglx_texture_Release( TxList.lst + itex );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this texture is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < TxList.free_count; cnt++ )
        {
            if ( itex == TxList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    if ( TxList.free_count >= TX_COUNT )
        return bfalse;

    // do not put anything below TX_SPECIAL_LAST back onto the free stack
    if ( itex >= TX_SPECIAL_LAST )
    {
        TxList.free_ref[TxList.free_count] = REF_TO_INT( itex );

        TxList.free_count++;
        TxList.update_guid++;
    }

    return btrue;
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
        Uint32 txid = ego_texture_load_vfs( TxList.lst + retval, filename, key );
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
    oglx_texture_t * ptex = TxList_get_ptr( itex );

    if ( !oglx_texture_Valid( ptex ) )
    {
        return NULL;
    }

    return ptex;
}
