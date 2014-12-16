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

/// @file game/ChrList.inl
/// @brief
/// @details

#include "game/PrtList.h"

#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_PRT_RANGE( IPRT )    ( ((PRT_REF)(IPRT)) < MIN(maxparticles, MAX_PRT) )
#define DEFINED_PRT( IPRT )        ( VALID_PRT_RANGE( IPRT ) && DEFINED_PPRT_RAW   ( PrtList.lst + (IPRT)) )
#define ALLOCATED_PRT( IPRT )      ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PPRT_RAW ( PrtList.lst + (IPRT)) )
#define ACTIVE_PRT( IPRT )         ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PPRT_RAW    ( PrtList.lst + (IPRT)) )
#define WAITING_PRT( IPRT )        ( VALID_PRT_RANGE( IPRT ) && WAITING_PPRT_RAW   ( PrtList.lst + (IPRT)) )
#define TERMINATED_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && TERMINATED_PPRT_RAW( PrtList.lst + (IPRT)) )

#define GET_INDEX_PPRT( PPRT )      LAMBDA(NULL == (PPRT), INVALID_PRT_IDX, (size_t)GET_INDEX_POBJ( PPRT, INVALID_PRT_IDX ))
#define GET_REF_PPRT( PPRT )        ((PRT_REF)GET_INDEX_PPRT( PPRT ))
#define VALID_PRT_PTR( PPRT )       ( (NULL != (PPRT)) && VALID_PRT_RANGE( GET_REF_POBJ( PPRT, INVALID_PRT_REF) ) )
#define DEFINED_PPRT( PPRT )        ( VALID_PRT_PTR( PPRT ) && DEFINED_PPRT_RAW   ( PPRT ) )
#define ALLOCATED_PPRT( PPRT )      ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PPRT_RAW ( PPRT ) )
#define ACTIVE_PPRT( PPRT )         ( VALID_PRT_PTR( PPRT ) && ACTIVE_PPRT_RAW    ( PPRT ) )
#define WAITING_PPRT( PPRT )        ( VALID_PRT_PTR( PPRT ) && WAITING_PPRT_RAW   ( PPRT ) )
#define TERMINATED_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && TERMINATED_PPRT_RAW( PPRT ) )

// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"

// all particles that are ON are displayed
#define INGAME_PRT_BASE(IPRT)       ( VALID_PRT_RANGE( IPRT ) && INGAME_PPRT_BASE_RAW( PrtList.lst + (IPRT) ) )
#define INGAME_PPRT_BASE(PPRT)      ( VALID_PRT_PTR( PPRT ) && INGAME_PPRT_BASE_RAW( PPRT ) )

#define INGAME_PRT(IPRT)            LAMBDA( ego_object_spawn_depth > 0, DEFINED_PRT(IPRT), INGAME_PRT_BASE(IPRT) && (!PrtList.lst[IPRT].is_ghost) )
#define INGAME_PPRT(PPRT)           LAMBDA( ego_object_spawn_depth > 0, INGAME_PPRT_BASE(PPRT), DISPLAY_PPRT(PPRT) && ( !(PPRT)->is_ghost ) )

#define DISPLAY_PRT(IPRT)           INGAME_PRT_BASE(IPRT)
#define DISPLAY_PPRT(PPRT)          INGAME_PPRT_BASE(PPRT)

// macros without range checking
#define INGAME_PPRT_BASE_RAW(PPRT)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) && ON_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define DEFINED_PPRT_RAW( PPRT )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PPRT) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PPRT) ) )
#define ALLOCATED_PPRT_RAW( PPRT )      ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) )
#define ACTIVE_PPRT_RAW( PPRT )         ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) )
#define WAITING_PPRT_RAW( PPRT )        WAITING_PBASE   ( POBJ_GET_PBASE(PPRT) )
#define TERMINATED_PPRT_RAW( PPRT )     TERMINATED_PBASE( POBJ_GET_PBASE(PPRT) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE ego_bool _VALID_PRT_RANGE( const PRT_REF IPRT );
static INLINE ego_bool _DEFINED_PRT( const PRT_REF IPRT );
static INLINE ego_bool _ALLOCATED_PRT( const PRT_REF IPRT );
static INLINE ego_bool _ACTIVE_PRT( const PRT_REF IPRT );
static INLINE ego_bool _WAITING_PRT( const PRT_REF IPRT );
static INLINE ego_bool _TERMINATED_PRT( const PRT_REF IPRT );

static INLINE size_t  _GET_INDEX_PPRT( const prt_t * PPRT );
static INLINE PRT_REF _GET_REF_PPRT( const prt_t * PPRT );
static INLINE ego_bool  _DEFINED_PPRT( const prt_t * PPRT );
static INLINE ego_bool  _VALID_PRT_PTR( const prt_t * PPRT );
static INLINE ego_bool  _ALLOCATED_PPRT( const prt_t * PPRT );
static INLINE ego_bool  _ACTIVE_PPRT( const prt_t * PPRT );
static INLINE ego_bool  _TERMINATED_PPRT( const prt_t * PPRT );

static INLINE ego_bool _INGAME_PRT_BASE( const PRT_REF IPRT );
static INLINE ego_bool _INGAME_PPRT_BASE( const prt_t * PPRT );

static INLINE ego_bool _INGAME_PRT( const PRT_REF IPRT );
static INLINE ego_bool _INGAME_PPRT( const prt_t * PPRT );

static INLINE ego_bool _DISPLAY_PRT( const PRT_REF IPRT );
static INLINE ego_bool _DISPLAY_PPRT( const prt_t * PPRT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool _VALID_PRT_RANGE( const PRT_REF IPRT ) { return VALID_PRT_RANGE( IPRT ); }
static INLINE ego_bool _DEFINED_PRT( const PRT_REF IPRT )     { return DEFINED_PRT( IPRT );     }
static INLINE ego_bool _ALLOCATED_PRT( const PRT_REF IPRT )   { return ALLOCATED_PRT( IPRT );   }
static INLINE ego_bool _ACTIVE_PRT( const PRT_REF IPRT )      { return ACTIVE_PRT( IPRT );      }
static INLINE ego_bool _WAITING_PRT( const PRT_REF IPRT )     { return WAITING_PRT( IPRT );     }
static INLINE ego_bool _TERMINATED_PRT( const PRT_REF IPRT )  { return TERMINATED_PRT( IPRT );  }

static INLINE size_t  _GET_INDEX_PPRT( const prt_t * PPRT )  { return GET_INDEX_PPRT( PPRT );  }
static INLINE PRT_REF _GET_REF_PPRT( const prt_t * PPRT )    { return GET_REF_PPRT( PPRT );    }
static INLINE ego_bool  _DEFINED_PPRT( const prt_t * PPRT )    { return DEFINED_PPRT( PPRT );    }
static INLINE ego_bool  _VALID_PRT_PTR( const prt_t * PPRT )   { return VALID_PRT_PTR( PPRT );   }
static INLINE ego_bool  _ALLOCATED_PPRT( const prt_t * PPRT )  { return ALLOCATED_PPRT( PPRT );  }
static INLINE ego_bool  _ACTIVE_PPRT( const prt_t * PPRT )     { return ACTIVE_PPRT( PPRT );     }
static INLINE ego_bool  _TERMINATED_PPRT( const prt_t * PPRT ) { return TERMINATED_PPRT( PPRT ); }

static INLINE ego_bool _INGAME_PRT_BASE( const PRT_REF IPRT )  { return INGAME_PRT_BASE( IPRT );  }
static INLINE ego_bool _INGAME_PPRT_BASE( const prt_t * PPRT ) { return INGAME_PPRT_BASE( PPRT ); }

static INLINE ego_bool _INGAME_PRT( const PRT_REF IPRT )       { return INGAME_PRT( IPRT );  }
static INLINE ego_bool _INGAME_PPRT( const prt_t * PPRT )      { return INGAME_PPRT( PPRT ); }

static INLINE ego_bool _DISPLAY_PRT( const PRT_REF IPRT )      { return DISPLAY_PRT( IPRT ); }
static INLINE ego_bool _DISPLAY_PPRT( const prt_t * PPRT )     { return DISPLAY_PPRT( PPRT ); }
