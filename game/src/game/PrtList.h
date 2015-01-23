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

#pragma once

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the PrtList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.

#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL)  {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!_ACTIVE_PRT (IT)) continue; prt_bundle_set(&PRT_BDL, PrtList_get_ptr( IT ));
#define PRT_BEGIN_LOOP_DISPLAY(IT, PRT_BDL) {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!_DISPLAY_PRT(IT)) continue; prt_bundle_set(&PRT_BDL, PrtList_get_ptr( IT ));
#define PRT_END_LOOP() } prt_loop_depth--; EGOBOO_ASSERT(prt_loop_start_depth == prt_loop_depth); PrtList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( prt_t, PrtList, MAX_PRT );

extern size_t maxparticles;
extern bool maxparticles_dirty;

extern int    prt_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    PrtList_ctor();
void    PrtList_dtor();
void    PrtList_reinit();

PRT_REF PrtList_allocate( const bool force );

bool  PrtList_free_one( const PRT_REF iprt );
void    PrtList_free_all();

void    PrtList_update_used();

void    PrtList_cleanup();
void    PrtList_reset_all();

bool  PrtList_add_activation( const PRT_REF iprt );
bool  PrtList_add_termination( const PRT_REF iprt );
bool PrtList_request_terminate( const PRT_REF iprt );

int     PrtList_count_free();

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

bool _VALID_PRT_RANGE( const PRT_REF IPRT );
bool _DEFINED_PRT( const PRT_REF IPRT );
bool _ALLOCATED_PRT( const PRT_REF IPRT );
bool _ACTIVE_PRT( const PRT_REF IPRT );
bool _WAITING_PRT( const PRT_REF IPRT );
bool _TERMINATED_PRT( const PRT_REF IPRT );

size_t  _GET_INDEX_PPRT( const prt_t * PPRT );
PRT_REF _GET_REF_PPRT( const prt_t * PPRT );
bool  _DEFINED_PPRT( const prt_t * PPRT );
bool  _VALID_PRT_PTR( const prt_t * PPRT );
bool  _ALLOCATED_PPRT( const prt_t * PPRT );
bool  _ACTIVE_PPRT( const prt_t * PPRT );
bool  _TERMINATED_PPRT( const prt_t * PPRT );

bool _INGAME_PRT_BASE( const PRT_REF IPRT );
bool _INGAME_PPRT_BASE( const prt_t * PPRT );

bool _INGAME_PRT( const PRT_REF IPRT );
bool _INGAME_PPRT( const prt_t * PPRT );

bool _DISPLAY_PRT( const PRT_REF IPRT );
bool _DISPLAY_PPRT( const prt_t * PPRT );
