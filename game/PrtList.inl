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

/// @file ChrList.inl
/// @brief
/// @details

#include "PrtList.h"

#include "particle.h"

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

///--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_PRT_RANGE( const PRT_REF IPRT );
static INLINE bool_t DEFINED_PRT( const PRT_REF IPRT );
static INLINE bool_t ALLOCATED_PRT( const PRT_REF IPRT );
static INLINE bool_t ACTIVE_PRT( const PRT_REF IPRT );
static INLINE bool_t WAITING_PRT( const PRT_REF IPRT );
static INLINE bool_t TERMINATED_PRT( const PRT_REF IPRT );

static INLINE size_t  GET_INDEX_PPRT( const prt_t * PPRT );
static INLINE PRT_REF GET_REF_PPRT( const prt_t * PPRT );
static INLINE bool_t  DEFINED_PPRT( const prt_t * PPRT );
static INLINE bool_t  VALID_PRT_PTR( const prt_t * PPRT );
static INLINE bool_t  ALLOCATED_PPRT( const prt_t * PPRT );
static INLINE bool_t  ACTIVE_PPRT( const prt_t * PPRT );
static INLINE bool_t  TERMINATED_PPRT( const prt_t * PPRT );

static INLINE bool_t INGAME_PRT_BASE( const PRT_REF IPRT );
static INLINE bool_t INGAME_PPRT_BASE( const prt_t * PPRT );

static INLINE bool_t INGAME_PRT( const PRT_REF IPRT );
static INLINE bool_t INGAME_PPRT( const prt_t * PPRT );

static INLINE bool_t DISPLAY_PRT( const PRT_REF IPRT );
static INLINE bool_t DISPLAY_PPRT( const prt_t * PPRT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool_t VALID_PRT_RANGE( const PRT_REF IPRT ) { return BOOL_T( _VALID_PRT_RANGE( IPRT ) ); }
static INLINE bool_t DEFINED_PRT( const PRT_REF IPRT )     { return BOOL_T( _DEFINED_PRT( IPRT ) );     }
static INLINE bool_t ALLOCATED_PRT( const PRT_REF IPRT )   { return BOOL_T( _ALLOCATED_PRT( IPRT ) );   }
static INLINE bool_t ACTIVE_PRT( const PRT_REF IPRT )      { return BOOL_T( _ACTIVE_PRT( IPRT ) );      }
static INLINE bool_t WAITING_PRT( const PRT_REF IPRT )     { return BOOL_T( _WAITING_PRT( IPRT ) );     }
static INLINE bool_t TERMINATED_PRT( const PRT_REF IPRT )  { return BOOL_T( _TERMINATED_PRT( IPRT ) );  }

static INLINE size_t  GET_INDEX_PPRT( const prt_t * PPRT )  { return _GET_INDEX_PPRT( PPRT );  }
static INLINE PRT_REF GET_REF_PPRT( const prt_t * PPRT )    { return _GET_REF_PPRT( PPRT );    }
static INLINE bool_t  DEFINED_PPRT( const prt_t * PPRT )    { return BOOL_T( _DEFINED_PPRT( PPRT ) );    }
static INLINE bool_t  VALID_PRT_PTR( const prt_t * PPRT )   { return BOOL_T( _VALID_PRT_PTR( PPRT ) );   }
static INLINE bool_t  ALLOCATED_PPRT( const prt_t * PPRT )  { return BOOL_T( _ALLOCATED_PPRT( PPRT ) );  }
static INLINE bool_t  ACTIVE_PPRT( const prt_t * PPRT )     { return BOOL_T( _ACTIVE_PPRT( PPRT ) );     }
static INLINE bool_t  TERMINATED_PPRT( const prt_t * PPRT ) { return BOOL_T( _TERMINATED_PPRT( PPRT ) ); }

static INLINE bool_t INGAME_PRT_BASE( const PRT_REF IPRT )  { return BOOL_T( _INGAME_PRT_BASE( IPRT ) );  }
static INLINE bool_t INGAME_PPRT_BASE( const prt_t * PPRT ) { return BOOL_T( _INGAME_PPRT_BASE( PPRT ) ); }

static INLINE bool_t INGAME_PRT( const PRT_REF IPRT )       { return BOOL_T( _INGAME_PRT( IPRT ) );  }
static INLINE bool_t INGAME_PPRT( const prt_t * PPRT )      { return BOOL_T( _INGAME_PPRT( PPRT ) ); }

static INLINE bool_t DISPLAY_PRT( const PRT_REF IPRT )     { return BOOL_T( _DISPLAY_PRT( IPRT ) ); }
static INLINE bool_t DISPLAY_PPRT( const prt_t * PPRT )    { return BOOL_T( _DISPLAY_PPRT( PPRT ) ); }
