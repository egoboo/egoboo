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
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR)  { int chr_loop_start_depth = chr_loop_depth; chr_loop_depth++; \
 										  for(const auto &IT##_internal : _characterList) \
 										  	{ \
 										  		CHR_REF IT = IT##_internal.first; \
 										  		chr_t * PCHR = IT##_internal.second.get();
 										  		//if(!ACTIVE_PBASE(POBJ_GET_PBASE(PCHR))) continue;

#define CHR_END_LOOP() } chr_loop_depth--; EGOBOO_ASSERT(chr_loop_start_depth == chr_loop_depth); ChrList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

extern int chr_loop_depth;

extern std::unordered_map<CHR_REF, std::shared_ptr<chr_t>> _characterList; //List of all objects currently in the game

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------
CHR_REF ChrList_allocate( const PRO_REF profile, const CHR_REF override );

bool  ChrList_free_one( const CHR_REF ichr );

void    ChrList_cleanup();

bool ChrList_add_activation( const CHR_REF ichr );
bool ChrList_add_termination( const CHR_REF ichr );
bool ChrList_request_terminate( const CHR_REF ichr );

chr_t* ChrList_get_ptr(CHR_REF ichr);

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_CHR_RANGE( ICHR )    ( static_cast<CHR_REF>(ICHR) < MAX_CHR && ICHR != INVALID_CHR_REF )
#define DEFINED_CHR( ICHR )        ( VALID_CHR_RANGE( ICHR ) && ChrList_get_ptr(ICHR) != nullptr )
#define ALLOCATED_CHR( ICHR )      ( _characterList.find(ICHR) != _characterList.end() )
#define ACTIVE_CHR( ICHR )         ( VALID_CHR_RANGE( ICHR ) && ChrList_get_ptr(ICHR) != nullptr )

#define GET_INDEX_PCHR( PCHR )      ((nullptr == (PCHR)) ? INVALID_CHR_IDX : PCHR->characterID)
#define GET_REF_PCHR( PCHR )        ((CHR_REF)GET_INDEX_PCHR( PCHR ))
#define DEFINED_PCHR( PCHR )        ( nullptr != (PCHR) )
#define ALLOCATED_PCHR( PCHR )      ( nullptr != (PCHR) )
#define ACTIVE_PCHR( PCHR )         ( nullptr != (PCHR) && !PCHR->terminateRequested )
#define TERMINATED_PCHR( PCHR )     ( nullptr != (PCHR) && PCHR->terminateRequested )

#define INGAME_CHR(ICHR)            ( INGAME_PCHR(ChrList_get_ptr(ICHR)) )
#define INGAME_PCHR(PCHR)           ( nullptr != (PCHR) && !PCHR->terminateRequested )

