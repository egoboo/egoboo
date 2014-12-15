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

/// @file game/rpc.h
/// @brief Definitions for the Remote Procedure Call module
///
/// @details This module allows inter-thread and network control of certain game functions.
/// The main use of this at the moment (since we have no server), is to make it possible
/// to do things like loading modules in a worker thread. All graphics functions must be
/// called from the main thread...

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_rpc_base;
typedef struct s_rpc_base ego_rpc_base_t;

struct s_tx_request;
typedef struct s_tx_request tx_request_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a generic "remote procedure call" structure for handling "inter-thread" communication
struct s_rpc_base
{
    int    index;      ///< the index of this request
    int    guid;       ///< the request id number
    ego_bool allocated;  ///< is this request being used?
    ego_bool finished;   ///< has the request been fully processed?
    ego_bool abort;      ///< has the calling function requested an abort?

    int    data_type;  ///< a the type of the "inherited" data
    void * data;       ///< a pointer to the "inherited" data
};

static INLINE ego_bool ego_rpc_valid( const ego_rpc_base_t * prpc )           { return ( NULL != prpc ) && prpc->allocated; }
static INLINE ego_bool ego_rpc_matches( const ego_rpc_base_t * prpc, const int guid ) { return ( NULL != prpc ) && prpc->allocated && ( guid == prpc->guid ); }
static INLINE ego_bool ego_rpc_finished( const ego_rpc_base_t * prpc, const int guid ) { return !ego_rpc_matches( prpc, guid ) || ( prpc->finished ); }
static INLINE ego_bool ego_rpc_abort( ego_rpc_base_t * prpc, const int guid ) { if ( !ego_rpc_matches( prpc, guid ) ) return ego_false; prpc->abort = ego_true; return ego_true; }

ego_rpc_base_t * ego_rpc_base_ctor( ego_rpc_base_t * prpc, int data_type, void * data );
ego_rpc_base_t * ego_rpc_base_dtor( ego_rpc_base_t * prpc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a "remote procedure call" structure for handling calls to TxList_load_one_vfs()
/// and mnu_TxList_load_one_vfs()

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

tx_request_t * tx_request_ctor( tx_request_t * preq, int type );
tx_request_t * tx_request_dtor( tx_request_t * preq );

tx_request_t * ego_rpc_load_TxList( const char *filename, int itex_src, Uint32 key );
tx_request_t * ego_rpc_load_mnu_TxList( const char *filename );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bool ego_rpc_system_begin( void );
void   ego_rpc_system_end( void );
ego_bool ego_rpc_system_timestep( void );
