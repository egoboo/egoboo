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
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_ENC_RANGE( IENC )    ( ((IENC) < MAX_ENC) && ((IENC) >= 0) )
#define DEFINED_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define ALLOCATED_ENC( IENC )      ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define ACTIVE_ENC( IENC )         ( VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define WAITING_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && WAITING_PBASE   ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define TERMINATED_ENC( IENC )     ( VALID_ENC_RANGE( IENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )

#define GET_INDEX_PENC( PENC )      ((NULL == (PENC)) ? MAX_ENC : (size_t)GET_INDEX_POBJ( PENC, MAX_ENC ))
#define GET_REF_PENC( PENC )        ((ENC_REF)GET_INDEX_PENC( PENC ))
#define DEFINED_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && VALID_ENC_RANGE( GET_REF_POBJ( PENC, MAX_ENC) ) )
#define ALLOCATED_PENC( PENC )      ( VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) ) )
#define ACTIVE_PENC( PENC )         ( VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) )
#define TERMINATED_PENC( PENC )     ( VALID_ENC_PTR( PENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(PENC) ) )

// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_ENC_BASE(IENC)       ( VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && ON_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define INGAME_PENC_BASE(PENC)      ( VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) && ON_PBASE( POBJ_GET_PBASE(PENC) ) )

#define INGAME_ENC(IENC)            ( (ego_object_spawn_depth) > 0 ? DEFINED_ENC(IENC) : INGAME_ENC_BASE(IENC) )
#define INGAME_PENC(PENC)           ( (ego_object_spawn_depth) > 0 ? DEFINED_PENC(PENC) : INGAME_PENC_BASE(PENC) )

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the EncList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC)  {int IT##_internal; int enc_loop_start_depth = enc_loop_depth; enc_loop_depth++; for(IT##_internal=0;IT##_internal<EncList.used_count;IT##_internal++) { ENC_REF IT; enc_t * PENC = NULL; IT = (ENC_REF)EncList.used_ref[IT##_internal]; if(!ACTIVE_ENC (IT)) continue; PENC =  EncList.lst + IT;
#define ENC_END_LOOP() } enc_loop_depth--; EGOBOO_ASSERT(enc_loop_start_depth == enc_loop_depth); EncList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( enc_t, EncList, MAX_ENC );

extern int enc_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    EncList_init();
void    EncList_dtor();

ENC_REF EncList_allocate( const ENC_REF override );

bool_t  EncList_free_one( const ENC_REF ienc );
void    EncList_free_all();

bool_t  EncList_add_used( const ENC_REF ienc );

void    EncList_update_used();

void    EncList_cleanup();

bool_t EncList_add_activation( ENC_REF ienc );
bool_t EncList_add_termination( ENC_REF ienc );
