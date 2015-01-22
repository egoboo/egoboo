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

/// @file  game/ChrList.h
/// @brief Routines for character list management.

#pragma once

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/char.h"

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the ChrList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR)  {int IT##_internal; int chr_loop_start_depth = chr_loop_depth; chr_loop_depth++; for(IT##_internal=0;IT##_internal<ChrList.used_count;IT##_internal++) { CHR_REF IT; chr_t * PCHR = NULL; IT = (CHR_REF)ChrList.used_ref[IT##_internal]; if(!ACTIVE_CHR (IT)) continue; PCHR = ChrList_get_ptr( IT );
#define CHR_END_LOOP() } chr_loop_depth--; EGOBOO_ASSERT(chr_loop_start_depth == chr_loop_depth); ChrList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( chr_t, ChrList, MAX_CHR );

extern int chr_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    ChrList_ctor();
void    ChrList_dtor();
void    ChrList_reinit();

CHR_REF ChrList_allocate( const CHR_REF override );

bool  ChrList_free_one( const CHR_REF ichr );
void    ChrList_free_all();

void    ChrList_update_used();

void    ChrList_cleanup();

bool ChrList_add_activation( const CHR_REF ichr );
bool ChrList_add_termination( const CHR_REF ichr );
bool ChrList_request_terminate( const CHR_REF ichr );

int ChrList_count_free();
int ChrList_count_used();

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_CHR_RANGE( ICHR )    ( ((CHR_REF)(ICHR)) < MAX_CHR )
#define DEFINED_CHR( ICHR )        ( VALID_CHR_RANGE( ICHR ) && DEFINED_PCHR_RAW   ( ChrList.lst + (ICHR)) )
#define ALLOCATED_CHR( ICHR )      ( VALID_CHR_RANGE( ICHR ) && ALLOCATED_PCHR_RAW ( ChrList.lst + (ICHR)) )
#define ACTIVE_CHR( ICHR )         ( VALID_CHR_RANGE( ICHR ) && ACTIVE_PCHR_RAW    ( ChrList.lst + (ICHR)) )
#define WAITING_CHR( ICHR )        ( VALID_CHR_RANGE( ICHR ) && WAITING_PCHR_RAW   ( ChrList.lst + (ICHR)) )
#define TERMINATED_CHR( ICHR )     ( VALID_CHR_RANGE( ICHR ) && TERMINATED_PCHR_RAW( ChrList.lst + (ICHR)) )

#define GET_INDEX_PCHR( PCHR )      LAMBDA(NULL == (PCHR),  INVALID_CHR_IDX, (size_t)GET_INDEX_POBJ( PCHR, INVALID_CHR_IDX ))
#define GET_REF_PCHR( PCHR )        ((CHR_REF)GET_INDEX_PCHR( PCHR ))
#define VALID_CHR_PTR( PCHR )       ( (NULL != (PCHR)) && VALID_CHR_RANGE( GET_REF_POBJ( PCHR, INVALID_CHR_REF) ) )
#define DEFINED_PCHR( PCHR )        ( VALID_CHR_PTR( PCHR ) && DEFINED_PCHR_RAW   ( PCHR ) )
#define ALLOCATED_PCHR( PCHR )      ( VALID_CHR_PTR( PCHR ) && ALLOCATED_PCHR_RAW ( PCHR ) )
#define ACTIVE_PCHR( PCHR )         ( VALID_CHR_PTR( PCHR ) && ACTIVE_PCHR_RAW    ( PCHR ) )
#define WAITING_PCHR( PCHR )        ( VALID_CHR_PTR( PCHR ) && WAITING_PCHR_RAW   ( PCHR ) )
#define TERMINATED_PCHR( PCHR )     ( VALID_CHR_PTR( PCHR ) && TERMINATED_PCHR_RAW( PCHR ) )

// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game".
#define INGAME_CHR_BASE(ICHR)       ( VALID_CHR_RANGE( ICHR ) && INGAME_PCHR_BASE_RAW( ChrList.lst + (ICHR) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_CHR_PTR( PCHR ) && INGAME_PCHR_BASE_RAW( PCHR ) )

#define INGAME_CHR(ICHR)            LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_CHR(ICHR), INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PCHR(PCHR), INGAME_PCHR_BASE(PCHR) )

// macros without range checking
#define INGAME_PCHR_BASE_RAW(PCHR)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define DEFINED_PCHR_RAW( PCHR )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define ALLOCATED_PCHR_RAW( PCHR )      ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) )
#define ACTIVE_PCHR_RAW( PCHR )         ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) )
#define WAITING_PCHR_RAW( PCHR )        WAITING_PBASE   ( POBJ_GET_PBASE(PCHR) )
#define TERMINATED_PCHR_RAW( PCHR )     TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) )
