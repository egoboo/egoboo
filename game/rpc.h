#pragma once

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

/// @file rpc.h
/// @brief Definitions for the Remote Procedure Call module
///
/// @details This module allows inter-thread and network control of certain game functions.
/// The main use of this at the moment (since we have no server), is to make it possible
/// to do things like loading modules in a worker thread. All graphics functions must be
/// called from the main thread...

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a generic "remote procedure call" structure for handling "inter-thread" communication
struct s_rpc_base
{
    int    index;      ///< the index of this request
    int    guid;       ///< the request id number
    bool_t allocated;  ///< is this request being used?
    bool_t finished;   ///< has the request been fully processed?
    bool_t abort;      ///< has the calling function requested an abort?

    int    data_type;  ///< a the type of the "inherited" data
    void * data;       ///< a pointer to the "inherited" data
};
typedef struct s_rpc_base ego_rpc_base_t;

static INLINE bool_t ego_rpc_valid( ego_rpc_base_t * prpc )           { return ( NULL != prpc ) && prpc->allocated; }
static INLINE bool_t ego_rpc_matches( ego_rpc_base_t * prpc, int guid ) { return ( NULL != prpc ) && prpc->allocated && ( guid == prpc->guid ); }
static INLINE bool_t ego_rpc_finished( ego_rpc_base_t * prpc, int guid ) { return !ego_rpc_matches( prpc, guid ) || ( prpc->finished ); }
static INLINE bool_t ego_rpc_abort( ego_rpc_base_t * prpc, int guid ) { if ( !ego_rpc_matches( prpc, guid ) ) return bfalse; prpc->abort = btrue; return btrue; }

ego_rpc_base_t * ego_rpc_base_ctor( ego_rpc_base_t * prpc, int data_type, void * data ) ;
ego_rpc_base_t * ego_rpc_base_dtor( ego_rpc_base_t * prpc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a "remote procedure call" structure for handling calls to TxTexture_load_one_vfs()
/// and TxTitleImage_load_one_vfs()

struct s_tx_request
{
    // "base class" stuff
    ego_rpc_base_t ego_rpc_base;

    // the function call parameters
    STRING filename;
    TX_REF itex_src;
    Uint32 key;

    // the function call return value(s)
    TX_REF index;    /// the return value of the function
};

typedef struct s_tx_request tx_request_t;

tx_request_t * tx_request_ctor( tx_request_t * preq, int type );
tx_request_t * tx_request_dtor( tx_request_t * preq );

tx_request_t * ego_rpc_load_TxTexture( const char *filename, int itex_src, Uint32 key );
tx_request_t * ego_rpc_load_TxTitleImage( const char *filename );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_rpc_system_begin();
void   ego_rpc_system_end();
bool_t ego_rpc_system_timestep();
