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

#include "egoboo_typedef.h"

#include "egoboo_object.h"

#include "char.h"

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

void    ChrList_ctor( void );
void    ChrList_dtor( void );
void    ChrList_reinit( void );

CHR_REF ChrList_allocate( const CHR_REF override );

bool_t  ChrList_free_one( const CHR_REF ichr );
void    ChrList_free_all( void );

void    ChrList_update_used( void );

void    ChrList_cleanup( void );

bool_t ChrList_add_activation( const CHR_REF ichr );
bool_t ChrList_add_termination( const CHR_REF ichr );
bool_t ChrList_request_terminate( const CHR_REF ichr );

int ChrList_count_free( void );
int ChrList_count_used( void );
