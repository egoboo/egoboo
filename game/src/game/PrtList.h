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

/// @file game/PrtList.h
/// @brief Routines for particle list management

#include "game/egoboo_typedef.h"

#include "game/egoboo_object.h"

#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the PrtList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.

#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL)  {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!ACTIVE_PRT (IT)) continue; prt_bundle_set(&PRT_BDL, PrtList_get_ptr( IT ));
#define PRT_BEGIN_LOOP_DISPLAY(IT, PRT_BDL) {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!DISPLAY_PRT(IT)) continue; prt_bundle_set(&PRT_BDL, PrtList_get_ptr( IT ));
#define PRT_END_LOOP() } prt_loop_depth--; EGOBOO_ASSERT(prt_loop_start_depth == prt_loop_depth); PrtList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( prt_t, PrtList, MAX_PRT );

extern size_t maxparticles;
extern ego_bool maxparticles_dirty;

extern int    prt_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    PrtList_ctor( void );
void    PrtList_dtor( void );
void    PrtList_reinit( void );

PRT_REF PrtList_allocate( const ego_bool force );

ego_bool  PrtList_free_one( const PRT_REF iprt );
void    PrtList_free_all( void );

void    PrtList_update_used( void );

void    PrtList_cleanup( void );
void    PrtList_reset_all( void );

ego_bool  PrtList_add_activation( const PRT_REF iprt );
ego_bool  PrtList_add_termination( const PRT_REF iprt );
ego_bool PrtList_request_terminate( const PRT_REF iprt );

int     PrtList_count_free( void );
