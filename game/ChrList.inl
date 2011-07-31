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
#define _DEFINED_CHR( ICHR )        ( _VALID_CHR_RANGE( ICHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define _ALLOCATED_CHR( ICHR )      ( _VALID_CHR_RANGE( ICHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define _ACTIVE_CHR( ICHR )         ( _VALID_CHR_RANGE( ICHR ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define _WAITING_CHR( ICHR )        ( _VALID_CHR_RANGE( ICHR ) && WAITING_PBASE   ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define _TERMINATED_CHR( ICHR )     ( _VALID_CHR_RANGE( ICHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )

#define _GET_INDEX_PCHR( PCHR )      ((NULL == (PCHR)) ? MAX_CHR : (size_t)GET_INDEX_POBJ( PCHR, MAX_CHR ))
#define _GET_REF_PCHR( PCHR )        ((CHR_REF)_GET_INDEX_PCHR( PCHR ))
#define _DEFINED_PCHR( PCHR )        ( _VALID_CHR_PTR( PCHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define _VALID_CHR_PTR( PCHR )       ( (NULL != (PCHR)) && _VALID_CHR_RANGE( GET_REF_POBJ( PCHR, MAX_CHR) ) )
#define _ALLOCATED_PCHR( PCHR )      ( _VALID_CHR_PTR( PCHR ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define _ACTIVE_PCHR( PCHR )         ( _VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define _TERMINATED_PCHR( PCHR )     ( _VALID_CHR_PTR( PCHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) ) )

// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define _INGAME_CHR_BASE(ICHR)       ( _VALID_CHR_RANGE( ICHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && ON_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define _INGAME_PCHR_BASE(PCHR)      ( _VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )

#define _INGAME_CHR(ICHR)            ( (ego_object_spawn_depth) > 0 ? _DEFINED_CHR(ICHR) : _INGAME_CHR_BASE(ICHR) )
#define _INGAME_PCHR(PCHR)           ( (ego_object_spawn_depth) > 0 ? _DEFINED_PCHR(PCHR) : _INGAME_PCHR_BASE(PCHR) )

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

static INLINE bool_t VALID_CHR_RANGE( const CHR_REF ICHR ) { return _VALID_CHR_RANGE( ICHR ); }
static INLINE bool_t DEFINED_CHR( const CHR_REF ICHR )     { return _DEFINED_CHR( ICHR );     }
static INLINE bool_t ALLOCATED_CHR( const CHR_REF ICHR )   { return _ALLOCATED_CHR( ICHR );   }
static INLINE bool_t ACTIVE_CHR( const CHR_REF ICHR )      { return _ACTIVE_CHR( ICHR );      }
static INLINE bool_t WAITING_CHR( const CHR_REF ICHR )     { return _WAITING_CHR( ICHR );     }
static INLINE bool_t TERMINATED_CHR( const CHR_REF ICHR )  { return _TERMINATED_CHR( ICHR );  }

static INLINE size_t  GET_INDEX_PCHR( const chr_t * PCHR )  { return _GET_INDEX_PCHR( PCHR );  }
static INLINE CHR_REF GET_REF_PCHR( const chr_t * PCHR )    { return _GET_REF_PCHR( PCHR );    }
static INLINE bool_t  DEFINED_PCHR( const chr_t * PCHR )    { return _DEFINED_PCHR( PCHR );    }
static INLINE bool_t  VALID_CHR_PTR( const chr_t * PCHR )   { return _VALID_CHR_PTR( PCHR );   }
static INLINE bool_t  ALLOCATED_PCHR( const chr_t * PCHR )  { return _ALLOCATED_PCHR( PCHR );  }
static INLINE bool_t  ACTIVE_PCHR( const chr_t * PCHR )     { return _ACTIVE_PCHR( PCHR );     }
static INLINE bool_t  TERMINATED_PCHR( const chr_t * PCHR ) { return _TERMINATED_PCHR( PCHR ); }

static INLINE bool_t INGAME_CHR_BASE( const CHR_REF ICHR )  { return _INGAME_CHR_BASE( ICHR );  }
static INLINE bool_t INGAME_PCHR_BASE( const chr_t * PCHR )       { return _INGAME_PCHR_BASE( PCHR ); }

static INLINE bool_t INGAME_CHR( const CHR_REF ICHR )       { return _INGAME_CHR( ICHR );  }
static INLINE bool_t INGAME_PCHR( const chr_t * PCHR )      { return _INGAME_PCHR( PCHR ); }
