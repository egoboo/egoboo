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
// testing functions
//--------------------------------------------------------------------------------------------

bool_t VALID_PRT_RANGE( PRT_REF IPRT );
bool_t DEFINED_PRT( PRT_REF IPRT );
bool_t ALLOCATED_PRT( PRT_REF IPRT );
bool_t ACTIVE_PRT( PRT_REF IPRT );
bool_t WAITING_PRT( PRT_REF IPRT );
bool_t TERMINATED_PRT( PRT_REF IPRT );

REF_T   GET_INDEX_PPRT( prt_t * PPRT );
PRT_REF GET_REF_PPRT( prt_t * PPRT );
bool_t  DEFINED_PPRT( prt_t * PPRT );
bool_t  VALID_PRT_PTR( prt_t * PPRT );
bool_t  ALLOCATED_PPRT( prt_t * PPRT );
bool_t  ACTIVE_PPRT( prt_t * PPRT );
bool_t  TERMINATED_PPRT( prt_t * PPRT );

bool_t INGAME_PRT_BASE(PRT_REF IPRT);
bool_t INGAME_PPRT_BASE(prt_t * PPRT);

bool_t INGAME_PRT(PRT_REF IPRT);
bool_t INGAME_PPRT(prt_t * PPRT);

bool_t DISPLAY_PRT(PRT_REF IPRT);
bool_t DISPLAY_PPRT(prt_t * PPRT);

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define _VALID_PRT_RANGE( IPRT )    ( ((IPRT) < maxparticles) && ((IPRT) >= 0) && ((IPRT) < MAX_PRT) )
#define _DEFINED_PRT( IPRT )        ( _VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define _ALLOCATED_PRT( IPRT )      ( _VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define _ACTIVE_PRT( IPRT )         ( _VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define _WAITING_PRT( IPRT )        ( _VALID_PRT_RANGE( IPRT ) && WAITING_PBASE   ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define _TERMINATED_PRT( IPRT )     ( _VALID_PRT_RANGE( IPRT ) && TERMINATED_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )

#define _GET_INDEX_PPRT( PPRT )      ((NULL == (PPRT)) ? MAX_PRT : (size_t)GET_INDEX_POBJ( PPRT, MAX_PRT ))
#define _GET_REF_PPRT( PPRT )        ((PRT_REF)_GET_INDEX_PPRT( PPRT ))
#define _DEFINED_PPRT( PPRT )        ( _VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PPRT) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PPRT) ) )
#define _VALID_PRT_PTR( PPRT )       ( (NULL != (PPRT)) && _VALID_PRT_RANGE( GET_REF_POBJ( PPRT, MAX_PRT) ) )
#define _ALLOCATED_PPRT( PPRT )      ( _VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define _ACTIVE_PPRT( PPRT )         ( _VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define _TERMINATED_PPRT( PPRT )     ( _VALID_PRT_PTR( PPRT ) && TERMINATED_PBASE( POBJ_GET_PBASE(PPRT) ) )

// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"

// all particles that are ON are displayed
#define _INGAME_PRT_BASE(IPRT)       ( _VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && ON_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define _INGAME_PPRT_BASE(PPRT)      ( _VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) && ON_PBASE( POBJ_GET_PBASE(PPRT) ) )

#define _INGAME_PRT(IPRT)            ( ( (ego_object_spawn_depth) > 0 ? _DEFINED_PRT(IPRT) : _INGAME_PRT_BASE(IPRT) ) && (!PrtList.lst[IPRT].is_ghost) )
#define _INGAME_PPRT(PPRT)           ( ( (ego_object_spawn_depth) > 0 ? _INGAME_PPRT_BASE(PPRT) : _DISPLAY_PPRT(PPRT) ) && ( !(PPRT)->is_ghost ) )

#define _DISPLAY_PRT(IPRT)           _INGAME_PRT_BASE(IPRT)
#define _DISPLAY_PPRT(PPRT)          _INGAME_PPRT_BASE(PPRT)

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