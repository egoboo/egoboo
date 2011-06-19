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

/// @file EncList.h
/// @brief Routines for enchant list management

#include "egoboo_object.h"

#include "enchant.h"

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

bool_t VALID_ENC_RANGE( const ENC_REF IENC );
bool_t DEFINED_ENC( const ENC_REF IENC );
bool_t ALLOCATED_ENC( const ENC_REF IENC );
bool_t ACTIVE_ENC( const ENC_REF IENC );
bool_t WAITING_ENC( const ENC_REF IENC );
bool_t TERMINATED_ENC( const ENC_REF IENC );

size_t  GET_INDEX_PENC( const enc_t * PENC );
ENC_REF GET_REF_PENC( const enc_t * PENC );
bool_t  DEFINED_PENC( const enc_t * PENC );
bool_t  VALID_ENC_PTR( const enc_t * PENC );
bool_t  ALLOCATED_PENC( const enc_t * PENC );
bool_t  ACTIVE_PENC( const enc_t * PENC );
bool_t  TERMINATED_PENC( const enc_t * PENC );

bool_t INGAME_ENC_BASE( const ENC_REF IENC );
bool_t INGAME_PENC_BASE( const enc_t * PENC );

bool_t INGAME_ENC( const ENC_REF IENC );
bool_t INGAME_PENC( const enc_t * PENC );

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define _VALID_ENC_RANGE( IENC )    ( ((IENC) < MAX_ENC) && ((IENC) >= 0) )
#define _DEFINED_ENC( IENC )        ( _VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _ALLOCATED_ENC( IENC )      ( _VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _ACTIVE_ENC( IENC )         ( _VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _WAITING_ENC( IENC )        ( _VALID_ENC_RANGE( IENC ) && WAITING_PBASE   ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _TERMINATED_ENC( IENC )     ( _VALID_ENC_RANGE( IENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )

#define _GET_INDEX_PENC( PENC )      ((NULL == (PENC)) ? MAX_ENC : (size_t)GET_INDEX_POBJ( PENC, MAX_ENC ))
#define _GET_REF_PENC( PENC )        ((ENC_REF)_GET_INDEX_PENC( PENC ))
#define _DEFINED_PENC( PENC )        ( _VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define _VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && _VALID_ENC_RANGE( GET_REF_POBJ( PENC, MAX_ENC) ) )
#define _ALLOCATED_PENC( PENC )      ( _VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) ) )
#define _ACTIVE_PENC( PENC )         ( _VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) )
#define _TERMINATED_PENC( PENC )     ( _VALID_ENC_PTR( PENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(PENC) ) )

// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define _INGAME_ENC_BASE(IENC)       ( _VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && ON_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _INGAME_PENC_BASE(PENC)      ( _VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) && ON_PBASE( POBJ_GET_PBASE(PENC) ) )

#define _INGAME_ENC(IENC)            ( (ego_object_spawn_depth) > 0 ? _DEFINED_ENC(IENC) : _INGAME_ENC_BASE(IENC) )
#define _INGAME_PENC(PENC)           ( (ego_object_spawn_depth) > 0 ? _DEFINED_PENC(PENC) : _INGAME_PENC_BASE(PENC) )

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

void    EncList_init( void );
void    EncList_dtor( void );

ENC_REF EncList_allocate( const ENC_REF override );

bool_t  EncList_free_one( const ENC_REF ienc );
void    EncList_free_all( void );

bool_t  EncList_add_used( const ENC_REF ienc );

void    EncList_update_used( void );

void   EncList_cleanup( void );

bool_t EncList_add_activation( const ENC_REF ienc );
bool_t EncList_add_termination( const ENC_REF ienc );
bool_t EncList_request_terminate( const ENC_REF ienc );
