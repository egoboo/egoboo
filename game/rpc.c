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

/// @file rpc.c
/// @brief The implementation of the Remote Procedure Calls
/// @details

#include "rpc.h"

#include "texture.h"
#include "menu.h"

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_TX_TEXTURE_REQ 100

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_LIST_STATIC( tx_request_t, TxReqList, MAX_TX_TEXTURE_REQ );

static bool_t _rpc_system_initialized = bfalse;
static int    _rpc_system_guid;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void   TxReqList_ctor();
static void   TxReqList_dtor();
static bool_t TxReqList_timestep();
static size_t TxReqList_get_free( int type );
static bool_t TxReqList_free_one( int index );

static int ego_rpc_system_get_guid();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_rpc_base_t * ego_rpc_base_ctor( ego_rpc_base_t * prpc, int data_type, void * data )
{
    /// @details BB@> construct the somple rpc data element

    if ( ego_rpc_valid( prpc ) ) return NULL;

    prpc->allocated = btrue;
    prpc->finished  = bfalse;
    prpc->abort     = bfalse;

    prpc->guid      = ego_rpc_system_get_guid();
    prpc->data_type = data_type;
    prpc->data      = data;

    return prpc;
}

//--------------------------------------------------------------------------------------------
ego_rpc_base_t * ego_rpc_base_dtor( ego_rpc_base_t * prpc )
{
    /// @details BB@> deconstruct the somple rpc data element

    if ( NULL == prpc ) return NULL;
    if ( !ego_rpc_valid( prpc ) ) return prpc;

    memset( prpc, 0, sizeof( *prpc ) );

    return prpc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
tx_request_t * tx_request_ctor( tx_request_t * preq, int type )
{
    if ( NULL == preq ) return NULL;

    if ( NULL == ego_rpc_base_ctor( &( preq->ego_rpc_base ), type, preq ) ) return NULL;

    preq->filename[0] = CSTR_END;
    preq->index       = ( TX_REF )INVALID_TX_TEXTURE;
    preq->key         = ( Uint32 )( ~0 );

    return preq;
}

//--------------------------------------------------------------------------------------------
tx_request_t * tx_request_dtor( tx_request_t * preq )
{
    ego_rpc_base_t save_base;

    if ( NULL == preq ) return NULL;

    if ( NULL == ego_rpc_base_dtor( &( preq->ego_rpc_base ) ) ) return NULL;

    // store the deconstructed base
    memcpy( &save_base, &( preq->ego_rpc_base ), sizeof( save_base ) );

    // zro out the memory
    memset( preq, 0, sizeof( *preq ) );

    // restore the deconstructed base
    memcpy( &save_base, &( preq->ego_rpc_base ), sizeof( save_base ) );

    return preq;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_rpc_system_begin()
{
    /// @details BB@> initialize all the rpc arrays here

    if ( _rpc_system_initialized ) return btrue;

    TxReqList_ctor();

    _rpc_system_initialized = btrue;

    return _rpc_system_initialized;
}

//--------------------------------------------------------------------------------------------
void ego_rpc_system_end()
{
    /// @details BB@> de-initialize all the rpc arrays here

    if ( !_rpc_system_initialized ) return;

    TxReqList_dtor();

    _rpc_system_initialized = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t ego_rpc_system_timestep()
{
    /// @details BB@> step through a single request of each type

    if ( !ego_rpc_system_begin() ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
int ego_rpc_system_get_guid()
{
    return ++_rpc_system_guid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void TxReqList_ctor()
{
    TREQ_REF cnt;

    TxReqList.free_count = 0;
    TxReqList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        TxReqList.free_ref[cnt] = MAX_TX_TEXTURE_REQ;
        TxReqList.used_ref[cnt] = MAX_TX_TEXTURE_REQ;
    }

    for ( cnt = 0; cnt < MAX_TX_TEXTURE_REQ; cnt++ )
    {
        tx_request_t * preq = TxReqList.lst + cnt;

        // blank out all the data, including the obj_base data
        memset( preq, 0, sizeof( *preq ) );

        // tx_request "constructor" (type -1 == type unknown)
        tx_request_ctor( preq, -1 );

        // set the index
        preq->ego_rpc_base.index = REF_TO_INT( cnt );

        // push the characters onto the free stack
        TxReqList.free_ref[TxReqList.free_count] = TxReqList.free_count;

        TxReqList.free_count++;
        TxReqList.update_guid++;
    }
}

//--------------------------------------------------------------------------------------------
void TxReqList_dtor()
{
    TREQ_REF cnt;

    for ( cnt = 0; cnt < MAX_TX_TEXTURE_REQ; cnt++ )
    {
        // character "constructor"
        tx_request_dtor( TxReqList.lst + cnt );
    }

    TxReqList.free_count = 0;
    TxReqList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        TxReqList.free_ref[cnt] = MAX_TX_TEXTURE_REQ;
        TxReqList.used_ref[cnt] = MAX_TX_TEXTURE_REQ;
    }
}

//--------------------------------------------------------------------------------------------
size_t TxReqList_get_free( int type )
{
    /// @details ZZ@> This function returns the next free index or MAX_TX_TEXTURE_REQ if there are none

    size_t retval = MAX_TX_TEXTURE_REQ;

    if ( TxReqList.free_count > 0 )
    {
        TxReqList.free_count--;
        TxReqList.update_guid++;

        retval = TxReqList.free_ref[TxReqList.free_count];
    }

    if ( retval >= 0 && retval < MAX_TX_TEXTURE_REQ )
    {
        tx_request_ctor( TxReqList.lst + ( TREQ_REF )retval, type );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t TxReqList_free_one( int ireq )
{
    /// @details ZZ@>

    bool_t         retval;
    tx_request_t * preq;

    if ( ireq < 0 || ireq >= MAX_TX_TEXTURE_REQ ) return bfalse;
    preq = TxReqList.lst + ( TREQ_REF )ireq;

    // destruct the request
    tx_request_dtor( preq );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this character is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < TxReqList.free_count; cnt++ )
        {
            if ( ireq == TxReqList.free_ref[cnt] ) return bfalse;
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( TxReqList.free_count < MAX_TX_TEXTURE_REQ )
    {
        TxReqList.free_ref[TxReqList.free_count] = ireq;

        TxReqList.free_count++;
        TxReqList.update_guid++;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t TxReqList_timestep()
{
    /// @details BB@> TxReqList_timestep() is called by the main thread.
    ///               Requests are submitted by worker threads

    // take off the back of the list
    tx_request_t * preq;
    int index;
    bool_t retval;

    // grab the index the 1st ting
    index = TxReqList.used_count - 1;
    if ( index < 0 ) return bfalse;
    preq = TxReqList.lst + ( TREQ_REF )index;

    // ??lock the list??

    if ( preq->ego_rpc_base.abort )
    {
        TxReqList_free_one( preq->ego_rpc_base.index );
        return btrue;
    }

    retval = bfalse;
    switch ( preq->ego_rpc_base.data_type )
    {
        case 1:
            // TxTexture_load_one_vfs()
            preq->index = TxTexture_load_one_vfs( preq->filename, preq->itex_src, preq->key );
            preq->ego_rpc_base.finished = btrue;
            retval = btrue;
            break;

        case 2:
            // TxTitleImage_load_one_vfs()
            preq->index = TxTitleImage_load_one_vfs( preq->filename );
            preq->ego_rpc_base.finished = btrue;
            retval = btrue;
            break;

        default:
            TxReqList_free_one( preq->ego_rpc_base.index );
            break;
    }

    // ??unlock the list??

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
tx_request_t * ego_rpc_load_TxTexture( const char *filename, int itex_src, Uint32 key )
{
    /// @details BB@> request that the main thread loads the texture

    tx_request_t * preq;
    size_t index;

    // find a free request for TxTexture (type 1)
    index = TxReqList_get_free( 1 );
    if ( index == MAX_TX_TEXTURE_REQ ) return NULL;
    preq = TxReqList.lst + ( TREQ_REF )index;

    // fill in the data
    strncpy( preq->filename, filename, SDL_arraysize( preq->filename ) );
    preq->itex_src = itex_src;
    preq->key      = key;

    return preq;
}

//--------------------------------------------------------------------------------------------
tx_request_t * ego_rpc_load_TxTitleImage( const char *filename )
{
    /// @details BB@> request that the main thread loads the texture

    tx_request_t * preq;
    size_t index;

    // find a free request for TxTitleImage (type 2)
    index = TxReqList_get_free( 2 );
    if ( index == MAX_TX_TEXTURE_REQ ) return NULL;
    preq = TxReqList.lst + ( TREQ_REF )index;

    // fill in the data
    strncpy( preq->filename, filename, SDL_arraysize( preq->filename ) );

    return preq;
}
