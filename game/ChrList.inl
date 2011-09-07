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

#include "ChrList.h"

#include "char.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define _VALID_CHR_RANGE( ICHR )    ( ((ICHR) < MAX_CHR) && ((ICHR) >= 0) )
#define _DEFINED_CHR( ICHR )        ( _VALID_CHR_RANGE( ICHR ) && _DEFINED_PCHR_RAW   ( ChrList.lst + (ICHR)) )
#define _ALLOCATED_CHR( ICHR )      ( _VALID_CHR_RANGE( ICHR ) && _ALLOCATED_PCHR_RAW ( ChrList.lst + (ICHR)) )
#define _ACTIVE_CHR( ICHR )         ( _VALID_CHR_RANGE( ICHR ) && _ACTIVE_PCHR_RAW    ( ChrList.lst + (ICHR)) )
#define _WAITING_CHR( ICHR )        ( _VALID_CHR_RANGE( ICHR ) && _WAITING_PCHR_RAW   ( ChrList.lst + (ICHR)) )
#define _TERMINATED_CHR( ICHR )     ( _VALID_CHR_RANGE( ICHR ) && _TERMINATED_PCHR_RAW( ChrList.lst + (ICHR)) )

#define _GET_INDEX_PCHR( PCHR )      LAMBDA((NULL == (PCHR)),  MAX_CHR, (size_t)GET_INDEX_POBJ( PCHR, MAX_CHR ))
#define _GET_REF_PCHR( PCHR )        ((CHR_REF)_GET_INDEX_PCHR( PCHR ))
#define _VALID_CHR_PTR( PCHR )       ( (NULL != (PCHR)) && _VALID_CHR_RANGE( GET_REF_POBJ( PCHR, MAX_CHR) ) )
#define _DEFINED_PCHR( PCHR )        ( _VALID_CHR_PTR( PCHR ) && _DEFINED_PCHR_RAW   ( PCHR ) )
#define _ALLOCATED_PCHR( PCHR )      ( _VALID_CHR_PTR( PCHR ) && _ALLOCATED_PCHR_RAW ( PCHR ) )
#define _ACTIVE_PCHR( PCHR )         ( _VALID_CHR_PTR( PCHR ) && _ACTIVE_PCHR_RAW    ( PCHR ) )
#define _WAITING_PCHR( PCHR )        ( _VALID_CHR_PTR( PCHR ) && _WAITING_PCHR_RAW   ( PCHR ) )
#define _TERMINATED_PCHR( PCHR )     ( _VALID_CHR_PTR( PCHR ) && _TERMINATED_PCHR_RAW( PCHR ) )

// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define _INGAME_CHR_BASE(ICHR)       ( _VALID_CHR_RANGE( ICHR ) && _INGAME_PCHR_BASE_RAW( ChrList.lst + (ICHR) ) )
#define _INGAME_PCHR_BASE(PCHR)      ( _VALID_CHR_PTR( PCHR ) && _INGAME_PCHR_BASE_RAW( PCHR ) )

#define _INGAME_CHR(ICHR)            LAMBDA( ego_object_spawn_depth > 0, _DEFINED_CHR(ICHR), _INGAME_CHR_BASE(ICHR) )
#define _INGAME_PCHR(PCHR)           LAMBDA( ego_object_spawn_depth > 0, _DEFINED_PCHR(PCHR), _INGAME_PCHR_BASE(PCHR) )

// macros without range checking
#define _INGAME_PCHR_BASE_RAW(PCHR)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define _DEFINED_PCHR_RAW( PCHR )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define _ALLOCATED_PCHR_RAW( PCHR )      ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) )
#define _ACTIVE_PCHR_RAW( PCHR )         ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) )
#define _WAITING_PCHR_RAW( PCHR )        WAITING_PBASE   ( POBJ_GET_PBASE(PCHR) )
#define _TERMINATED_PCHR_RAW( PCHR )     TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_CHR_RANGE( const CHR_REF ICHR );
static INLINE bool_t DEFINED_CHR( const CHR_REF ICHR );
static INLINE bool_t ALLOCATED_CHR( const CHR_REF ICHR );
static INLINE bool_t ACTIVE_CHR( const CHR_REF ICHR );
static INLINE bool_t WAITING_CHR( const CHR_REF ICHR );
static INLINE bool_t TERMINATED_CHR( const CHR_REF ICHR );

static INLINE size_t  GET_INDEX_PCHR( const chr_t * PCHR );
static INLINE CHR_REF GET_REF_PCHR( const chr_t * PCHR );
static INLINE bool_t  DEFINED_PCHR( const chr_t * PCHR );
static INLINE bool_t  VALID_CHR_PTR( const chr_t * PCHR );
static INLINE bool_t  ALLOCATED_PCHR( const chr_t * PCHR );
static INLINE bool_t  ACTIVE_PCHR( const chr_t * PCHR );
static INLINE bool_t  TERMINATED_PCHR( const chr_t * PCHR );

static INLINE bool_t INGAME_CHR_BASE( const CHR_REF ICHR );
static INLINE bool_t INGAME_PCHR_BASE( const chr_t * PCHR );

static INLINE bool_t INGAME_CHR( const CHR_REF ICHR );
static INLINE bool_t INGAME_PCHR( const chr_t * PCHR );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_CHR_RANGE( const CHR_REF ICHR ) { return BOOL_T( _VALID_CHR_RANGE( ICHR ) ); }
static INLINE bool_t DEFINED_CHR( const CHR_REF ICHR )     { return BOOL_T( _DEFINED_CHR( ICHR ) );     }
static INLINE bool_t ALLOCATED_CHR( const CHR_REF ICHR )   { return BOOL_T( _ALLOCATED_CHR( ICHR ) );   }
static INLINE bool_t ACTIVE_CHR( const CHR_REF ICHR )      { return BOOL_T( _ACTIVE_CHR( ICHR ) );      }
static INLINE bool_t WAITING_CHR( const CHR_REF ICHR )     { return BOOL_T( _WAITING_CHR( ICHR ) );     }
static INLINE bool_t TERMINATED_CHR( const CHR_REF ICHR )  { return BOOL_T( _TERMINATED_CHR( ICHR ) );  }

static INLINE size_t  GET_INDEX_PCHR( const chr_t * PCHR )  { return _GET_INDEX_PCHR( PCHR );  }
static INLINE CHR_REF GET_REF_PCHR( const chr_t * PCHR )    { return _GET_REF_PCHR( PCHR );    }
static INLINE bool_t  DEFINED_PCHR( const chr_t * PCHR )    { return BOOL_T( _DEFINED_PCHR( PCHR ) );    }
static INLINE bool_t  VALID_CHR_PTR( const chr_t * PCHR )   { return BOOL_T( _VALID_CHR_PTR( PCHR ) );   }
static INLINE bool_t  ALLOCATED_PCHR( const chr_t * PCHR )  { return BOOL_T( _ALLOCATED_PCHR( PCHR ) );  }
static INLINE bool_t  ACTIVE_PCHR( const chr_t * PCHR )     { return BOOL_T( _ACTIVE_PCHR( PCHR ) );     }
static INLINE bool_t  TERMINATED_PCHR( const chr_t * PCHR ) { return BOOL_T( _TERMINATED_PCHR( PCHR ) ); }

static INLINE bool_t INGAME_CHR_BASE( const CHR_REF ICHR )  { return BOOL_T( _INGAME_CHR_BASE( ICHR ) );  }
static INLINE bool_t INGAME_PCHR_BASE( const chr_t * PCHR ) { return BOOL_T( _INGAME_PCHR_BASE( PCHR ) ); }

static INLINE bool_t INGAME_CHR( const CHR_REF ICHR )       { return BOOL_T( _INGAME_CHR( ICHR ) );  }
static INLINE bool_t INGAME_PCHR( const chr_t * PCHR )      { return BOOL_T( _INGAME_PCHR( PCHR ) ); }
