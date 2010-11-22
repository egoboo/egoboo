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

/// @file PrtList.h
/// @brief Routines for particle list management

#include "egoboo_object.h"

#include "particle.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_PRT_RANGE( IPRT )    ( ((IPRT) < maxparticles) && ((IPRT) >= 0) && ((IPRT) < MAX_PRT) )
#define DEFINED_PRT( IPRT )        ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define ALLOCATED_PRT( IPRT )      ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define ACTIVE_PRT( IPRT )         ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define WAITING_PRT( IPRT )        ( VALID_PRT_RANGE( IPRT ) && WAITING_PBASE   ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define TERMINATED_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && TERMINATED_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )

#define GET_INDEX_PPRT( PPRT )      ((NULL == (PPRT)) ? MAX_PRT : (size_t)GET_INDEX_POBJ( PPRT, MAX_PRT ))
#define GET_REF_PPRT( PPRT )        ((PRT_REF)GET_INDEX_PPRT( PPRT ))
#define DEFINED_PPRT( PPRT )        ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PPRT) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PPRT) ) )
#define VALID_PRT_PTR( PPRT )       ( (NULL != (PPRT)) && VALID_PRT_RANGE( GET_REF_POBJ( PPRT, MAX_PRT) ) )
#define ALLOCATED_PPRT( PPRT )      ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define ACTIVE_PPRT( PPRT )         ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define TERMINATED_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && TERMINATED_PBASE( POBJ_GET_PBASE(PPRT) ) )

// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"

// all particles that are ON are displayed
#define INGAME_PRT_BASE(IPRT)       ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && ON_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define INGAME_PRT_PBASE(PPRT)      ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) && ON_PBASE( POBJ_GET_PBASE(PPRT) ) )

#define INGAME_PRT(IPRT)            ( ( (ego_object_spawn_depth) > 0 ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && (!PrtList.lst[IPRT].is_ghost) )
#define INGAME_PPRT(PPRT)           ( ( (ego_object_spawn_depth) > 0 ? INGAME_PRT_PBASE(PPRT) : DISPLAY_PPRT(PPRT) ) && ( !(PPRT)->is_ghost ) )

#define DISPLAY_PRT(IPRT)           INGAME_PRT_BASE(IPRT)
#define DISPLAY_PPRT(PPRT)          INGAME_PRT_PBASE(PPRT)

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the PrtList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.

#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL)  {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!ACTIVE_PRT (IT)) continue; prt_bundle_set(&PRT_BDL, PrtList.lst + IT);
#define PRT_BEGIN_LOOP_DISPLAY(IT, PRT_BDL) {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_bundle_t PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!DISPLAY_PRT(IT)) continue; prt_bundle_set(&PRT_BDL, PrtList.lst + IT);
#define PRT_END_LOOP() } prt_loop_depth--; EGOBOO_ASSERT(prt_loop_start_depth == prt_loop_depth); PrtList_cleanup(); }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( prt_t, PrtList, MAX_PRT );

extern size_t maxparticles;
extern bool_t maxparticles_dirty;

extern int    prt_loop_depth;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    PrtList_init();
void    PrtList_dtor();

PRT_REF PrtList_allocate( bool_t force );

bool_t  PrtList_free_one( const PRT_REF iprt );
void    PrtList_free_all();

bool_t  PrtList_add_used( const PRT_REF iprt );

void    PrtList_update_used();

void    PrtList_cleanup();

bool_t PrtList_add_activation( PRT_REF iprt );
bool_t PrtList_add_termination( PRT_REF iprt );

void PrtList_cleanup();