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

/// @file ChrList.h
/// @brief Routines for character list management

#include "egoboo_object.h"

#include "char.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_CHR_RANGE( ICHR )    ( ((ICHR) < MAX_CHR) && ((ICHR) >= 0) )
#define DEFINED_CHR( ICHR )        ( VALID_CHR_RANGE( ICHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define ALLOCATED_CHR( ICHR )      ( VALID_CHR_RANGE( ICHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define ACTIVE_CHR( ICHR )         ( VALID_CHR_RANGE( ICHR ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define WAITING_CHR( ICHR )        ( VALID_CHR_RANGE( ICHR ) && WAITING_PBASE   ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define TERMINATED_CHR( ICHR )     ( VALID_CHR_RANGE( ICHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )

#define GET_INDEX_PCHR( PCHR )      ((NULL == (PCHR)) ? MAX_CHR : (size_t)GET_INDEX_POBJ( PCHR, MAX_CHR ))
#define GET_REF_PCHR( PCHR )        ((CHR_REF)GET_INDEX_PCHR( PCHR ))
#define DEFINED_PCHR( PCHR )        ( VALID_CHR_PTR( PCHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define VALID_CHR_PTR( PCHR )       ( (NULL != (PCHR)) && VALID_CHR_RANGE( GET_REF_POBJ( PCHR, MAX_CHR) ) )
#define ALLOCATED_PCHR( PCHR )      ( VALID_CHR_PTR( PCHR ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define ACTIVE_PCHR( PCHR )         ( VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define TERMINATED_PCHR( PCHR )     ( VALID_CHR_PTR( PCHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) ) )

// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_CHR_BASE(ICHR)       ( VALID_CHR_RANGE( ICHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && ON_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )

#define INGAME_CHR(ICHR)            ( (ego_object_spawn_depth) > 0 ? DEFINED_CHR(ICHR) : INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           ( (ego_object_spawn_depth) > 0 ? DEFINED_PCHR(PCHR) : INGAME_PCHR_BASE(PCHR) )

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the ChrList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR)  {int IT##_internal; int chr_loop_start_depth = chr_loop_depth; chr_loop_depth++; for(IT##_internal=0;IT##_internal<ChrList.used_count;IT##_internal++) { CHR_REF IT; chr_t * PCHR = NULL; IT = (CHR_REF)ChrList.used_ref[IT##_internal]; if(!ACTIVE_CHR (IT)) continue; PCHR =  ChrList.lst + IT;
#define CHR_END_LOOP() } chr_loop_depth--; EGOBOO_ASSERT(chr_loop_start_depth == chr_loop_depth); ChrList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( chr_t, ChrList, MAX_CHR );

extern int chr_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    ChrList_init();
void    ChrList_dtor();

CHR_REF ChrList_allocate( const CHR_REF override );

bool_t  ChrList_free_one( const CHR_REF ichr );
void    ChrList_free_all();

bool_t  ChrList_add_used( const CHR_REF ichr );

void    ChrList_update_used();

void    ChrList_cleanup();

bool_t ChrList_add_activation( CHR_REF ichr );
bool_t ChrList_add_termination( CHR_REF ichr );