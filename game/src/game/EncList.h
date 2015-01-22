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

/// @file  game/EncList.h
/// @brief Routines for enchant list management

#pragma once

#include "game/egoboo_typedef.h"

#include "game/egoboo_object.h"
#include "game/enchant.h"

//Forward declarations
struct enc_t;

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the EncList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC)  {int IT##_internal; int enc_loop_start_depth = enc_loop_depth; enc_loop_depth++; for(IT##_internal=0;IT##_internal<EncList.used_count;IT##_internal++) { ENC_REF IT; enc_t * PENC = NULL; IT = (ENC_REF)EncList.used_ref[IT##_internal]; if(!ACTIVE_ENC (IT)) continue; PENC =  EncList_get_ptr( IT );
#define ENC_END_LOOP() } enc_loop_depth--; EGOBOO_ASSERT(enc_loop_start_depth == enc_loop_depth); EncList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( enc_t, EncList, MAX_ENC );

extern int enc_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    EncList_ctor();
void    EncList_dtor();

void    EncList_reinit();

ENC_REF EncList_allocate( const ENC_REF override );

bool  EncList_free_one( const ENC_REF ienc );
void    EncList_free_all();

void    EncList_update_used();

void   EncList_cleanup();

bool EncList_add_activation( const ENC_REF ienc );
bool EncList_add_termination( const ENC_REF ienc );
bool EncList_request_terminate( const ENC_REF ienc );

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_ENC_RANGE( IENC )    ( ((ENC_REF)(IENC)) < MAX_ENC )
#define DEFINED_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && DEFINED_PENC_RAW   ( EncList.lst + (IENC)) )
#define ALLOCATED_ENC( IENC )      ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PENC_RAW ( EncList.lst + (IENC)) )
#define ACTIVE_ENC( IENC )         ( VALID_ENC_RANGE( IENC ) && ACTIVE_PENC_RAW    ( EncList.lst + (IENC)) )
#define WAITING_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && WAITING_PENC_RAW   ( EncList.lst + (IENC)) )
#define TERMINATED_ENC( IENC )     ( VALID_ENC_RANGE( IENC ) && TERMINATED_PENC_RAW( EncList.lst + (IENC)) )

#define GET_INDEX_PENC( PENC )      LAMBDA(NULL == (PENC), INVALID_ENC_IDX, (size_t)GET_INDEX_POBJ( PENC, INVALID_ENC_IDX ))
#define GET_REF_PENC( PENC )        ((ENC_REF)GET_INDEX_PENC( PENC ))
#define VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && VALID_ENC_RANGE( GET_REF_POBJ( PENC, INVALID_ENC_REF) ) )
#define DEFINED_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && DEFINED_PENC_RAW   ( PENC ) )
#define ALLOCATED_PENC( PENC )      ( VALID_ENC_PTR( PENC ) && ALLOCATED_PENC_RAW ( PENC ) )
#define ACTIVE_PENC( PENC )         ( VALID_ENC_PTR( PENC ) && ACTIVE_PENC_RAW    ( PENC ) )
#define WAITING_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && WAITING_PENC_RAW   ( PENC ) )
#define TERMINATED_PENC( PENC )     ( VALID_ENC_PTR( PENC ) && TERMINATED_PENC_RAW( PENC ) )

// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_ENC_BASE(IENC)       ( VALID_ENC_RANGE( IENC ) && INGAME_PENC_BASE_RAW( EncList.lst + (IENC) ) )
#define INGAME_PENC_BASE(PENC)      ( VALID_ENC_PTR( PENC ) && INGAME_PENC_BASE_RAW( PENC ) )

#define INGAME_ENC(IENC)            LAMBDA( Ego::Entities::spawnDepth > 0, DEFINED_ENC(IENC), INGAME_ENC_BASE(IENC) )
#define INGAME_PENC(PENC)           LAMBDA( Ego::Entities::spawnDepth > 0, DEFINED_PENC(PENC), INGAME_PENC_BASE(PENC) )

// macros without range checking
#define INGAME_PENC_BASE_RAW(PENC)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) && ON_PBASE( POBJ_GET_PBASE(PENC) ) )
#define DEFINED_PENC_RAW( PENC )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define ALLOCATED_PENC_RAW( PENC )      ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) )
#define ACTIVE_PENC_RAW( PENC )         ACTIVE_PBASE( POBJ_GET_PBASE(PENC) )
#define WAITING_PENC_RAW( PENC )        WAITING_PBASE   ( POBJ_GET_PBASE(PENC) )
#define TERMINATED_PENC_RAW( PENC )     TERMINATED_PBASE( POBJ_GET_PBASE(PENC) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

bool _VALID_ENC_RANGE( const ENC_REF IENC );
bool _DEFINED_ENC( const ENC_REF IENC );
bool _ALLOCATED_ENC( const ENC_REF IENC );
bool _ACTIVE_ENC( const ENC_REF IENC );
bool _WAITING_ENC( const ENC_REF IENC );
bool _TERMINATED_ENC( const ENC_REF IENC );

size_t  _GET_INDEX_PENC( const enc_t * PENC );
ENC_REF _GET_REF_PENC( const enc_t * PENC );
bool  _DEFINED_PENC( const enc_t * PENC );
bool  _VALID_ENC_PTR( const enc_t * PENC );
bool  _ALLOCATED_PENC( const enc_t * PENC );
bool  _ACTIVE_PENC( const enc_t * PENC );
bool  _TERMINATED_PENC( const enc_t * PENC );

bool _INGAME_ENC_BASE( const ENC_REF IENC );
bool _INGAME_PENC_BASE( const enc_t * PENC );

bool _INGAME_ENC( const ENC_REF IENC );
bool _INGAME_PENC( const enc_t * PENC );