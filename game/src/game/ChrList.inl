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

#include "game/ChrList.h"

#include "game/char.h"

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
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_CHR_BASE(ICHR)       ( VALID_CHR_RANGE( ICHR ) && INGAME_PCHR_BASE_RAW( ChrList.lst + (ICHR) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_CHR_PTR( PCHR ) && INGAME_PCHR_BASE_RAW( PCHR ) )

#define INGAME_CHR(ICHR)            LAMBDA( ego_object_spawn_depth > 0, DEFINED_CHR(ICHR), INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           LAMBDA( ego_object_spawn_depth > 0, DEFINED_PCHR(PCHR), INGAME_PCHR_BASE(PCHR) )

// macros without range checking
#define INGAME_PCHR_BASE_RAW(PCHR)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define DEFINED_PCHR_RAW( PCHR )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define ALLOCATED_PCHR_RAW( PCHR )      ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) )
#define ACTIVE_PCHR_RAW( PCHR )         ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) )
#define WAITING_PCHR_RAW( PCHR )        WAITING_PBASE   ( POBJ_GET_PBASE(PCHR) )
#define TERMINATED_PCHR_RAW( PCHR )     TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE ego_bool _VALID_CHR_RANGE( const CHR_REF ICHR );
static INLINE ego_bool _DEFINED_CHR( const CHR_REF ICHR );
static INLINE ego_bool _ALLOCATED_CHR( const CHR_REF ICHR );
static INLINE ego_bool _ACTIVE_CHR( const CHR_REF ICHR );
static INLINE ego_bool _WAITING_CHR( const CHR_REF ICHR );
static INLINE ego_bool _TERMINATED_CHR( const CHR_REF ICHR );

static INLINE size_t  _GET_INDEX_PCHR( const chr_t * PCHR );
static INLINE CHR_REF _GET_REF_PCHR( const chr_t * PCHR );
static INLINE ego_bool  _DEFINED_PCHR( const chr_t * PCHR );
static INLINE ego_bool  _VALID_CHR_PTR( const chr_t * PCHR );
static INLINE ego_bool  _ALLOCATED_PCHR( const chr_t * PCHR );
static INLINE ego_bool  _ACTIVE_PCHR( const chr_t * PCHR );
static INLINE ego_bool  _TERMINATED_PCHR( const chr_t * PCHR );

static INLINE ego_bool _INGAME_CHR_BASE( const CHR_REF ICHR );
static INLINE ego_bool _INGAME_PCHR_BASE( const chr_t * PCHR );

static INLINE ego_bool _INGAME_CHR( const CHR_REF ICHR );
static INLINE ego_bool _INGAME_PCHR( const chr_t * PCHR );

//--------------------------------------------------------------------------------------------
// BLAH
//--------------------------------------------------------------------------------------------

static INLINE ego_bool _VALID_CHR_RANGE( const CHR_REF ICHR ) { return VALID_CHR_RANGE( ICHR ); }
static INLINE ego_bool _DEFINED_CHR( const CHR_REF ICHR )     { return DEFINED_CHR( ICHR );     }
static INLINE ego_bool _ALLOCATED_CHR( const CHR_REF ICHR )   { return ALLOCATED_CHR( ICHR );   }
static INLINE ego_bool _ACTIVE_CHR( const CHR_REF ICHR )      { return ACTIVE_CHR( ICHR );      }
static INLINE ego_bool _WAITING_CHR( const CHR_REF ICHR )     { return WAITING_CHR( ICHR );     }
static INLINE ego_bool _TERMINATED_CHR( const CHR_REF ICHR )  { return TERMINATED_CHR( ICHR );  }

static INLINE size_t  _GET_INDEX_PCHR( const chr_t * PCHR )  { return GET_INDEX_PCHR( PCHR );  }
static INLINE CHR_REF _GET_REF_PCHR( const chr_t * PCHR )    { return GET_REF_PCHR( PCHR );    }
static INLINE ego_bool  _DEFINED_PCHR( const chr_t * PCHR )    { return DEFINED_PCHR( PCHR );    }
static INLINE ego_bool  _VALID_CHR_PTR( const chr_t * PCHR )   { return VALID_CHR_PTR( PCHR );   }
static INLINE ego_bool  _ALLOCATED_PCHR( const chr_t * PCHR )  { return ALLOCATED_PCHR( PCHR );  }
static INLINE ego_bool  _ACTIVE_PCHR( const chr_t * PCHR )     { return ACTIVE_PCHR( PCHR );     }
static INLINE ego_bool  _TERMINATED_PCHR( const chr_t * PCHR ) { return TERMINATED_PCHR( PCHR ); }

static INLINE ego_bool _INGAME_CHR_BASE( const CHR_REF ICHR )  { return INGAME_CHR_BASE( ICHR );  }
static INLINE ego_bool _INGAME_PCHR_BASE( const chr_t * PCHR ) { return INGAME_PCHR_BASE( PCHR ); }

static INLINE ego_bool _INGAME_CHR( const CHR_REF ICHR )       { return TO_EGO_BOOL( INGAME_CHR( ICHR ) );  }
static INLINE ego_bool _INGAME_PCHR( const chr_t * PCHR )      { return TO_EGO_BOOL( INGAME_PCHR( PCHR ) ); }
