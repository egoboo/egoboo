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

/// @file  game/rpc.c
/// @brief The implementation of the Remote Procedure Calls.
/// @details

#include "egolib/egolib.h"
#include "game/rpc.h"
#include "game/graphic_texture.h"
#include "game/menu.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_TX_REQ 100

#define INVALID_TX_REQ_IDX MAX_TX_REQ
#define INVALID_TX_REQ_REF ((TX_REF)INVALID_TX_REQ_IDX)

#define VALID_TX_REQ_RANGE(VAL) ( ((VAL)>=0) && ((VAL) < MAX_TX_REQ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_LIST_STATIC( tx_request_t, TxReqList, MAX_TX_REQ );

static bool _rpc_system_initialized = false;
static int    _rpc_system_guid;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void   TxReqList_ctor();
static void   TxReqList_dtor();
static bool TxReqList_timestep();
static size_t TxReqList_get_free_ref( int type );
static bool TxReqList_free_one( int index );

static int ego_rpc_system_get_guid();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( tx_request_t, TxReqList, MAX_TX_REQ );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_rpc_base_t * ego_rpc_base_ctor( ego_rpc_base_t * prpc, int data_type, void * data )
{
    /// @author BB
    /// @details construct the somple rpc data element

    if ( ego_rpc_valid( prpc ) ) return NULL;

    prpc->allocated = true;
    prpc->finished  = false;
    prpc->abort     = false;

    prpc->guid      = ego_rpc_system_get_guid();
    prpc->data_type = data_type;
    prpc->data      = data;

    return prpc;
}

//--------------------------------------------------------------------------------------------
ego_rpc_base_t * ego_rpc_base_dtor( ego_rpc_base_t * prpc )
{
    /// @author BB
    /// @details deconstruct the somple rpc data element

    if ( NULL == prpc ) return NULL;
    if ( !ego_rpc_valid( prpc ) ) return prpc;

    BLANK_STRUCT_PTR( prpc )

    return prpc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
tx_request_t * tx_request_ctor( tx_request_t * preq, int type )
{
    if ( NULL == preq ) return NULL;

    if ( NULL == ego_rpc_base_ctor( &( preq->ego_rpc_base ), type, preq ) ) return NULL;

    preq->filename[0] = CSTR_END;
    preq->index       = INVALID_TX_REF;
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
    BLANK_STRUCT_PTR( preq )

    // restore the deconstructed base
    memcpy( &save_base, &( preq->ego_rpc_base ), sizeof( save_base ) );

    return preq;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool ego_rpc_system_begin()
{
    /// @author BB
    /// @details initialize all the rpc arrays here

    if ( _rpc_system_initialized ) return true;

    TxReqList_ctor();

    _rpc_system_initialized = true;

    return _rpc_system_initialized;
}

//--------------------------------------------------------------------------------------------
void ego_rpc_system_end()
{
    /// @author BB
    /// @details de-initialize all the rpc arrays here

    if ( !_rpc_system_initialized ) return;

    TxReqList_dtor();

    _rpc_system_initialized = false;
}

//--------------------------------------------------------------------------------------------
bool ego_rpc_system_timestep()
{
    /// @author BB
    /// @details step through a single request of each type

    if ( !ego_rpc_system_begin() ) return false;

    return true;
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
        TxReqList.free_ref[cnt] = INVALID_TX_REQ_IDX;
        TxReqList.used_ref[cnt] = INVALID_TX_REQ_IDX;
    }

    for ( cnt = 0; cnt < MAX_TX_REQ; cnt++ )
    {
        tx_request_t * preq = TxReqList.lst + cnt;

        // blank out all the data, including the obj_base data
        BLANK_STRUCT_PTR( preq )

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

    for ( cnt = 0; cnt < MAX_TX_REQ; cnt++ )
    {
        // character "constructor"
        tx_request_dtor( TxReqList.lst + cnt );
    }

    TxReqList.free_count = 0;
    TxReqList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        TxReqList.free_ref[cnt] = INVALID_TX_REQ_IDX;
        TxReqList.used_ref[cnt] = INVALID_TX_REQ_IDX;
    }
}

//--------------------------------------------------------------------------------------------
size_t TxReqList_get_free_ref( int type )
{
    /// @author ZZ
    /// @details This function returns the next free index or INVALID_TX_REQ_IDX if there are none

    size_t retval = INVALID_TX_REQ_IDX;
    size_t loops  = 0;

    while ( TxReqList.free_count > 0 )
    {
        TxReqList.free_count--;
        TxReqList.update_guid++;

        retval = TxReqList.free_ref[TxReqList.free_count];

        if ( VALID_TX_REQ_RANGE( retval ) )
        {
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %d loops.\n", __FUNCTION__, loops );
    }

    if ( VALID_TX_REQ_RANGE( retval ) )
    {
        tx_request_ctor( TxReqList.lst + retval, type );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool TxReqList_free_one( int ireq )
{
    /// @author ZZ
    /// @details

    bool         retval;
    tx_request_t * preq;

    preq = TxReqList_get_ptr( ireq );
    if ( NULL == preq ) return false;

    // destruct the request
    tx_request_dtor( preq );

#if defined(_DEBUG)
    {
        int cnt;
        // determine whether this character is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < TxReqList.free_count; cnt++ )
        {
            if ( ireq == TxReqList.free_ref[cnt] ) return false;
        }
    }
#endif

    // push it on the free stack
    retval = false;
    if ( TxReqList.free_count < MAX_TX_REQ )
    {
        TxReqList.free_ref[TxReqList.free_count] = ireq;

        TxReqList.free_count++;
        TxReqList.update_guid++;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool TxReqList_timestep()
{
    /// @author BB
    /// @details TxReqList_timestep() is called by the main thread.
    ///               Requests are submitted by worker threads

    // take off the back of the list
    tx_request_t * preq;
    int index;
    bool retval;

    // grab the index the 1st ting
    index = TxReqList.used_count - 1;

    // ??lock the list??
    preq = TxReqList_get_ptr( index );
    if ( NULL == preq ) return false;

    if ( preq->ego_rpc_base.abort )
    {
        TxReqList_free_one( preq->ego_rpc_base.index );
        return true;
    }

    retval = false;
    switch ( preq->ego_rpc_base.data_type )
    {
        case 1:
            // TxList_load_one_vfs()
            preq->index = TxList_load_one_vfs( preq->filename, preq->itex_src, preq->key );
            preq->ego_rpc_base.finished = true;
            retval = true;
            break;

        case 2:
            // mnu_TxList_load_one_vfs()
            preq->index = mnu_TxList_load_one_vfs( preq->filename, preq->itex_src, preq->key );
            preq->ego_rpc_base.finished = true;
            retval = true;
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
tx_request_t * ego_rpc_load_TxList( const char *filename, int itex_src, Uint32 key )
{
    /// @author BB
    /// @details request that the main thread loads the texture

    tx_request_t * preq;
    size_t index;

    // find a free request for TxList (type 1)
    index = TxReqList_get_free_ref( 1 );

    preq = TxReqList_get_ptr( index );
    if ( NULL == preq ) return NULL;

    // fill in the data
    strncpy( preq->filename, filename, SDL_arraysize( preq->filename ) );
    preq->itex_src = itex_src;
    preq->key      = key;

    return preq;
}

//--------------------------------------------------------------------------------------------
tx_request_t * ego_rpc_load_mnu_TxList( const char *filename )
{
    /// @author BB
    /// @details request that the main thread loads the texture

    tx_request_t * preq;
    size_t index;

    // find a free request for mnu_TxList (type 2)
    index = TxReqList_get_free_ref( 2 );

    preq = TxReqList_get_ptr( index );
    if ( NULL == preq ) return NULL;

    // fill in the data
    strncpy( preq->filename, filename, SDL_arraysize( preq->filename ) );

    return preq;
}
